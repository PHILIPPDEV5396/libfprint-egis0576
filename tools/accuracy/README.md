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
  Makefile       -> score-vendor, score-cleanroom, score-gabor (C compiler + libm, nothing else)
  evaluate.py    two-fold FAR/FRR for all three flavours -> table + results.json
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

produces `tools/accuracy/score-vendor`, `tools/accuracy/score-cleanroom` and
`tools/accuracy/score-gabor`, compiled straight from `driver/egis0576/`. The
vendor sources need the relaxed flag set the libfprint build also uses for
them (`-w -fno-stack-protector -fpermissive -fwrapv ...`); the other two are
plain C99. `make -C tools/accuracy clean` removes everything again.

**The Gabor flavour** (`score-gabor`) is the clean-room adapter on a different
front-end: `driver/egis0576/gabor/egis_match_gabor.c` replaces the isotropic
high-pass with an orientation-selective Gabor enhancement, a per-pixel mask
and a rotation search, and it ships with its own operating point
(`gabor/egis_cr_tuning_gabor.h`, accept at NCC 0.78 -> score 5000). On the
reference dataset it is the first configuration whose genuine and impostor
populations do not overlap (lowest genuine press 0.81, highest impostor 0.69,
0 % / 0 % at the shipped threshold, and 0 % / 0 % with the threshold chosen on
one fold and applied to the other). That is one person, one unit, one session,
and every parameter was chosen on that dataset -- which is exactly why a run
on another unit is worth more than anything measured here. Its header
comment carries the full measurement and what it does not show.

**The clean-room matcher's search width.** Its translation search is ±6 px as
shipped, and on the reference dataset that costs most of its accuracy: widening
it to 19 takes the false-reject rate from 35 % to 3.3 % at the same threshold,
with no false accepts, and nothing else changes
([`docs/matcher-comparison.md`](../../docs/matcher-comparison.md)). Whether that
holds on other units is exactly what is unknown, so the kit can build it either
way:

```
make -C tools/accuracy clean && make -C tools/accuracy EM_SRCH=19
```

Thaddeus Stepanovich's file is never edited — his constants are `#ifndef`-guarded
upstream since his 97dbf8a, so the switch is one define, and built without it
the scorer reproduces his shipped numbers exactly. **If you are
reporting results, please run it both ways** and post both `results.json` files;
the pair is what settles whether the wider search helps everyone or only one
laptop. The kit can be run from any
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
and `--flavour vendor` / `--flavour cleanroom` / `--flavour gabor` to run one
matcher only (repeatable).

A dataset from an interrupted capture works: fingers with fewer than two
presses (or with a frame missing) are skipped with a warning and listed under
`dataset.skipped` in `results.json`; `--fingers a,b,c` evaluates a subset. With
a single evaluable finger there are no impostor trials, so FAR and EER are
reported as `null`.

For each flavour it prints the table `docs/matcher-comparison.md` uses — this
is the *format*, filled in with the reference unit's vendor numbers, and is not
what your run should be expected to produce:

```
| | genuine (n = 60) | impostor (n = 480) |
|---|---:|---:|
| score min / median / max | 5609 / 8572 / 12436 | 0 / 0 / 0 |
| at threshold 5000 | FRR 0.0 % | FAR 0.0 % |
```

**There is no expected result.** The three runs published so far
(`docs/matcher-comparison.md`) gave vendor FRRs of 0 %, 3.33 % and 61.7 % and
clean-room FRRs of 35 %, 73.3 % and 93.3 % on the same protocol. A run that
looks nothing like the table above is still a valid run and is exactly the kind
worth posting.

Four fields worth reading when a run looks bad: `frames_no_features` and
`genuine_presses_no_features` say how often the engine could not extract
features from a frame at all, as opposed to extracting them and finding no
match. A genuine press that scored 0 while not being counted in
`genuine_presses_no_features` was compared against the template and did not
match; a press counted there had nothing extractable in any of its frames. The
published runs predate these fields.

One field to ignore for the vendor flavour: `eer`. Every vendor impostor
comparison in all three runs scored exactly 0, so FAR is 0 at every threshold
≥ 1, there is no FAR/FRR crossing, and the search in `evaluate.py` just returns
FRR/2 at the lowest non-zero genuine score. Report the vendor result as the FRR
at threshold 5000 plus the number of genuine presses that scored exactly 0. The
clean-room `eer` is a real crossing.

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
- **Runs disagree a lot, and nobody knows why yet.** Across the three published
  runs the vendor FRR ranges from 0 % to 61.7 %. Because each run is a different
  person on a different unit, person and unit cannot be told apart, and the two
  per-unit values this file records (the exposure calibration and the enrolment
  codes) both order those runs the wrong way. Treat your own number as one
  point, not as a verdict on your hardware.
- The impostor set is the **same person's other fingers, adjacent fingers
  included** — deliberately the hard case, and a stricter test than random
  strangers. A FAR of 0 % on 480 such trials says a lot; a FAR above 0 %
  on this set is a real finding — and it has now happened once, for the
  clean-room flavour (5 of 480 on irvingpop's unit, `docs/matcher-comparison.md`).
  Note also that those 480 comparisons come from 60 distinct presses, each
  scored against the 8 templates not built from its own finger, so they are not
  480 independent trials.
- **A vendor score of 0 is ambiguous**: it means "no match" *or* "the extractor
  produced nothing usable", and the summary cannot tell them apart. When genuine
  presses score 0 (it happened on two of the three published runs), a 0 on the
  impostor side says correspondingly less.
- Scores are not comparable between flavours except through the operating
  point: the vendor matcher's score is its own minutiae score; the clean-room
  score is an NCC scaled so that its published operating point 0.53 lands on
  the driver's threshold 5000 (divide by 9434 to get the NCC back); the Gabor
  flavour scales its own operating point 0.78 to 5000 (divide by 6410).
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
- Reading the per-fold `codes` in `results.json`: with the **vendor** matcher
  `1` = need more frames, `2` = enrolment complete, and `4` or `-8` mean the
  frame registered against the template but added nothing new (they differ only
  in the stage counter) — `-8` is therefore a *success*, not a rejection. The
  only vendor rejection code is `-1`, an image with fewer than 11 minutiae.
  With the **clean-room** matcher, `-2` means the frame's coverage was below its
  enrolment gate.

## `kit_version 3` (2026-09-23): the driver's rule, on both populations

Until now the kit reported FRR under the driver's accept rule and FAR under
its own, looser one — the press maximum. That is not a detail: it compares a
strict rule against a lenient one, and **no false-accept figure this project
ever published was the driver's**. It is fixed, and a re-run of any existing
dataset (no new captures) now prints both:

| | what it means |
|---|---|
| `frr` / `far` | any frame of the press cleared the threshold |
| `frr_confirmed` / `far_confirmed` | the driver: two consecutive finger-on frames over the threshold, the second corroborated on the un-flat-fielded bytes, and only frames with raw variance ≥ 250 counted at all |

Quote the pair, never one of each. `score.c` prints the corroboration verdict
as a third column for this; an older scorer simply omits it and every frame
counts as corroborated, which is what the kit assumed before.

## `pairdiag`: why a unit separates, or does not

`evaluate.py` says *whether* the populations separate. When they do not, the
next question is what the alignment search had to work with, and press-level
scores cannot answer it. `pairdiag` does, from the same dataset:

```bash
make pairdiag
python3 evaluate.py <dataset> --pair-list      # writes <dataset>/pairdiag.lst
./pairdiag <dataset>/baseline.npy <dataset>/pairdiag.lst > pairdiag.json
```

It compares the first frame of every press with the first frame of every
other press, through the Gabor matcher's own code, and prints one JSON block
of counts, rates and per-pair scalars with finger labels; no image data,
nothing a frame can be rebuilt from — safe to paste. On the kit's 60 presses
a run takes about 1.5 minutes on the maintainer's laptop (about 140 s when it runs on
battery), most of it in `research_floor`. One line on stderr says what it is
doing; another appears if list lines had to be skipped (an unreadable frame,
a bad label, a malformed line), which `presses_skipped` counts.

The first blocks: frame coverage against the probe and enrolment gates, and
for genuine and impostor pairs alike the correlation and the masked overlap
**at the winning alignment** (each unordered pair once, the earlier press as
the template). `pairs_unscorable` counts the pairs whose default search found
no alignment with ≥ 800 px of overlap (154 of 1770 on the reference unit's
2026-09-13 session, 6 on its 2026-09-18); they are left out of every block of
pairs but `zero_shift` and `research_floor`. Read the first blocks like this:
coverage well under the 0.70–0.75 of a well-placed frame says the presses
carry little ridge area (placement, force, a small contact patch). An impostor
median that sits close to the genuine median says the matcher is too
permissive on that skin, which is the project's problem and not the
reporter's. The reference unit's 2026-09-13 session reads coverage median
0.713, genuine NCC median 0.903, impostor median 0.270, impostor max 0.666;
its 2026-09-18 session reads 0.725 / 0.631 / 0.260 / 0.873, which is the
failing case.

`by_overlap_floor` is a **filter**. It drops the pairs whose winning alignment
lies below each floor and reports what is left. It cannot move a winner to
another alignment, so its maxima are a lower bound on what the exhaustive
re-search (`research_floor`, below) finds, and in practice on what a matcher
rebuilt with that floor scores (on both reference sessions the filter equals
that rebuild at 1200 and 1600 px), not that matcher's result.

### What a match is made of (2026-09-26)

The Gabor NCC measures ridge flow and ridge period, not identity. Two
different fingers that put near-parallel ridges of one angle and period on the
70×57 px window score like one finger. The root-cause investigation of
2026-09-26 found this on the reference unit itself: on its 2026-09-18 session
the driver's rule accepts 18 of 480 impostor presses, all of them right thumb
against right index, where the vendor matcher accepts none. The blocks after
`impostor` help tell whether *your* unit's failures are that mechanism, a
capture without fine detail, or a pattern fixed to the sensor.

Unless the table says otherwise, these blocks use **ordered** pairs: every
press once as the template and once as the probe. The matcher resamples only
the probe, so the two directions differ. "Accepted" means `em_match` ≥ 0.78,
the driver's own comparison with the period check applied.

| field | what it is |
|---|---|
| `identity` | `resid` is the fine structure the Gabor filter discards: the local-contrast-normalised frame minus its least-squares multiple of the Gabor output (pores, ridge-width modulation, ridge endings), correlated at the alignment the search picked. The same skin usually reproduces it; a flow coincidence has nothing to reproduce. `genuine_accepted` gives its spread over the accepted genuine pairs and the fraction reaching 0.10 and 0.20. `impostor_ge_070` covers the impostor pairs at NCC ≥ 0.70, `impostor_accepted` the ones `em_match` accepts. `auc_resid_ge_070` and `auc_ncc_ge_070` say how well the residual and the NCC itself separate the `genuine_ge_070.n` genuine pairs from the `impostor_ge_070.n` impostor pairs at NCC ≥ 0.70 (0.5 = not at all) |
| `zero_shift` | the Gabor NCC of different fingers with no shift and no rotation: mean, standard error and median over the unordered impostor pairs (at zero shift both directions give the same number) whose frames overlap by ≥ 800 px in place (a fixed floor, whatever `GABOR_EXTRA` sets, so the field stays comparable across units) |
| `flow` | `coherence_*` is the per-frame coherence of the structure tensor summed over the mask of the Gabor image (1 = every ridge in the frame parallel), over the `frames` that pass the probe gate. `impostor_orient_rms_lt4_frac` / `_lt6_frac` is the fraction of the `impostor_pairs` whose ridge-angle fields differ by less than 4° / 6° RMS at the winning alignment. `impostor_ge_070_orient_rms_median` is that RMS over impostor pairs at NCC ≥ 0.70, and `genuine_accepted_orient_rms_median` is the unit's own yardstick for it |
| `by_finger_pair` | per pair of fingers, over its impostor comparisons in both directions: `n`, NCC median and maximum, how many reach 0.70 (`n_ge_070`) and how many `em_match` accepts (`n_accepted`) |
| `impostor_accepted_list` | every impostor pair `em_match` accepts, highest NCC first, at most 50 (`impostor_accepted_list_omitted` counts the rest). Each entry gives template finger `a`, probe finger `b`, the NCC, the alignment (overlap in px, shift, rotation in °), the orientation RMS, `resid`, and each frame's coherence |
| `research_floor` | the alignment search **re-run exhaustively** (every rotation, every shift, every pixel) over alignments with ≥ 1200, 1400, 1600 or 2000 px of overlap. It re-searches every i < j pair, the ones `by_overlap_floor` covers plus the `pairs_unscorable` ones (pairs the default search found no alignment ≥ 800 px for). It reports the best impostor NCC, how many impostor pairs reach 0.78 in plain NCC (before the period check, so also an upper bound on `em_match`'s accepts), and the genuine median, where a genuine pair with no alignment above the floor counts as −1, as the rebuilt matcher would return it. `genuine_pairs` / `impostor_pairs` count the pairs that do have one |

`research_floor` is exhaustive, so it is an **upper bound** on what a rebuild
with `-DEG_MIN_OVERLAP` finds: that rebuild's coarse-to-fine pass visits only
part of the space. On the reference unit's 2026-09-18 session at 1600 px, it
finds an impostor pair at 0.844 where the rebuild stops at 0.829. At 2000 px
the filter, a rebuild and the re-search all differ: 0.411, 0.500 and 0.514 on
2026-09-13, 0.715, 0.775 and 0.794 on 2026-09-18, where only the re-search
puts an impostor pair above 0.78. At the default floor the two searches agree
on every maximum on both sessions. On individual pairs they do not: the
exhaustive search is higher on 62 % and 57 % of the impostor pairs (1312 and
1436; median +0.015 and +0.007).

The reference unit, both sessions (same person, fingers and baseline
procedure, five days apart; 60 presses each, `--pair-list` as written):

| | 2026-09-13 | 2026-09-18 |
|---|---:|---:|
| `genuine_accepted` n, `resid_median`, `frac_resid_ge_010`, `frac_resid_ge_020` | 399, 0.605, 0.895, 0.862 | 239, 0.559, 0.812, 0.749 |
| `impostor_ge_070` n, `resid_max` | 0, – | 82, 0.204 |
| `impostor_accepted` n, `n_resid_ge_020`, `resid_max` | 0 of 2619, 0, – | 18 of 2872, 0, 0.089 |
| `auc_resid_ge_070`, `auc_ncc_ge_070` | –, – | 0.831, 0.889 |
| `zero_shift` mean (SE), n | +0.0015 (0.0020), 1297 | −0.0011 (0.0021), 1403 |
| `coherence_median`, `impostor_orient_rms_lt6_frac` | 0.682, 0.005 | 0.772, 0.034 |
| `genuine_accepted_orient_rms_median`, `impostor_ge_070_orient_rms_median` | 2.3°, – | 2.2°, 5.2° |
| `research_floor` `impostor_max` at 1200, 1400, 1600, 2000 px | 0.666, 0.666, 0.577, 0.514 | 0.865, 0.865, 0.844, 0.794 |
| … and `by_overlap_floor` (the filter) at the same floors | 0.666, 0.666, 0.577, 0.411 | 0.865, 0.865, 0.829, 0.715 |
| `research_floor` `impostor_n_ge_078` at the same floors | 0, 0, 0, 0 | 15, 13, 8, 1 |
| `research_floor` `genuine_pairs` at the same floors | 298, 289, 287, 268 | 317, 301, 289, 276 |
| `research_floor` `genuine_median` at the same floors | 0.8835, 0.8690, 0.8446, 0.4161 | 0.5974, 0.5699, 0.5232, 0.3095 |

All 18 impostor pairs `identity.impostor_accepted` counts on 2026-09-18
(first frames, ordered, of 2872 scored impostor pairs, the sum of
`by_finger_pair`'s `n`) are right thumb against right index
(`by_finger_pair`). That 18 is a pair count; that it equals the 18 of 480
presses the driver's rule accepts is a coincidence.

**How to read your run.** Each of the four outcomes below points at a
different cause:

- **(a) The same mechanism as the reference unit's 2026-09-18.** Your accepted
  impostors have `resid` near 0 (`impostor_accepted.n_resid_ge_020` = 0) while
  `genuine_accepted.resid_median` stays far above it. On the owner's frames no
  accepted impostor reached 0.20 in any of four session combinations,
  cross-session included (54 pairs, maximum 0.169), while 78.8 % of 1050
  accepted genuine pairs did. Then your failures are flow coincidences, and
  fine structure that separates them is in most of your frames. A matcher
  that demands it at the winning alignment is a candidate on your unit too,
  not a finished one: on the reference unit's 2026-09-18 session the crude
  residual check (accept only at a residual of at least 0.10) reached 0 of 480
  false accepts at 18 of 60 false rejects, against 11 of 480 and 12 of 60
  without it (one first frame per press on the kit's folds), and the colliding
  right thumb reproduces its own residual poorly (median 0.03 over 37 accepted
  genuine pairs, against 0.21–0.66 for the other fingers). `by_finger_pair` and
  `impostor_accepted_list` show which fingers collide; a low `orient_rms_deg`
  on frames of high coherence is the signature.
- **(b) Frames without fine structure.** Your genuine residual is about as low
  as your impostor residual, both near 0: `genuine_accepted.resid_median` and
  `frac_resid_ge_010` far below the reference unit's 0.56–0.60 and 0.81–0.90,
  and `auc_resid_ge_070` near 0.5. Then this residual finds no detail beyond
  ridge flow in your captures, at the alignments the Gabor search picks. That
  points at the capture rather than the matcher, but does not prove it: fine
  structure decorrelates under small misregistration and skin distortion, and
  on the reference unit's 2026-09-18 session the right thumb's own accepted
  genuine pairs have a residual median of 0.03 (37 pairs, 35 % reach 0.10)
  while the vendor matcher matches that thumb. Run the vendor flavour on the
  same dataset before concluding the capture is the limit.
- **(c) A fixed pattern.** `zero_shift.impostor_ncc_mean` lies clearly above 0,
  several standard errors, where the reference unit reads 0.00 ± 0.002. Then
  something fixed to your sensor survives the flat-field and makes different
  fingers correlate in place. That is a baseline problem of that unit.
- **(d) Small overlap, or not.** If `impostor.ncc_max` reaches 0.78, look at
  `research_floor` `impostor_max` at 1200 px. Below 0.78, your impostor tail
  lives in small overlaps, and a higher floor removes it, at the genuine cost
  `research_floor`'s `genuine_pairs` and `genuine_median` show. At 0.78 or
  above, impostors still reach the accept level (plain NCC, before the period
  check) over a large overlap. Then the floor is not the cause, and (a)–(c)
  say which is; on the reference unit's 2026-09-18 it was (a) (0.865). The
  filter value in `by_overlap_floor` can only be lower.

These readings rest on one person per unit, so on any one unit person and
sensor cannot be told apart. The residual check behind (a) was measured on
the owner's frames only: in-sample, first frames, one person. It is a
diagnostic prototype, not a matcher.

To measure a proposed constant on your own captures without editing anything:

```bash
make clean && make GABOR_EXTRA=-DEG_MIN_OVERLAP=1400 && python3 evaluate.py <dataset> --flavour gabor
```
