# What the sensor can and cannot be tuned to

Measured on the reference unit (Lenovo Yoga 7 14ARB7, `bcdDevice` 15.72) after the
v0.4.0 plaintext migration, to answer one question: **is there image quality left
on the table?**

Short answer: **no.** The driver already runs at the sensor's operating point.
Three plausible improvements were tested and all three measured out as
non-improvements. This document records them so they are not re-litigated.

## 1. Uploading a measured background to the sensor (`0x73`) — no effect

Windows persists a per-device background frame (registry value `vdm_bk`, 3990
bytes, on this unit `min 20 / max 37 / mean 31.7 / var 4.0`) and a `bad_pixel` map
(all zero here — this unit has no defective pixels). The driver's init instead
uploads a constant `0x20` fill after `EGIS 73 0f 96`.

Replacing that with a freshly captured no-finger frame **is accepted by the sensor**
(it answers `SIGE 0f 96 01`) but changes nothing on the image:

| | frame mean | frame variance |
|---|---:|---:|
| before re-upload | 93.4 | 140.4 |
| after re-upload | 93.4 | 139.9 |

So `0x73` is not a background the sensor subtracts on this fetch path. Windows may
use it in another one — its registry carries `FetchImageMode = 0x80000003`, and
`EGIS 64 0f 96` is only one way to fetch a frame. Establishing that is open
research, not a change worth guessing at.

The driver's own per-boot, host-side flat field already removes fixed-pattern
noise; that is unaffected by any of this.

## 2. Raising the gain (reg `0x12`, `sensor_gain`) — no signal to gain

Sweep with a finger held still, against no-finger baselines at the same setting.
"Ridge/noise" is the ratio of spectral energy in the ridge band (period 4–12 px)
to the noise band (period < 3 px).

| gain | finger var | no-finger var | ratio | ridge/noise | clipped low |
|---|---:|---:|---:|---:|---:|
| 0x03 | 498.6 | 90.7 | **5.50×** | **11.11** | 0 |
| 0x04 | 614.2 | 114.9 | 5.35× | 10.87 | 0 |
| **0x05** (shipped) | 741.7 | 140.4 | 5.28× | 10.46 | 1 |
| 0x06 | 866.5 | 170.5 | 5.08× | 10.10 | 18 |
| 0x07 | 1002.8 | 203.1 | 4.94× | 9.97 | 83 |
| 0x08 | 1200.9 | 237.3 | 5.06× | 10.66 | 250 |

Signal and noise scale together — the ratio is flat to slightly falling — and from
`0x06` upward the sensor starts clipping pixels to 0, destroying information. The
shipped `0x05` is effectively the right setting. The marginally better numbers at
`0x03`/`0x04` are within run-to-run spread and would not justify moving the
operating point (which invalidates enrolled templates).

## 3. The exposure register (reg `0x0f`, `sensor_dc_c`) — a razor-thin window

This is the register the driver's calibration actually searches. Its transfer
curve is far steeper than the code's `[0, 0x3f]` search range suggests:

| dc_c | no-finger mean | no-finger var | |
|---|---:|---:|---|
| 0x10 | 0.0 | 0.0 | fully black |
| 0x18 | 0.0 | 0.0 | fully black |
| **0x20** (init value) | **93.7** | **140.7** | the only usable setting tested |
| 0x28 | 255.0 | 0.2 | fully white |
| 0x30 | 255.0 | 0.0 | fully white |
| 0x38 | 255.0 | 0.0 | fully white |

The sweep stepped in 8s, so the usable window is bounded by the measurements at
roughly `0x1C`–`0x24` rather than measured edge to edge. The baked init value sits
inside it, and the calibration target (frame mean `0x58` = 88) is within a few counts
of what `0x20` produces unaided — which is why calibration is close to a no-op on
this unit, exactly as the comment on `egis_dev_calibrate`
(`driver/egis0576/egis0576_proto.c`) says.

**Consequence for per-frame auto-exposure:** `egis_dev_autoexpose` exists and, since
the plaintext migration, register reads may interleave with capture, so it is now
mechanically possible (verified: frame variance 140.1 before a register read,
140.4 after). It is deliberately **not wired into the capture loop.** Its step-table
arithmetic computes jumps that would overshoot a ±4-count window straight into
saturation, and there is no unit available on which a badly-exposed starting point
could be tested. Shipping an untested behaviour change to other people's hardware
is the mistake that produced the TLS transport.

## 4. Cancelling an in-flight USB transfer — wedges the sensor, do not

Measured 2026-09-13 while shortening the driver's cancel latency. A revision of
`egis0576_proto.c` passed a `GCancellable` to every `g_usb_device_bulk_transfer`
/ `g_usb_device_control_transfer`, so that a cancel would abort the URB the
worker was blocked in instead of waiting out its timeout. The mechanism itself
worked exactly as intended:

| step | result |
|---|---|
| getframe with a pre-cancelled cancellable | returned `CANCELLED` in **0.0 ms** |
| cancel from another thread mid-capture-loop | loop stopped after **30 ms** (a read timeout is 800 ms) |
| **next** getframe, cancellable cleared | `GetFrame failed` after **18 s** — six bulk-OUT timeouts of 3 s |

That last line is the finding. The sensor whose IN transfer was unlinked
mid-frame stopped accepting bulk OUT altogether. It then ignored the EP0
`ForceResetDevice`, a sysfs `authorized=0` timed out (`ETIMEDOUT`), a hub-port
link reset made the kernel re-enumerate it and fail on the very first descriptor
read (`device descriptor read/64, error -110`), and the device dropped off the
bus. Only cutting board power brought it back.

A transfer *timeout* never does this: it fires only when no data is flowing. A
*cancel* can land in the middle of a frame's data, and this firmware does not
survive that. So the rule for this sensor is: **never hand a `GCancellable` to
gusb.** The driver instead consults the cancellable at every transfer boundary
(between readiness-poll iterations, before and after the init replay, at
getframe entry, in the recovery path). The init replay and a getframe's
preamble-plus-read are each sent as one unit and checked only at their
boundaries — abandoning either half-way would be an untested sensor state — so
the cancel latency bound is one such sequence: up to ~2.6 s inside a getframe
on a sensor that has stopped answering, against ~11 s before. That is the
deliberate trade for never touching a URB in flight.

The proof/regression harness for this lives outside the repository
(`cancel-proof.c`: pre-cancelled → immediate; mid-loop cancel → stops at the next
boundary; and, decisively, *the sensor is still usable afterwards*). It refuses
to run against a build that passes a cancellable to gusb.

## What this means

The driver is at the sensor's operating point. The remaining limits are physical:
a 70×57 px patch carries too little ridge area for any clean-room matcher that has
been tried — an early `FpImageDevice` version measured 0 % genuine accept with
NBIS minutiae, and a later study of five further approaches reached usable
genuine-accept rates but could not keep an adjacent same-hand finger out. That is
why the vendor's own matcher is used; see
[`PROVENANCE.md`](../PROVENANCE.md#why-the-reverse-engineered-matcher-and-not-a-clean-room-one).

**Not measured, and worth being honest about:** no EER/FAR/FRR has ever been
established for this driver. The security statement in the README is a qualitative
observation on one unit, not a certified accuracy figure.

## Reproducing

The measurement scripts are not shipped in this repository — they drive the sensor
directly over raw USB and are development tools, not part of the driver. The
procedure is: `ForceResetDevice`, replay `egis_init.h` in the clear, then for each
register setting capture no-finger and finger frames with the per-frame trigger
sequence plus `EGIS 64 0f 96`, and compare variance and ridge-band energy.
