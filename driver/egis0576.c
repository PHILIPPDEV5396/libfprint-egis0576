/*
 * Egis Technology Inc. (aka. LighTuning) EH576 (1c7a:0576) driver for libfprint
 *
 * A tiny press-type secure image sensor (70x57 px). The sensor is a TLS-PSK
 * peer: on open it is the TLS *client* and we (the host) are the TLS *server*.
 * After the handshake the EGIS/SIGE command protocol — including image capture —
 * runs encrypted over the USB bulk endpoints. The secure channel + Windows-exact
 * init/calibration replay + GetFrame live in drivers/egis0576/egis0576_tls.c.
 *
 * Captured 70x57 frames are matched host-side with Egis' own feature extractor +
 * matcher, reverse-engineered from the Windows driver and reimplemented as native
 * C (drivers/egis0576/egis_engine.*). Templates are stored as opaque blobs in
 * each print's fpi-data (plaintext, like every other libfprint driver).
 *
 * The TLS transport uses blocking synchronous USB, which must not run in the
 * fprintd main loop (nested-mainloop re-entrancy hangs it), so the capture loop
 * runs in a worker thread and marshals its results back with g_idle_add.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 */

#define FP_COMPONENT "egis0576"

#include "egis0576.h"
#include "egis0576/egis_engine.h"
#include "egis0576/egis0576_tls.h"
#include "drivers_api.h"

#define EGIS0576_ENROLL_STAGES 12
/* In TLS/Windows-calibrated mode the no-finger frame has a fixed-pattern
 * variance around ~163; a real finger pushes it well past 300. Hysteresis: */
#define EGIS0576_FINGER_ON_VAR  250.0
#define EGIS0576_FINGER_OFF_VAR 215.0
#define EGIS0576_POLL_SLEEP_US  5000    /* small gap between captures */
#define EGIS0576_BASELINE_FRAMES 8      /* no-finger frames averaged into the flat-field baseline */
#define EGIS0576_BASELINE_MAX_VAR 210.0 /* stricter than FINGER_OFF but with headroom for the
                                          * calibrated-gain no-finger level; a hovering finger would
                                          * otherwise contaminate the baseline (true no-finger ~163) */

typedef enum {
  PH_AWAIT_ON,     /* waiting for a finger */
  PH_AWAIT_OFF,    /* waiting for the finger to lift before the next touch */
} CapturePhase;

struct _FpDeviceEgis0576
{
  FpDevice      parent;

  EgisTls      *tls;            /* the encrypted channel to the sensor */

  /* capture runs in a worker thread (blocking sync USB must not run in the
   * main loop); results are marshalled back with g_idle_add. */
  GThread      *thread;
  gint          cancel;         /* atomic; set on main thread, read by worker */
  gint          session_stale;  /* atomic; set by suspend/resume on the main
                                 * thread, consumed by the worker: the sensor
                                 * dropped its TLS-PSK session across s2idle (USB
                                 * stays powered but the session dies), so it must
                                 * be re-established before the next capture */

  guint8        frame[EGIS_TLS_IMG];
  guint8        corrected[EGIS_TLS_IMG];    /* flat-field corrected frame */
  guint8        baseline[EGIS_TLS_IMG];     /* per-session no-finger reference */
  gboolean      have_baseline;
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

  for (gsize i = 0; i < EGIS_TLS_IMG; i++)
    sum += buf[i];
  mean = sum / (gdouble) EGIS_TLS_IMG;
  for (gsize i = 0; i < EGIS_TLS_IMG; i++)
    {
      gdouble d = (gdouble) buf[i] - mean;
      var += d * d;
    }
  return var / (gdouble) EGIS_TLS_IMG;
}

/* Per-frame preprocessing (egis_preprocess, from egis_preprocess.c) is now the
 * BYTE-EXACT translation of the Windows pipeline (min-subtract -> invert ->
 * auto-brightness -> Otsu stretch to black-level 0x8c -> vertical flip). It is
 * declared in egis_engine.h and validated on fp_final (genuine 8/10 preserved,
 * impostor 0/12 — my earlier hand-written approximation destroyed minutiae, 0/10).
 * The min-subtract + Otsu-stretch normalise per-session brightness/contrast so a
 * template enrolled in one TLS session matches a probe from another. */

/* Process-global per-boot flat-field baseline. The sensor's fixed-pattern noise
 * (~163 var, no finger) differs per power-cycle; subtracting it per-pixel makes
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
static guint8   g_baseline[EGIS_TLS_IMG];
static gboolean g_baseline_valid = FALSE;

typedef struct { guint32 acc[EGIS_TLS_IMG]; int count; } BaselineAcc;

/* Feed one no-finger frame into the accumulator; publishes the global baseline
 * once EGIS0576_BASELINE_FRAMES have been gathered. No-op once already valid. */
static void
baseline_feed (BaselineAcc *b, const guint8 *frame)
{
  int i;

  if (g_baseline_valid)
    return;
  for (i = 0; i < EGIS_TLS_IMG; i++)
    b->acc[i] += frame[i];
  b->count++;
  if (b->count >= EGIS0576_BASELINE_FRAMES)
    {
      for (i = 0; i < EGIS_TLS_IMG; i++)
        g_baseline[i] = (guint8) (b->acc[i] / b->count);
      g_baseline_valid = TRUE;
      fp_dbg ("flat-field baseline built from %d no-finger frames (cached for boot)", b->count);
    }
}

/* Apply the per-boot flat-field: out[i] = clamp(raw[i] - baseline[i] + mean).
 * Removes per-pixel fixed-pattern while preserving the DC level (min-subtract in
 * egis_preprocess re-zeroes DC afterwards). No-op until a baseline exists. */
static void
flat_field (const guint8 *raw, guint8 *out)
{
  long sum = 0;
  int i, mean;

  if (!g_baseline_valid)
    {
      memcpy (out, raw, EGIS_TLS_IMG);
      return;
    }
  for (i = 0; i < EGIS_TLS_IMG; i++)
    sum += g_baseline[i];
  mean = (int) (sum / EGIS_TLS_IMG);
  for (i = 0; i < EGIS_TLS_IMG; i++)
    {
      int v = (int) raw[i] - (int) g_baseline[i] + mean;
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

/* Re-establish ONLY the TLS-PSK session on the already-claimed, still-enumerated
 * device — the s2idle recovery. The USB device stayed powered and the interface
 * is still claimed, but the sensor silently dropped its TLS session, so every
 * field of the live EgisTls (keys, record sequence counters, reassembly buffer)
 * is stale. free+new+open discards all of it and does a fresh flush_in +
 * 0x21/9 trigger + PSK handshake + Windows init replay.
 *
 * Deliberately does NOT: re-claim the interface (still claimed -> EBUSY),
 * re-init the host match engine (host state, intact), re-run gain calibration
 * (persists in the powered sensor; the init replay never writes reg 0x0f, and
 * calibration needs a no-finger window we can't guarantee on resume), or
 * recapture the flat-field baseline (gain-tied, per-boot, still valid) — so
 * cross-boot matching is preserved.
 *
 * Blocks ~1 s of synchronous gusb — safe ONLY because it runs in the capture
 * worker thread, never on the fprintd main loop. On failure the session is left
 * flagged stale so the next worker eagerly re-handshakes instead of tripping
 * over a dead session. */
static gboolean
worker_reinit (FpDeviceEgis0576 *self, GError **error)
{
  GUsbDevice *usb = fpi_device_get_usb_device (FP_DEVICE (self));

  g_clear_pointer (&self->tls, egis_tls_free);
  self->tls = egis_tls_new (usb);
  if (!egis_tls_open (self->tls, error))
    return FALSE;                            /* leave session_stale TRUE */
  g_atomic_int_set (&self->session_stale, FALSE);
  return TRUE;
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
  gboolean saw_finger = FALSE;   /* verify/identify: a press is in progress */
  int best_score = -1;           /* verify/identify: best score seen this press */
  guint8 ffframe[EGIS_TLS_IMG];  /* flat-fielded frame (baseline-subtracted) */
  BaselineAcc bl = { { 0 }, 0 }; /* opportunistic no-finger baseline accumulator */

  while (!finishing && !g_atomic_int_get (&self->cancel))
    {
      GError *error = NULL;
      gdouble var;
      const guint8 *probe;

      /* Eager fast-path: a suspend/resume since the last frame dropped the
       * sensor's TLS session (flag set by the vfuncs). Re-handshake HERE (worker
       * thread, blocking-safe) before getframe so we don't burn a getframe
       * timeout on a dead session. Honour a concurrent cancel first: suspend
       * cancels the action, and we must not run ~1 s of USB in the fragile
       * post-resume window when we're being torn down. */
      if (g_atomic_int_get (&self->session_stale) &&
          !g_atomic_int_get (&self->cancel))
        {
          if (!worker_reinit (self, &error))
            {
              Msg *m = g_new0 (Msg, 1);
              m->kind = M_ERROR;
              m->error = error;              /* -> fpi_device_action_error -> password fallback */
              post_msg (dev, m);
              return NULL;
            }
        }

      if (!egis_tls_getframe (self->tls, self->frame, &error))
        {
          /* Backstop that also covers the idle-at-suspend case (no vfunc fired,
           * so session_stale was FALSE and the eager path was skipped) and any
           * unfreeze race: a dead session makes getframe return an error in
           * bounded time (read_record now has a wall-clock deadline). If we were
           * cancelled (suspend), fall through so the loop exits with a clean
           * cancel instead of running a re-handshake. Otherwise re-establish the
           * session once, in this worker thread, and retry. Give up — with the
           * real device error -> password fallback — only if the re-handshake or
           * the retry getframe also fails. */
          if (g_atomic_int_get (&self->cancel))
            {
              g_clear_error (&error);
              break;
            }
          fp_dbg ("getframe failed (%s); re-establishing TLS session and retrying",
                  error ? error->message : "?");
          g_clear_error (&error);

          if (!worker_reinit (self, &error) ||
              !egis_tls_getframe (self->tls, self->frame, &error))
            {
              Msg *m = g_new0 (Msg, 1);
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
        baseline_feed (&bl, self->frame);

      /* (1) per-boot flat-field: subtract the fixed-pattern baseline so a template
       * enrolled one boot matches a probe from another (without it, cross-boot=0);
       * a no-op until the baseline exists. (2) BYTE-EXACT Windows preprocessing
       * (min-subtract, invert, auto-brightness, Otsu-stretch, flip) — validated on
       * fp_final (8/10). Applied identically to enroll + verify + identify. */
      flat_field (self->frame, ffframe);
      egis_preprocess (ffframe, self->corrected);
      probe = self->corrected;

      if (action == FPI_DEVICE_ACTION_ENROLL)
        {
          /* one add per finger press: capture on-press, then wait for lift */
          if (phase == PH_AWAIT_ON)
            {
              if (var >= EGIS0576_FINGER_ON_VAR)
                {
                  Msg *m = g_new0 (Msg, 1);
                  int prog = 0;
                  int r;
                  r = egis_enroll_add (probe, &prog);
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
                      m->kind = M_ENROLL_RETRY;
                      m->stage = self->enroll_count;
                    }
                  post_msg (dev, m);
                  phase = PH_AWAIT_OFF;
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
              if (action == FPI_DEVICE_ACTION_VERIFY)
                {
                  int score = egis_verify (probe, 0);
                  if (score > best_score)
                    best_score = score;
                  fp_dbg ("verify score %d (best %d, var %.0f)", score, best_score, var);
                  if (score >= EGIS_THRESHOLD)
                    {
                      Msg *m = g_new0 (Msg, 1);
                      m->kind = M_VERIFY_REPORT;
                      m->score = score;
                      post_msg (dev, m);
                      finishing = TRUE;
                    }
                }
              else /* IDENTIFY */
                {
                  int idx = -1;
                  int s = egis_identify (probe, &idx);
                  if (s > best_score)
                    best_score = s;
                  fp_dbg ("identify score %d idx %d (best %d, var %.0f)", s, idx, best_score, var);
                  if (idx >= 0)
                    {
                      Msg *m = g_new0 (Msg, 1);
                      m->kind = M_IDENTIFY_REPORT;
                      m->idx = idx;
                      post_msg (dev, m);
                      finishing = TRUE;
                    }
                }
            }
          else if (var < EGIS0576_FINGER_OFF_VAR && saw_finger)
            {
              /* finger lifted without any frame crossing the threshold */
              Msg *m = g_new0 (Msg, 1);
              if (action == FPI_DEVICE_ACTION_VERIFY)
                {
                  m->kind = M_VERIFY_REPORT;
                  m->score = best_score < 0 ? 0 : best_score;
                }
              else
                {
                  m->kind = M_IDENTIFY_REPORT;
                  m->idx = -1;
                }
              fp_dbg ("finger lifted, final best %d -> no match", best_score);
              post_msg (dev, m);
              finishing = TRUE;
            }
        }

      g_usleep (EGIS0576_POLL_SLEEP_US);
    }

  if (g_atomic_int_get (&self->cancel))
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
  fpi_device_report_finger_status_changes (dev,
                                           FP_FINGER_STATUS_NEEDED,
                                           FP_FINGER_STATUS_NONE);
  self->thread = g_thread_new ("egis0576-capture", capture_thread, dev);
}

/* ------------------------------------------------------------------ */
/* Device vfuncs                                                      */
/* ------------------------------------------------------------------ */

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

  self->tls = egis_tls_new (fpi_device_get_usb_device (dev));
  if (!egis_tls_open (self->tls, &error))
    {
      g_clear_pointer (&self->tls, egis_tls_free);
      g_usb_device_release_interface (fpi_device_get_usb_device (dev),
                                      EGIS0576_INTF, 0, NULL);
      fpi_device_open_complete (dev, error);
      return;
    }
  /* One-time per-device gain calibration (Windows calibrate_gain), before any
   * capture and once per fprintd lifetime. A binary search over the fine-gain
   * register converges the no-finger frame mean to a fixed target, so the exposure
   * is the SAME on any EH576 unit — this is what makes the driver device-independent
   * (the baked init gain is one unit's value). Validated on the reference unit:
   * reliable verify-match at ~7500. No finger is expected at open. Non-fatal (keeps
   * the baked gain on error). Set EGIS0576_NO_CALIBRATE=1 to fall back to fixed gain
   * on a unit where calibration misbehaves. */
  if (!g_getenv ("EGIS0576_NO_CALIBRATE") && !egis_tls_calibrate_gain (self->tls, &error))
    {
      fp_dbg ("gain calibration skipped: %s", error ? error->message : "?");
      g_clear_error (&error);
    }
  g_atomic_int_set (&self->session_stale, FALSE);
  fpi_device_open_complete (dev, NULL);
}

static void
egis0576_close (FpDevice *dev)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576 (dev);
  GError *error = NULL;

  /* A capture worker may still be running (close racing a cancel/resume). It owns
   * self->tls, so stop and join it before freeing the session or it would touch
   * freed memory. The join is bounded: the worker is at most one read_record
   * deadline away from noticing self->cancel and returning. */
  if (self->thread)
    {
      g_atomic_int_set (&self->cancel, TRUE);
      g_thread_join (self->thread);
      self->thread = NULL;
    }
  g_clear_pointer (&self->tls, egis_tls_free);
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
  const guint8 *blobs[5];
  int sizes[5];
  g_autoptr(GPtrArray) vars = g_ptr_array_new_with_free_func ((GDestroyNotify) g_variant_unref);
  guint n = 0;

  g_clear_pointer (&self->gallery_prints, g_ptr_array_unref);
  self->gallery_prints = g_ptr_array_new ();   /* borrowed refs, gallery order */

  fpi_device_get_identify_data (dev, &prints);
  if (prints && prints->len > 5)
    fp_warn ("identify gallery has %u prints but the engine holds 5; "
             "only the first 5 are matched", prints->len);
  for (guint i = 0; prints && i < prints->len && n < 5; i++)
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
  g_atomic_int_set (&self->cancel, TRUE);
}

static void
egis0576_suspend (FpDevice *dev)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576 (dev);

  fp_dbg ("suspend: invalidating TLS session, cancelling in-flight capture");

  /* The TLS-PSK session will not survive s2idle (sensor stays powered but drops
   * the session). Flag it so the next capture re-handshakes in the worker. */
  g_atomic_int_set (&self->session_stale, TRUE);

  /* Cancel the running action exactly as egismoc does (egismoc.c:1571-1578):
   * stop our worker (sets self->cancel -> worker exits and posts a cancelled
   * M_ERROR) and cancel the device cancellable so libfprint/fprintd sees the
   * action end. Complete with NULL: we are NOT promising the *current* action
   * survives (we are cancelling it), and NULL takes suspend_complete's immediate
   * return path (fpi-device.c:1815-1823) without libfprint ALSO cancelling with
   * its own FP_DEVICE_ERROR_BUSY. fprintd re-issues verify/identify on resume,
   * which re-establishes the session in the worker thread. Do NO USB here — this
   * is the fprintd main loop. */
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
   * consumes the flag; if a worker is mid-getframe on the dead session, its
   * bounded getframe-failure retry re-handshakes. In the normal flow suspend
   * cancelled the action, so current_action == NONE and libfprint completes
   * resume itself without ever calling this (fpi-device.c:1655-1666). */
  g_atomic_int_set (&self->session_stale, TRUE);
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
