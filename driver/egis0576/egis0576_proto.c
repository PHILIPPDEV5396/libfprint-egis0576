/* egis0576_proto.c — plaintext EGIS/SIGE protocol to the EgisTec EH576.
 * FpiSsm machines over FpiUsbTransfer on the device main loop; no crypto, no
 * session state, no thread.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 */
#define FP_COMPONENT "egis0576"

#include "drivers_api.h"
#include "egis0576_proto.h"
#include "egis_init.h"
#include <string.h>

#define EP_OUT 0x01
#define EP_IN  0x82
#define INTF   0

/* Class request 9 on the interface selects the sensor's protocol mode:
 *   wValue = 0x0000  ->  enter TLS-PSK session mode (this driver never does)
 *   wValue = 0x00ff  ->  ForceResetDevice: back to plaintext, re-enumerates */
#define MODE_REQUEST 9
#define FORCE_RESET  0x00ff

/* Exposure register. Init record 25 ("EGIS 63 09 0b" + 11 data bytes) is a
 * block write to registers 0x09..0x13, and four of those bytes are exactly the
 * per-unit values Windows keeps for this sensor in the device's registry
 * Device Parameters (Enum -> USB -> VID_1C7A&PID_0576 -> <serial>):
 *
 *     reg 0x0d = sensor_vref_sel (0x0f)
 *     reg 0x0e = sensor_dc_p     (0x08)
 *     reg 0x0f = sensor_dc_c     (0x20)   <- a DC offset; what the calibration moves
 *     reg 0x12 = sensor_gain     (0x05)   <- the gain; left at the baked value
 *
 * Both move the exposure. reg 0x0f has a usable window of only ~8 counts on
 * the reference unit (docs/sensor-tuning.md), which is why the calibration
 * below is a bounded binary search on it and nothing touches reg 0x12. */
#define REG_DC_C 0x0f

/* Target no-finger frame mean for the exposure search. The vendor uses 0x40 plus
 * a per-finger auto-exposure pass; we anchor instead to the reference unit's
 * well-matching level (~0x58), which is a no-op there and adaptive elsewhere. */
#define EGIS_EXPOSURE_TARGET 0x58
#define EGIS_CAL_ITERATIONS  6

/* Transfer sizes and timeouts (ms). The reply timeouts are the vendor's; a
 * healthy sensor answers a register read in ~1 ms and delivers a frame in
 * ~30 ms, so a timeout only ever fires on a sensor that has stopped talking. */
#define EGIS_REPLY_LEN       64
#define EGIS_CHUNK           4096
#define EGIS_OUT_TIMEOUT     3000
#define EGIS_READ_TIMEOUT    800    /* register read reply, init record reply, frame chunk */
#define EGIS_WRITE_TIMEOUT   300    /* register write reply, frame preamble reply */
#define EGIS_DRAIN_TIMEOUT   30
#define EGIS_RESET_TIMEOUT   500
#define EGIS_DRAIN_MAX       8
#define EGIS_READY_TRIES     10
#define EGIS_READY_GAP_MS    50
#define EGIS_UPLOAD_CHUNKS   8      /* payload records following the record-15 upload command */

struct EgisDev {
  FpDevice *dev;                /* borrowed */
  int       dc_c;               /* cached reg 0x0f */
  int       dc_c_calibrated;    /* what the calibration found, or -1. Handed in by the
                                 * owner (egis_dev_set_calibration) because it must outlive
                                 * this EgisDev: the driver opens a fresh one per open(),
                                 * and the replay resets reg 0x0f to the baked value every
                                 * time. */

  /* Per-machine scratch. The machines below never run concurrently except
   * calibrate, which runs frame as its sub-machine; their fields are disjoint. */
  gboolean  reset_if_stuck;     /* init */
  int       drains;
  int       ready_tries;
  int       record;
  int       in_upload;
  gboolean  in_critical;

  guint8     *img;              /* frame chain */
  int         got;
  int         step;
  EgisFrameCb frame_cb;
  gpointer    frame_ud;

  guint8    cal_img[EGIS_IMG];  /* calibrate */
  int       cal_lo, cal_hi, cal_mid, cal_best, cal_best_diff, cal_it;
};

/* ---------------------------------------------------------------- USB ---- */

/* Every transfer below is submitted with a NULL GCancellable, deliberately.
 * fpi_usb_transfer_submit forwards a cancellable to gusb, which aborts the URB
 * (libusb_cancel_transfer) when it fires. Measured on the reference unit
 * (docs/sensor-tuning.md §4): aborting an in-flight bulk transfer wedged the
 * sensor at USB level -- bulk OUT NAKed for 3 s timeouts, and afterwards the
 * EP0 ForceReset was ignored, a sysfs deauthorize timed out, and a hub-port
 * link reset only made it drop off the bus; nothing short of cutting board
 * power brought it back. A timeout never does that (it only fires when no
 * data is flowing), a cancel can hit mid-frame. So cancellation is a check of
 * fpi_device_action_is_cancelled() between transfer sequences (the header
 * lists them and their latency), and the action's cancellable -- which
 * fpi_device_critical_enter() does not shield, only the vfuncs -- is never
 * given to a transfer. */

/* Fail @ssm with G_IO_ERROR_CANCELLED iff the current action was cancelled. */
static gboolean
egis_check_cancelled (FpiSsm *ssm, FpDevice *dev)
{
  if (!fpi_device_action_is_cancelled (dev))
    return FALSE;
  fpi_ssm_mark_failed (ssm, g_error_new_literal (G_IO_ERROR, G_IO_ERROR_CANCELLED,
                                                 "cancelled"));
  return TRUE;
}

/* One "EGIS" command: the OUT transfer and, if a reply is expected, the IN
 * transfer that collects it. The callback gets the reply bytes and their
 * count; n < 0 means the sensor said nothing within the timeout. That is an
 * expected outcome (the readiness poll of a silent sensor, the ignored replies
 * of the init replay and the frame preamble), so it is reported, not failed.
 * Under umockdev a recorded timeout replays as a 0-byte completion, which the
 * callers treat the same way. An OUT failure, on the other hand, is handed to
 * the callback as @error (transfer full; the machines fail on it, see
 * egis_cmd_failed): a sensor that does not even accept a command (bulk OUT
 * NAKed for the 3 s timeout, the state measured after an aborted URB; or gone
 * from the bus) is not going to answer the rest of the sequence either, and
 * failing on the first such transfer bounds a dead-sensor action at seconds
 * instead of the tens of seconds that running every remaining command into
 * its own timeout would cost. */
typedef void (*EgisReplyCb) (EgisDev      *d,
                             gpointer      ud,
                             const guint8 *reply,
                             gssize        n,
                             GError       *error);

typedef struct {
  EgisDev    *d;
  gpointer    ud;
  guint       reply_timeout;    /* 0: no reply expected */
  EgisReplyCb cb;
} EgisCmd;

static void
egis_cmd_in_cb (FpiUsbTransfer *t, FpDevice *dev, gpointer ud, GError *error)
{
  EgisCmd *c = ud;
  gssize n = error ? -1 : t->actual_length;

  g_clear_error (&error);
  c->cb (c->d, c->ud, t->buffer, n, NULL);
  g_free (c);
}

static void
egis_cmd_out_cb (FpiUsbTransfer *t, FpDevice *dev, gpointer ud, GError *error)
{
  EgisCmd *c = ud;
  FpiUsbTransfer *in;

  if (error || c->reply_timeout == 0)
    {
      c->cb (c->d, c->ud, NULL, error ? -1 : 0, error);
      g_free (c);
      return;
    }
  in = fpi_usb_transfer_new (dev);
  fpi_usb_transfer_fill_bulk (in, EP_IN, EGIS_REPLY_LEN);
  fpi_usb_transfer_submit (in, c->reply_timeout, NULL, egis_cmd_in_cb, c);
}

static void
egis_cmd (EgisDev *d, gpointer ud, const guint8 *body, gsize n,
          guint reply_timeout, EgisReplyCb cb)
{
  FpiUsbTransfer *out = fpi_usb_transfer_new (d->dev);
  EgisCmd *c = g_new0 (EgisCmd, 1);
  guint8 *buf = g_malloc (4 + n);

  memcpy (buf, "EGIS", 4);
  memcpy (buf + 4, body, n);
  c->d = d;
  c->ud = ud;
  c->reply_timeout = reply_timeout;
  c->cb = cb;
  fpi_usb_transfer_fill_bulk_full (out, EP_OUT, buf, 4 + n, g_free);
  fpi_usb_transfer_submit (out, EGIS_OUT_TIMEOUT, NULL, egis_cmd_out_cb, c);
}

/* For the machines: fail @ssm with the command's @error (if any). */
static gboolean
egis_cmd_failed (FpiSsm *ssm, GError *error)
{
  if (!error)
    return FALSE;
  fpi_ssm_mark_failed (ssm, error);
  return TRUE;
}

/* ReadRegister(reg): "EGIS" 60 reg 00 -> "SIGE" reg <value> <status>. */
static void
egis_readreg (EgisDev *d, gpointer ud, guint8 reg, EgisReplyCb cb)
{
  guint8 body[3] = { 0x60, reg, 0x00 };

  egis_cmd (d, ud, body, sizeof body, EGIS_READ_TIMEOUT, cb);
}

/* The register value out of a ReadRegister reply, or -1 if there was none. */
static int
egis_reg_value (const guint8 *r, gssize n)
{
  return (n >= 6 && r[0] == 'S') ? r[5] : -1;
}

/* WriteRegister(reg, val): "EGIS" 61 reg val; the reply is not looked at. */
static void
egis_writereg (EgisDev *d, gpointer ud, guint8 reg, guint8 val, EgisReplyCb cb)
{
  guint8 body[3] = { 0x61, reg, val };

  egis_cmd (d, ud, body, sizeof body, EGIS_WRITE_TIMEOUT, cb);
}

/* ---------------------------------------------------------------- init ---- */

enum {
  INIT_DRAIN,        /* discard whatever a previous session left queued on EP IN */
  INIT_READY,        /* poll register 0 until the sensor reports ready */
  INIT_NOT_READY,    /* it never did: optionally ForceReset, then fail */
  INIT_REPLAY,       /* the vendor bring-up, one unit */
  INIT_READ_DC_C,    /* cache the exposure register the replay left */
  INIT_REAPPLY,      /* put the calibrated exposure back, if one is known */
  INIT_NUM_STATES,
};

static void
init_drain_cb (FpiUsbTransfer *t, FpDevice *dev, gpointer ud, GError *error)
{
  EgisDev *d = ud;
  /* The n <= 0 exit (a timeout, or the 0-byte completion umockdev replays for
   * one) is the normal end of the drain; the bound is for leftover replies. */
  gboolean more = error == NULL && t->actual_length > 0;

  g_clear_error (&error);
  if (more && ++d->drains < EGIS_DRAIN_MAX)
    fpi_ssm_jump_to_state (t->ssm, INIT_DRAIN);
  else
    fpi_ssm_next_state (t->ssm);
}

/* The vendor's check_and_recovery (egis_fp_common_5XX.c): poll register 0 until
 * it reports ready before touching the sensor at all. The vendor polls up to
 * 3000 times; a healthy sensor answers on the first try (measured: ~1 ms), and
 * one that is in session mode never answers, so a short budget is enough to
 * tell the two apart without stalling an authentication. */
static void
init_ready_cb (EgisDev *d, gpointer ud, const guint8 *r, gssize n, GError *error)
{
  FpiSsm *ssm = ud;
  int v = egis_reg_value (r, n);

  if (egis_cmd_failed (ssm, error))
    return;
  if (v >= 0 && (v & 0xfe) == 0xaa)
    fpi_ssm_jump_to_state (ssm, INIT_REPLAY);
  else if (++d->ready_tries < EGIS_READY_TRIES)
    fpi_ssm_jump_to_state_delayed (ssm, INIT_READY, EGIS_READY_GAP_MS);
  else
    fpi_ssm_next_state (ssm);                /* INIT_NOT_READY */
}

static void
init_not_ready_fail (FpiSsm *ssm, EgisDev *d)
{
  fpi_ssm_mark_failed (ssm,
                       fpi_device_error_new_msg (FP_DEVICE_ERROR_PROTO,
                                                 "Sensor did not answer the readiness poll%s",
                                                 d->reset_if_stuck
                                                 ? " -- reset issued, retry once it re-enumerates" : ""));
}

static void
init_reset_cb (FpiUsbTransfer *t, FpDevice *dev, gpointer ud, GError *error)
{
  /* The request is accepted immediately and the device drops off the bus on
   * the next transfer to it (header); its own outcome is of no interest. */
  g_clear_error (&error);
  init_not_ready_fail (t->ssm, ud);
}

static void init_replay_send (FpiSsm *ssm, EgisDev *d);

static void
init_replay_leave (EgisDev *d)
{
  if (d->in_critical)
    fpi_device_critical_leave (d->dev);
  d->in_critical = FALSE;
}

static void
init_replay_advance (FpiSsm *ssm, EgisDev *d)
{
  if (++d->record < EGIS_INIT_RECORD_COUNT)
    {
      init_replay_send (ssm, d);
      return;
    }
  init_replay_leave (d);
  fpi_ssm_next_state (ssm);
}

static void
init_replay_in_cb (FpiUsbTransfer *t, FpDevice *dev, gpointer ud, GError *error)
{
  g_clear_error (&error);                    /* the SIGE reply is consumed, not read */
  init_replay_advance (t->ssm, ud);
}

static void
init_replay_out_cb (FpiUsbTransfer *t, FpDevice *dev, gpointer ud, GError *error)
{
  EgisDev *d = ud;
  const guint8 *rec = t->buffer;
  gboolean is_cmd = t->length >= 5 && memcmp (rec, "EGIS", 4) == 0;
  gboolean is_upload = is_cmd && rec[4] == 0x73;

  if (error)
    {
      init_replay_leave (d);
      g_prefix_error (&error, "init record %d: ", d->record);
      fpi_ssm_mark_failed (t->ssm, error);
      return;
    }

  /* Record 15 ("EGIS 73 0f 96") is an upload: its 3990-byte payload follows
   * immediately in the next eight records and the sensor answers only once
   * that is complete, so nothing may be read in between -- doing so stalls
   * the upload and wedges the sensor. Every other command gets its reply
   * consumed. */
  if (is_upload)
    d->in_upload = EGIS_UPLOAD_CHUNKS;
  else if (d->in_upload > 0)
    d->in_upload--;
  else if (is_cmd)
    {
      FpiUsbTransfer *in = fpi_usb_transfer_new (dev);

      in->ssm = t->ssm;
      fpi_usb_transfer_fill_bulk (in, EP_IN, EGIS_REPLY_LEN);
      fpi_usb_transfer_submit (in, EGIS_READ_TIMEOUT, NULL, init_replay_in_cb, d);
      return;
    }
  init_replay_advance (t->ssm, d);
}

static void
init_replay_send (FpiSsm *ssm, EgisDev *d)
{
  FpiUsbTransfer *out = fpi_usb_transfer_new (d->dev);

  out->ssm = ssm;
  fpi_usb_transfer_fill_bulk_full (out, EP_OUT,
                                   (guint8 *) egis_init_records[d->record].data,
                                   egis_init_records[d->record].len, NULL);
  fpi_usb_transfer_submit (out, EGIS_OUT_TIMEOUT, NULL, init_replay_out_cb, d);
}

static void
init_dc_c_cb (EgisDev *d, gpointer ud, const guint8 *r, gssize n, GError *error)
{
  FpiSsm *ssm = ud;
  int v = egis_reg_value (r, n);

  if (egis_cmd_failed (ssm, error))
    return;
  d->dc_c = MAX (v, 0);
  /* Record 25 of the replay block-writes regs 0x09..0x13 and so puts reg 0x0f
   * back to the baked value. The calibrated exposure lives only in the sensor
   * and in this cache, both of which the replay just overwrote, so re-apply
   * the value the calibration found earlier in this process -- every open
   * after the first (fprintd opens on each claim) and every post-resume
   * re-init would otherwise run with the baked value, and the per-boot
   * flat-field baseline is exposure-tied. */
  if (d->dc_c_calibrated < 0 || d->dc_c == d->dc_c_calibrated)
    {
      fpi_ssm_mark_completed (ssm);
      return;
    }
  if (v < 0)
    fp_dbg ("re-applying calibrated dc_c 0x%02x after bring-up "
            "(could not read the register back)", d->dc_c_calibrated);
  else
    fp_dbg ("re-applied calibrated dc_c 0x%02x after bring-up (replay left 0x%02x)",
            d->dc_c_calibrated, v);
  fpi_ssm_next_state (ssm);                  /* INIT_REAPPLY */
}

static void
init_reapply_cb (EgisDev *d, gpointer ud, const guint8 *r, gssize n, GError *error)
{
  FpiSsm *ssm = ud;

  if (egis_cmd_failed (ssm, error))
    return;
  d->dc_c = d->dc_c_calibrated;
  fpi_ssm_mark_completed (ssm);
}

static void
init_run_state (FpiSsm *ssm, FpDevice *dev)
{
  EgisDev *d = fpi_ssm_get_data (ssm);

  switch (fpi_ssm_get_cur_state (ssm))
    {
    case INIT_DRAIN:
      {
        FpiUsbTransfer *in = fpi_usb_transfer_new (dev);

        in->ssm = ssm;
        fpi_usb_transfer_fill_bulk (in, EP_IN, EGIS_CHUNK);
        fpi_usb_transfer_submit (in, EGIS_DRAIN_TIMEOUT, NULL, init_drain_cb, d);
        break;
      }

    case INIT_READY:
      /* Once cancelled, the remaining iterations (850 ms each on a silent
       * sensor) are pointless -- stop at this boundary. Failing here also
       * keeps a cancel from firing the ForceReset below: taking the device
       * off the bus is not what a cancel asked for. */
      if (egis_check_cancelled (ssm, dev))
        break;
      egis_readreg (d, ssm, 0x00, init_ready_cb);
      break;

    case INIT_NOT_READY:
      /* A cancel that landed during the last poll is honoured here, so the
       * ForceReset is never issued for a cancelled action. */
      if (egis_check_cancelled (ssm, dev))
        break;
      if (d->reset_if_stuck)
        {
          FpiUsbTransfer *ctl = fpi_usb_transfer_new (dev);

          ctl->ssm = ssm;
          fpi_usb_transfer_fill_control (ctl, G_USB_DEVICE_DIRECTION_HOST_TO_DEVICE,
                                         G_USB_DEVICE_REQUEST_TYPE_CLASS,
                                         G_USB_DEVICE_RECIPIENT_INTERFACE,
                                         MODE_REQUEST, FORCE_RESET, INTF, 0);
          fpi_usb_transfer_submit (ctl, EGIS_RESET_TIMEOUT, NULL, init_reset_cb, d);
        }
      else
        {
          init_not_ready_fail (ssm, d);
        }
      break;

    case INIT_REPLAY:
      /* The WHOLE replay is one unit: a cancel is honoured before the first
       * record and after the last, never in between, and the critical section
       * has libfprint hold back suspend for its duration (~0.3 s on a healthy
       * sensor). Stopping half-way would leave the sensor with a prefix of the
       * vendor sequence, a state nobody has tested it in. */
      if (egis_check_cancelled (ssm, dev))
        break;
      fpi_device_critical_enter (dev);
      d->in_critical = TRUE;
      d->record = 0;
      d->in_upload = 0;
      init_replay_send (ssm, d);
      break;

    case INIT_READ_DC_C:
      /* The sensor is fully initialised here; a cancel that landed during the
       * replay is honoured now, before the cache read (800 ms on a silent
       * sensor). */
      if (egis_check_cancelled (ssm, dev))
        break;
      egis_readreg (d, ssm, REG_DC_C, init_dc_c_cb);
      break;

    case INIT_REAPPLY:
      egis_writereg (d, ssm, REG_DC_C, (guint8) d->dc_c_calibrated, init_reapply_cb);
      break;

    default:
      g_assert_not_reached ();
    }
}

FpiSsm *
egis_dev_init_ssm (EgisDev *d, gboolean reset_if_stuck)
{
  FpiSsm *ssm = fpi_ssm_new_full (d->dev, init_run_state, INIT_NUM_STATES,
                                  INIT_NUM_STATES, "init");

  d->reset_if_stuck = reset_if_stuck;
  d->drains = 0;
  d->ready_tries = 0;
  d->in_critical = FALSE;
  fpi_ssm_set_data (ssm, d, NULL);
  return ssm;
}

/* --------------------------------------------------------------- frame ---- */

static const struct { guint8 b[5]; gsize n; } FRAME_PREAMBLE[] = {
  { { 0x63, 0x2c, 0x02, 0x00, 0x57 }, 5 },
  { { 0x60, 0x2d, 0x00 },             3 },
  { { 0x62, 0x67, 0x03 },             3 },
  { { 0x60, 0x0f, 0x00 },             3 },
  { { 0x63, 0x2c, 0x02, 0x00, 0x13 }, 5 },
  { { 0x60, 0x00, 0x00 },             3 },
};

static void
frame_finish (EgisDev *d, GError *error)
{
  EgisFrameCb cb = d->frame_cb;

  d->frame_cb = NULL;
  cb (d->dev, d->frame_ud, error);
}

static void frame_read (EgisDev *d);

static void
frame_read_cb (FpiUsbTransfer *t, FpDevice *dev, gpointer ud, GError *error)
{
  EgisDev *d = ud;
  gssize n = error ? -1 : t->actual_length;

  g_clear_error (&error);
  if (n > 0)
    {
      n = MIN (n, EGIS_IMG - d->got);
      memcpy (d->img + d->got, t->buffer, n);
      d->got += n;
    }
  if (d->got >= EGIS_IMG)
    frame_finish (d, NULL);
  else if (n <= 0)
    frame_finish (d, fpi_device_error_new_msg (FP_DEVICE_ERROR_PROTO,
                                               "Short frame (%d of %d)", d->got, EGIS_IMG));
  else
    frame_read (d);                          /* the next chunk */
}

static void
frame_read (EgisDev *d)
{
  FpiUsbTransfer *in = fpi_usb_transfer_new (d->dev);

  fpi_usb_transfer_fill_bulk (in, EP_IN, EGIS_CHUNK);
  fpi_usb_transfer_submit (in, EGIS_READ_TIMEOUT, NULL, frame_read_cb, d);
}

static void
frame_request_cb (EgisDev *d, gpointer ud, const guint8 *r, gssize n, GError *error)
{
  if (error)
    {
      frame_finish (d, error);
      return;
    }
  d->got = 0;
  frame_read (d);
}

static void
frame_preamble_cb (EgisDev *d, gpointer ud, const guint8 *r, gssize n, GError *error)
{
  static const guint8 getframe[3] = { 0x64, 0x0f, 0x96 };

  if (error)
    {
      frame_finish (d, error);
      return;
    }
  if (++d->step < (int) G_N_ELEMENTS (FRAME_PREAMBLE))
    egis_cmd (d, NULL, FRAME_PREAMBLE[d->step].b, FRAME_PREAMBLE[d->step].n,
              EGIS_WRITE_TIMEOUT, frame_preamble_cb);
  else
    egis_cmd (d, NULL, getframe, sizeof getframe, 0, frame_request_cb);
}

void
egis_dev_frame (EgisDev *d, guint8 *img, EgisFrameCb cb, gpointer user_data)
{
  g_return_if_fail (d->frame_cb == NULL);   /* one frame at a time */
  d->img = img;
  d->frame_cb = cb;
  d->frame_ud = user_data;
  d->step = 0;
  egis_cmd (d, NULL, FRAME_PREAMBLE[0].b, FRAME_PREAMBLE[0].n,
            EGIS_WRITE_TIMEOUT, frame_preamble_cb);
}

/* ------------------------------------------------------------ exposure ---- */

/* Full range [0,0x3f], no unit-specific cap: the search finds each unit's own
 * operating point. best tracks the closest-to-target mean seen, so a unit that
 * saturates early still converges somewhere sane.
 *
 * MEASURED on the reference unit (see docs/sensor-tuning.md): this register's
 * transfer curve is far steeper than the search range suggests --
 * 0x18 -> frame mean 0, 0x20 -> 94, 0x28 -> 255 -- so the usable window is only
 * about 0x1C..0x24 and most of [0,0x3f] is saturation. The search still
 * converges because the mean is monotonic in the register, but do not read the
 * wide range as evidence that the sensor tolerates a wide range. The target
 * is within a few counts of what the baked init value produces unaided, which
 * is why this is close to a no-op here and adaptive elsewhere. */

enum {
  CAL_WRITE,         /* try the midpoint */
  CAL_FRAME,         /* one no-finger frame at it */
  CAL_EVAL,          /* narrow the bracket */
  CAL_APPLY,         /* settle on the best value seen */
  CAL_NUM_STATES,
};

static void
cal_write_cb (EgisDev *d, gpointer ud, const guint8 *r, gssize n, GError *error)
{
  FpiSsm *ssm = ud;

  if (egis_cmd_failed (ssm, error))
    return;
  d->dc_c = d->cal_mid;
  fpi_ssm_next_state (ssm);
}

static void
cal_frame_cb (FpDevice *dev, gpointer ud, GError *error)
{
  FpiSsm *ssm = ud;

  if (error)
    fpi_ssm_mark_failed (ssm, error);
  else
    fpi_ssm_next_state (ssm);
}

static void
cal_apply_cb (EgisDev *d, gpointer ud, const guint8 *r, gssize n, GError *error)
{
  FpiSsm *ssm = ud;

  if (egis_cmd_failed (ssm, error))
    return;
  d->dc_c = d->cal_best;
  d->dc_c_calibrated = d->cal_best;
  fp_dbg ("exposure calibrated: dc_c 0x%02x", d->cal_best);
  fpi_ssm_mark_completed (ssm);
}

static void
cal_run_state (FpiSsm *ssm, FpDevice *dev)
{
  EgisDev *d = fpi_ssm_get_data (ssm);

  switch (fpi_ssm_get_cur_state (ssm))
    {
    case CAL_WRITE:
      d->cal_mid = (d->cal_lo + d->cal_hi) >> 1;
      egis_writereg (d, ssm, REG_DC_C, (guint8) d->cal_mid, cal_write_cb);
      break;

    case CAL_FRAME:
      /* A capture failure fails the search, leaving reg 0x0f at the last
       * value tried and the calibration unset (header); so does a cancelled
       * open, checked here because the frame chain does not. */
      if (egis_check_cancelled (ssm, dev))
        break;
      egis_dev_frame (d, d->cal_img, cal_frame_cb, ssm);
      break;

    case CAL_EVAL:
      {
        long sum = 0;
        int level, diff;

        for (int i = 0; i < EGIS_IMG; i++)
          sum += d->cal_img[i];
        level = (int) (sum / EGIS_IMG);        /* mean of the no-finger frame */
        diff = ABS (level - EGIS_EXPOSURE_TARGET);
        if (diff < d->cal_best_diff)
          {
            d->cal_best_diff = diff;
            d->cal_best = d->cal_mid;
          }
        if (level > EGIS_EXPOSURE_TARGET)
          d->cal_hi = d->cal_mid;
        else
          d->cal_lo = d->cal_mid;
        if (++d->cal_it < EGIS_CAL_ITERATIONS)
          fpi_ssm_jump_to_state (ssm, CAL_WRITE);
        else
          fpi_ssm_next_state (ssm);
        break;
      }

    case CAL_APPLY:
      egis_writereg (d, ssm, REG_DC_C, (guint8) d->cal_best, cal_apply_cb);
      break;

    default:
      g_assert_not_reached ();
    }
}

FpiSsm *
egis_dev_calibrate_ssm (EgisDev *d)
{
  FpiSsm *ssm = fpi_ssm_new_full (d->dev, cal_run_state, CAL_NUM_STATES,
                                  CAL_NUM_STATES, "calibrate");

  d->cal_lo = 0;
  d->cal_hi = 0x3f;
  d->cal_best = 0x1f;
  d->cal_best_diff = 0x100;
  d->cal_it = 0;
  fpi_ssm_set_data (ssm, d, NULL);
  return ssm;
}

void
egis_dev_set_calibration (EgisDev *d, int dc_c)
{
  d->dc_c_calibrated = dc_c;
}

int
egis_dev_get_calibration (EgisDev *d)
{
  return d->dc_c_calibrated;
}

/* -------------------------------------------------------------- object ---- */

EgisDev *
egis_dev_new (FpDevice *dev)
{
  EgisDev *d = g_new0 (EgisDev, 1);

  d->dev = dev;
  d->dc_c_calibrated = -1;
  return d;
}

void
egis_dev_free (EgisDev *d)
{
  g_free (d);
}
