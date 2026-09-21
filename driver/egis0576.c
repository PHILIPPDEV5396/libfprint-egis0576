/*
 * Egis Technology Inc. (aka. LighTuning) EH576 (1c7a:0576) driver for libfprint
 *
 * A tiny press-type image sensor (70x57 px), driven with the vendor's own
 * plaintext EGIS/SIGE command protocol over the USB bulk endpoints — the same one
 * its Windows driver speaks. The transport, the vendor init/calibration replay and
 * GetFrame live in driver/egis0576/egis0576_proto.c.
 *
 * Captured 70x57 frames are matched host-side with Egis' own feature extractor +
 * matcher, reverse-engineered from the Windows driver and reimplemented as native
 * C (driver/egis0576/egis_engine.*). Templates are stored as opaque blobs in
 * each print's fpi-data (plaintext, like every other libfprint driver).
 *
 * The transport is blocking USB (each transfer parks the calling thread for up
 * to its timeout). The one-time bring-up in open() runs on the fprintd main
 * thread, as libfprint dispatches it; every capture runs in a worker thread and
 * marshals its results back with g_idle_add, so the main loop never stalls
 * while a finger is on the sensor. The transport drives gusb's async API on a
 * private GMainContext rather than gusb's sync wrappers, so neither caller
 * depends on the default context -- see docs/worker-thread.md for why that
 * matters.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 */

#define FP_COMPONENT "egis0576"

#include "egis0576.h"
#include "egis0576/egis_engine.h"
#include "egis0576/egis0576_proto.h"
#include "drivers_api.h"

#define EGIS0576_ENROLL_STAGES 12
/* Enrolment takes the first finger-on frame of a press, and on a press that
 * lands at the edge (which is exactly what the placement steering asks for)
 * that frame is often partial contact: coverage under the gate, refused, and
 * the user has to press again for nothing. Instead, a refusal for coverage
 * keeps sampling the SAME press for up to this many frames (~300 ms) before
 * it is reported; the finger settles and coverage rises. Measured on the
 * reference unit: 5 of 17 presses of one enrolment were refused on their
 * first frame, none of them for placement. */
#define EGIS0576_ENROLL_SETTLE_FRAMES 8
/* After the vendor init + exposure calibration the no-finger frame has a
 * fixed-pattern variance around ~140 (measured 140.4 at the shipped gain,
 * docs/sensor-tuning.md; ~160 was the figure through the former TLS transport);
 * a real finger pushes it well past 300. Hysteresis: */
#define EGIS0576_FINGER_ON_VAR  250.0
#define EGIS0576_FINGER_OFF_VAR 215.0
#define EGIS0576_POLL_SLEEP_US  5000    /* small gap between captures */
#define EGIS0576_BASELINE_FRAMES 8      /* no-finger frames averaged into the flat-field baseline */
#define EGIS0576_BASELINE_MAX_VAR 210.0 /* stricter than FINGER_OFF but with headroom for the
                                          * calibrated-gain no-finger level; a hovering finger would
                                          * otherwise contaminate the baseline (true no-finger ~140) */

typedef enum {
  PH_AWAIT_ON,     /* waiting for a finger */
  PH_AWAIT_OFF,    /* waiting for the finger to lift before the next touch */
} CapturePhase;

struct _FpDeviceEgis0576
{
  FpDevice      parent;

  EgisDev      *sensor;         /* the plaintext channel to the sensor */

  /* capture runs in a worker thread (blocking USB must not run in the main
   * loop); results are marshalled back with g_idle_add. */
  GThread      *thread;
  gint          cancel;         /* atomic; set on main thread, read by worker */

  /* Per-capture GCancellable the worker's transport polls at every transfer
   * boundary (egis0576_proto.c never hands it to gusb: aborting an in-flight
   * URB wedges this sensor at USB level, measured). Without it a cancel had to
   * wait out whole *sequences* of transfers -- on a sensor that has stopped
   * answering, a failed getframe (~2.6 s) followed by the re-init's readiness
   * poll (10 x 850 ms) is ~11 s before a cancel was noticed, and a replay on a
   * sensor that stops answering mid-way is 24 reads x 800 ms, ~19 s; now it
   * waits for at most the one SEQUENCE in flight (a getframe: ~2.6 s on a dead
   * sensor; the init replay: ~0.3 s; one readiness-poll iteration: 850 ms).
   *
   * OWNERSHIP: the main thread owns the one and only ref. It is created in
   * start_capture (fresh object per capture, never g_cancellable_reset) and
   * handed to the EgisDev as a borrowed pointer BEFORE g_thread_new, so the
   * worker only ever sees a fully constructed object; it is cancelled from
   * egis0576_cancel / egis0576_close on the main thread (GCancellable is
   * thread-safe for that); and it is detached from the EgisDev and unreffed in
   * finish_teardown / egis0576_close only AFTER g_thread_join has returned.
   * The pointer therefore never changes while a worker exists, and the worker
   * can never dereference a cancellable the main thread is freeing. */
  GCancellable *capture_cancellable;

  gint          needs_reinit;   /* atomic; set by suspend/resume on the main
                                 * thread, consumed by the worker: the sensor
                                 * does not reliably keep its bring-up state
                                 * across s2idle (USB stays powered, but the
                                 * capture pipeline comes back wedged), so it is
                                 * re-initialised before the next capture */

  guint8        frame[EGIS_IMG];
  guint8        corrected[EGIS_IMG];    /* flat-field corrected frame */
  /* Per-instance, i.e. per fprintd process: the flat-field baseline (see
   * baseline_feed) and the exposure calibration egis_dev_calibrate found in
   * the first open(), re-applied to every later EgisDev the driver opens. */
  guint8        baseline[EGIS_IMG];     /* no-finger reference, valid iff have_baseline */
  gboolean      have_baseline;
  int           dc_c_calibrated;        /* -1 until the first open() calibrated */
  guint         enroll_count;

  /* identify: prints in gallery order (borrowed refs), for result mapping */
  GPtrArray    *gallery_prints;
};
G_DECLARE_FINAL_TYPE (FpDeviceEgis0576, fpi_device_egis0576, FPI, DEVICE_EGIS0576, FpDevice);
G_DEFINE_TYPE (FpDeviceEgis0576, fpi_device_egis0576, FP_TYPE_DEVICE);

/* ------------------------------------------------------------------ */
/* Frame statistics                                                   */
/* ------------------------------------------------------------------ */

static gdouble
frame_variance (const guint8 *buf)
{
  gdouble sum = 0.0, var = 0.0, mean;

  for (gsize i = 0; i < EGIS_IMG; i++)
    sum += buf[i];
  mean = sum / (gdouble) EGIS_IMG;
  for (gsize i = 0; i < EGIS_IMG; i++)
    {
      gdouble d = (gdouble) buf[i] - mean;
      var += d * d;
    }
  return var / (gdouble) EGIS_IMG;
}

/* Per-frame preprocessing (egis_preprocess, from egis_preprocess.c) is now the
 * BYTE-EXACT translation of the Windows pipeline (min-subtract -> invert ->
 * auto-brightness -> Otsu stretch to black-level 0x8c -> vertical flip). It is
 * declared in egis_engine.h and validated on fp_final (genuine 8/10 preserved,
 * impostor 0/12 — my earlier hand-written approximation destroyed minutiae, 0/10).
 * The min-subtract + Otsu-stretch normalise per-session brightness/contrast so a
 * template enrolled in one capture session matches a probe from another. */

/* Process-global per-boot flat-field baseline. The sensor's fixed-pattern noise
 * (~140 var, no finger) differs per power-cycle; subtracting it per-pixel makes
 * frames comparable ACROSS boots (without it a template enrolled one boot scores
 * exactly 0 against a probe from another boot, though within-boot it matches
 * up to ~19000). It is collected OPPORTUNISTICALLY from no-finger frames during
 * any operation (before the first press, and between enroll presses) and cached
 * for the fprintd process lifetime (~= this boot). This is what makes the
 * "official" GNOME flow work: GNOME says "place your finger" immediately with no
 * dedicated finger-off window, so a blocking pre-capture would just hang; instead
 * the baseline builds itself from the natural no-finger moments and, once found,
 * is reused by every later enroll/verify without re-capturing. Single-sensor
 * driver, so a process global is fine. */
typedef struct { guint32 acc[EGIS_IMG]; int count; } BaselineAcc;

/* Feed one no-finger frame into the accumulator; publishes the global baseline
 * once EGIS0576_BASELINE_FRAMES have been gathered. No-op once already valid. */
static void
baseline_feed (FpDeviceEgis0576 *self, BaselineAcc *b, const guint8 *frame)
{
  int i;

  if (self->have_baseline)
    return;
  for (i = 0; i < EGIS_IMG; i++)
    b->acc[i] += frame[i];
  b->count++;
  if (b->count >= EGIS0576_BASELINE_FRAMES)
    {
      for (i = 0; i < EGIS_IMG; i++)
        self->baseline[i] = (guint8) (b->acc[i] / b->count);
      self->have_baseline = TRUE;
      fp_dbg ("flat-field baseline built from %d no-finger frames (cached for boot)", b->count);
    }
}

/* Apply the per-boot flat-field: out[i] = clamp(raw[i] - baseline[i] + mean).
 * Removes per-pixel fixed-pattern while preserving the DC level (min-subtract in
 * egis_preprocess re-zeroes DC afterwards). No-op until a baseline exists. */
static void
flat_field (FpDeviceEgis0576 *self, const guint8 *raw, guint8 *out)
{
  long sum = 0;
  int i, mean;

  if (!self->have_baseline)
    {
      memcpy (out, raw, EGIS_IMG);
      return;
    }
  for (i = 0; i < EGIS_IMG; i++)
    sum += self->baseline[i];
  mean = (int) (sum / EGIS_IMG);
  for (i = 0; i < EGIS_IMG; i++)
    {
      int v = (int) raw[i] - (int) self->baseline[i] + mean;
      out[i] = v < 0 ? 0 : (v > 255 ? 255 : v);
    }
}

/* ------------------------------------------------------------------ */
/* Template blob <-> FpPrint fpi-data                                 */
/* ------------------------------------------------------------------ */

static const guint8 *
print_get_blob (FpPrint *print, GVariant **var_out, gsize *len)
{
  GVariant *var = NULL;

  g_object_get (print, "fpi-data", &var, NULL);
  if (!var || !g_variant_is_of_type (var, G_VARIANT_TYPE ("ay")))
    {
      g_clear_pointer (&var, g_variant_unref);
      return NULL;
    }
  *var_out = var;
  return g_variant_get_fixed_array (var, len, 1);
}

static gboolean
load_one_print (FpPrint *print)
{
  g_autoptr(GVariant) var = NULL;
  gsize len = 0;
  const guint8 *blob = print_get_blob (print, &var, &len);
  const guint8 *blobs[1];
  int sizes[1];

  if (!blob || len == 0)
    return FALSE;
  blobs[0] = blob;
  sizes[0] = (int) len;
  return egis_gallery_load (blobs, sizes, 1) == 0;
}

/* ------------------------------------------------------------------ */
/* Worker thread <-> main thread marshalling                          */
/* ------------------------------------------------------------------ */

typedef enum {
  M_ENROLL_PROGRESS,
  M_ENROLL_RETRY,
  M_ENROLL_DONE,
  M_VERIFY_REPORT,
  M_IDENTIFY_REPORT,
  M_COMPLETE,        /* finish verify/identify (report already sent) */
  M_ERROR,
} MsgKind;

typedef struct {
  FpDevice *dev;
  MsgKind   kind;
  guint     stage;         /* enroll progress */
  guint8   *blob;          /* enroll done (transfer) */
  int       size;
  int       score;         /* verify */
  int       idx;           /* identify gallery index, -1 = none */
  gboolean  extract_fail;  /* verify/identify: retry */
  GError   *error;         /* M_ERROR (transfer) */
} Msg;

static void
finish_teardown (FpDeviceEgis0576 *self, FpDevice *dev)
{
  if (self->thread)
    {
      g_thread_join (self->thread);
      self->thread = NULL;
    }
  /* Worker is gone: nothing can be inside a transfer any more, so detach the
   * borrowed pointer from the channel and drop our ref (see the struct). */
  if (self->sensor)
    egis_dev_set_cancellable (self->sensor, NULL);
  g_clear_object (&self->capture_cancellable);
  g_clear_pointer (&self->gallery_prints, g_ptr_array_unref);
  fpi_device_report_finger_status (dev, FP_FINGER_STATUS_NONE);
}

/* runs on the main thread */
static gboolean
idle_handle_msg (gpointer data)
{
  Msg *m = data;
  FpDevice *dev = m->dev;
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576 (dev);

  switch (m->kind)
    {
    case M_ENROLL_PROGRESS:
      fpi_device_enroll_progress (dev, m->stage, NULL, NULL);
      break;

    case M_ENROLL_RETRY:
      fpi_device_enroll_progress (dev, m->stage, NULL,
                                  fpi_device_retry_new (FP_DEVICE_RETRY_CENTER_FINGER));
      break;

    case M_VERIFY_REPORT:
      if (m->extract_fail)
        fpi_device_verify_report (dev, FPI_MATCH_ERROR, NULL,
                                  fpi_device_retry_new (FP_DEVICE_RETRY_CENTER_FINGER));
      else
        fpi_device_verify_report (dev,
                                  m->score >= EGIS_THRESHOLD ? FPI_MATCH_SUCCESS : FPI_MATCH_FAIL,
                                  NULL, NULL);
      break;

    case M_IDENTIFY_REPORT:
      if (m->extract_fail)
        {
          fpi_device_identify_report (dev, NULL, NULL,
                                      fpi_device_retry_new (FP_DEVICE_RETRY_CENTER_FINGER));
        }
      else
        {
          FpPrint *match = NULL;
          if (m->idx >= 0 && self->gallery_prints && m->idx < (gint) self->gallery_prints->len)
            match = g_ptr_array_index (self->gallery_prints, m->idx);
          fpi_device_identify_report (dev, match, NULL, NULL);
        }
      break;

    case M_ENROLL_DONE:
      finish_teardown (self, dev);
      if (m->size <= 0 || !m->blob)
        {
          fpi_device_enroll_complete (dev, NULL,
                                      fpi_device_error_new_msg (FP_DEVICE_ERROR_GENERAL,
                                                                "Failed to build template"));
        }
      else
        {
          FpPrint *print = NULL;
          fpi_device_get_enroll_data (dev, &print);
          fpi_print_set_type (print, FPI_PRINT_RAW);
          g_object_set (print, "fpi-data",
                        g_variant_new_from_data (G_VARIANT_TYPE ("ay"), m->blob, m->size,
                                                 TRUE, g_free, m->blob), NULL);
          fpi_device_enroll_complete (dev, g_object_ref (print), NULL);
        }
      break;

    case M_COMPLETE:
      {
        FpiDeviceAction action = fpi_device_get_current_action (dev);
        finish_teardown (self, dev);
        if (action == FPI_DEVICE_ACTION_VERIFY)
          fpi_device_verify_complete (dev, NULL);
        else
          fpi_device_identify_complete (dev, NULL);
      }
      break;

    case M_ERROR:
      finish_teardown (self, dev);
      fpi_device_action_error (dev, g_steal_pointer (&m->error));
      break;
    }

  g_free (m);
  return G_SOURCE_REMOVE;
}

static void
post_msg (FpDevice *dev, Msg *m)
{
  m->dev = dev;
  g_idle_add (idle_handle_msg, m);
}

/* Re-run the sensor bring-up on the already-claimed, still-enumerated device —
 * the s2idle recovery. The USB device stayed powered and the interface is still
 * claimed, but the sensor comes back from suspend with its capture pipeline in an
 * unusable state, so the readiness poll + init replay are done again.
 *
 * Deliberately does NOT: re-claim the interface (still claimed -> EBUSY),
 * re-init the host match engine (host state, intact), re-run exposure calibration
 * (it needs a no-finger window we can't guarantee on resume. NOTE: the replay's
 * record 25 block-writes regs 0x09..0x13 and so puts reg 0x0f back to the baked
 * 0x20 — see the register table at the top of egis0576_proto.c — but
 * egis_dev_open re-applies the value the first open()'s calibration found,
 * so the exposure stays the calibrated one across this re-init and across every
 * re-open), or recapture the flat-field baseline (exposure-tied, per-boot, and
 * still valid precisely because the exposure is re-applied) — so cross-boot
 * matching is preserved.
 *
 * Passes reset_if_stuck=FALSE: a ForceResetDevice here would take the device off
 * the bus mid-recovery. If the sensor is unresponsive the flag stays set and the
 * next worker tries again.
 *
 * Blocks ~0.3 s in USB transfers — safe ONLY because it runs in the capture
 * worker thread, never on the fprintd main loop. On failure the device is left
 * flagged so the next worker re-initialises eagerly rather than capturing into a
 * dead pipeline. */
static gboolean
worker_reinit (FpDeviceEgis0576 *self, GError **error)
{
  GUsbDevice *usb = fpi_device_get_usb_device (FP_DEVICE (self));

  /* Until the bring-up below has succeeded the sensor is in an unknown state
   * (a cancel is only honoured before or after the replay, never inside it,
   * but a failed record leaves a prefix of the vendor sequence and a cancel
   * honoured before the replay leaves the sensor un-initialised), so the NEXT
   * worker must re-init eagerly whichever path called us; cleared again on
   * success. */
  g_atomic_int_set (&self->needs_reinit, TRUE);
  g_clear_pointer (&self->sensor, egis_dev_free);
  self->sensor = egis_dev_new (usb);
  egis_dev_set_calibration (self->sensor, self->dc_c_calibrated);
  /* The fresh channel must carry this capture's cancellable, or the re-init's
   * own ~25 x 800 ms of transfers would be uncancellable again. */
  egis_dev_set_cancellable (self->sensor, self->capture_cancellable);
  if (!egis_dev_open (self->sensor, FALSE, error))
    return FALSE;                            /* leave needs_reinit TRUE */
  g_atomic_int_set (&self->needs_reinit, FALSE);
  return TRUE;
}

/* Worker-side cancel test for a failed egis_dev_* call: TRUE if the capture was
 * cancelled — either the flag is up, or egis_dev_* reported
 * G_IO_ERROR_CANCELLED from one of its between-transfer checks. Both are set
 * from the same place (egis0576_cancel / close, flag first), so they agree in
 * practice; checking both keeps a cancelled call from ever being mistaken for
 * a sensor failure that warrants a re-init. Consumes *error when TRUE. */
static gboolean
worker_cancelled (FpDeviceEgis0576 *self, GError **error)
{
  if (g_atomic_int_get (&self->cancel) ||
      g_error_matches (*error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    {
      g_clear_error (error);
      return TRUE;
    }
  return FALSE;
}

/* runs in the worker thread */
static gpointer
capture_thread (gpointer data)
{
  FpDevice *dev = data;
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576 (dev);
  FpiDeviceAction action = fpi_device_get_current_action (dev);
  CapturePhase phase = PH_AWAIT_ON;
  gboolean finishing = FALSE;
  gboolean cancelled = FALSE;    /* left the loop because the capture was cancelled */
  gboolean saw_finger = FALSE;   /* verify/identify: a press is in progress */
  int best_score = -1;           /* verify/identify: best score seen this press */
  /* Two-frame confirmation (verify/identify). A frame over the threshold is
   * held as a CANDIDATE; success is reported only when the NEXT finger-on
   * frame also clears the threshold (identify: for the same print), differs
   * from it byte-wise, and its raw bytes corroborate the accept
   * (egis_verify_raw_ok). Why: the sensor has been seen re-serving a stale
   * frame of an earlier press with fresh noise (tsteppy's driver guards for
   * the same reason), and with "first frame over threshold wins" one such
   * frame of the enrolled finger unlocks under any finger. A genuine press
   * clears the threshold on consecutive frames -- measured 60 of 60 presses
   * on the reference dataset, the accept moving from frame 0.07 to 1.10 on
   * average, i.e. one frame (~35 ms) of latency. A candidate that the finger
   * lifts on, or that the next frame contradicts, is never reported as a
   * match. */
  gboolean cand_valid = FALSE;
  int cand_score = -1, cand_idx = -1;
  int settle_tries = 0;          /* enrol: coverage refusals on the current press */
  guint8 cand_raw[EGIS_IMG];
  guint8 ffframe[EGIS_IMG];  /* flat-fielded frame (baseline-subtracted) */
  BaselineAcc bl = { { 0 }, 0 }; /* opportunistic no-finger baseline accumulator */

  while (!finishing && !g_atomic_int_get (&self->cancel))
    {
      GError *error = NULL;
      gdouble var;
      const guint8 *probe;

      /* Eager fast-path: a suspend/resume since the last frame left the sensor
       * needing re-initialisation (flag set by the vfuncs). Re-init HERE (worker
       * thread, blocking-safe) before getframe so we don't burn a getframe
       * timeout on a sensor that cannot answer. Honour a concurrent cancel
       * first: suspend cancels the action, and we must not run the re-init
       * (~0.3 s of USB on a healthy sensor; on one that has stopped answering
       * the readiness poll fails first, 10 x 850 ms, and a replay that goes
       * silent mid-way costs up to 24 x 800 ms) in the fragile post-resume
       * window when we're being torn down. */
      if (g_atomic_int_get (&self->needs_reinit) &&
          !g_atomic_int_get (&self->cancel))
        {
          if (!worker_reinit (self, &error))
            {
              Msg *m;
              if (worker_cancelled (self, &error))  /* cancel mid-re-init: not a sensor fault */
                {
                  cancelled = TRUE;
                  break;
                }
              m = g_new0 (Msg, 1);
              m->kind = M_ERROR;
              m->error = error;              /* -> fpi_device_action_error -> password fallback */
              post_msg (dev, m);
              return NULL;
            }
        }

      if (!egis_dev_getframe (self->sensor, self->frame, &error))
        {
          /* Backstop that also covers the idle-at-suspend case (no vfunc fired,
           * so needs_reinit was FALSE and the eager path was skipped) and any
           * unfreeze race: on a dead or silent sensor getframe returns an error
           * in bounded time (its bulk reads time out — 300 ms per preamble
           * reply, 800 ms per frame chunk — ~2.6 s worst case). If we were
           * cancelled (suspend, VerifyStop, close) — flag up, or a check between
           * transfers saw the cancel — leave with a clean cancel instead of
           * running a re-init. Otherwise re-initialise the sensor once via
           * worker_reinit (readiness poll + vendor replay, reset_if_stuck=FALSE),
           * in this worker thread, and retry, re-checking for a cancel between
           * the two steps. Give up — with the real device error -> password
           * fallback — only if the re-init or the retry getframe also fails for
           * a reason other than a cancel. */
          if (worker_cancelled (self, &error))
            {
              cancelled = TRUE;
              break;
            }
          fp_dbg ("getframe failed (%s); re-initialising the sensor and retrying",
                  error ? error->message : "?");
          g_clear_error (&error);

          if (!worker_reinit (self, &error))
            {
              Msg *m;
              if (worker_cancelled (self, &error))
                {
                  cancelled = TRUE;
                  break;
                }
              m = g_new0 (Msg, 1);
              m->kind = M_ERROR;
              m->error = error;
              post_msg (dev, m);
              return NULL;
            }
          if (g_atomic_int_get (&self->cancel))
            {
              cancelled = TRUE;
              break;
            }
          if (!egis_dev_getframe (self->sensor, self->frame, &error))
            {
              Msg *m;
              if (worker_cancelled (self, &error))
                {
                  cancelled = TRUE;
                  break;
                }
              m = g_new0 (Msg, 1);
              m->kind = M_ERROR;
              m->error = error;
              post_msg (dev, m);
              return NULL;
            }
        }
      var = frame_variance (self->frame);   /* finger-on/off uses RAW variance */


      /* Build the flat-field baseline opportunistically from no-finger frames
       * (before the first press / between presses) — never blocks, works with
       * GNOME's immediate "place finger" flow, and caches for the whole boot. */
      if (var < EGIS0576_BASELINE_MAX_VAR)
        baseline_feed (self, &bl, self->frame);

      /* (1) per-boot flat-field: subtract the fixed-pattern baseline so a template
       * enrolled one boot matches a probe from another (without it, cross-boot=0);
       * a no-op until the baseline exists. (2) BYTE-EXACT Windows preprocessing
       * (min-subtract, invert, auto-brightness, Otsu-stretch, flip) — validated on
       * fp_final (8/10). Applied identically to enroll + verify + identify. */
      flat_field (self, self->frame, ffframe);
      egis_preprocess (ffframe, self->corrected);
      probe = self->corrected;

      if (action == FPI_DEVICE_ACTION_ENROLL)
        {
          /* one add per finger press: capture on-press, then wait for lift */
          if (phase == PH_AWAIT_ON)
            {
              if (var >= EGIS0576_FINGER_ON_VAR)
                {
                  Msg *m;
                  int prog = 0;
                  int r;
                  r = egis_enroll_add (probe, &prog);
                  if (r == -2 && settle_tries < EGIS0576_ENROLL_SETTLE_FRAMES)
                    {
                      /* partial contact: let the press settle, try the next frame */
                      settle_tries++;
                      g_usleep (EGIS0576_POLL_SLEEP_US);
                      continue;
                    }
                  m = g_new0 (Msg, 1);
                  settle_tries = 0;
                  if (r == 1 || r == 2)
                    {
                      self->enroll_count++;
                      fp_dbg ("enroll stage %u/%u (var %.0f)", self->enroll_count,
                              EGIS0576_ENROLL_STAGES, var);
                      m->kind = M_ENROLL_PROGRESS;
                      m->stage = self->enroll_count;
                      finishing = (self->enroll_count >= EGIS0576_ENROLL_STAGES);
                    }
                  else
                    {
                      /* -2: coverage under the gate; 4: same-press duplicate;
                       * 5: same placement as a stored frame -- all three come
                       * back to the user as "adjust your finger and try again",
                       * which is the right hint for each of them. */
                      fp_dbg ("enroll press refused (code %d) at stage %u/%u (var %.0f)%s",
                              r, self->enroll_count, EGIS0576_ENROLL_STAGES, var,
                              r == 5 ? " -- same placement, asking for a shifted press" : "");
                      m->kind = M_ENROLL_RETRY;
                      m->stage = self->enroll_count;
                    }
                  post_msg (dev, m);
                  phase = PH_AWAIT_OFF;
                }
              else if (var < EGIS0576_FINGER_OFF_VAR && settle_tries > 0)
                {
                  /* lifted before any frame of the press reached the coverage
                   * gate: that press was too light or too partial, say so */
                  Msg *m = g_new0 (Msg, 1);
                  fp_dbg ("enroll press lifted after %d partial frames -- retry", settle_tries);
                  settle_tries = 0;
                  m->kind = M_ENROLL_RETRY;
                  m->stage = self->enroll_count;
                  post_msg (dev, m);
                }
            }
          else /* PH_AWAIT_OFF */
            {
              if (var < EGIS0576_FINGER_OFF_VAR)
                phase = PH_AWAIT_ON;
            }
        }
      else /* VERIFY / IDENTIFY: score EVERY frame while the finger is down and
            * keep the best. A single press moves through partial->full contact;
            * grabbing just the first frame (as before) often caught a poor
            * transitional image (score 0). Report SUCCESS as soon as any frame
            * crosses the threshold; report the final result when the finger
            * lifts. This makes a genuine finger match reliably within one press. */
        {
          if (var >= EGIS0576_FINGER_ON_VAR)
            {
              saw_finger = TRUE;
              {
                int score, idx;
                if (action == FPI_DEVICE_ACTION_VERIFY)
                  {
                    idx = 0;
                    score = egis_verify (probe, 0);
                  }
                else
                  {
                    idx = -1;
                    score = egis_identify (probe, &idx);
                  }
                if (score > best_score)
                  best_score = score;
                fp_dbg ("%s score %d idx %d (best %d, var %.0f)%s",
                        action == FPI_DEVICE_ACTION_VERIFY ? "verify" : "identify",
                        score, idx, best_score, var, cand_valid ? " [candidate held]" : "");

                if (score >= EGIS_THRESHOLD && idx >= 0)
                  {
                    if (cand_valid && idx == cand_idx
                        && memcmp (cand_raw, self->frame, EGIS_IMG) != 0)
                      {
                        /* second consecutive frame over the threshold for the
                         * same print, and not a byte-identical re-serve */
                        if (egis_verify_raw_ok (self->frame, idx))
                          {
                            Msg *m = g_new0 (Msg, 1);
                            m->kind = action == FPI_DEVICE_ACTION_VERIFY
                                      ? M_VERIFY_REPORT : M_IDENTIFY_REPORT;
                            m->score = MAX (score, cand_score);
                            m->idx = idx;
                            fp_dbg ("match confirmed by a second frame (%d, %d)",
                                    cand_score, score);
                            post_msg (dev, m);
                            finishing = TRUE;
                          }
                        else
                          {
                            fp_warn ("match not corroborated on the raw frame "
                                     "(flat-field baseline suspect) -- discarded");
                            cand_valid = FALSE;
                          }
                      }
                    else
                      {
                        cand_valid = TRUE;
                        cand_score = score;
                        cand_idx = idx;
                        memcpy (cand_raw, self->frame, EGIS_IMG);
                      }
                  }
                else
                  {
                    /* a frame below the threshold breaks the chain: a stale
                     * frame stands alone, a genuine press does not */
                    if (cand_valid)
                      fp_warn ("candidate frame (%d) not confirmed by the next "
                               "frame (%d) -- possible stale capture", cand_score, score);
                    cand_valid = FALSE;
                  }
              }
            }
          else if (var < EGIS0576_FINGER_OFF_VAR && saw_finger)
            {
              /* finger lifted without a confirmed match. An unconfirmed
               * candidate is NOT a match (see cand_valid above), so the
               * reported score is capped below the threshold. A press that
               * produced nothing scorable at all (every frame -1: coverage
               * under the gate) is a placement problem, not a failed
               * attempt -- ask for a retry instead of spending one of
               * fprintd's tries. */
              Msg *m = g_new0 (Msg, 1);
              if (action == FPI_DEVICE_ACTION_VERIFY)
                {
                  m->kind = M_VERIFY_REPORT;
                  m->score = best_score < 0 ? 0 : MIN (best_score, EGIS_THRESHOLD - 1);
                }
              else
                {
                  m->kind = M_IDENTIFY_REPORT;
                  m->idx = -1;
                }
              m->extract_fail = (best_score < 0);
              if (cand_valid)
                fp_warn ("finger lifted on an unconfirmed candidate (%d) -- no match",
                         cand_score);
              fp_dbg ("finger lifted, final best %d -> %s", best_score,
                      best_score < 0 ? "nothing scorable, retry" : "no match");
              post_msg (dev, m);
              finishing = TRUE;
            }
        }

      g_usleep (EGIS0576_POLL_SLEEP_US);
    }

  if (cancelled || g_atomic_int_get (&self->cancel))
    {
      Msg *m = g_new0 (Msg, 1);
      m->kind = M_ERROR;
      m->error = g_error_new (G_IO_ERROR, G_IO_ERROR_CANCELLED, "cancelled");
      post_msg (dev, m);
      return NULL;
    }

  if (action == FPI_DEVICE_ACTION_ENROLL)
    {
      Msg *m = g_new0 (Msg, 1);
      m->kind = M_ENROLL_DONE;
      m->size = egis_enroll_finish (&m->blob);
      post_msg (dev, m);
    }
  else
    {
      Msg *m = g_new0 (Msg, 1);
      m->kind = M_COMPLETE;
      post_msg (dev, m);
    }
  return NULL;
}

static void
start_capture (FpDevice *dev)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576 (dev);

  g_atomic_int_set (&self->cancel, FALSE);

  /* Fresh cancellable per capture (see the struct for ownership). The previous
   * one was released in finish_teardown after its worker was joined, so this
   * can only be NULL here; a leftover would mean a worker is still running,
   * which libfprint's one-action-at-a-time contract rules out. It is attached
   * to the channel BEFORE the thread exists so the worker never observes it
   * changing. */
  if (G_UNLIKELY (self->capture_cancellable != NULL))
    {
      /* A leftover means a worker is (or was) still running: an invariant
       * violation, not a state to continue from. But the action must not be
       * abandoned either -- libfprint would keep it as current forever and
       * every later call, close included, would fail BUSY. Fail it cleanly. */
      g_warn_if_reached ();
      fpi_device_action_error (dev, fpi_device_error_new_msg (FP_DEVICE_ERROR_BUSY,
                                                              "EH576: a capture is already running"));
      return;
    }
  self->capture_cancellable = g_cancellable_new ();
  egis_dev_set_cancellable (self->sensor, self->capture_cancellable);

  fpi_device_report_finger_status_changes (dev,
                                           FP_FINGER_STATUS_NEEDED,
                                           FP_FINGER_STATUS_NONE);
  self->thread = g_thread_new ("egis0576-capture", capture_thread, dev);
}

/* ------------------------------------------------------------------ */
/* Device vfuncs                                                      */
/* ------------------------------------------------------------------ */

static void egis0576_cancel (FpDevice *dev);   /* used by egis0576_close */

static void
egis0576_open (FpDevice *dev)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576 (dev);
  GError *error = NULL;

  if (egis_engine_init () != 0)
    {
      fpi_device_open_complete (dev,
                                fpi_device_error_new_msg (FP_DEVICE_ERROR_GENERAL,
                                                          "Failed to initialise Egis match engine"));
      return;
    }
  g_usb_device_claim_interface (fpi_device_get_usb_device (dev),
                                EGIS0576_INTF, 0, &error);
  if (error)
    {
      fpi_device_open_complete (dev, error);
      return;
    }

  self->sensor = egis_dev_new (fpi_device_get_usb_device (dev));
  egis_dev_set_calibration (self->sensor, self->dc_c_calibrated);
  if (!egis_dev_open (self->sensor, TRUE, &error))
    {
      g_clear_pointer (&self->sensor, egis_dev_free);
      g_usb_device_release_interface (fpi_device_get_usb_device (dev),
                                      EGIS0576_INTF, 0, NULL);
      fpi_device_open_complete (dev, error);
      return;
    }
  /* One-time per-device exposure calibration (the vendor's calibrate_gain),
   * before any capture and once per fprintd lifetime. A binary search over reg
   * 0x0f converges the no-finger frame mean to a fixed target, so the exposure is
   * the SAME on any EH576 unit — this is what makes the driver device-independent
   * (the baked init values belong to one unit). Validated on the reference unit:
   * reliable verify-match at ~7500. No finger is expected at open. Non-fatal
   * (on error the search is simply retried at the next open; see
   * egis_dev_calibrate). */
  if (!egis_dev_calibrate (self->sensor, &error))
    {
      fp_dbg ("exposure calibration skipped: %s", error ? error->message : "?");
      g_clear_error (&error);
    }
  self->dc_c_calibrated = egis_dev_get_calibration (self->sensor);
  g_atomic_int_set (&self->needs_reinit, FALSE);
  fpi_device_open_complete (dev, NULL);
}

static void
egis0576_close (FpDevice *dev)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576 (dev);
  GError *error = NULL;

  /* Normally unreachable: libfprint refuses close while an action is current,
   * and the worker only exists between start_capture and the terminal message
   * that completes the action (finish_teardown joins it first), so by the time
   * close can run there is no thread. Kept as a defensive backstop should that
   * ever change (a close racing a cancel/resume): the worker owns self->sensor,
   * so it must be stopped and joined before the session is freed, or it would
   * touch freed memory.
   *
   * If it ever IS reached while the worker is busy, the cancel below is seen
   * at the worker's next cancellation point and this join blocks the main
   * thread until it gets there. In the capture loop that is the end of the
   * current transfer sequence (a getframe, ~2.6 s worst case on a silent
   * sensor); inside worker_reinit it is the end of the vendor replay, which
   * is deliberately uncancellable as a unit -- up to ~25 records x 800 ms if
   * the sensor has stopped answering. The join cannot deadlock: the transport
   * completes transfers on a private GMainContext the worker iterates itself,
   * so it never needs this thread's default context (it did before v0.4.3,
   * see docs/worker-thread.md). Stalling fprintd's main loop for seconds is
   * still wrong, which is why the path must stay unreachable, and why it
   * warns rather than pretending to be free. */
  if (self->thread)
    {
      g_warn_if_reached ();              /* see above: must not happen */
      egis0576_cancel (dev);
      g_thread_join (self->thread);
      self->thread = NULL;
    }
  /* Joined (or never started): safe to detach and release the cancellable. */
  if (self->sensor)
    egis_dev_set_cancellable (self->sensor, NULL);
  g_clear_object (&self->capture_cancellable);
  g_clear_pointer (&self->sensor, egis_dev_free);
  g_usb_device_release_interface (fpi_device_get_usb_device (dev),
                                  EGIS0576_INTF, 0, &error);
  fpi_device_close_complete (dev, error);
}

static void
egis0576_enroll (FpDevice *dev)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576 (dev);

  self->enroll_count = 0;
  if (egis_enroll_begin () != 0)
    {
      fpi_device_enroll_complete (dev, NULL,
                                  fpi_device_error_new_msg (FP_DEVICE_ERROR_GENERAL,
                                                            "enroll begin failed"));
      return;
    }
  start_capture (dev);
}

static void
egis0576_verify (FpDevice *dev)
{
  FpPrint *print = NULL;

  fpi_device_get_verify_data (dev, &print);
  if (!load_one_print (print))
    {
      fpi_device_verify_complete (dev,
                                  fpi_device_error_new_msg (FP_DEVICE_ERROR_DATA_INVALID,
                                                            "Stored print has no valid egis0576 template"));
      return;
    }
  start_capture (dev);
}

static void
egis0576_identify (FpDevice *dev)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576 (dev);
  GPtrArray *prints = NULL;
  g_autofree const guint8 **blobs = NULL;
  g_autofree int *sizes = NULL;
  g_autoptr(GPtrArray) vars = g_ptr_array_new_with_free_func ((GDestroyNotify) g_variant_unref);
  guint n = 0, cap = (guint) MAX (egis_gallery_capacity (), 1);

  g_clear_pointer (&self->gallery_prints, g_ptr_array_unref);
  self->gallery_prints = g_ptr_array_new ();   /* borrowed refs, gallery order */

  fpi_device_get_identify_data (dev, &prints);
  if (prints && prints->len > cap)
    {
      /* fprintd passes every print of the user; matching only a prefix would
       * make the later fingers silently unusable for login. Fail loudly
       * instead (the vendor engine holds 5, the clean-room flavours 16). */
      g_clear_pointer (&self->gallery_prints, g_ptr_array_unref);
      fpi_device_identify_complete (dev,
                                    fpi_device_error_new_msg (FP_DEVICE_ERROR_DATA_INVALID,
                                                              "%u prints to match but this matcher holds at most %u; "
                                                              "delete some enrolled fingers", prints->len, cap));
      return;
    }
  blobs = g_new0 (const guint8 *, prints ? prints->len + 1 : 1);
  sizes = g_new0 (int, prints ? prints->len + 1 : 1);
  for (guint i = 0; prints && i < prints->len; i++)
    {
      FpPrint *print = g_ptr_array_index (prints, i);
      GVariant *var = NULL;
      gsize len = 0;
      const guint8 *blob = print_get_blob (print, &var, &len);
      if (blob && len)
        {
          g_ptr_array_add (vars, var);
          g_ptr_array_add (self->gallery_prints, print);
          blobs[n] = blob;
          sizes[n] = (int) len;
          n++;
        }
    }
  if (n == 0)
    {
      /* Empty gallery — fprintd's pre-enroll duplicate check with nothing
       * enrolled. Trivially "no match": finish without touching the sensor. */
      g_clear_pointer (&self->gallery_prints, g_ptr_array_unref);
      fpi_device_identify_report (dev, NULL, NULL, NULL);
      fpi_device_identify_complete (dev, NULL);
      return;
    }
  if (egis_gallery_load (blobs, sizes, n) != 0)
    {
      g_clear_pointer (&self->gallery_prints, g_ptr_array_unref);
      fpi_device_identify_complete (dev,
                                    fpi_device_error_new_msg (FP_DEVICE_ERROR_DATA_INVALID,
                                                              "No valid egis0576 templates to match"));
      return;
    }
  start_capture (dev);
}

static void
egis0576_cancel (FpDevice *dev)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576 (dev);

  fp_dbg ("cancelling");

  /* Order matters: the flag goes up first, then the cancellable is triggered.
   * The worker's transport polls the cancellable between transfers (it is never
   * handed to gusb, so nothing wakes the worker) and egis_dev_* then fail with
   * G_IO_ERROR_CANCELLED, which worker_cancelled treats as "check the flag";
   * g_cancellable_cancel is a full barrier, so a worker that sees the cancelled
   * state is guaranteed to also see the flag. Setting only the flag (as before)
   * left the worker to wait out whole transfer SEQUENCES -- on a sensor that
   * stopped answering, a failed getframe plus the re-init's wait_ready is
   * ~11 s, and a replay stalling mid-way is ~19 s (24 reads x 800 ms); the
   * cancellable is polled at every transfer boundary, so now it waits for at
   * most the one transfer SEQUENCE in flight (a getframe: ~2.6 s on a dead
   * sensor; the init replay, which is one unit: ~0.3 s; one readiness-poll
   * iteration: 850 ms -- see egis0576_proto.h; it is never handed to gusb --
   * aborting a URB wedges this sensor, see egis0576_proto.c). NULL when no
   * capture is running. Main thread only, like every other writer of these
   * fields. */
  g_atomic_int_set (&self->cancel, TRUE);
  if (self->capture_cancellable)
    g_cancellable_cancel (self->capture_cancellable);
}

static void
egis0576_suspend (FpDevice *dev)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576 (dev);

  fp_dbg ("suspend: flagging the sensor for re-init, cancelling in-flight capture");

  /* The sensor does not come back from s2idle usable (it stays powered, but the
   * capture pipeline is wedged). Flag it so the next capture re-runs the vendor
   * bring-up in the worker. */
  g_atomic_int_set (&self->needs_reinit, TRUE);

  /* Cancel the running action exactly as egismoc does (egismoc.c:1571-1578):
   * stop our worker via egis0576_cancel — which sets self->cancel AND cancels
   * the per-capture cancellable, so the worker stops at the next transfer
   * boundary instead of finishing a whole re-init sequence — and cancel the
   * device cancellable so libfprint/fprintd sees the
   * action end. Complete with NULL: we are NOT promising the *current* action
   * survives (we are cancelling it), and NULL takes suspend_complete's immediate
   * return path (fpi-device.c:1815-1823) without libfprint ALSO cancelling with
   * its own FP_DEVICE_ERROR_BUSY. fprintd re-issues verify/identify on resume,
   * which re-initialises the sensor (worker_reinit) in the worker thread. Do NO
   * USB here — this is the fprintd main loop. */
  egis0576_cancel (dev);
  g_cancellable_cancel (fpi_device_get_cancellable (dev));
  fpi_device_suspend_complete (dev, NULL);
}

static void
egis0576_resume (FpDevice *dev)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576 (dev);

  /* Defensive: also flag here in case an action somehow survived to resume
   * (keep-alive / unfreeze race). No USB — this is the main loop. The worker
   * consumes the flag; if a worker is mid-getframe on a dead sensor, its
   * bounded getframe-failure path re-initialises the sensor and retries. In
   * the normal flow suspend cancelled the action, so current_action == NONE
   * and libfprint completes resume itself without ever calling this
   * (fpi-device.c:1655-1666). */
  g_atomic_int_set (&self->needs_reinit, TRUE);
  fpi_device_resume_complete (dev, NULL);
}

/* ------------------------------------------------------------------ */
/* Driver data                                                        */
/* ------------------------------------------------------------------ */

static const FpIdEntry id_table[] = {
  { .vid = 0x1c7a, .pid = 0x0576, },
  { .vid = 0, .pid = 0, },
};

static void
fpi_device_egis0576_init (FpDeviceEgis0576 *self)
{
  self->dc_c_calibrated = -1;
}

static void
fpi_device_egis0576_class_init (FpDeviceEgis0576Class *klass)
{
  FpDeviceClass *dev_class = FP_DEVICE_CLASS (klass);

  dev_class->id = "egis0576";
  dev_class->full_name = "Egis Technology Inc. (aka. LighTuning) EH576";
  dev_class->type = FP_DEVICE_TYPE_USB;
  dev_class->id_table = id_table;
  dev_class->scan_type = FP_SCAN_TYPE_PRESS;
  dev_class->nr_enroll_stages = EGIS0576_ENROLL_STAGES;

  /* Capacitive sensor is safe to stream continuously; opt out of the
   * temperature model so long enrollments / lock-screen polling are not
   * cancelled with FP_DEVICE_ERROR_TOO_HOT (also grants ALWAYS_ON). */
  dev_class->temp_hot_seconds = -1;

  dev_class->open = egis0576_open;
  dev_class->close = egis0576_close;
  dev_class->enroll = egis0576_enroll;
  dev_class->verify = egis0576_verify;
  dev_class->identify = egis0576_identify;
  dev_class->cancel = egis0576_cancel;
  dev_class->suspend = egis0576_suspend;
  dev_class->resume = egis0576_resume;

  fpi_device_class_auto_initialize_features (dev_class);
}
