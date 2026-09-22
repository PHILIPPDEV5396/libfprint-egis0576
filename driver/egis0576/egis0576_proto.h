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
 *
 * Everything here is asynchronous on the device's main loop: the bring-up and
 * the exposure calibration are FpiSsm machines handed back to the driver to
 * start (as a sub-machine of its own, or with a completion callback of its
 * choice); a frame capture is a chain of FpiUsbTransfers with a completion
 * callback, one at a time per EgisDev. Nothing blocks, nothing runs on
 * another thread.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 */
#ifndef EGIS0576_PROTO_H
#define EGIS0576_PROTO_H

#include "drivers_api.h"

#define EGIS_IMG 3990                  /* 70 x 57, 8-bit */

typedef struct EgisDev EgisDev;

/* The transport state for one FpDevice (register cache, exposure calibration).
 * @dev is borrowed and must outlive the EgisDev. The USB interface is the
 * caller's to claim and release. */
EgisDev *egis_dev_new (FpDevice *dev);
void     egis_dev_free (EgisDev *d);

/* CANCELLATION. No transfer built here is ever handed a GCancellable: aborting
 * an in-flight URB wedges this sensor at USB level until board power is cut
 * (measured, see egis0576_proto.c). Instead every machine below consults
 * fpi_device_action_is_cancelled() -- and the stop request below -- at its
 * transfer-sequence boundaries and fails with G_IO_ERROR_CANCELLED there. The
 * latency of a cancel is therefore one transfer SEQUENCE, not one URB: up to
 * ~2.6 s inside a frame capture on a sensor that has stopped answering (6 x
 * 300 ms preamble reads + one 800 ms frame read), ~0.3 s for the init replay
 * on a healthy sensor, 850 ms per readiness-poll iteration. */

/* Ask the running init machine to stop at its next boundary as if the action
 * had been cancelled (it fails with G_IO_ERROR_CANCELLED there). For the
 * driver's suspend: libfprint keeps the action's cancellable, so a park has
 * to be requested another way. Cleared when the next machine starts. */
void egis_dev_request_stop (EgisDev *d);

/* Bring the sensor up: drain the IN endpoint, poll it ready (the vendor's
 * check_and_recovery), replay the vendor init/calibration sequence, then
 * re-apply the calibrated exposure if one is known. ~0.3 s on a healthy sensor.
 *
 * The replay is one unit under fpi_device_critical_enter()/leave(): while the
 * sensor keeps answering, a cancel or stop is honoured before its first
 * record and after its last, never in between, and libfprint defers the
 * suspend vfunc for its duration. Stopping a sensor that is answering
 * half-way would leave it with a prefix of the vendor sequence, a state nobody
 * has tested it in. A sensor that has stopped answering is a different matter:
 * once a record's reply times out, a pending cancel or stop is honoured right
 * there (the sensor is off its sequence already, and the next bring-up starts
 * over), so a stop is never more than one reply timeout plus one command away
 * -- under the 5 s logind gives a sleep delay -- instead of the ~19 s that
 * feeding every remaining record into the void would cost.
 *
 * Fails with FP_DEVICE_ERROR_PROTO if the sensor does not answer the
 * readiness poll. The one case that happens in practice is a sensor left in
 * TLS session mode by a pre-plaintext build of this driver; pass
 * reset_if_stuck=TRUE to issue a ForceResetDevice for it. The request itself
 * is accepted immediately and the sensor stays on the bus; it drops off (and
 * re-enumerates ~0.4 s later) on the NEXT transfer to it, whoever makes it
 * (docs/sensor-tuning.md §6). So this open fails, that next access takes the
 * ENODEV, and the caller gets a fresh device object from the hotplug on which
 * the open then succeeds. */
FpiSsm *egis_dev_init_ssm (EgisDev *d, gboolean reset_if_stuck);

/* Capture one EGIS_IMG-byte frame into @img (which must stay valid until @cb
 * ran): the per-frame trigger sequence, GetFrame, and the bytes in however
 * many chunks they come. ~0.1 s. Not a machine of its own but a chain of
 * transfers, so that the capture loop's ~30 frames a second cost no FpiSsm
 * each. @cb gets NULL, or the error (transfer full): FP_DEVICE_ERROR_PROTO
 * for a short or missing frame, the gusb error for a command the sensor did
 * not accept. The sequence arms the sensor for a frame and runs to its end;
 * the caller checks for a cancel before starting it. One frame at a time:
 * starting another before @cb ran is a programming error (refused, @cb never
 * called). */
typedef void (*EgisFrameCb) (FpDevice *dev,
                             gpointer  user_data,
                             GError   *error);
void egis_dev_frame (EgisDev    *d,
                     guint8     *img,
                     EgisFrameCb cb,
                     gpointer    user_data);

/* Per-unit exposure calibration: a bounded binary search over register 0x0f so
 * the no-finger frame mean is the same on any EH576 unit, whatever per-unit
 * values the baked init carries. Needs a no-finger window, so the driver runs
 * it once, in the first open(). The value found is kept in this EgisDev and
 * must also be kept by the OWNER across EgisDev instances (egis_dev_get/
 * set_calibration): the driver opens a fresh EgisDev per open(), and the
 * bring-up replay puts reg 0x0f back to the baked value each time, so the init
 * machine re-applies the handed-in value after every replay. Templates must be
 * re-enrolled if it moves the exposure. Fails on a capture error, leaving the
 * register at the last value tried and the calibration unset, so the next
 * open's replay restores the baked value and the search is retried. The
 * caller skips it when a calibration was handed in. */
FpiSsm *egis_dev_calibrate_ssm (EgisDev *d);
void    egis_dev_set_calibration (EgisDev *d, int dc_c);   /* -1 = none */
int     egis_dev_get_calibration (EgisDev *d);              /* -1 = none */

#endif
