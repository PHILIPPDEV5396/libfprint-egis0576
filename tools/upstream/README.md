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
repository. What works: something conductive with ridges 0.2–0.5 mm apart
(a fine metal file, a coin's reeded edge, a knurled metal part); check a
candidate first with the driver itself (twelve enrolment presses, then a
verify, then a verify with the second object, which must not match).

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
