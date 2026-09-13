/* egis0576_proto.h — plaintext EGIS/SIGE protocol to the EgisTec EH576.
 *
 * This is the protocol the sensor's own vendor driver speaks. Commands go out as
 * "EGIS" + cmd + two parameter bytes on bulk EP 0x01; replies come back as "SIGE"
 * + echoed selector + value + status on EP 0x82. Image frames are 3990 raw bytes
 * (70x57, 8-bit) fetched with "EGIS 64 0f 96".
 *
 * It replaces an earlier TLS-PSK transport. That mode exists in the sensor and is
 * reachable with class request 0x21/9 wValue=0, but the vendor driver installed
 * for this device never uses it, entering it is a one-way door that leaves the
 * sensor unusable for any other OS until a ForceResetDevice, and it measurably
 * bought nothing: driving this same command sequence in the clear yields a better
 * signal-to-baseline ratio (finger/no-finger frame variance 1069/140 = 7.6x,
 * against 890/160 = 5.6x through the TLS path on the same unit).
 */
#ifndef EGIS0576_PROTO_H
#define EGIS0576_PROTO_H

#include <glib.h>
#include <gio/gio.h>
#include <gusb.h>

#define EGIS_IMG 3990                  /* 70 x 57, 8-bit */

typedef struct EgisDev EgisDev;

/* Bind to an already-claimed GUsbDevice. */
EgisDev *egis_dev_new (GUsbDevice *usb);
void     egis_dev_free (EgisDev *d);

/* Attach the capture's GCancellable. BORROWED: the caller keeps the only ref
 * and must detach (NULL) or free it only after no egis_dev_* call can be in
 * flight. Pass NULL to detach.
 *
 * It is a FLAG, consulted between transfers only -- it is never handed to
 * gusb. Measured on hardware (2026-09-13): aborting an in-flight bulk transfer
 * (what gusb does with a cancelled GCancellable) wedged the sensor at USB
 * level until a board power cycle. So a cancel is honoured at the next transfer
 * boundary: before/after the readiness poll's iterations, before and after the
 * init replay (which is sent as one unit), at getframe entry. The latency bound
 * is therefore one transfer *sequence*, not one URB: up to ~2.6 s inside a
 * getframe on a sensor that has stopped answering (6 x 300 ms preamble reads +
 * one 800 ms frame read), ~0.3 s for the init replay on a healthy sensor, and
 * 850 ms per readiness-poll iteration. egis_dev_* then fail with
 * G_IO_ERROR_CANCELLED. */
void     egis_dev_set_cancellable (EgisDev *d, GCancellable *cancellable);

/* Bring the sensor up: poll it ready (the vendor's check_and_recovery), then
 * replay the vendor init/calibration sequence. Blocks (~0.3 s).
 *
 * The replay resets the exposure register (0x0f) to the baked value; if
 * egis_dev_calibrate has already run in this process, the calibrated value is
 * re-applied at the end of the open, so the exposure (and the exposure-tied
 * flat-field baseline) stays valid across every re-open and post-resume re-init.
 *
 * Fails with G_IO_ERROR_NOT_INITIALIZED if the sensor does not answer the
 * plaintext readiness poll. The one case that happens in practice is a sensor
 * left in TLS session mode by a pre-plaintext build of this driver; pass
 * reset_if_stuck=TRUE to issue a ForceResetDevice for it. The request itself
 * is accepted immediately and the sensor stays on the bus; it drops off (and
 * re-enumerates ~0.4 s later) on the NEXT transfer to it, whoever makes it
 * (docs/sensor-tuning.md §6). So this open fails, that next access takes the
 * ENODEV, and the caller gets a fresh device object from the hotplug on which
 * the open then succeeds. */
gboolean egis_dev_open (EgisDev *d, gboolean reset_if_stuck, GError **error);

/* Capture one EGIS_IMG-byte frame: per-frame trigger sequence, then GetFrame.
 * Blocks ~0.1 s. img must hold EGIS_IMG bytes. */
gboolean egis_dev_getframe (EgisDev *d, guint8 *img, GError **error);

/* One-time per-device exposure calibration (the vendor's calibrate_gain): a
 * binary search over register 0x0f so the no-finger frame mean is the same on
 * any EH576 unit, whatever per-unit values the baked init carries. Runs once per
 * fprintd process, before any capture; the value found is cached process-wide
 * and re-applied by egis_dev_open after every later bring-up (the sensor itself
 * does not keep it -- the replay overwrites it). Templates must be re-enrolled
 * if it moves the exposure. Returns FALSE on capture error, leaving the
 * register at the last value the search tried and the process-wide cache unset,
 * so the next open's replay restores the baked value and the search is retried. */
gboolean egis_dev_calibrate (EgisDev *d, GError **error);

/* One auto-exposure step from a frame's min/max; TRUE iff the exposure changed,
 * so the caller can re-take its flat-field baseline. Mechanically possible
 * since the plaintext migration (register reads may interleave with capture),
 * but deliberately NOT wired into the capture loop: its step-table jumps would
 * overshoot reg 0x0f's ~8-count usable window into saturation. See the comment
 * on the definition in egis0576_proto.c and docs/sensor-tuning.md. */
gboolean egis_dev_autoexpose (EgisDev *d, int frame_min, int frame_max);

#endif
