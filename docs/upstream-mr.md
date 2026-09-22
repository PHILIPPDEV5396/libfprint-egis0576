# Upstream merge request — draft text

The description for the libfprint merge request, kept here so it is reviewed
with the code it describes. Facts only; every number has a measurement behind
it in this repository. Update it whenever a number in the driver changes.

---

## egis0576: add a driver for the EgisTec EH576 (1c7a:0576)

The EgisTec (LighTuning) EH576 is a small capacitive press sensor — 70×57
pixels, 3.5×2.9 mm of skin, no hardware finger detection — found in Lenovo
consumer laptops (reference: Yoga 7 14ARB7; also reported working on an
IdeaPad Flex 5 14ITL05 and one further unit by testers of the out-of-tree
driver). It has no vendor Linux driver. This driver has been in out-of-tree use
since July 2026 (https://github.com/PHILIPPDEV5396/libfprint-egis0576, which
keeps the long-form documentation and the measurement kit referred to below).

### Protocol

The sensor speaks a plaintext command protocol over two bulk endpoints
(`"EGIS"` + command out, `"SIGE"` + reply in), the same one its Windows
driver uses. The driver replays the vendor driver's bring-up sequence (33
records, recorded from that driver's own sessions), runs a bounded binary
search on the exposure register once per process so the no-finger frame mean
is the same on every unit, and fetches 3990-byte frames with a fixed
per-frame command sequence. The sensor also has a TLS-PSK session mode; it is
never entered (it is a one-way door for other operating systems, and it
measurably buys nothing).

Two hardware facts shape the code and are documented at the point of use:

- **An aborted URB wedges the sensor** until board power is cut (measured:
  bulk OUT NAKs for the 3 s timeout, ForceReset ignored, port reset only makes
  it drop off the bus). Every `fpi_usb_transfer_submit` therefore passes a
  NULL cancellable; cancellation is checked between transfer sequences, and
  the bring-up replay runs as one unit under `fpi_device_critical_enter`.
- **The sensor does not come back from s2idle initialised** (it stays
  powered, the capture pipeline is wedged). The driver re-runs the bring-up
  after resume, and a frame that fails is answered with one re-init and a
  retry before the action fails.

### Architecture

An `FpDevice` (not an `FpImageDevice`, see *Matcher*): one capture `FpiSsm`
per action, the transport as `FpiSsm` machines (bring-up, calibration) and a
transfer chain (frame) over `FpiUsbTransfer`, the matcher in a `GTask` thread
(the `secugen` pattern). Finger presence is detected from the raw frame's
variance (integer-exact, so a recorded test replays identically on every
architecture) and reported as `FP_FINGER_STATUS_PRESENT`.

Suspend keeps the action, as `fpi_device_suspend_complete()` asks: the capture
machine parks at its next transfer boundary and completes the suspend only
then (a re-init in flight stops at its own boundaries, so the park never
outlasts logind's delay); resume restarts it at the loop head with a fresh
bring-up. Measured on the reference unit with the system's own s2idle: parked
1.5 s before `PM: suspend entry`, the same verify matched on the first press
after waking. A device removed while parked is released through
`notify::removed`. USB autosuspend (`ID_AUTOSUSPEND=1`, the entry moves from
the known-unsupported list to the driver's block) was validated: the sensor
autosuspends 2 s after close, comes back on open, and is held active while
open.

`temp_hot_seconds = -1`: a capacitive sensor with no emitter, driven at ~30
frames/s while an action runs; with the default model a lock screen left
showing its dialog for three minutes would lose fingerprint authentication to
`FP_DEVICE_ERROR_TOO_HOT` for nine, for a device that does not warm. If a
maintainer prefers the default model here, that is a one-line change.

### Matcher

libfprint's own path for image sensors is NBIS. On these frames `mindtct`
finds a median of **one** minutia (714-frame dataset, 33 % of frames with
none, max 7; with perimeter points kept, median two) — nothing `bozorth3` can
match. An own extractor found ~7 per frame with ~50 % repeatability between
adjacent frames of one press; mosaicking is useless because presses land on
the same spot. So the driver matches by correlation: each of 12 enrolment
presses is kept as a frame; a probe frame is compared with every stored frame
by masked normalised cross-correlation over a shift and rotation search, on a
Gabor-filtered ridge map with a local-ridge-period consistency check. The
`EmFrame` representation and the two-function interface are Thaddeus
Stepanovich's design (his LGPL header is included unchanged); the front-end,
the check and the driver-side adapter are original work.

Measured (kit in the out-of-tree repository; one person per unit, impostors =
the same person's other fingers, which is the ceiling this could be measured
against):

- reference unit, 60 genuine / 480 impostor presses: genuine NCC
  0.80 / 0.97 / 0.99 (min / median / max), impostor 0.06 / 0.41 / 0.69; at the
  shipped threshold 0.78: 0 false rejects, 0 false accepts; held-out
  threshold across folds 0.75, same result;
- a second unit (T. Stepanovich, Yoga 7 16IRL8, his own captures, 29
  genuine / 88 impostor decisions, held-out cross-fold): 6.9 % FRR at 1.1 %
  FAR — his populations overlap (weakest genuine 0.60, strongest impostor
  0.815), so the threshold is unit-dependent and this is the number to quote,
  not the 0 / 0 above;
- a session five days later on the reference unit: 0.83–0.94 on fingers
  whose presses covered the enrolled skin, which is what the enrolment
  steering (refusing a press that lands where one already is) is for;
- live through fprintd: genuine presses 0.86–0.95, another finger 0.55.

Two measured attacks on the *driver* are closed, not just the matcher: the
sensor was seen re-serving a stale frame of an earlier press with fresh noise,
so a match is reported only when two consecutive frames clear the threshold
and differ; and a poisoned flat-field baseline (a smudge during the no-finger
frames) could lift an impostor to 0.92, so the confirming frame must also
corroborate on its raw bytes.

**Templates are frames.** A correlation matcher has nothing else to match
against; the stored bytes are flat-fielded images of the fingertip, 47,880
bytes for a print, in fprintd's root-only store like every other driver's
template. Reviewers should weigh that; the alternative was no driver.

Physical artefacts were not tried. Same-person impostors only. The threshold
is calibrated on one unit; the second unit's held-out optimum was 0.81.

### Provenance and licensing

Everything submitted is LGPL-2.1-or-later. The vendor's Windows driver was
examined for interoperability only, and what came out of it is protocol fact:
the bring-up record bytes, the per-frame command sequence and the roles of
four registers (the same values Windows keeps in the device's registry entry).
No vendor code, table or constant is in the submission; the out-of-tree
repository also carries a reverse-engineered vendor matcher for comparison,
which is not and will not be submitted. The first published description of
the plaintext framing was the unlicensed Pengu601/EgisTec-EH576 project; a
written statement of what was and was not taken from it is in the
repository's PROVENANCE.md (in short: nothing but the per-frame command
sequence, which both projects observed from the vendor driver independently).

### Testing

- `tests/egis0576/custom.py` with a umockdev recording made with two
  textured non-finger objects (no fingerprint is committed): enrol, verify
  (match), identify (match), verify against the other object (no match),
  identify with an empty gallery, in one session.
- Hardware, reference unit: enrol / verify / identify through fprintd and
  the GNOME lock screen; cancel answered within one transfer sequence
  (3 ms measured); s2idle with a verify running (above); autosuspend with
  `power/control=auto`; no warning or critical in `G_MESSAGES_DEBUG=all`
  runs.
- `scripts/uncrustify.sh --check` passes; `meson test` 38 ok / 0 fail on
  master with the driver laid in.

The wiki's Unsupported-Devices page needs 1c7a:0576 removed (I cannot edit
it).

### Credits

Thaddeus Stepanovich (correlation matcher design, second-unit measurements
and review), the testers sam-dant and irvingpop (datasets from two more
units), and Pengu601 (the first plaintext protocol description).
