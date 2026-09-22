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
with two **textured non-finger objects** — never a finger, since the pcap
contains every frame the sensor delivered and is committed to a public
repository.

Finding an object that works is trial and error, so check candidates with
the fast probe (two presses, ~20 s, no enrolment) rather than a full
enrolment:

```bash
tools/upstream/objprobe.py
```

It prints the matcher's coverage per frame and two scores that are not the
same thing: the **correlation** between the two presses (how alike they are)
and the **decision** the driver would see, which is that correlation or -1
when the ridge-period consistency check rejects the pair as
synthetic-looking. A tool that reads only the decision cannot tell "these
presses do not match" from "these presses match and the check threw it
away", and its maximum lands just under the check's 0.70 gate. What the
probe is looking for:

- **coverage ≥ 0.60 on both presses** — the object must produce ridge-*like*
  structure, not just contrast. A rigid metal part gives plenty of variance
  (measured 1300–2500, far above the finger-on threshold of 250) and still
  fails here: its texture is not a ridge field, so the coherence mask keeps
  almost nothing.
- **best cross NCC ≥ 0.78** — two presses must reproduce each other. This is
  where rigid objects fail for a second reason: a finger deforms and lands
  the same way twice, a rigid object touches somewhere slightly different
  every time. Measured on one candidate: 12 enrolment frames that correlated
  0.16–0.67 *with each other*, against 0.82–0.97 for a finger.

- **no rejections by the period check** — the driver rejects a pair whose
  ridge periods are not consistent with the template's. That is the
  anti-spoofing check, and it fires on two kinds of object: one whose texture
  is a regular grating (what it was built for), and one whose contact patch
  is too small to give the check enough blocks to look at. The probe says
  which, per pair.

So the useful shape is **elastic, with irregular ridge-like texture at
0.2–0.5 mm, and a contact patch as large as a fingertip's**: textured rubber
or silicone, an eraser with a pattern pressed into it, a piece of leather.
Avoid regular gratings (a coin's reeded edge): the period check exists to
reject exactly those.

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
