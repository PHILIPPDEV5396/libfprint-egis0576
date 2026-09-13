/* egis0576_proto.c — plaintext EGIS/SIGE protocol to the EgisTec EH576.
 * Blocking gusb bulk transfers driven by the async API on a private
 * GMainContext (see the USB section); no crypto, no session state. */
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

/* Exposure registers, from the vendor's own per-unit values (see the naming note
 * on auto_expose_mm below): 0x0f is sensor_dc_c, a DC offset; 0x12 is
 * sensor_gain. Both move the exposure. */
#define REG_DC_C 0x0f
#define REG_GAIN 0x12

/* Target no-finger frame mean for the exposure search. The vendor uses 0x40 plus
 * a per-finger auto-exposure pass; we anchor instead to the reference unit's
 * well-matching level (~0x58), which is a no-op there and adaptive elsewhere. */
#define EGIS_EXPOSURE_TARGET 0x58

struct EgisDev {
  GUsbDevice   *usb;
  GCancellable *cancellable; /* borrowed (see egis_dev_set_cancellable); NULL = uncancellable */
  unsigned char dc_c;      /* cached reg 0x0f */
  unsigned char gain;      /* cached reg 0x12 (indexes EGIS_STEP_TABLE) */
};

/* reg 0x12 indexes this per-step table (byte-exact from the vendor driver's
 * .rdata @0x180077170). */
static const int EGIS_STEP_TABLE[16] = {
  2048, 14684, 17121, 19558, 20623, 24371, 26829, 29286,
  31744, 34202, 36454, 38912, 41370, 43827, 46285, 48742
};

/* ---------------------------------------------------------------- USB ---- */

/* Every transfer below blocks the calling thread until it completes or times
 * out. The caller is the capture worker for everything a capture does
 * (re-init when needed, GetFrame), and the fprintd MAIN thread for the
 * one-time bring-up in the open() vfunc (readiness poll, ForceReset, replay,
 * exposure calibration) -- libfprint dispatches open() inline and this driver
 * does not spawn a thread for it. Blocking is done by running the ASYNC gusb
 * API against a private GMainContext, not by calling gusb's own synchronous
 * wrappers. The distinction matters -- it is the fix for a lost-wakeup hang
 * found on 2026-09-13 (docs/worker-thread.md):
 *
 * gusb's g_usb_device_bulk_transfer() spins a GMainLoop on the GUsbContext's
 * main context, which is the DEFAULT GMainContext -- the one the fprintd main
 * thread owns for the lifetime of its g_main_loop_run. The GTask behind the
 * transfer completes on the thread-default context of the thread that created
 * it, and on the worker that is also the default context. So the completion
 * idle -- and with it the g_main_loop_quit that is meant to release the worker
 * -- is dispatched by the MAIN thread. If that quit lands after
 * libusb_submit_transfer but before the worker has entered g_main_loop_run,
 * its effect is lost: g_main_loop_quit clears is_running (already clear) and
 * broadcasts the context's condition variable, but nobody is waiting on it
 * yet. The worker's g_main_loop_run then fails to acquire the context (the
 * main thread owns it), only THEN sets is_running = TRUE -- overwriting the
 * quit -- and waits on that condition variable forever (glib gmain.c,
 * g_main_loop_run / g_main_loop_quit). Seen on hardware: helper.ret already 7
 * (the bulk OUT had completed), libusb with no transfer in flight, the loop's
 * is_running set, the worker parked in
 * g_main_context_wait_internal, and fprintd never answering the verify.
 *
 * Verified against gusb 0.4.9 (gusb-device.c): g_usb_device_bulk_transfer_async
 * and g_usb_device_control_transfer_async both create their GTask with
 * g_task_new() after entry -- for the real (non-emulated) device path directly
 * before libusb_submit_transfer -- and neither pushes or pops a thread-default
 * context of its own. g_task_new captures g_main_context_ref_thread_default()
 * at that moment. So with a private context pushed as thread-default around the
 * async call, the completion idle is attached to THAT context, which only this
 * thread iterates, inside g_main_loop_run, i.e. after is_running is set. No
 * other thread can quit the loop, nothing can be lost, and the worker no longer
 * depends on the main thread making progress at all. (gusb's internal libusb
 * event thread, "GUsbEventThread", still signals completion via g_task_return;
 * it is not inside a source dispatch, so GTask always queues the completion
 * as an idle on the task's context -- ours -- instead of the default one.)
 *
 * On the main thread (open()) the same helper is safe for the same reason, and
 * it is a behaviour change worth knowing: the fresh context is unowned, so
 * g_main_loop_run acquires it and the completion lands on it. Before v0.4.3
 * gusb's sync wrapper nested a loop on the DEFAULT context here, re-entrantly
 * dispatching fprintd's own sources (D-Bus and all) from inside open(); now
 * they are simply not dispatched for the duration of the bring-up, which is
 * what a blocking open() should look like.
 *
 * RULE: never call the gusb sync API (g_usb_device_bulk_transfer,
 * _control_transfer, _interrupt_transfer) from a thread that does not own the
 * GUsbContext's main context. */

typedef struct {
  GMainLoop *loop;
  gssize     ret;
  GError   **error;
} SyncXfer;

static void
bulk_done (GObject *src, GAsyncResult *res, gpointer ud)
{
  SyncXfer *x = ud;

  x->ret = g_usb_device_bulk_transfer_finish (G_USB_DEVICE (src), res, x->error);
  g_main_loop_quit (x->loop);
}

static void
control_done (GObject *src, GAsyncResult *res, gpointer ud)
{
  SyncXfer *x = ud;

  x->ret = g_usb_device_control_transfer_finish (G_USB_DEVICE (src), res, x->error);
  g_main_loop_quit (x->loop);
}

/* Bulk transfer on @ep, blocking until done. Returns the byte count, or -1 on
 * error/timeout (with *error set). @cancellable is deliberately always NULL
 * (see the comment above usb_out). */
static gssize
egis_bulk (EgisDev *d, guint8 ep, guint8 *buf, gsize len, guint timeout_ms,
           GError **error)
{
  g_autoptr(GMainContext) ctx = g_main_context_new ();
  SyncXfer x = { NULL, -1, error };

  g_main_context_push_thread_default (ctx);
  x.loop = g_main_loop_new (ctx, FALSE);
  g_usb_device_bulk_transfer_async (d->usb, ep, buf, len, timeout_ms, NULL,
                                    bulk_done, &x);
  g_main_loop_run (x.loop);
  g_main_loop_unref (x.loop);
  g_main_context_pop_thread_default (ctx);
  return x.ret;
}

/* Control transfer, same shape as egis_bulk. */
static gssize
egis_control (EgisDev *d, GUsbDeviceDirection dir, GUsbDeviceRequestType type,
              GUsbDeviceRecipient recipient, guint8 request, guint16 value,
              guint16 idx, guint8 *buf, gsize len, guint timeout_ms,
              GError **error)
{
  g_autoptr(GMainContext) ctx = g_main_context_new ();
  SyncXfer x = { NULL, -1, error };

  g_main_context_push_thread_default (ctx);
  x.loop = g_main_loop_new (ctx, FALSE);
  g_usb_device_control_transfer_async (d->usb, dir, type, recipient, request,
                                       value, idx, buf, len, timeout_ms, NULL,
                                       control_done, &x);
  g_main_loop_run (x.loop);
  g_main_loop_unref (x.loop);
  g_main_context_pop_thread_default (ctx);
  return x.ret;
}

/* The cancellable is consulted BETWEEN transfers only -- it is never handed to
 * gusb. Measured on the reference unit: aborting an in-flight bulk transfer
 * (libusb_cancel_transfer, which is what gusb does with a cancelled
 * GCancellable) wedged the sensor at USB level -- bulk OUT NAKed for 3 s
 * timeouts, and afterwards the EP0 ForceReset was ignored, a sysfs deauthorize
 * timed out, and a hub-port link reset only made it drop off the bus; nothing
 * short of cutting board power brought it back. A timeout never does that
 * (it only fires when no data is flowing), a cancel can hit mid-frame. So a
 * cancel costs at most one transfer timeout of latency (800 ms read, 3 s
 * write) inside a sequence -- and the two sequences that must not be
 * interrupted (the init replay, and getframe's preamble + read, which arms the
 * sensor for a frame) are checked only at their boundaries: up to ~2.6 s for a
 * getframe on a sensor that has stopped answering. Abandoning either half-way
 * would be a new, untested sensor state; that is a deliberate trade. */

/* TRUE iff the transfer completed (any byte count), exactly as the gusb sync
 * wrapper's "helper.ret != -1" used to report it. */
static gboolean
usb_out (EgisDev *d, const unsigned char *b, int n)
{
  return egis_bulk (d, EP_OUT, (guint8 *) b, n, 3000, NULL) != -1;
}

/* Bytes read, or -1 on error/timeout. */
static int
usb_in (EgisDev *d, unsigned char *b, int cap, int timeout)
{
  return (int) egis_bulk (d, EP_IN, b, cap, timeout, NULL);
}

/* TRUE (and *error set to G_IO_ERROR_CANCELLED) iff the attached cancellable has
 * been triggered. Safe with a NULL cancellable (never cancelled). */
static gboolean
egis_cancelled (EgisDev *d, GError **error)
{
  return g_cancellable_set_error_if_cancelled (d->cancellable, error);
}

/* Discard whatever a previous session left queued on the IN endpoint. */
static void
flush_in (EgisDev *d)
{
  unsigned char tmp[4096];

  for (int i = 0; i < 8 && usb_in (d, tmp, sizeof tmp, 30) > 0; i++)
    ;
}

/* ------------------------------------------------------------ commands ---- */

/* Send one "EGIS" command and collect its reply. Returns the reply length, or
 * -1 if the sensor said nothing. */
static int
egis_cmd (EgisDev *d, const unsigned char *body, int n,
          unsigned char *reply, int reply_cap, int timeout)
{
  unsigned char buf[32] = { 0x45, 0x47, 0x49, 0x53 };   /* "EGIS" */

  g_assert (n <= (int) sizeof buf - 4);
  memcpy (buf + 4, body, n);
  if (!usb_out (d, buf, 4 + n))
    return -1;
  if (!reply)
    return 0;
  return usb_in (d, reply, reply_cap, timeout);
}

/* ReadRegister(reg): "EGIS" 60 reg 00 -> "SIGE" reg <value> <status>. */
static int
egis_readreg (EgisDev *d, int reg)
{
  unsigned char body[3] = { 0x60, (unsigned char) reg, 0x00 };
  unsigned char r[64];
  int n = egis_cmd (d, body, 3, r, sizeof r, 800);

  return (n >= 6 && r[0] == 'S') ? r[5] : -1;
}

static void
egis_writereg (EgisDev *d, int reg, int val)
{
  unsigned char body[3] = { 0x61, (unsigned char) reg, (unsigned char) val };
  unsigned char r[64];

  egis_cmd (d, body, 3, r, sizeof r, 300);
}

/* The vendor's check_and_recovery (egis_fp_common_5XX.c): poll register 0 until
 * it reports ready before touching the sensor at all. The vendor polls up to
 * 3000 times; a healthy sensor answers on the first try (measured: ~1 ms), and
 * one that is in session mode never answers, so a short budget is enough to tell
 * the two apart without stalling an authentication. */
static gboolean
egis_wait_ready (EgisDev *d)
{
  for (int i = 0; i < 10; i++)
    {
      int v;

      /* Once cancelled, the remaining iterations (850 ms each on a silent
       * sensor) are pointless -- stop at this boundary. */
      if (g_cancellable_is_cancelled (d->cancellable))
        return FALSE;
      v = egis_readreg (d, 0x00);
      if (v >= 0 && (v & 0xfe) == 0xaa)
        return TRUE;
      g_usleep (50 * 1000);
    }
  return FALSE;
}

/* ---------------------------------------------------------------- open ---- */

/* Process-global: the exposure calibration runs once per fprintd process
 * (it needs a no-finger window, which only the first open can guarantee), and
 * the value it found is cached here so egis_dev_open can re-apply it after
 * every later bring-up -- the vendor replay resets reg 0x0f to the baked value
 * and each EgisDev has its own dc_c cache, so neither survives a re-open on
 * its own. THREADING: written only by egis_dev_calibrate, which libfprint
 * dispatches inside the open() vfunc on the main thread, at a point where no
 * capture worker exists (one action at a time; the worker is created after
 * open and joined before close). Read later by egis_dev_open on both threads
 * -- main thread for each open(), worker thread for a post-resume re-init --
 * with g_thread_new / g_thread_join providing the ordering. */
static gboolean g_exposure_calibrated = FALSE;
static unsigned char g_dc_c_calibrated;

gboolean
egis_dev_open (EgisDev *d, gboolean reset_if_stuck, GError **error)
{
  if (egis_cancelled (d, error))
    return FALSE;
  flush_in (d);

  if (!egis_wait_ready (d))
    {
      /* Cancelled while polling: report that, and above all do NOT fire the
       * ForceReset — it takes the device off the bus, which is not what a
       * cancel (or a close racing a worker) asked for. */
      if (egis_cancelled (d, error))
        return FALSE;
      if (reset_if_stuck)
        egis_control (d, G_USB_DEVICE_DIRECTION_HOST_TO_DEVICE,
                      G_USB_DEVICE_REQUEST_TYPE_CLASS,
                      G_USB_DEVICE_RECIPIENT_INTERFACE,
                      MODE_REQUEST, FORCE_RESET, INTF,
                      NULL, 0, 500, NULL);
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_INITIALIZED,
                   "EH576: sensor did not answer the readiness poll%s",
                   reset_if_stuck ? " — reset issued, retry once it re-enumerates"
                                  : "");
      return FALSE;
    }

  /* Replay the vendor bring-up. Record 15 ("EGIS 73 0f 96") is an upload: its
   * 3990-byte payload follows immediately in the next eight records and the
   * sensor answers only once that is complete, so nothing may be read in
   * between -- doing so stalls the upload and wedges the sensor. For the same
   * reason the command and its eight payload records are sent as one unit.
   *
   * The WHOLE replay is treated as one unit for cancellation: a cancel is
   * honoured before the first record and after the last, never in between.
   * Stopping half-way would leave the sensor with a prefix of the vendor
   * sequence, a state nobody has tested it in, and the entire replay takes
   * ~0.3 s on a healthy sensor, so the latency gained would be negligible. */
  if (egis_cancelled (d, error))
    return FALSE;
  int in_upload = 0;
  for (int i = 0; i < EGIS_INIT_RECORD_COUNT; i++)
    {
      const unsigned char *rec = egis_init_records[i].data;
      int len = egis_init_records[i].len;
      gboolean is_cmd = len >= 5 && memcmp (rec, "EGIS", 4) == 0;
      gboolean is_upload = is_cmd && rec[4] == 0x73;
      unsigned char r[64];

      if (!usb_out (d, rec, len))
        {
          g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                       "EH576: init record %d failed", i);
          return FALSE;
        }
      if (is_upload)
        in_upload = 8;                       /* the payload chunks that follow */
      else if (in_upload > 0)
        in_upload--;
      else if (is_cmd)
        usb_in (d, r, sizeof r, 800);        /* consume the SIGE reply */
    }

  /* The sensor is fully initialised here; a cancel that landed during the
   * replay is honoured now, before the two cache reads (800 ms each on a
   * silent sensor) and before the caller clears its re-init flag. */
  if (egis_cancelled (d, error))
    return FALSE;
  int dc_c_read = egis_readreg (d, REG_DC_C);

  d->dc_c = (unsigned char) MAX (dc_c_read, 0);
  /* Record 25 of the replay block-writes regs 0x09..0x13 and so puts reg 0x0f
   * back to the baked value. The calibrated exposure lives only in the sensor
   * and in this cache, both of which the replay just overwrote, so re-apply
   * the value egis_dev_calibrate found earlier in this process -- every open
   * after the first (fprintd opens on each claim) and every post-resume
   * re-init would otherwise run with the baked value, and the per-boot
   * flat-field baseline is exposure-tied. */
  if (g_exposure_calibrated && d->dc_c != g_dc_c_calibrated)
    {
      if (dc_c_read < 0)
        g_debug ("EH576: re-applying calibrated dc_c 0x%02x after bring-up "
                 "(could not read the register back)", g_dc_c_calibrated);
      else
        g_debug ("EH576: re-applied calibrated dc_c 0x%02x after bring-up "
                 "(replay left 0x%02x)", g_dc_c_calibrated, dc_c_read);
      egis_writereg (d, REG_DC_C, g_dc_c_calibrated);
      d->dc_c = g_dc_c_calibrated;
    }
  d->gain = (unsigned char) MAX (egis_readreg (d, REG_GAIN), 0);
  return TRUE;
}

/* ------------------------------------------------------------- capture ---- */

/* Per-frame trigger sequence, then GetFrame. */
static const struct { unsigned char b[5]; int n; } FRAME_PREAMBLE[] = {
  { { 0x63, 0x2c, 0x02, 0x00, 0x57 }, 5 },
  { { 0x60, 0x2d, 0x00 },             3 },
  { { 0x62, 0x67, 0x03 },             3 },
  { { 0x60, 0x0f, 0x00 },             3 },
  { { 0x63, 0x2c, 0x02, 0x00, 0x13 }, 5 },
  { { 0x60, 0x00, 0x00 },             3 },
};

gboolean
egis_dev_getframe (EgisDev *d, guint8 *img, GError **error)
{
  unsigned char req[3] = { 0x64, 0x0f, 0x96 };
  unsigned char o[4096];
  int got = 0;

  if (egis_cancelled (d, error))
    return FALSE;
  for (guint i = 0; i < G_N_ELEMENTS (FRAME_PREAMBLE); i++)
    {
      unsigned char r[64];
      egis_cmd (d, FRAME_PREAMBLE[i].b, FRAME_PREAMBLE[i].n, r, sizeof r, 300);
    }
  if (egis_cmd (d, req, 3, NULL, 0, 0) < 0)
    {
      if (egis_cancelled (d, error))
        return FALSE;
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "EH576: GetFrame failed");
      return FALSE;
    }
  while (got < EGIS_IMG)
    {
      int n = usb_in (d, o, sizeof o, 800);

      if (n <= 0)
        break;
      if (n > EGIS_IMG - got)
        n = EGIS_IMG - got;
      memcpy (img + got, o, n);
      got += n;
    }
  if (got < EGIS_IMG)
    {
      /* A cancelled read shows up here as a short frame; report it as the
       * cancel it is so the caller does not mistake it for a dead sensor. */
      if (egis_cancelled (d, error))
        return FALSE;
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "EH576: short frame (%d of %d)", got, EGIS_IMG);
      return FALSE;
    }
  return TRUE;
}

/* ------------------------------------------------------------ exposure ---- */

/* One auto-exposure step: nudge the exposure from the frame min/max so the frame
 * spans 0..0xff. Adapts PER DEVICE — the baked init values belong to one unit.
 * Returns 1 if something changed, 0 if the frame was already well exposed.
 *
 * REGISTER NAMING, resolved from the vendor's own data — do not re-litigate:
 * 0x0f and 0x12 are not both gain registers. Init record 25 is "EGIS 63 09 0b"
 * + 83 24 00 44 0f 08 20 20 01 05 12, an 11-byte block written to registers
 * 0x09..0x13. Four of those bytes are exactly the per-unit values Windows stores
 * for this sensor (registry: Enum -> USB -> VID_1C7A&PID_0576 -> <serial> ->
 * Device Parameters), three landing consecutively in the order the vendor's
 * registry loader FUN_18000b2d0 reads them:
 *
 *     reg 0x0d = sensor_vref_sel (0x0f)
 *     reg 0x0e = sensor_dc_p     (0x08)
 *     reg 0x0f = sensor_dc_c     (0x20)   <- a DC offset, NOT a "fine gain"
 *     reg 0x12 = sensor_gain     (0x05)   <- the actual gain
 *
 * Both move exposure, which is why the coarse/fine treatment works: the vendor's
 * own auto-exposure (FUN_180008cd0) decrements reg 0x12 by one on an over-bright
 * frame and otherwise trims reg 0x0f, and those two are the only registers it
 * ever writes with computed rather than constant values. */
static int
auto_expose_mm (EgisDev *d, int mn, int mx)
{
  int step, v;

  if (mn > 0 && mx < 0xff)              /* already well exposed */
    return 0;
  if (mn == 0 && mx == 0xff)            /* full span: back the gain off a step */
    {
      if (d->gain > 1)
        {
          egis_writereg (d, REG_GAIN, d->gain - 1);
          d->gain--;
          return 1;
        }
      return 0;
    }
  step = EGIS_STEP_TABLE[d->gain & 0x0f];
  v = d->dc_c;
  if (mx == 0xff)                       /* too bright: trim the offset down */
    {
      int target = mn << 10;
      while (v > 0 && target >= step) { target -= step; v--; }
    }
  else                                  /* too dark: raise it, cap 0x40 */
    {
      int target = (0xff - mx) << 10;
      while (v < 0x40 && target >= step) { v++; target -= step; }
    }
  if ((unsigned char) v != d->dc_c)
    {
      egis_writereg (d, REG_DC_C, v);
      d->dc_c = (unsigned char) v;
      return 1;
    }
  return 0;
}

/* NOT wired into the capture loop, deliberately. Since the plaintext migration a
 * register read may interleave with capture (measured: frame variance 140.1
 * before, 140.4 after), so this is mechanically possible for the first time. But
 * reg 0x0f has a usable window of only ~8 counts (docs/sensor-tuning.md), and the
 * step-table arithmetic above computes jumps that would overshoot it straight
 * into saturation. No unit with a badly-exposed starting point is available to
 * test that on, and shipping an untested behaviour change to other people's
 * hardware is exactly the mistake that produced the TLS transport. Kept because
 * the mechanism is sound and a future maintainer with such a unit will want it. */
gboolean
egis_dev_autoexpose (EgisDev *d, int frame_min, int frame_max)
{
  return auto_expose_mm (d, frame_min, frame_max) != 0;
}

gboolean
egis_dev_calibrate (EgisDev *d, GError **error)
{
  unsigned char img[EGIS_IMG];
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
   * below is within a few counts of what the baked init value produces unaided,
   * which is why this is close to a no-op here and adaptive elsewhere. */
  int lo = 0, hi = 0x3f, best = 0x1f, best_diff = 0x100;

  if (g_exposure_calibrated)
    return TRUE;                        /* once per process; egis_dev_open re-applies it */
  for (int it = 0; it < 6; it++)
    {
      int mid = (lo + hi) >> 1, level, diff;
      long sum = 0;

      egis_writereg (d, REG_DC_C, mid);
      d->dc_c = (unsigned char) mid;
      if (!egis_dev_getframe (d, img, error))
        return FALSE;                   /* leaves reg 0x0f at the last value tried;
                                           the flag stays FALSE, so the next open's
                                           replay restores the baked value and the
                                           search is retried */
      for (int i = 0; i < EGIS_IMG; i++)
        sum += img[i];
      level = (int) (sum / EGIS_IMG);   /* mean of the no-finger frame */
      diff = ABS (level - EGIS_EXPOSURE_TARGET);
      if (diff < best_diff) { best_diff = diff; best = mid; }
      if (level > EGIS_EXPOSURE_TARGET) hi = mid;
      else                              lo = mid;
    }
  egis_writereg (d, REG_DC_C, best);
  d->dc_c = (unsigned char) best;
  g_dc_c_calibrated = (unsigned char) best;
  g_exposure_calibrated = TRUE;
  return TRUE;
}

/* -------------------------------------------------------------- object ---- */

EgisDev *
egis_dev_new (GUsbDevice *usb)
{
  EgisDev *d = g_new0 (EgisDev, 1);

  d->usb = usb;
  return d;
}

void
egis_dev_set_cancellable (EgisDev *d, GCancellable *cancellable)
{
  d->cancellable = cancellable;         /* borrowed, see the header */
}

void
egis_dev_free (EgisDev *d)
{
  g_free (d);                           /* cancellable is not ours to unref */
}
