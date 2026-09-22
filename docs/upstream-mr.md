# Upstream merge request — draft text

The description for the libfprint merge request, kept here so it is reviewed
with the code it describes. Facts only; every number has a measurement behind
it in this repository. Update it whenever a number in the driver changes.

---

## egis0576: add a driver for the EgisTec EH576 (1c7a:0576)

The EgisTec (LighTuning) EH576 is a small capacitive press sensor — 70×57
pixels of roughly 5.5×4.5 mm of skin (inferred from the measured 6.4 px
ridge period; the sensor's pitch is in no datasheet this project has), no
hardware finger detection — found in Lenovo
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

`temp_hot_seconds = -1`: a capacitive sensor with no emitter, declaring
100 mA at 5 V, driven at ~30 frames/s while an action runs; with the default
model a lock screen left showing its dialog for three minutes would lose
fingerprint authentication to `FP_DEVICE_ERROR_TOO_HOT` for nine. If a
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

Two measured attacks on the *driver* are closed: the sensor was seen
re-serving a stale frame of an earlier press with fresh noise, so a match is
reported only when two consecutive frames clear the threshold and differ; and
a poisoned flat-field baseline (a smudge during the no-finger frames) could
lift an impostor to 0.92, so the confirming frame must also corroborate on its
raw bytes.

A third one is **not** closed, and the driver should not be read as claiming
otherwise: a fabricated ridge-textured artefact. A masked texture correlation
on 5.5×4.5 mm matches any patch of locally parallel ridges at the right period
and angle. A ridge-period consistency check rejects generated textures whose
period is constant (sine 0.87 → 0.70, arc 0.91 → 0.70 against real templates),
but a period-modulated grating, a loop pattern or filtered ridge noise go
through it at 0.80–0.90, and against an attacker who can read the score it is
worth 0.002 NCC. What it rejects it rejects as a non-match, which costs the
presenter an attempt — the engine keeps "nothing to score" (which the driver
answers with a retry) for frames it genuinely could not judge. This sensor offers no liveness signal, so presentation-attack
detection is not available at all; the barrier is physical access plus making
the artefact, as it is for every matcher without liveness detection. The
numbers and the widened-grid measurement are in the out-of-tree
docs/matcher-comparison.md.

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

Hardware, reference unit (Yoga 7 14ARB7, AMD): enrol, verify and identify
through fprintd and the GNOME lock screen; a cancel answered within one
transfer sequence (3 ms measured); s2idle with a verify running, which
parks 1.5 s before the kernel's suspend entry and matches on the first
press after waking; USB autosuspend with `power/control=auto`; no warning
or critical in `G_MESSAGES_DEBUG=all` runs. Three further units are
reported working by their owners (issues linked from the out-of-tree
README), two of them with accuracy datasets.

`scripts/uncrustify.sh --check` passes, and `meson test` on master with
this driver laid in is 38 ok / 0 fail.

**There is no `tests/egis0576/` in this MR, and I would like your view on
what should go in it.** A umockdev recording contains every frame the
sensor delivered, so recording one with a finger would publish a
fingerprint of mine in this repository, which I am not willing to do. The
alternative is a textured non-finger object, and that limits what the test
can assert: this driver matches by correlating ridge texture and checking
that the probe's local ridge period reproduces the template's, and no
household object reproduces a fingerprint's ridge-period statistics — the
best candidate I measured enrols cleanly (coverage 0.63 against a 0.60
gate) with its two presses correlating at 0.947, and the driver still
rejects it, because the period estimator finds 2 usable blocks of 270: its
grooves are finer than the 0.27–0.74 mm the estimator can measure.

So a recording I can make covers open with the exposure calibration, an
empty-gallery identify, twelve enrolment stages including the settle loop
and the placement steering, template serialisation, a verify and an
asynchronous identify that must **not** match, a print with no template
refused with `DATA_INVALID`, the finger-status transitions and a clean
close — everything except the successful-match report, which is covered
by the hardware runs above. The script for exactly that is written and
ready (`tools/upstream/tests/egis0576/custom.py` in the out-of-tree
repository); it needs about fourteen presses at the machine and I will
record it on request. If you would rather have a recording in which the
object matches, that needs a co-operative artefact I have not found, and
I would want to agree with you first that it is worth publishing one.

The wiki's Unsupported-Devices page needs 1c7a:0576 removed (I cannot edit
it).

### Credits

Thaddeus Stepanovich (correlation matcher design, second-unit measurements
and review), the testers sam-dant and irvingpop (datasets from two more
units), and Pengu601 (the first plaintext protocol description), anthonythayes and
sam-dant (compatibility reports and the long-lock measurement that produced
the capture loop's back-off).
