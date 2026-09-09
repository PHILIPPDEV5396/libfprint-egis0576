/* egis0576_proto.c — plaintext EGIS/SIGE protocol to the EgisTec EH576.
 * Synchronous gusb bulk transfers; no crypto, no session state. */
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

static gboolean
usb_out (EgisDev *d, const unsigned char *b, int n)
{
  gsize act = 0;
  return g_usb_device_bulk_transfer (d->usb, EP_OUT, (guint8 *) b, n, &act,
                                     3000, NULL, NULL);
}

static int
usb_in (EgisDev *d, unsigned char *b, int cap, int timeout)
{
  gsize act = 0;

  if (!g_usb_device_bulk_transfer (d->usb, EP_IN, b, cap, &act, timeout, NULL, NULL))
    return -1;
  return (int) act;
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
      int v = egis_readreg (d, 0x00);

      if (v >= 0 && (v & 0xfe) == 0xaa)
        return TRUE;
      g_usleep (50 * 1000);
    }
  return FALSE;
}

/* ---------------------------------------------------------------- open ---- */

gboolean
egis_dev_open (EgisDev *d, gboolean reset_if_stuck, GError **error)
{
  flush_in (d);

  if (!egis_wait_ready (d))
    {
      if (reset_if_stuck)
        g_usb_device_control_transfer (d->usb,
                                       G_USB_DEVICE_DIRECTION_HOST_TO_DEVICE,
                                       G_USB_DEVICE_REQUEST_TYPE_CLASS,
                                       G_USB_DEVICE_RECIPIENT_INTERFACE,
                                       MODE_REQUEST, FORCE_RESET, INTF,
                                       NULL, 0, NULL, 500, NULL, NULL);
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_INITIALIZED,
                   "EH576: sensor did not answer the readiness poll%s",
                   reset_if_stuck ? " — reset issued, retry once it re-enumerates"
                                  : "");
      return FALSE;
    }

  /* Replay the vendor bring-up. Record 15 ("EGIS 73 0f 96") is an upload: its
   * 3990-byte payload follows immediately in the next eight records and the
   * sensor answers only once that is complete, so nothing may be read in
   * between — doing so stalls the upload and wedges the sensor. */
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
      if (is_cmd && !is_upload)
        usb_in (d, r, sizeof r, 800);      /* consume the SIGE reply */
    }

  d->dc_c = (unsigned char) MAX (egis_readreg (d, REG_DC_C), 0);
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

  for (guint i = 0; i < G_N_ELEMENTS (FRAME_PREAMBLE); i++)
    {
      unsigned char r[64];
      egis_cmd (d, FRAME_PREAMBLE[i].b, FRAME_PREAMBLE[i].n, r, sizeof r, 300);
    }
  if (egis_cmd (d, req, 3, NULL, 0, 0) < 0)
    {
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

gboolean
egis_dev_autoexpose (EgisDev *d, int frame_min, int frame_max)
{
  return auto_expose_mm (d, frame_min, frame_max) != 0;
}

static gboolean g_exposure_calibrated = FALSE;

gboolean
egis_dev_calibrate (EgisDev *d, GError **error)
{
  unsigned char img[EGIS_IMG];
  /* Full range [0,0x3f], no unit-specific cap: the search finds each unit's own
   * operating point. best tracks the closest-to-target mean seen, so a unit that
   * saturates early still converges somewhere sane. */
  int lo = 0, hi = 0x3f, best = 0x1f, best_diff = 0x100;

  if (g_exposure_calibrated)
    return TRUE;                        /* once per boot; the value persists */
  for (int it = 0; it < 6; it++)
    {
      int mid = (lo + hi) >> 1, level, diff;
      long sum = 0;

      egis_writereg (d, REG_DC_C, mid);
      d->dc_c = (unsigned char) mid;
      if (!egis_dev_getframe (d, img, error))
        return FALSE;                   /* keep the baked value on failure */
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
egis_dev_free (EgisDev *d)
{
  g_free (d);
}
