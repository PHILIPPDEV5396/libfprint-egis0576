# The upstream submission

What goes to libfprint is one matcher flavour laid flat into
`libfprint/drivers/egis0576/`, two meson entries, the hwdb bookkeeping and a
umockdev driver test. `make-tree.py` produces exactly that from this
repository, so the submission is reproducible and reviewable here:

```bash
git clone https://gitlab.freedesktop.org/libfprint/libfprint.git /tmp/lf
tools/upstream/make-tree.py /tmp/lf --tests tools/upstream/tests/egis0576
cd /tmp/lf && meson setup build && ninja -C build && scripts/uncrustify.sh --check && meson test -C build
```

The script's docstring lists the file mapping. The merge-request text is
[`docs/upstream-mr.md`](../../docs/upstream-mr.md); the audit it closes is
[`docs/upstream-gaps.md`](../../docs/upstream-gaps.md).

## The driver test (`tests/egis0576/`)

libfprint replays a recorded USB session under umockdev and runs
`custom.py` against it (`tests/README.md` upstream). The recording is made
with a **textured non-finger object** — never a finger, since the pcap
contains every frame the sensor delivered and is committed to a public
repository.

That choice decides what the test can assert, and it is worth stating
plainly. The driver matches by correlating ridge texture and checking that
the probe's local ridge period reproduces the template's. **No household
object reproduces a fingerprint's ridge-period statistics**, and that is not
a defect of the search — it is the check working. The best candidate found
enrols cleanly (coverage 0.63 against a 0.60 gate) and its two presses
correlate at **0.947**, and the driver still rejects every pair, because the
ridge-period estimator finds 2 usable blocks of 270: the object's grooves are
finer than the 0.27–0.74 mm it can measure. Objects tried and measured with
`objprobe.py`: a rigid metal part (contact patch too small, presses reproduce
at only 0.750) and a fine-grooved elastic one (the 0.947 above).

So the test asserts what the recording can honestly support:

- open with the exposure calibration, and an empty-gallery identify that
  must not touch the sensor (what fprintd does before every enrolment);
- twelve enrolment stages, including the settle loop and the placement
  steering, and a template that survives serialisation;
- a verify and an asynchronous identify against that template that **must
  not match** — the object is not a finger, and the driver says so;
- a print carrying no egis0576 template refused with `DATA_INVALID`;
- finger-status transitions around every action, and a clean close.

It pins the whole action path and the driver's refusal of a non-finger. What
it cannot pin is the successful-match report; that path is covered by the
hardware runs in [`docs/matcher-comparison.md`](../../docs/matcher-comparison.md)
and by the live logs quoted there. A maintainer who would rather have a
matching recording can say so — it needs a co-operative artefact, and this
project would rather ship no fingerprint in a public pcap.

Choosing a candidate object, if you want to try anyway: `objprobe.py`
(above) prints per press how many of the 270 blocks have a measurable ridge
period and whether they are pinned at either edge of the 3.5–9.5 px window,
so it says *which way* a texture is wrong. Aim for the sensor's own ridge
period, about 0.5 mm, elastic, with a fingertip-sized contact patch.

Recording, from a libfprint build directory with the driver laid in
(needs `umockdev`, `tshark`, `usbmon` loaded, root):

```bash
sudo modprobe usbmon
sudo tests/create-driver-test.py --test custom egis0576
```

`custom.py` prints what to press when. The result — `tests/egis0576/device`
(the umockdev device dump) and `custom.pcapng` — goes into
`tools/upstream/tests/egis0576/` here and is laid into the checkout by
`make-tree.py --tests`.

Things that make or break the replay, all handled in the driver:

- the number of frames an action consumes is decided by the matcher, so the
  recording pins the build configuration: re-record when any threshold, gate
  or the enrolment stage count changes;
- finger detection is integer-exact and the poll gap is 0 under
  `FP_DEVICE_EMULATION`, so a replay consumes the recorded frames on every
  architecture and is not paced by the recording session's wall clock;
- timeouts replay as 0-byte completions; the drain, the readiness poll and
  the ignored replies all take `n <= 0` as their normal exit;
- the first transfer of a session (the 30 ms drain) can in principle lose a
  race against libusb's own timeout on a very slow replay (valgrind, loaded
  CI); if it ever shows, exclude the test from the valgrind setup rather than
  touching the measured timeout.
