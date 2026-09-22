/*
 * Egis Technology Inc. (aka. LighTuning) EH576 (1c7a:0576) driver for libfprint
 *
 * Copyright (C) 2026 Philipp Oster
 *
 * A tiny press-type capacitive sensor (70x57 px, no hardware finger detect),
 * driven with the vendor's own plaintext EGIS/SIGE command protocol over the
 * USB bulk endpoints -- the same one its Windows driver speaks. The transport,
 * the vendor init/calibration replay and GetFrame live in
 * egis0576/egis0576_proto.c as FpiSsm machines over FpiUsbTransfer.
 *
 * The driver polls frames while an action runs, tells finger-on from
 * finger-off by frame variance, and matches host-side with the correlation
 * matcher behind egis0576/egis_engine.h (a per-instance handle). Matching
 * runs in a GTask thread; everything else -- USB, finger detection, the
 * flat-field baseline, the enrolment state -- runs on the device main loop,
 * one FpiSsm per action.
 *
 * Templates are stored as opaque bytes in each print's fpi-data (plaintext,
 * like every other libfprint driver).
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
#include <stdlib.h>

#define EGIS0576_ENROLL_STAGES EGIS_ENROLL_STAGES   /* one source: egis_engine.h */
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
 * a real finger pushes it well past 300. Hysteresis (integers: the comparison
 * is done on the exact integer statistic, see frame_variance, so the
 * finger-on/off decision is the same on every architecture and a recorded
 * test replays the same frame sequence everywhere): */
#define EGIS0576_FINGER_ON_VAR  250
#define EGIS0576_FINGER_OFF_VAR 215
/* Gap between frames. The sensor delivers a frame in ~10 ms, so the loop is
 * paced by these: with no finger on the sensor 30 ms (a press is noticed
 * within one gap, and an idle lock screen costs ~3 % of a core instead of the
 * ~9 % that polling flat out did); while a finger is down 5 ms, so the
 * ~250 ms in which a static press keeps its contrast yields ~17 frames for
 * the matcher and the lift is seen promptly. */
#define EGIS0576_POLL_GAP_IDLE_MS   30
#define EGIS0576_POLL_GAP_FINGER_MS 5
#define EGIS0576_BASELINE_FRAMES 8      /* no-finger frames averaged into the flat-field baseline */
#define EGIS0576_BASELINE_MAX_VAR 210   /* stricter than FINGER_OFF but with headroom for the
                                          * calibrated-gain no-finger level; a hovering finger would
                                          * otherwise contaminate the baseline (true no-finger ~140) */

typedef enum {
  PH_AWAIT_ON,     /* waiting for a finger */
  PH_AWAIT_OFF,    /* waiting for the finger to lift before the next touch */
} CapturePhase;

/* The flat-field baseline accumulator (see baseline_feed). */
typedef struct {
  guint32 acc[EGIS_IMG];
  int     count;
} BaselineAcc;

/* The prints of a verify/identify action, decoded, for the gallery load that
 * runs off the main loop at the start of the capture (load_thread). */
typedef struct {
  EgisEngine    *engine;        /* borrowed, exclusively the task's while it runs */
  GPtrArray     *vars;          /* the GVariants the blobs point into (owned refs) */
  const guint8 **blobs;
  int           *sizes;
  int            n;
} LoadJob;

/* One probe frame handed to the matcher (match_thread): a snapshot of the
 * flat-fielded and the raw frame, the action, and what the two-frame
 * confirmation needs; the results come back in the same struct. */
typedef struct {
  EgisEngine     *engine;       /* borrowed, exclusively the task's while it runs */
  FpiDeviceAction action;
  guint8          probe[EGIS_IMG];
  guint8          raw[EGIS_IMG];
  gboolean        check_raw;    /* a candidate for cand_idx is held, and this frame differs from it */
  int             cand_idx;
  /* results */
  int             score;        /* verify/identify */
  int             idx;          /* identify: gallery index, -1 = none */
  gboolean        raw_ok;       /* raw corroboration, iff check_raw and score >= threshold for cand_idx */
  int             ret;          /* enrol: egis_enroll_add's code */
} MatchJob;

struct _FpDeviceEgis0576
{
  FpDevice      parent;

  EgisDev      *sensor;         /* the plaintext channel to the sensor, open..close */
  EgisEngine   *engine;         /* the host matcher (egis_engine.h), per instance */

  gboolean      needs_reinit;   /* set by suspend/resume: the sensor does not
                                 * reliably keep its bring-up state across s2idle
                                 * (USB stays powered, but the capture pipeline
                                 * comes back wedged), so it is re-initialised
                                 * before the next frame */
  /* Suspend with an action running (see egis0576_suspend): the capture
   * machine parks at its next transfer boundary -- nothing in flight, nothing
   * scheduled -- and only then is the suspend completed; resume (or a cancel)
   * restarts it at the loop head. */
  gboolean      suspending;     /* the suspend vfunc ran; park at the next boundary */
  gboolean      parked;         /* parked: waiting for resume or cancel */

  /* Per instance, i.e. per fprintd process: the flat-field baseline (see
   * baseline_feed) and the exposure calibration the first open() found,
   * handed to every later EgisDev the driver opens. */
  guint8        baseline[EGIS_IMG];     /* no-finger reference, valid iff have_baseline */
  gboolean      have_baseline;
  int           dc_c_calibrated;        /* -1 until the first open() calibrated */

  /* The running action's capture: its machine and state. */
  FpiSsm         *ssm;
  FpiDeviceAction action;
  LoadJob        *load;         /* verify/identify: the gallery to load first */
  guint8          frame[EGIS_IMG];      /* the raw frame just captured */
  guint8          ffframe[EGIS_IMG];    /* its flat-fielded version, the matcher's probe */
  guint64         var;          /* its variance statistic (frame_variance) */
  BaselineAcc     bl;           /* opportunistic no-finger baseline accumulator */
  CapturePhase    phase;
  gboolean        finger_present;       /* as last reported */
  gboolean        frame_retried;        /* a frame failure was already answered with a re-init */
  guint           enroll_count;
  int             settle_tries;         /* enrol: coverage refusals on the current press */
  gboolean        saw_finger;           /* verify/identify: a press is in progress */
  int             best_score;           /* verify/identify: best score seen this press */
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
  gboolean        cand_valid;
  int             cand_score, cand_idx;
  guint8          cand_raw[EGIS_IMG];

  /* identify: prints in gallery order (borrowed refs), for result mapping */
  GPtrArray    *gallery_prints;
};
G_DECLARE_FINAL_TYPE (FpDeviceEgis0576, fpi_device_egis0576, FPI, DEVICE_EGIS0576, FpDevice);
G_DEFINE_TYPE (FpDeviceEgis0576, fpi_device_egis0576, FP_TYPE_DEVICE);

/* ------------------------------------------------------------------ */
/* Frame statistics                                                   */
/* ------------------------------------------------------------------ */

/* The frame's variance times EGIS_IMG^2, exactly, in integers (the sums fit:
 * 3990 x 255^2 < 2^32, their squares < 2^64). var_ge (v, t) is then
 * "variance >= t" with no floating point anywhere in the decision. */
static guint64
frame_variance (const guint8 *buf)
{
  guint64 sum = 0, sumsq = 0;

  for (gsize i = 0; i < EGIS_IMG; i++)
    {
      sum += buf[i];
      sumsq += (guint64) buf[i] * buf[i];
    }
  return (guint64) EGIS_IMG * sumsq - sum * sum;
}

static inline gboolean
var_ge (guint64 v, guint64 t)
{
  return v >= t * EGIS_IMG * EGIS_IMG;
}

/* The variance itself, for the log. */
static inline gdouble
var_dbl (FpDeviceEgis0576 *self)
{
  return (gdouble) self->var / ((gdouble) EGIS_IMG * EGIS_IMG);
}

/* Per-instance flat-field baseline. The sensor's fixed-pattern noise
 * (~140 var, no finger) differs per power-cycle; subtracting it per-pixel makes
 * frames comparable ACROSS boots (without it a template enrolled one boot scores
 * exactly 0 against a probe from another boot, though within-boot it matches
 * up to ~19000). It is collected OPPORTUNISTICALLY from no-finger frames during
 * any operation (before the first press, and between enroll presses) and cached
 * for the fprintd process lifetime (~= this boot). This is what makes the
 * "official" GNOME flow work: GNOME says "place your finger" immediately with no
 * dedicated finger-off window, so a blocking pre-capture would just hang; instead
 * the baseline builds itself from the natural no-finger moments and, once found,
 * is reused by every later enroll/verify without re-capturing. */

/* Feed one no-finger frame into the accumulator; publishes the baseline once
 * EGIS0576_BASELINE_FRAMES have been gathered. No-op once already valid. */
static void
baseline_feed (FpDeviceEgis0576 *self, const guint8 *frame)
{
  BaselineAcc *b = &self->bl;
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
 * Removes per-pixel fixed-pattern while preserving the DC level. No-op until
 * a baseline exists. */
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

/* Template storage ("fpi-data"): a (qay) tuple, format version + the bytes
 * egis_enroll_finish() produced, which are opaque to the driver. The version
 * is bumped whenever the engine's bytes change meaning, so a print enrolled
 * by an older build (or by another matcher flavour, which shares nothing but
 * the frame size) fails cleanly with FP_DEVICE_ERROR_DATA_INVALID and the
 * user re-enrols, instead of being scored on the wrong scale. Version 1 was
 * a bare 'ay' with a 16-byte header inside it (v0.4.x). */
#define EGIS0576_PRINT_VERSION 2

static GVariant *
print_wrap_blob (const guint8 *blob, gsize len)
{
  GVariant *data = g_variant_new_fixed_array (G_VARIANT_TYPE_BYTE, blob, len, 1);

  return g_variant_new ("(q@ay)", (guint16) EGIS0576_PRINT_VERSION, data);
}

static const guint8 *
print_get_blob (FpPrint *print, GVariant **var_out, gsize *len)
{
  g_autoptr(GVariant) var = NULL;
  GVariant *data = NULL;
  guint16 version = 0;

  g_object_get (print, "fpi-data", &var, NULL);
  if (!var || !g_variant_is_of_type (var, G_VARIANT_TYPE ("(qay)")))
    return NULL;
  g_variant_get (var, "(q@ay)", &version, &data);
  if (version != EGIS0576_PRINT_VERSION)
    {
      g_variant_unref (data);
      return NULL;
    }
  *var_out = data;                       /* the caller owns this ref */
  return g_variant_get_fixed_array (data, len, 1);
}

static void
load_job_free (LoadJob *job)
{
  g_clear_pointer (&job->vars, g_ptr_array_unref);
  g_free (job->blobs);
  g_free (job->sizes);
  g_free (job);
}

/* Decode the templates of @prints into a LoadJob (NULL if none is valid). The
 * prints that decoded are listed, in gallery order, in self->gallery_prints. */
static LoadJob *
load_job_new (FpDeviceEgis0576 *self, GPtrArray *prints)
{
  LoadJob *job = g_new0 (LoadJob, 1);

  g_clear_pointer (&self->gallery_prints, g_ptr_array_unref);
  self->gallery_prints = g_ptr_array_new ();   /* borrowed refs, gallery order */
  job->engine = self->engine;
  job->vars = g_ptr_array_new_with_free_func ((GDestroyNotify) g_variant_unref);
  job->blobs = g_new0 (const guint8 *, prints->len + 1);
  job->sizes = g_new0 (int, prints->len + 1);
  for (guint i = 0; i < prints->len; i++)
    {
      FpPrint *print = g_ptr_array_index (prints, i);
      GVariant *var = NULL;
      gsize len = 0;
      const guint8 *blob = print_get_blob (print, &var, &len);

      if (blob && len)
        {
          g_ptr_array_add (job->vars, var);
          g_ptr_array_add (self->gallery_prints, print);
          job->blobs[job->n] = blob;
          job->sizes[job->n] = (int) len;
          job->n++;
        }
      else
        {
          g_clear_pointer (&var, g_variant_unref);   /* decoded, but empty */
        }
    }
  if (job->n == 0)
    {
      load_job_free (job);
      return NULL;
    }
  return job;
}

/* ------------------------------------------------------------------ */
/* Matcher threads                                                    */
/* ------------------------------------------------------------------ */

/* The engine handle is used from these thread functions only while their
 * GTask runs, and from the main loop only between actions (enroll begin /
 * finish) or after the task returned, so it is never touched concurrently.
 * The functions get a heap snapshot and never see the FpDevice. */

static void
load_thread (GTask *task, gpointer source, gpointer task_data, GCancellable *cancellable)
{
  LoadJob *job = task_data;

  if (g_task_return_error_if_cancelled (task))
    return;
  if (egis_gallery_load (job->engine, job->blobs, job->sizes, job->n) != 0)
    {
      g_task_return_new_error (task, FP_DEVICE_ERROR, FP_DEVICE_ERROR_DATA_INVALID,
                               "Stored print has no valid egis0576 template");
      return;
    }
  g_task_return_boolean (task, TRUE);
}

static void
match_thread (GTask *task, gpointer source, gpointer task_data, GCancellable *cancellable)
{
  MatchJob *j = task_data;

  if (g_task_return_error_if_cancelled (task))
    return;
  if (j->action == FPI_DEVICE_ACTION_ENROLL)
    {
      j->ret = egis_enroll_add (j->engine, j->probe, NULL);
    }
  else if (j->action == FPI_DEVICE_ACTION_VERIFY)
    {
      j->idx = 0;
      j->score = egis_verify (j->engine, j->probe, 0);
    }
  else
    {
      j->idx = -1;
      j->score = egis_identify (j->engine, j->probe, &j->idx);
    }
  if (j->action != FPI_DEVICE_ACTION_ENROLL && j->check_raw &&
      j->score >= EGIS_THRESHOLD && j->idx == j->cand_idx)
    j->raw_ok = egis_verify_raw_ok (j->engine, j->raw, j->idx) != 0;
  g_task_return_boolean (task, TRUE);
}

/* ------------------------------------------------------------------ */
/* Capture                                                            */
/* ------------------------------------------------------------------ */

enum {
  CAP_LOAD,          /* verify/identify: the gallery precompute, off the main loop */
  CAP_REINIT,        /* the loop head: bring the sensor up again first if flagged */
  CAP_FRAME,         /* one frame */
  CAP_RECOVER,       /* the frame failed: re-init once, then retry it */
  CAP_PROCESS,       /* finger detection, baseline, matching */
  CAP_NUM_STATES,
};

/* Fail @ssm with G_IO_ERROR_CANCELLED iff the action was cancelled. The
 * transport's machines do the same at their own boundaries; here it is the
 * loop's boundary. */
static gboolean
capture_cancelled (FpiSsm *ssm, FpDevice *dev)
{
  if (!fpi_device_action_is_cancelled (dev))
    return FALSE;
  fpi_ssm_mark_failed (ssm, g_error_new_literal (G_IO_ERROR, G_IO_ERROR_CANCELLED,
                                                 "cancelled"));
  return TRUE;
}

/* Fail the capture with @error (transfer full) -- as G_IO_ERROR_CANCELLED
 * if the action was cancelled meanwhile, whatever the transport reported:
 * a cancelled action must never be mistaken for a sensor failure. */
static void
capture_fail (FpDeviceEgis0576 *self, GError *error)
{
  if (fpi_device_action_is_cancelled (FP_DEVICE (self)) &&
      !g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    {
      g_error_free (error);
      error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_CANCELLED, "cancelled");
    }
  fpi_ssm_mark_failed (self->ssm, error);
}

static void
capture_next (FpDeviceEgis0576 *self, FpiSsm *ssm)
{
  int gap = self->finger_present ? EGIS0576_POLL_GAP_FINGER_MS : EGIS0576_POLL_GAP_IDLE_MS;

  /* Under umockdev the frames come from a recording, and the gap would only
   * make its replay take as long as the session that produced it; the URB
   * sequence is the same with or without it. */
  if (fpi_device_emulation_mode_enabled (FP_DEVICE (self)))
    gap = 0;
  fpi_ssm_jump_to_state_delayed (ssm, CAP_REINIT, gap);
}

/* At a transfer boundary with a suspend pending: park the machine (it stays
 * in its state, nothing in flight, nothing scheduled) and complete the
 * suspend now that the bus is quiet. TRUE iff parked. */
static void capture_press_reset (FpDeviceEgis0576 *self);

static gboolean
capture_park_if_suspending (FpDeviceEgis0576 *self, FpiSsm *ssm)
{
  if (!self->suspending)
    return FALSE;
  self->suspending = FALSE;
  self->parked = TRUE;
  /* Whatever press was in progress is over by the time we resume; without
   * this the first frame after resume would report its stale scores as a
   * lift ("no match" for a press nobody made after waking). */
  capture_press_reset (self);
  fp_dbg ("parked for suspend in state %d", fpi_ssm_get_cur_state (ssm));
  fpi_device_suspend_complete (FP_DEVICE (self), NULL);
  return TRUE;
}

static void report_finger (FpDeviceEgis0576 *self, gboolean present);

/* Forget the press in progress (its scores, candidate, settle count and the
 * PRESENT status). The enrol phase is left alone: PH_AWAIT_OFF flips on the
 * first no-finger frame, and a finger still resting on the sensor is not a
 * new press. */
static void
capture_press_reset (FpDeviceEgis0576 *self)
{
  self->saw_finger = FALSE;
  self->cand_valid = FALSE;
  self->best_score = -1;
  self->settle_tries = 0;
  report_finger (self, FALSE);
}

static void
report_finger (FpDeviceEgis0576 *self, gboolean present)
{
  if (present == self->finger_present)
    return;
  self->finger_present = present;
  if (present)
    fpi_device_report_finger_status_changes (FP_DEVICE (self),
                                             FP_FINGER_STATUS_PRESENT,
                                             FP_FINGER_STATUS_NONE);
  else
    fpi_device_report_finger_status_changes (FP_DEVICE (self),
                                             FP_FINGER_STATUS_NONE,
                                             FP_FINGER_STATUS_PRESENT);
}

/* Re-run the sensor bring-up on the already-claimed, still-enumerated device —
 * the s2idle recovery, and the answer to a frame that failed. The USB device
 * stayed powered and the interface is still claimed, but the sensor comes
 * back from suspend with its capture pipeline in an unusable state, so the
 * readiness poll + init replay are done again.
 *
 * Deliberately does NOT: re-claim the interface (still claimed -> EBUSY),
 * re-init the host match engine (host state, intact), re-run exposure
 * calibration (it needs a no-finger window we can't guarantee on resume; the
 * init machine re-applies the value the first open()'s calibration found, so
 * the exposure stays the calibrated one), or recapture the flat-field
 * baseline (exposure-tied, per-boot, and still valid precisely because the
 * exposure is re-applied) -- so cross-boot matching is preserved.
 *
 * Passes reset_if_stuck=FALSE: a ForceResetDevice here would take the device
 * off the bus mid-action. */
static void
reinit_done (FpiSsm *init, FpDevice *dev, GError *error)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576 (dev);

  if (error)
    {
      /* Left flagged: the next capture re-initialises eagerly rather than
       * capturing into a dead pipeline. A stop requested for a suspend parks
       * (the bring-up is redone after resume); a cancel is reported as
       * such. */
      if (self->suspending && !fpi_device_action_is_cancelled (dev) &&
          g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        {
          g_error_free (error);
          capture_park_if_suspending (self, self->ssm);
          return;
        }
      capture_fail (self, error);
      return;
    }
  /* A suspend that arrived during the replay (deferred by its critical
   * section) has set the flag again by now; the frame state parks. Only a
   * bring-up nobody has invalidated since clears it. */
  if (!self->suspending)
    self->needs_reinit = FALSE;
  fpi_ssm_jump_to_state (self->ssm, CAP_FRAME);
}

static void
reinit_start (FpDeviceEgis0576 *self)
{
  /* Until the bring-up succeeds the sensor is in an unknown state (a failed
   * record leaves a prefix of the vendor sequence, a cancel honoured before
   * the replay leaves it un-initialised), so the next capture must re-init
   * eagerly whichever path called us; cleared again on success. */
  self->needs_reinit = TRUE;
  fpi_ssm_start (egis_dev_init_ssm (self->sensor, FALSE), reinit_done);
}

static void
frame_done (FpDevice *dev, gpointer ud, GError *error)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576 (dev);

  if (!error)
    {
      self->frame_retried = FALSE;
      fpi_ssm_jump_to_state (self->ssm, CAP_PROCESS);
      return;
    }
  /* Covers the idle-at-suspend case (no vfunc fired, so needs_reinit was
   * FALSE and the eager path was skipped) and any unfreeze race: on a dead or
   * silent sensor the frame chain fails in bounded time (~2.6 s worst
   * case). If we were cancelled meanwhile, leave with a clean cancel instead
   * of running a re-init. Otherwise re-initialise the sensor once and retry;
   * give up -- with the real device error -> password fallback -- only if the
   * re-init or the retried frame also fails. */
  if (fpi_device_action_is_cancelled (dev) ||
      g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED) || self->frame_retried)
    {
      capture_fail (self, error);
      return;
    }
  fp_dbg ("frame capture failed (%s); re-initialising the sensor and retrying",
          error->message);
  g_error_free (error);
  self->frame_retried = TRUE;
  fpi_ssm_jump_to_state (self->ssm, CAP_RECOVER);
}

static void
load_done (GObject *source, GAsyncResult *result, gpointer user_data)
{
  FpiSsm *ssm = user_data;
  GError *error = NULL;

  if (!g_task_propagate_boolean (G_TASK (result), &error))
    {
      fpi_ssm_mark_failed (ssm, error);
      return;
    }
  fpi_ssm_next_state (ssm);
}

static void
enroll_result (FpDeviceEgis0576 *self, FpiSsm *ssm, const MatchJob *j)
{
  FpDevice *dev = FP_DEVICE (self);
  int r = j->ret;

  if (r == -2 && self->settle_tries < EGIS0576_ENROLL_SETTLE_FRAMES)
    {
      /* partial contact: let the press settle, try the next frame */
      self->settle_tries++;
      capture_next (self, ssm);
      return;
    }
  self->settle_tries = 0;
  self->phase = PH_AWAIT_OFF;
  if (r == 1 || r == 2)
    {
      self->enroll_count++;
      fp_dbg ("enroll stage %u/%u (var %.0f)", self->enroll_count,
              EGIS0576_ENROLL_STAGES, var_dbl (self));
      fpi_device_enroll_progress (dev, self->enroll_count, NULL, NULL);
      if (self->enroll_count >= EGIS0576_ENROLL_STAGES)
        {
          fpi_ssm_mark_completed (ssm);
          return;
        }
    }
  else
    {
      /* -2: coverage under the gate; 4: same-press duplicate; 5: same
       * placement as a stored frame -- all three come back to the user as
       * "adjust your finger and try again", which is the right hint for
       * each of them. */
      fp_dbg ("enroll press refused (code %d) at stage %u/%u (var %.0f)%s",
              r, self->enroll_count, EGIS0576_ENROLL_STAGES, var_dbl (self),
              r == 5 ? " -- same placement, asking for a shifted press" : "");
      fpi_device_enroll_progress (dev, self->enroll_count, NULL,
                                  fpi_device_retry_new (FP_DEVICE_RETRY_CENTER_FINGER));
    }
  capture_next (self, ssm);
}

static void
match_result (FpDeviceEgis0576 *self, FpiSsm *ssm, const MatchJob *j)
{
  FpDevice *dev = FP_DEVICE (self);
  int score = j->score, idx = j->idx;

  if (score > self->best_score)
    self->best_score = score;
  fp_dbg ("%s score %d idx %d (best %d, var %.0f)%s",
          self->action == FPI_DEVICE_ACTION_VERIFY ? "verify" : "identify",
          score, idx, self->best_score, var_dbl (self),
          self->cand_valid ? " [candidate held]" : "");

  if (score >= EGIS_THRESHOLD && idx >= 0)
    {
      if (j->check_raw && idx == self->cand_idx)
        {
          /* second consecutive frame over the threshold for the same print,
           * and not a byte-identical re-serve */
          if (j->raw_ok)
            {
              fp_dbg ("match confirmed by a second frame (%d, %d)", self->cand_score, score);
              if (self->action == FPI_DEVICE_ACTION_VERIFY)
                fpi_device_verify_report (dev, FPI_MATCH_SUCCESS, NULL, NULL);
              else
                fpi_device_identify_report (dev,
                                            idx < (int) self->gallery_prints->len
                                            ? g_ptr_array_index (self->gallery_prints, idx) : NULL,
                                            NULL, NULL);
              fpi_ssm_mark_completed (ssm);
              return;
            }
          fp_dbg ("match not corroborated on the raw frame (flat-field baseline "
                  "suspect) -- discarded");
          self->cand_valid = FALSE;
        }
      else
        {
          self->cand_valid = TRUE;
          self->cand_score = score;
          self->cand_idx = idx;
          memcpy (self->cand_raw, self->frame, EGIS_IMG);
        }
    }
  else
    {
      /* a frame below the threshold breaks the chain: a stale frame stands
       * alone, a genuine press does not */
      if (self->cand_valid)
        fp_dbg ("candidate frame (%d) not confirmed by the next frame (%d) "
                "-- possible stale capture", self->cand_score, score);
      self->cand_valid = FALSE;
    }
  capture_next (self, ssm);
}

static void
match_done (GObject *source, GAsyncResult *result, gpointer user_data)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576 (source);
  FpiSsm *ssm = user_data;
  const MatchJob *j = g_task_get_task_data (G_TASK (result));
  GError *error = NULL;

  if (!g_task_propagate_boolean (G_TASK (result), &error))
    {
      fpi_ssm_mark_failed (ssm, error);
      return;
    }
  if (self->action == FPI_DEVICE_ACTION_ENROLL)
    enroll_result (self, ssm, j);
  else
    match_result (self, ssm, j);
}

/* Hand the frame just captured to the matcher, off the main loop. */
static void
match_start (FpDeviceEgis0576 *self, FpiSsm *ssm)
{
  FpDevice *dev = FP_DEVICE (self);
  g_autoptr(GTask) task = NULL;
  MatchJob *j = g_new0 (MatchJob, 1);

  j->engine = self->engine;
  j->action = self->action;
  memcpy (j->probe, self->ffframe, EGIS_IMG);
  memcpy (j->raw, self->frame, EGIS_IMG);
  j->check_raw = self->cand_valid && memcmp (self->cand_raw, self->frame, EGIS_IMG) != 0;
  j->cand_idx = self->cand_idx;

  task = g_task_new (dev, fpi_device_get_cancellable (dev), match_done, ssm);
  g_task_set_source_tag (task, match_thread);
  g_task_set_check_cancellable (task, TRUE);
  g_task_set_task_data (task, j, g_free);
  g_task_run_in_thread (task, match_thread);
}

/* The main-loop half of a frame: finger detection on the raw variance, the
 * opportunistic baseline, the flat-field -- microsecond loops -- and the
 * decision whether the matcher gets to see it. */
static void
capture_process (FpDeviceEgis0576 *self, FpiSsm *ssm)
{
  FpDevice *dev = FP_DEVICE (self);
  guint64 var;

  self->var = var = frame_variance (self->frame);   /* finger-on/off uses RAW variance */

  /* Build the flat-field baseline opportunistically from no-finger frames
   * (before the first press / between presses) -- never blocks, works with
   * GNOME's immediate "place finger" flow, and caches for the whole boot. */
  if (!var_ge (var, EGIS0576_BASELINE_MAX_VAR))
    baseline_feed (self, self->frame);

  /* Per-boot flat-field: subtract the fixed-pattern baseline so a template
   * enrolled one boot matches a probe from another (without it, cross-boot=0);
   * a no-op until the baseline exists. Applied identically to enroll + verify
   * + identify; the matcher's own preprocessing is its own business. */
  flat_field (self, self->frame, self->ffframe);

  if (self->action == FPI_DEVICE_ACTION_ENROLL)
    {
      /* one add per finger press: capture on-press, then wait for lift */
      if (self->phase == PH_AWAIT_ON)
        {
          if (var_ge (var, EGIS0576_FINGER_ON_VAR))
            {
              report_finger (self, TRUE);
              match_start (self, ssm);
              return;
            }
          if (!var_ge (var, EGIS0576_FINGER_OFF_VAR) && self->settle_tries > 0)
            {
              /* lifted before any frame of the press reached the coverage
               * gate: that press was too light or too partial, say so */
              fp_dbg ("enroll press lifted after %d partial frames -- retry",
                      self->settle_tries);
              self->settle_tries = 0;
              report_finger (self, FALSE);
              fpi_device_enroll_progress (dev, self->enroll_count, NULL,
                                          fpi_device_retry_new (FP_DEVICE_RETRY_CENTER_FINGER));
            }
        }
      else if (!var_ge (var, EGIS0576_FINGER_OFF_VAR))   /* PH_AWAIT_OFF */
        {
          self->phase = PH_AWAIT_ON;
          report_finger (self, FALSE);
        }
      capture_next (self, ssm);
      return;
    }

  /* VERIFY / IDENTIFY: score EVERY frame while the finger is down. A single
   * press moves through partial->full contact, and grabbing just the first
   * frame often caught a poor transitional image; report the result when a
   * frame pair confirms a match (match_result), or when the finger lifts. */
  if (var_ge (var, EGIS0576_FINGER_ON_VAR))
    {
      self->saw_finger = TRUE;
      report_finger (self, TRUE);
      match_start (self, ssm);
      return;
    }
  if (!var_ge (var, EGIS0576_FINGER_OFF_VAR) && self->saw_finger)
    {
      /* finger lifted without a confirmed match. An unconfirmed candidate is
       * NOT a match (see cand_valid). A press that produced nothing scorable
       * at all (every frame -1: coverage under the gate) is a placement
       * problem, not a failed attempt -- ask for a retry instead of spending
       * one of fprintd's tries. */
      gboolean nothing = self->best_score < 0;

      if (self->cand_valid)
        fp_dbg ("finger lifted on an unconfirmed candidate (%d) -- no match", self->cand_score);
      fp_dbg ("finger lifted, final best %d -> %s", self->best_score,
              nothing ? "nothing scorable, retry" : "no match");
      report_finger (self, FALSE);
      if (self->action == FPI_DEVICE_ACTION_VERIFY)
        fpi_device_verify_report (dev, nothing ? FPI_MATCH_ERROR : FPI_MATCH_FAIL, NULL,
                                  nothing ? fpi_device_retry_new (FP_DEVICE_RETRY_CENTER_FINGER) : NULL);
      else
        fpi_device_identify_report (dev, NULL, NULL,
                                    nothing ? fpi_device_retry_new (FP_DEVICE_RETRY_CENTER_FINGER) : NULL);
      fpi_ssm_mark_completed (ssm);
      return;
    }
  capture_next (self, ssm);
}

static void
capture_run_state (FpiSsm *ssm, FpDevice *dev)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576 (dev);

  switch (fpi_ssm_get_cur_state (ssm))
    {
    case CAP_LOAD:
      if (self->load)
        {
          g_autoptr(GTask) task = g_task_new (dev, fpi_device_get_cancellable (dev),
                                              load_done, ssm);

          g_task_set_source_tag (task, load_thread);
          g_task_set_check_cancellable (task, TRUE);
          g_task_set_task_data (task, g_steal_pointer (&self->load),
                                (GDestroyNotify) load_job_free);
          g_task_run_in_thread (task, load_thread);
        }
      else
        {
          fpi_ssm_next_state (ssm);
        }
      break;

    case CAP_REINIT:
      /* The loop head. A cancel is honoured first; a pending suspend parks
       * the machine here. Then: a suspend/resume since the last frame left
       * the sensor needing re-initialisation (flag set by the vfuncs) -- do
       * it before the frame, so as not to burn a frame timeout on a sensor
       * that cannot answer. */
      if (capture_cancelled (ssm, dev) || capture_park_if_suspending (self, ssm))
        break;
      if (self->needs_reinit)
        reinit_start (self);
      else
        fpi_ssm_next_state (ssm);
      break;

    case CAP_FRAME:
      if (capture_cancelled (ssm, dev) || capture_park_if_suspending (self, ssm))
        break;
      egis_dev_frame (self->sensor, self->frame, frame_done, NULL);
      break;

    case CAP_RECOVER:
      if (capture_park_if_suspending (self, ssm))
        break;                           /* resume re-initialises anyway */
      reinit_start (self);
      break;

    case CAP_PROCESS:
      /* A frame that arrived with a suspend pending is not looked at: the
       * matcher would start a thread we then wait for, and the press it
       * belongs to is over by the time we resume. */
      if (capture_cancelled (ssm, dev) || capture_park_if_suspending (self, ssm))
        break;
      capture_process (self, ssm);
      break;

    default:
      g_assert_not_reached ();
    }
}

static void capture_complete (FpDeviceEgis0576 *self, FpDevice *dev);

static void
capture_done (FpiSsm *ssm, FpDevice *dev, GError *error)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576 (dev);

  self->ssm = NULL;
  self->parked = FALSE;
  g_clear_pointer (&self->load, load_job_free);
  if (error)
    {
      g_clear_pointer (&self->gallery_prints, g_ptr_array_unref);
      fpi_device_action_error (dev, error);
    }
  else
    {
      capture_complete (self, dev);
    }
  /* The action ended before the machine reached a boundary to park at (a
   * match on the last frame, a sensor failure): the suspend is completed
   * here instead, with nothing of ours left running. */
  if (self->suspending)
    {
      self->suspending = FALSE;
      fpi_device_suspend_complete (dev, NULL);
    }
}

static void
capture_complete (FpDeviceEgis0576 *self, FpDevice *dev)
{
  g_clear_pointer (&self->gallery_prints, g_ptr_array_unref);
  if (self->action == FPI_DEVICE_ACTION_ENROLL)
    {
      FpPrint *print = NULL;
      guint8 *blob = NULL;
      int size = egis_enroll_finish (self->engine, &blob);

      if (size <= 0 || !blob)
        {
          fpi_device_enroll_complete (dev, NULL,
                                      fpi_device_error_new_msg (FP_DEVICE_ERROR_GENERAL,
                                                                "Failed to build template"));
        }
      else
        {
          fpi_device_get_enroll_data (dev, &print);
          fpi_print_set_type (print, FPI_PRINT_RAW);
          g_object_set (print, "fpi-data", print_wrap_blob (blob, size), NULL);
          fpi_device_enroll_complete (dev, g_object_ref (print), NULL);
        }
      free (blob);                       /* libc malloc'd by egis_enroll_finish */
    }
  else if (self->action == FPI_DEVICE_ACTION_VERIFY)
    {
      fpi_device_verify_complete (dev, NULL);
    }
  else
    {
      fpi_device_identify_complete (dev, NULL);
    }
}

static void
start_capture (FpDevice *dev)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576 (dev);

  self->action = fpi_device_get_current_action (dev);
  self->phase = PH_AWAIT_ON;
  self->finger_present = FALSE;
  self->frame_retried = FALSE;
  self->settle_tries = 0;
  self->saw_finger = FALSE;
  self->best_score = -1;
  self->cand_valid = FALSE;
  self->cand_score = -1;
  self->cand_idx = -1;
  self->parked = FALSE;
  memset (&self->bl, 0, sizeof self->bl);

  fpi_device_report_finger_status_changes (dev,
                                           FP_FINGER_STATUS_NEEDED,
                                           FP_FINGER_STATUS_NONE);
  self->ssm = fpi_ssm_new_full (dev, capture_run_state, CAP_NUM_STATES,
                                CAP_NUM_STATES, "capture");
  fpi_ssm_silence_debug (self->ssm);       /* it loops ~30 times a second */
  fpi_ssm_start (self->ssm, capture_done);
}

/* ------------------------------------------------------------------ */
/* Device vfuncs                                                      */
/* ------------------------------------------------------------------ */

static void
open_fail (FpDevice *dev, GError *error)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576 (dev);

  g_clear_pointer (&self->sensor, egis_dev_free);
  g_usb_device_release_interface (fpi_device_get_usb_device (dev), EGIS0576_INTF, 0, NULL);
  fpi_device_open_complete (dev, error);
}

static void
open_calibrate_done (FpiSsm *cal, FpDevice *dev, GError *error)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576 (dev);

  /* A cancelled open is a failed open, whichever machine saw the cancel.
   * Any other failure is non-fatal: the search is simply retried at the next
   * open (see egis_dev_calibrate_ssm). */
  if (error && g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    {
      open_fail (dev, error);
      return;
    }
  self->needs_reinit = FALSE;
  if (error)
    {
      /* The search stopped with reg 0x0f at whatever midpoint it had just
       * tried, which on this sensor can be a saturating value: have the next
       * action replay the bring-up first, whose record 25 restores the baked
       * exposure (nothing is re-applied while the calibration is unset). */
      fp_dbg ("exposure calibration skipped: %s", error->message);
      g_error_free (error);
      self->needs_reinit = TRUE;
    }
  self->dc_c_calibrated = egis_dev_get_calibration (self->sensor);
  fpi_device_open_complete (dev, NULL);
}

static void
open_init_done (FpiSsm *init, FpDevice *dev, GError *error)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576 (dev);

  if (error)
    {
      open_fail (dev, error);
      return;
    }
  if (egis_dev_get_calibration (self->sensor) >= 0)
    {
      open_calibrate_done (NULL, dev, NULL);
      return;
    }
  /* One-time per-device exposure calibration (the vendor's calibrate_gain),
   * before any capture and once per fprintd lifetime. A binary search over reg
   * 0x0f converges the no-finger frame mean to a fixed target, so the exposure
   * is the SAME on any EH576 unit -- this is what makes the driver
   * device-independent (the baked init values belong to one unit). No finger
   * is expected at open. */
  fpi_ssm_start (egis_dev_calibrate_ssm (self->sensor), open_calibrate_done);
}

static void
egis0576_open (FpDevice *dev)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576 (dev);
  GError *error = NULL;

  if (!self->engine)
    self->engine = egis_engine_new ();
  if (!self->engine)
    {
      fpi_device_open_complete (dev,
                                fpi_device_error_new_msg (FP_DEVICE_ERROR_GENERAL,
                                                          "Failed to initialise the match engine"));
      return;
    }
  if (!g_usb_device_claim_interface (fpi_device_get_usb_device (dev),
                                     EGIS0576_INTF, 0, &error))
    {
      fpi_device_open_complete (dev, error);
      return;
    }

  self->sensor = egis_dev_new (dev);
  egis_dev_set_calibration (self->sensor, self->dc_c_calibrated);
  fpi_ssm_start (egis_dev_init_ssm (self->sensor, TRUE), open_init_done);
}

static void
egis0576_close (FpDevice *dev)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576 (dev);
  GError *error = NULL;

  /* No action can be current here (libfprint refuses close while one is),
   * so no machine of ours is running and no transfer is in flight. */
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
  if (egis_enroll_begin (self->engine) != 0)
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
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576 (dev);
  FpPrint *print = NULL;
  g_autoptr(GPtrArray) prints = g_ptr_array_new ();

  fpi_device_get_verify_data (dev, &print);
  g_ptr_array_add (prints, print);
  self->load = load_job_new (self, prints);
  if (!self->load)
    {
      g_clear_pointer (&self->gallery_prints, g_ptr_array_unref);
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
  guint cap = (guint) MAX (egis_gallery_capacity (self->engine), 1);

  fpi_device_get_identify_data (dev, &prints);
  if (prints && prints->len > cap)
    {
      /* fprintd passes every print of the user; matching only a prefix would
       * make the later fingers silently unusable for login. Fail loudly
       * instead. */
      fpi_device_identify_complete (dev,
                                    fpi_device_error_new_msg (FP_DEVICE_ERROR_DATA_INVALID,
                                                              "%u prints to match but this matcher holds at most %u; "
                                                              "delete some enrolled fingers", prints->len, cap));
      return;
    }
  self->load = prints ? load_job_new (self, prints) : NULL;
  if (!self->load)
    {
      /* Empty gallery -- fprintd's pre-enroll duplicate check with nothing
       * enrolled. Trivially "no match": finish without touching the sensor. */
      g_clear_pointer (&self->gallery_prints, g_ptr_array_unref);
      fpi_device_identify_report (dev, NULL, NULL, NULL);
      fpi_device_identify_complete (dev, NULL);
      return;
    }
  start_capture (dev);
}

static void
egis0576_cancel (FpDevice *dev)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576 (dev);

  /* The capture machine and the transport's machines consult the action's
   * cancellable at every transfer-sequence boundary (they never hand it to a
   * transfer, see egis0576_proto.c), and the matcher task checks it too. The
   * action ends with G_IO_ERROR_CANCELLED at the next boundary: at most one
   * frame sequence (~2.6 s on a sensor that has stopped answering, ~0.1 s on
   * a healthy one), the init replay (~0.3 s), or one readiness-poll
   * iteration (850 ms). A machine parked for suspend has no next boundary of
   * its own; it is sent to the loop head, which fails it. */
  fp_dbg ("cancelling");
  if (self->parked)
    {
      self->parked = FALSE;
      fpi_ssm_jump_to_state (self->ssm, CAP_REINIT);
    }
}

/* Suspend with an action running. The action is kept, as the
 * fpi_device_suspend_complete() contract asks for a NULL completion: the
 * capture machine parks at its next transfer boundary (capture_park_if_
 * suspending) -- so the suspend is completed, and with it fprintd's sleep
 * inhibitor released, only once no transfer is in flight and none is
 * scheduled -- and resume sends it back to the loop head, where the sensor
 * is re-initialised before the next frame. The latency of the completion is
 * the same as a cancel's (one transfer sequence). If the action ends on its
 * own first, capture_done completes the suspend. No USB here. */
static void
egis0576_suspend (FpDevice *dev)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576 (dev);

  /* The sensor does not come back from s2idle usable (it stays powered, but
   * the capture pipeline is wedged): re-run the vendor bring-up first. */
  self->needs_reinit = TRUE;
  /* No capture machine: the action has already ended in the driver's view
   * (its completion is queued in an idle libfprint has not run yet) or it
   * never started one. Nothing of ours is on the bus; complete now. */
  if (!self->ssm)
    {
      fp_dbg ("suspend: no capture running");
      fpi_device_suspend_complete (dev, NULL);
      return;
    }
  fp_dbg ("suspend: parking the capture at its next transfer boundary");
  self->suspending = TRUE;
  /* An init machine in flight (a re-init) stops at its next boundary too, so
   * the park never waits out a readiness poll (8.5 s) or a silent replay:
   * logind gives a sleep delay 5 s. */
  egis_dev_request_stop (self->sensor);
}

/* The device left the bus. A capture with a transfer in flight fails on its
 * own; one parked for suspend has nothing in flight and no resume coming
 * (libfprint refuses resume on a removed device), so it is ended here, with
 * no USB. */
static void
egis0576_removed (FpDeviceEgis0576 *self)
{
  gboolean removed = FALSE;

  g_object_get (self, "removed", &removed, NULL);
  if (!removed || !self->parked)
    return;
  fp_dbg ("removed while parked for suspend");
  self->parked = FALSE;
  fpi_ssm_mark_failed (self->ssm, fpi_device_error_new (FP_DEVICE_ERROR_REMOVED));
}

static void
egis0576_resume (FpDevice *dev)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576 (dev);

  fp_dbg ("resume: %s", self->parked ? "restarting the parked capture" : "no parked capture");
  self->needs_reinit = TRUE;
  fpi_device_resume_complete (dev, NULL);
  if (self->parked)
    {
      self->parked = FALSE;
      fpi_ssm_jump_to_state (self->ssm, CAP_REINIT);
    }
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
  g_signal_connect (self, "notify::removed", G_CALLBACK (egis0576_removed), NULL);
}

static void
fpi_device_egis0576_finalize (GObject *object)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576 (object);

  /* No action can be current here, so nothing of ours is in flight; the
   * sensor is normally freed by close(), but a device destroyed while open
   * (unplugged while claimed) never gets one. */
  g_clear_pointer (&self->sensor, egis_dev_free);
  g_clear_pointer (&self->load, load_job_free);
  g_clear_pointer (&self->gallery_prints, g_ptr_array_unref);
  g_clear_pointer (&self->engine, egis_engine_free);
  G_OBJECT_CLASS (fpi_device_egis0576_parent_class)->finalize (object);
}

static void
fpi_device_egis0576_class_init (FpDeviceEgis0576Class *klass)
{
  FpDeviceClass *dev_class = FP_DEVICE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = fpi_device_egis0576_finalize;

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
