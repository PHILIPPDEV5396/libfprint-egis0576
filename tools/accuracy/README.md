# Matcher accuracy kit for the EH576

Reproduce the FAR/FRR measurement in `docs/matcher-comparison.md` on your own
EH576 laptop, with your own fingers, for both host matchers (vendor and
clean-room) — and report a **summary** back, never the frames. About 15
minutes end to end.

```
tools/accuracy/
  capture.py     acquire a dataset from the sensor (pyusb + numpy)
  egis_eh576.py  the plaintext EGIS/SIGE transport capture.py uses (pyusb only)
  score.c        offline scorer, built once per matcher from driver/egis0576
  Makefile       -> score-vendor, score-cleanroom (C compiler + libm, nothing else)
  evaluate.py    two-fold FAR/FRR for both flavours -> table + results.json
```

## PRIVACY — read this first

The frames `capture.py` writes are **images of your fingerprints: your
biometric data.** They are stored in a directory only your user can read
(mode 0700, under your `$HOME`, never `/tmp`), they are only ever read by the
scorer on your machine, and they are **not to be posted, attached, uploaded or
sent to anyone — including the maintainers of this driver.** Nobody working on
this driver will ever ask you for them; if someone does, refuse.

The only thing to share is `results.json` from `evaluate.py`. It contains:

- scores, counts, thresholds and error rates;
- your finger *labels* (the `--fingers` names; keep them generic), press and
  frame counts;
- the exposure calibration values (two register bytes, a mean grey level and
  the six-step search trace: six `[register, mean]` pairs);
- the capture date and the evaluation date (day only);
- the `--model` and `--distro` strings you *choose* to pass;
- the `--note` text, **verbatim** — it is free text that goes straight into the
  shareable file, so put nothing personal in it (no names, no serial numbers,
  no e-mail addresses).

It contains no frame data, no file names, nothing an image could be rebuilt
from. `capture.py` refuses to write under `/tmp`, `/var/tmp`, `/dev/shm` or
inside the repository (symlinks resolved), and `evaluate.py` never copies
frames anywhere: the scorer gets its frame lists in memory, or in a private
directory inside the dataset.

When you are done, delete the dataset directory if you do not want to keep it:
`rm -rf ~/egis-accuracy/<date>`.

## 1. Requirements

- an EgisTec EH576 (`lsusb -d 1c7a:0576`), with this driver installed so the
  sensor is in plaintext mode (`install.sh`);
- Python 3 with `pyusb` and `numpy` (Fedora: `sudo dnf install python3-pyusb
  python3-numpy`; Debian/Ubuntu: `sudo apt install python3-usb python3-numpy`;
  Arch: `sudo pacman -S python-pyusb python-numpy`). Use the distro packages,
  not `pip install --user`: a per-user copy is invisible to root, so `sudo
  python3 capture.py` would stop at once with "numpy is required". The dry run
  below warns when that is the case;
- `gcc` and `make`; libm — nothing else, no libfprint build tree, no meson,
  no glib (other compilers are untested: the vendor sources need gcc's
  relaxed flags);
- `sudo`: the capture stops and starts `fprintd` (it owns the sensor while it
  runs) and needs permission to open the USB device. The documented way is
  to run `capture.py` itself with `sudo` (section 3): it writes into *your*
  home and hands every file to *you* as it is written, and root needs no sudo
  timestamp for the `unmask`/`start` at the end — a timestamp expires after a
  few minutes (5 on Fedora), i.e. in the middle of a capture. If you prefer
  to run it unprivileged, or to use the `probe`/`reset` helpers without
  `sudo`, add a udev rule once:

  ```
  # /etc/udev/rules.d/70-egis-eh576.rules
  SUBSYSTEM=="usb", ATTRS{idVendor}=="1c7a", ATTRS{idProduct}=="0576", TAG+="uaccess"
  ```
  The number matters: `uaccess` is applied by
  `/usr/lib/udev/rules.d/73-seat-late.rules`, so the file must sort before it
  (`99-` silently does nothing). `uaccess` grants the device to whoever is
  logged in at the console, on every systemd distribution, no group needed.
  On a Debian system without it use `GROUP="plugdev", MODE="0660"` instead and
  be in that group. Then `sudo udevadm control --reload && sudo udevadm trigger`.

Check everything without touching the sensor (works without pyusb installed,
and tells you so):

```
python3 tools/accuracy/capture.py --dry-run
```

## 2. Build the scorers

```
make -C tools/accuracy
```

produces `tools/accuracy/score-vendor` and `tools/accuracy/score-cleanroom`,
compiled straight from `driver/egis0576/`. The vendor sources need the relaxed
flag set the libfprint build also uses for them (`-w -fno-stack-protector
-fpermissive -fwrapv ...`); the clean-room matcher is plain C99. `make -C
tools/accuracy clean` removes everything again. The kit can be run from any
directory; it finds the repository from its own location.

## 3. Capture (about 10 minutes)

```
sudo python3 tools/accuracy/capture.py
```

Defaults: five fingers (`R-thumb,R-index,L-index,R-middle,L-middle`), 12
presses each, dataset in `~/egis-accuracy/<date_time>/` — *your* home, not
root's, and every directory and file is handed to your user the moment it is
created. Change with `--fingers a,b,c`, `--presses N` (even), `--out DIR`.
`--out` must not exist yet or must be an empty directory: a non-empty one is
refused; a pre-existing empty `--out` is made 0700 and handed to your user only
if that is actually needed, and beyond that only directories the tool created
itself are ever chowned (a pre-existing parent is left alone).

Running it without `sudo` (udev rule, section 1) works too: the tool then
refreshes the sudo timestamp after every accepted press (`sudo -n -v`, never
prompting) so the fprintd `unmask`/`start` at the end does not run into an
expired timestamp; if it does anyway, the manual commands are in the
troubleshooting section.

What the tool does, in order:

1. `sudo systemctl mask --runtime fprintd` and `sudo systemctl stop fprintd`.
   The *runtime* mask keeps D-Bus from re-activating fprintd behind your back
   during the capture (a lock screen or a login prompt would), and it vanishes
   at reboot, so a tool that died cannot leave fprintd masked for good. If
   either command fails the tool stops before touching the sensor.
2. ForceReset of the sensor in a separate process. The request is accepted
   at once but takes effect on the *next* transfer to the device, which then
   drops off the bus and re-enumerates (~0.4 s); the reset child sends that
   transfer itself and returns only when the fresh device answers, so nothing
   that runs after it — the capture, or fprintd at the end — inherits an
   armed reset. A fresh process avoids stale libusb handles.
3. Replay of the driver's own bring-up from `driver/egis0576/egis_init.h`.
4. **The driver's per-open exposure calibration**: a six-step binary search
   over register `0x0f` until the no-finger frame mean is closest to `0x58`,
   exactly as `egis_dev_calibrate()` does it. This is what makes the capture
   valid on a unit whose operating point differs from the reference unit's:
   without it such a unit records saturated frames. The register value before
   and after, the search trace and the final no-finger mean are recorded in
   `manifest.json`; `evaluate.py` copies all of them, the trace included, into
   `dataset.exposure` of `results.json`.
5. The flat-field baseline (mean of 8 no-finger frames), then the fingers,
   one at a time. The manifest is rewritten after every press.
6. Afterwards: release the sensor, ForceReset it again, `unmask` and `start`
   fprintd — also when you Ctrl-C it.

What it records is exactly what the driver sees: baseline = mean of 8
no-finger frames; finger-on at raw variance ≥ 250, finger-off below 215; every
frame while the finger is down (max 12), because the driver's verify loop
scores every one of them and enrols the first.

**Do not lock the screen or switch user during the capture** (and do not
suspend): fprintd is stopped and masked, so the lock screen would have no
fingerprint login anyway, and anything that reactivates it competes for the
sensor.

Tips for a meaningful dataset:

- **5 fingers × 12 presses** is the recommended size: 12 presses give 6 for
  enrolment and 6 genuine trials per fold (60 genuine trials in total), and
  five fingers of the same person give 480 *hard* impostor trials — adjacent
  same-hand fingers are the case that separates matchers. Fewer presses make
  the FRR figure too coarse to compare; more is welcome.
- Press as you would to unlock the laptop: natural pressure, hold about a
  second, lift completely. Vary the placement a little between presses (a bit
  left/right, a bit rotated) — that is what a real day of logins looks like,
  and the tail of that distribution is what the numbers are about.
- Do not run it right after a suspend/resume; reboot-fresh is best. Do not
  unplug, suspend, or `kill -9` the tool mid-capture: the sensor must never
  have a USB transfer aborted under it (`docs/sensor-tuning.md` §4).

Stopping early is fine: Ctrl-C, a plain `kill` (SIGTERM) and closing the
terminal (SIGHUP) all take the same path — the tool keeps what was captured,
releases and resets the sensor and gives fprintd back, and a second Ctrl-C
during that cleanup is ignored. Ctrl-\ (SIGQUIT) is treated like Ctrl-C: it
takes the same path instead of core-dumping the tool mid-transfer. **Ctrl-C
is honoured at the end of the current frame / sequence**, never in the middle
of one: a frame's trigger sequence plus its reads, the bring-up replay and
one calibration step each arm the sensor as a unit, and abandoning one
half-way is an untested sensor state (the driver makes exactly the same
choice, `docs/sensor-tuning.md` §4). On a healthy sensor that is a few
milliseconds after the keypress; **on a sensor that has stopped answering, a
frame is up to ~2.6 s of read timeouts and the bring-up replay up to ~20 s**;
a bulk-OUT write timeout (3 s) ends the sequence early, because the sensor is
then already unresponsive — wait it out, do not `kill -9`. One signal is
*dropped* rather than deferred: a Ctrl-C (or SIGTERM/SIGHUP/SIGQUIT) that
arrives while the step 2 reset child runs — the tool is immune for the
child's whole lifetime, nothing is recorded, and the capture simply carries
on; repeat it at the next prompt. Note that the sensor *is* reset after a
closed terminal too, just without you seeing it. The reset child itself
ignores Ctrl-C entirely (it is over in a second or two and has nothing to
clean up). A partial dataset evaluates (see below).

If the tool ever dies hard while running under `sudo` (a `kill -9`, a power
loss), frames and directories are handed to your user right after they are
created, so there should be nothing root-owned left; if there is anyway,
`sudo chown -R $USER: <your --out directory>` fixes it. A hard death also skips the
unmask: `sudo systemctl unmask --runtime fprintd && sudo systemctl start fprintd`,
or reboot.

## 4. Evaluate

```
python3 tools/accuracy/evaluate.py ~/egis-accuracy/<date_time> \
    --model "Lenovo Yoga 7 14ARB7" --distro "Fedora 44"
```

`--model` and `--distro` are optional and are the only hardware information
recorded; leave them off if you prefer. Add `--best-frame` for the sensitivity
variant (enrol the highest-contrast frame of each press instead of the first)
and `--flavour vendor` / `--flavour cleanroom` to run one matcher only.

A dataset from an interrupted capture works: fingers with fewer than two
presses (or with a frame missing) are skipped with a warning and listed under
`dataset.skipped` in `results.json`; `--fingers a,b,c` evaluates a subset. With
a single evaluable finger there are no impostor trials, so FAR and EER are
reported as `null`.

For each flavour it prints the table `docs/matcher-comparison.md` uses:

```
| | genuine (n = 60) | impostor (n = 480) |
|---|---:|---:|
| score min / median / max | 5609 / 8572 / 12436 | 0 / 0 / 0 |
| at threshold 5000 | FRR 0.0 % | FAR 0.0 % |
```

and writes `results.json` (by default into the dataset directory; `--out` to
put it elsewhere). A fold that could not be scored is recorded under
`failures` with the scorer's exit code and a fixed reason string, never with
the scorer's raw output.

## 5. Report

Open an issue titled "accuracy: <laptop model>" and paste **`results.json`,
nothing else.** Mention anything unusual about the run (sweaty/dry fingers,
a re-run, a finger that never triggered). Do not attach frames, screenshots
of frames, or the dataset directory, and do not attach it to an e-mail either.

## 6. Caveats — what one run does and does not show

- It is **one person, one session, one flat-field baseline.** It is not a
  population error rate. Skin, ridge spacing and finger size differ between
  people; the per-boot flat field is the thing most likely to move scores
  between sessions. Runs from more people and from a second session after a
  reboot are exactly what is missing.
- The impostor set is the **same person's other fingers, adjacent fingers
  included** — deliberately the hard case, and a stricter test than random
  strangers. A FAR of 0 % on 480 such trials says a lot; a FAR above 0 %
  on this set is a real finding.
- Scores are not comparable between flavours except through the operating
  point: the vendor matcher's score is its own minutiae score; the clean-room
  score is an NCC scaled so that its published operating point 0.53 lands on
  the driver's threshold 5000 (divide by 9434 to get the NCC back).
- The enrolment used here (first finger-on frame of six presses) is the
  driver's, but the evaluation deliberately keeps every press that produced
  at least one finger-on frame, so a press the driver's enrolment would have
  asked you to repeat is still counted.

## Troubleshooting

The `probe` and `reset` helpers talk to the sensor directly, so **stop fprintd
first** (it holds the device otherwise) and let it come back afterwards. They
need the udev rule from section 1, or run them with `sudo` too:

```
sudo systemctl stop fprintd
python3 tools/accuracy/egis_eh576.py probe     # PLAINTEXT / TLS / no answer
python3 tools/accuracy/egis_eh576.py reset     # ForceResetDevice; returns once the sensor has re-enumerated (~2 s)
sudo systemctl start fprintd
```

`reset` ignores Ctrl-C, SIGTERM, SIGHUP and Ctrl-\ (SIGQUIT) outright: it
bounds itself (at most ~50 s, typically ~2 s: a 10 s device search, one 2 s
control transfer, then a short wait for the re-enumerated device to answer)
and there is nothing an interrupt could usefully do — only cut the
reset short. `capture.py`
runs the same `reset` as its cleanup child; the 300 s "SIGTERM nudge" it
sends to a child that has not returned by then is therefore a no-op on the
child, and the parent simply keeps waiting for it to finish on its own, which
is intended: it is never killed.

- `no permission to open the sensor` — see the udev rule above, or `sudo`.
- `the interface is busy` / `sensor never became ready` — another process
  holds the sensor: is `fprintd` really stopped (`systemctl status fprintd`)?
  Is another fingerprint tool running? The message lists the causes; the
  usual one is fprintd. (`probe` and `reset` hit this case too — fprintd
  holding the device shows up as *busy*, not as *no answer*.)
- `probe` says **TLS** — the sensor is in TLS session mode: an older driver
  build left it there, or you booted Windows in between. Run `reset` (with
  fprintd stopped), then `probe` again; if it stays TLS, install the current
  driver (`install.sh`) and reboot.
- `probe` says **no answer** — the sensor is unresponsive (the claim
  succeeded, so nobody else holds it, but it does not reply): run `reset`
  and `probe` again; if it stays silent, a full power cycle (shutdown, not
  reboot) recovers it.
- `could not mask/stop fprintd` — `sudo` failed or timed out; nothing was
  touched, run it again (with `sudo python3 ...` there is nothing to time out).
- `unmask failed` / fprintd not started at the end of an unprivileged run —
  the sudo timestamp expired anyway: `sudo systemctl unmask --runtime fprintd
  && sudo systemctl start fprintd`.
- `could not collect 8 clean no-finger frames` — something is on the sensor,
  or it is dirty; wipe it and rerun.
- `no frame during exposure calibration` — the sensor stopped answering right
  after the bring-up; `reset` and rerun, reboot-fresh if it repeats.
- `nothing detected` repeatedly — press firmer and hold; the finger-on gate is
  raw variance ≥ 250, a hovering or very light touch stays below it.
- Root-owned files in the dataset after a hard abort under `sudo`:
  `sudo chown -R $USER: <your --out directory>`.
- fprintd still masked after a hard abort: `sudo systemctl unmask --runtime
  fprintd` (or reboot; a runtime mask does not survive one).
- A fold failure in `evaluate.py` (`enrolment failed`) means every enrolment
  frame of that fold was rejected by the engine (too little coherent ridge
  area); it is reported in `results.json` under `failures`.
