# Matcher comparison and the measured FAR/FRR — three units

Two matchers can be built into this driver (`-Degis0576_matcher=vendor|cleanroom`,
see the README). This document compares them **on identical captures**, and
collects every accuracy measurement the project has.

There are three of them, all with the same kit (`tools/accuracy/`) and the same
protocol, each one person on one unit in one session:

- the **reference unit** (Lenovo Yoga 7 14ARB7, Fedora 44), the maintainer's own,
  measured 2026-09-13 — the run this document was originally written around;
- **sam-dant**'s run on a Lenovo IdeaPad Flex 5 14ITL05 / Zorin OS 18.1, and
- **irvingpop**'s run on a Yoga 6 13ALC6 / Ubuntu 26.04,

both posted in
[#5](https://github.com/PHILIPPDEV5396/libfprint-egis0576/issues/5) on
2026-09-13/14 and quoted here from their `results.json`.

**The three runs do not agree, and the spread is the headline.** The vendor
matcher rejected 0, 2 and 37 of 60 genuine presses at the shipped threshold. No
average of those three numbers is meaningful, and none of them is this driver's
false-reject rate; they are three measurements of three people on three units.
They are all reproduced below.

> **Read the Gabor sections below with the last one in mind (2026-09-26).**
> The clean 0 % / 0 % the Gabor front-end shows on the reference unit is a
> property of **one session** (2026-09-13), the one every Gabor constant was
> tuned on — not of the unit, and not of the matcher. The same unit, person and
> fingers five days later give 13 / 60 false rejects and 18 / 480 false accepts
> under the driver's own rule, where the vendor matcher on the same frames
> rejects 2 and accepts none. The front-end measures ridge flow and period, not
> identity. Statements in this file that said otherwise are left where they
> were and marked; the measurement and its consequences are in
> [the last section](#2026-09-26-what-prevents-a-universal-00).

## Summary

| | reference unit | sam-dant | irvingpop |
|---|---|---|---|
| Laptop | Lenovo Yoga 7 14ARB7 | Lenovo IdeaPad Flex 5 14ITL05 | Yoga 6 13ALC6 |
| Distro | Fedora 44 | Zorin OS 18.1 | Ubuntu 26.04 |
| Frames / presses | 714 / 60 | 720 / 60 | 720 / 60 |
| **Vendor** FRR at 5000 | **0 %** (0 / 60) | **3.33 %** (2 / 60) | **61.7 %** (37 / 60) |
| genuine min / median / max | 5609 / 8572 / 12436 | 0 / 11326 / 13967 | 0 / 0 / 13314 |
| genuine presses scoring exactly 0 | 0 | 1 | 33 |
| **Vendor** FAR at 5000 | 0 % (0 / 480) | 0 % (0 / 480) | 0 % (0 / 480) |
| impostor min / median / max | 0 / 0 / 0 | 0 / 0 / 0 | 0 / 0 / 0 |
| **Clean-room** FRR at 5000 | **35.0 %** | **73.3 %** (44 / 60) | **93.3 %** (56 / 60) |
| genuine min / median / max | 1220 / 6961 / 9135 | 965 / 2425.5 / 8994 | 1248 / 2865 / 7532 |
| **Clean-room** FAR at 5000 | 0 % (0 / 480) | 0 % (0 / 480) | **1.04 %** (5 / 480) |
| impostor min / median / max | 710 / 1638 / 4470 | 698 / 1697.5 / 4232 | 770 / 2517 / 5481 |
| Clean-room EER | 15 % (score 2268) | 35 % (score 1945) | 45 % (score 2651) |
| Exposure reg `0x0f`, baked → calibrated | 0x20 → 0x20 | 32 → 32 | 32 → **31** |
| No-finger frame mean (target 88) | 93.7 at 0x20 ¹ | 97.54 | 87.36 |
| Calibration trace `[reg, mean]` | not recorded | 31→75, 47→255, 39→249, 35→164, 33→120, 32→98 | 31→86, 47→255, 39→254, 35→178, 33→133, 32→110 |
| Vendor enrolment, frames per fold | 2–6 | 4–6 | 6 in all 10 folds |
| Vendor enrolment, `-8` (frame registered but added nothing new) | 19, across 8 of 10 folds | 10, across 7 of 10 folds | **none** |
| Vendor enrolment, `-1` (frame rejected, too few minutiae) | none | none | none |

¹ The reference unit's 93.7 is from the register sweep in
[`sensor-tuning.md` §3](sensor-tuning.md), not from the kit's own no-finger
baseline field, so it is not strictly the same measurement as the other two
columns. Its calibration was a no-op (`0x20` in, `0x20` out).

Read those rows against the FRR row before drawing conclusions from any of them.

*Exposure does not order the runs.* The unit whose calibration landed closest to
the target mean (87.36 against a target of 88) is the one with 61.7 % FRR; the
unit furthest from it (97.54) is at 3.33 %.

*Template size does not order them either, and rules itself out as the cause.*
The reference run has the **thinnest** templates of the three — two to six
frames per fold, one fold with only two — and rejected nothing. The failing run
has the fullest, six frames in every fold.

*One measured quantity does order all three: how often the engine reported a
newly added enrolment frame as already covered.* That is the `-8` code, and it
is a success, not a rejection — the frame registered against the template and
added no new area ([`egis_funcs.c:9661`](../driver/egis0576/egis_funcs.c)). The
reference run produced 19 of them, sam-dant's 10, irvingpop's none at all, which
is the same order as the FRR row. No enrolment frame was rejected outright
(`-1`, fewer than 11 minutiae) on any of the three, so this is not a minutiae
count failing. What it *means* is not settled by these numbers: with one person
per unit, unit, person, skin, press technique and session all change together,
and "frames of the same finger did not register against each other" fits how
someone places a finger as readily as anything about the hardware. It is simply
the one thing measured on all three runs that tracks the failure. See "What this
does and does not show".

## Dataset (all three runs)

One person, one session, one flat-field baseline (mean of 8 no-finger frames,
as the driver builds it at open). Five fingers — right thumb, right index, left
index, right middle, left middle — 12 presses each. Every frame captured while
the finger was down was kept, exactly as the driver's verify loop sees them
(finger-on at raw variance ≥ 250, finger-off below 215): **60 presses**, 714
frames on the reference unit and 720 on each of the other two (12 frames for
every press, i.e. the kit's per-press cap was reached each time). Frames were
stored raw; flat-fielding and preprocessing were applied offline, identically
for both matchers, by a scorer linked against each flavour's engine behind the
same `egis_engine.h` contract.

The impostor set is deliberately the hard one: the same person's *other*
fingers, including adjacent same-hand fingers — the case that sank every earlier
clean-room attempt (see `PROVENANCE.md`). There are no cross-person impostor
trials anywhere in this data.

## Protocol (all three runs)

Two-fold per finger: enrol on presses 0–5, test on 6–11, then the reverse.
Enrolment used the first finger-on frame of each press, as the driver does. A
probe press counts as **accepted** if any of its frames scores at or above the
shipped threshold (5000) — the driver's policy. Genuine trials: the same
finger's held-out presses (60 total). Impostor trials: every press of every
other finger against each template (480 total).

Those 480 are not 480 independent presses. Each of the 60 presses is scored
against the 8 templates that were *not* built from its own finger (4 other
fingers × 2 folds) — `evaluate.py:188-198` — so the impostor set is 60 distinct
presses, eight-fold reused, from one person.

Note also that the kit enrols 6 presses per fold while the shipped driver enrols
12 stages (`EGIS0576_ENROLL_STAGES`, `driver/egis0576.c:36`) and repeats any
press the engine rejects. The kit's templates are therefore built from less of
the finger than a real enrolment, and the kit drives USB itself rather than the
installed driver. None of the FRR figures here is the shipped driver's FRR.

## How to read a vendor score, and why the vendor "EER" is not one

The vendor matcher returns an integer, and **0 means either "no match" or "the
extractor produced nothing usable"** — the score alone cannot tell the two
apart. That matters twice:

- Every impostor comparison on every unit scored exactly 0 (1440 comparisons in
  total, `impostor_min`/`median`/`max` all 0 in all three runs). So the vendor
  FAR of 0/480 per unit is the statement that *no impostor comparison produced a
  score at all* — not that impostors were separated by a measured margin.
- Most genuine failures are also exactly 0 (irvingpop 33 of his 37 sub-threshold
  presses; sam-dant 1 of 2). Those presses are indistinguishable from the
  impostor population at any threshold.

Consequently the `eer` field the kit writes for the vendor flavour is
**degenerate and must not be quoted as an operating point**: with every impostor
at 0, FAR is 1.0 at threshold 0 and 0.0 at every threshold above it, so the
FAR/FRR search at `tools/accuracy/evaluate.py:205-211` has no crossing to find.
It lands on the lowest non-zero genuine score and reports FRR/2 there —
irvingpop `eer` 0.275 at `eer_threshold` 533 = 0.55/2; sam-dant 0.0083 at 4874 =
0.0167/2. Quote the FRR at the shipped threshold and the count of zero-scoring
genuine presses instead. The clean-room EERs (15 % / 35 % / 45 %) are real
crossings, because that matcher's impostor scores are not degenerate.

## Vendor matcher (default build), per unit

### Reference unit

| | genuine (n = 60) | impostor (n = 480) |
|---|---:|---:|
| score min / median / max | 5609 / 8572 / 12436 | **0 / 0 / 0** |
| at threshold 5000 | **FRR 0.0 %** | **FAR 0.0 %** |

Every impostor press — 480 of them, adjacent fingers included — scored
exactly 0. The lowest genuine press scored 5609, so on *this* run every
threshold from 1 to 5609 is error-free and the shipped 5000 sits near the top of
that window.

### sam-dant

| | genuine (n = 60) | impostor (n = 480) |
|---|---:|---:|
| score min / median / max | 0 / 11326 / 13967 | **0 / 0 / 0** |
| at threshold 5000 | **FRR 3.33 %** (2 / 60) | **FAR 0.0 %** |

Two presses missed: one scored 4874 (just under the line) and one scored 0. The
other 58 are at 7188 or above. His was the weaker of the two enrolments that
were recorded — ten frames came back `-8`, meaning they registered against the
template but added nothing new, across 7 of the 10 folds, leaving templates of
4 to 6 frames — and it is the better of those two runs.

### irvingpop

| | genuine (n = 60) | impostor (n = 480) |
|---|---:|---:|
| score min / median / max | 0 / 0 / 13314 | **0 / 0 / 0** |
| at threshold 5000 | **FRR 61.7 %** (37 / 60) | **FAR 0.0 %** |

37 of 60 genuine presses are below the threshold, 33 of them exactly 0; the
other four are 533, 634, 3575 and 4934. The 23 presses that do pass are not
weak — they run from 8530 to 13314, in the same range as the other two units.
The failures are spread over all five fingers (sub-threshold presses per finger,
12 each: R-thumb 9, R-index 8, L-index 8, R-middle 4, L-middle 8) and over both
folds. His vendor enrolment carries no rejection code (`-1`) anywhere, so every
enrolment frame yielded enough minutiae to be accepted — but it also carries no
`-8` anywhere, meaning the engine never once found a newly added frame already
covered by the template, where the other two runs did so 10 and 19 times. What
that implies about registration on this unit is an inference these scores cannot
settle. (His clean-room enrolment did reject two frames on coverage, code `-2`;
the two flavours run different enrolment logic.)

Because 0 covers both "no match" and "nothing extracted", and because the frames
do not exist any more (they never leave the tester's machine, by design), this
data cannot say *why* those presses scored 0. Threshold choice is not the answer:
at threshold 1 his FRR is still 55 % (the 33 zeros). The obvious candidates —
per-press contact quality, the kit's 6-press templates, his unit's operating
point — are not separable here, and the one unit-level number available
(no-finger mean 87.36 against a target of 88) is the *closest to target* of the
three.

## Clean-room matcher (Thaddeus Stepanovich's, `-Degis0576_matcher=cleanroom`)

Scores are his masked NCC scaled so that his published operating point, 0.53,
lands on the driver's threshold 5000 (score = NCC × 5000 / 0.53).

### Reference unit

| | genuine (n = 60) | impostor (n = 480) |
|---|---:|---:|
| score min / median / max | 1220 / 6961 / 9135 | 710 / 1638 / 4470 |
| in NCC terms | 0.13 / 0.74 / 0.97 | 0.08 / 0.17 / 0.47 |
| at threshold 5000 (NCC 0.53) | **FRR 35.0 %** | FAR 0.0 % |

The two populations **overlap**: the weakest genuine press (NCC 0.13) scores far
below the strongest impostor (0.47), so no threshold is error-free. The
equal-error point is 15 % at a score of 2268 (NCC 0.24). At the shipped 5000,
one genuine press in three is rejected.

**This is not an enrolment artefact.** The adapter gates enrolment frames on
coherent-ridge coverage (≥ 0.55) and rejects near-duplicates, so a template
built from first-contact frames could have been thin. It was not: templates
held 4–6 frames, with three coverage rejections across the ten folds. Enrolling
the highest-contrast frame of each press instead of the first changes nothing
material (FRR 33.3 %, genuine 1220 / 7131 / 9144, impostor max 4492). The vendor
matcher under the same best-frame enrolment stays at 0 % / 0 % (genuine
5566 / 8768 / 13101, every impostor 0) — while rejecting far more of those
frames during enrolment (down to 2 per template in some folds): it prefers
first-contact frames and separates regardless.

Where the tail comes from is visible in the raw data: the same finger's
presses correlate raw-against-raw at only 0.25–0.81 across placements (±6 px
translation search). His enhancement and coherence mask lift the median to
0.74, but presses placed far off or rotated stay in the tail, and a ±6 px
search cannot bring them back. His own figure — 0 % FAR, ~10 % FRR at 0.53 on
10 genuine and 16 impostor presses — is what sampling this same distribution at
n = 10 is expected to give; the 60-press run just resolves the tail.

### sam-dant

| | genuine (n = 60) | impostor (n = 480) |
|---|---:|---:|
| score min / median / max | 965 / 2425.5 / 8994 | 698 / 1697.5 / 4232 |
| at threshold 5000 (NCC 0.53) | **FRR 73.3 %** (44 / 60) | FAR 0.0 % |

EER 35 % at score 1945. The genuine distribution sits far lower than on the
reference unit (median 2425.5 against 6961) while the impostor distribution is
comparable, so the separation is much weaker: only 16 of his 60 genuine presses
reach 5000.

### irvingpop

| | genuine (n = 60) | impostor (n = 480) |
|---|---:|---:|
| score min / median / max | 1248 / 2865 / 7532 | 770 / 2517 / **5481** |
| at threshold 5000 (NCC 0.53) | **FRR 93.3 %** (56 / 60) | **FAR 1.04 %** (5 / 480) |

This run contains the project's **first measured false accepts**: five of the
480 impostor comparisons reach the threshold, at 5238, 5296, 5312, 5416 and
5481. They are not one confused finger pair, though they do sit just above the
line: all five are between 5238 and 5481, and no impostor comparison in that run
reached 5600. Decoding the index layout of `evaluate.py:188-198` places them on four
different template fingers (R-thumb fold 1, R-index fold 1, R-middle fold 0
twice, L-middle fold 1) and on four distinct physical presses (R-middle press 2
crosses against two different templates, at 5296 and 5416; plus L-middle 10,
L-middle 9 and R-thumb 11).

What they really show is that in this run the two distributions are not
separated at all: genuine median 2865 against impostor median 2517, and 57 of
the 60 genuine presses score *below* the largest impostor (5481). The EER is
45 % — near the 0.5 chance level. Raising the threshold to 5600 would zero the
FAR and reject 57 of 60 genuine presses.

Scope: five events out of 480 comparisons that come from only 60 distinct
presses of one person, on one unit, in one session. That is not a false-accept
rate. sam-dant's clean-room run on the same kit has impostor max 4232 and FAR 0,
so this collapse is specific to this run, not a property of the clean-room
engine as such — but it is the first time the clean-room flavour has admitted an
impostor at the shipped threshold, and `tools/accuracy/README.md` is right that
this counts as a real finding.

**Consequence.** As it stands the clean-room matcher is not a drop-in
replacement: it costs a third of genuine presses on the best of the three runs,
three quarters and nine tenths on the other two, and on one of them its genuine
and impostor scores are essentially indistinguishable. Whether that is fixable —
a wider or rotation-aware search, multi-frame templates scored jointly, a
different enhancement — is now a concrete question with three datasets' worth of
scores and a harness to answer it against.

## Gabor front-end (the shipped matcher since v0.5.0), reference unit only

The third flavour keeps Thaddeus Stepanovich's adapter and masked NCC and
replaces what is fed into it: an orientation-selective Gabor enhancement
instead of the isotropic high-pass, a per-pixel coherence mask instead of the
16×16-block one, and a rotation search (±10° in 2.5° steps, coarse-to-fine)
on top of the ±19 px translation search
(`driver/egis0576/gabor/egis_match_gabor.c`; the file's header carries the
full ablation, cost and caveats). It ships with its own operating point,
NCC 0.78 → score 5000 (score = NCC × 5000 / 0.78, so a perfect 1.0 logs as
6410), because both populations sit higher than with his front-end and his
0.53 would false-accept 14 % of same-person impostors. The threshold sits on
the genuine side of this unit's gap on purpose (mid-gap would be 0.75): a
false reject costs a retry, a false accept costs the login. *(2026-09-26:
"this unit's gap" is this **session's** gap. The same unit's 2026-09-18
session has no gap to sit in — see the last section.)*

Same captures, same protocol, same kit (`score-gabor`), 2026-09-18 (scores
on the 0.78 scale; the captures are the 2026-09-13 session's):

| | genuine (n = 60) | impostor (n = 480) |
|---|---:|---:|
| score min / median / max | 5203 / 6216 / 6350 | 391 / 2662 / 4426 |
| in NCC terms | 0.81 / 0.97 / 0.99 | 0.06 / 0.42 / 0.69 |
| at threshold 5000 (NCC 0.78) | **FRR 0.0 %** | **FAR 0.0 %** |

The populations **do not overlap**: the weakest genuine press (0.81) sits
0.12 above the strongest impostor (0.69), so no EER exists. Templates held
5–6 frames per fold (one first-contact frame in the whole set fell under the
coverage gate). The same comparison in an adapter-free harness, and with the
threshold chosen on one fold and applied to the other (placed mid-gap, as one
would in practice), gives 0 / 30 false rejects and 0 / 240 false accepts both
ways; the two folds put that threshold at 0.751 and 0.752. The stricter
cross-fold rule — threshold *at* the other fold's highest impostor — gives
0 % FRR at 1.5 % FAR, which is the honest statement of how well 480 samples
pin down the impostor tail.

What this run does **not** show, in the same words as for the other flavours:
it is one person, one unit, one session — and this time every parameter
(angle step, erosion, search size) was chosen on this very dataset, so the
in-sample figures above are the best case by construction. Three things were
checked against that: the cross-fold threshold above; that the shipped
coarse-to-fine search is not what produces the gap (a run with every angle
in the coarse pass and finer pixel decimation finds the same extremes to
four decimals on all 540 comparisons); and that the C front-end reproduces
its numpy prototype bit-for-bit. What was *not*
possible here is the only measurement that matters for the two units where
his front-end fell to 73 % and 93 % FRR: theirs. `make -C tools/accuracy`
now builds `score-gabor`, and `evaluate.py` runs it by default.

*(2026-09-26: one measurement that was possible here was not used for this:
the same unit's second session, captured on 2026-09-18, the day of this run,
and kept out of the FAR record (see "Across sessions" below). Run through
this same kit it gives FRR 13 / 60 and FAR 18 / 480 under the driver's rule
— the 0 % / 0 % above does not survive a second session of the unit it was
tuned on. See the last section.)*

**Second unit (2026-09-17, Thaddeus Stepanovich, Yoga 7 16IRL8 / Intel):**
he ran the front-end on his own two capture sets (29 genuine / 88 impostor
decisions, cross-fold with the strictest zero-false-accept threshold, folds
split by capture block; method and tables in his
[`docs/gabor-frontend-second-unit.md`](https://github.com/tsteppy/egistec-eh576-libfprint/blob/main/docs/gabor-frontend-second-unit.md)):

| configuration | held-out FRR | held-out FAR |
|---|---:|---:|
| his front-end as shipped (±6 / 800) | 51.7 % | 4.5 % |
| Gabor, raw frames | 17.2 % | 1.1 % |
| **Gabor + flat-field** | **6.9 %** | **1.1 %** |

So the front-end replicates as a large improvement, and the 0 % / 0 % above
does not: flat-fielded, his weakest genuine press scores 0.600 against a
strongest impostor of 0.815, so his populations still overlap and 6.9 % is a
threshold choice (two of 29 presses). Two further findings of his that
matter here: the per-boot flat-field this driver already applies is what
makes the threshold portable (his raw-frame folds wanted 0.832 / 0.870, his
flat-fielded ones 0.815 / 0.813), and it helps this front-end while hurting
his (51.7 % → 65.5 %). The threshold is still unit-dependent even flat-fielded
— mid-gap 0.75 here, 0.81 there — which is the open problem a third unit has to
inform. *(2026-09-26: it is worse than unit-dependent. "Here" is one session;
the same unit's next session, sam-dant's unit and irvingpop's have no gap at
all, so there is no mid-gap to place.)* One caveat on that comparison: his
numbers come from his capture path
(gain switching between register 0x12 = 0 and 6, one settled frame per press,
a 3-frame reference), not this driver's (calibrated exposure at a fixed gain,
8-frame baseline, every frame scored), so his impostor ceiling is a property
of that pipeline as much as of his unit. He also showed that his own front-end, retuned jointly (±26 px,
1400 px overlap floor), ties the Gabor one on *raw* frames pooled and loses
to it flat-fielded, so the ablation in the file's header, measured against a
fixed 800 px floor, is one dataset's answer to "what each piece contributes".

One more thing measured and rejected on this front-end, because it is the
obvious way to widen the margin: scoring a probe frame by its *second*-best
(or third-best) template frame instead of the best, so an impostor has to
fool two templates. It costs the genuine side far more than the impostor
side — genuine min 0.81 → 0.55 (2nd) → 0.30 (3rd) against impostor max
0.69 → 0.67 → 0.64 — for the same reason it failed on his front-end: a
genuine press overlaps one enrolled placement well, not two.

**Hardening (2026-09-18 → 21), from an adversarial review of the vendor-free
stack** — what it found, what closed it, and what it cost, all measured on the
reference dataset:

| finding | closed by | genuine cost |
|---|---|---:|
| a re-served stale frame of an earlier press unlocks at the first frame over the threshold | two-frame confirmation in the driver (next frame must also clear the threshold and differ byte-wise) | 0 / 60, +1 frame latency |
| a per-boot flat-field baseline captured with the enrolled finger resting lightly paints its ridges, inverted, into every later frame; a featureless smudge then scores 0.92–0.94 | every accept is corroborated on the un-flat-fielded frame (`egis_verify_raw_ok`, NCC ≥ 0.5; genuine raw probes score ≥ 0.83) | 0 / 60 |
| a smooth, ridge-free gradient gets full mask coverage and is enrolled | absolute ridge-evidence gate in the front-end (Gabor response sd ≥ 5 over the mask) | 0 / 60 |
| computer-generated ridge textures (sine 0.87, arc 0.91, loop/delta 0.91, ridge noise 0.88, period-modulated grating 0.91 against real templates) | local-ridge-period consistency at the winning alignment (`egis_match_check.h`) — **partly**, see below | 0 / 60 under both accept rules |

### The period check closes half of what was claimed

An earlier version of this table said "all blind families now ≤ 0.75". That was
true of the parameter grids the review had run and **not** true in general. Two
independent re-measurements (2026-09-22) over widened grids, six blind families
against the 30 real enrolment templates of the reference dataset, best score
reachable **with the check passing** (accept threshold 0.78, worst genuine
press 0.80):

| family | check off | check on | |
|---|---:|---:|---|
| sine (constant period) | 0.865 | 0.700 | closed |
| arc (constant period) | 0.909 | 0.699 | closed |
| chirp (smooth period gradient) | 0.784 | 0.768 | closed |
| filtered ridge noise | 0.875 | **0.795** | **passes** |
| loop / delta | 0.907 | **0.892** | **passes** |
| period-modulated grating | 0.914 | **0.895** | **passes** |

The check tests whether the probe's ridge period *varies* and whether that
variation *reproduces the template's*. It therefore closes families whose
period is constant, and an attacker who modulates the period — two extra sine
terms, no score oracle, no knowledge of the victim's print — walks through it
and false-accepts against three of the five enrolled fingers. Against an
attacker who can read the score and hill-climb, the check is worth
**0.000–0.002 NCC**: it does not raise his cost at all.

So the honest statement is: **this driver has no defence against a fabricated
artefact.** What it has is a filter for the laziest generated textures, the
two-frame confirmation and the raw-frame corroboration — none of which is
presentation-attack detection, which this sensor offers no signal for. That is
true of every matcher without liveness detection, but it should be read here
rather than inferred.

The check's genuine cost, after `EM_FQ_MIN_NBLK` came down from 20 to 12 (see
below), is 0 of 60 presses under the kit's best-frame rule **and** 0 of 60
under the driver's two-consecutive-frame rule, worst press 0.812 → 0.804. At
20 it cost one press of 60 under the driver's rule, and three of the
cross-session presses. `tools/accuracy/evaluate.py` now reports both rules
(`frr` and `frr_confirmed`), because the difference between them is exactly
where that cost was hiding: the kit accepted a press on a single frame over
the threshold, the driver needs two in a row.

A rejection by the check is a **non-match**, not a "nothing to score". The
two are different events and the driver answers them differently: a press it
could not judge at all (too little finger on the sensor) asks the user to
press again, and `fprintd` restarts it without spending one of the attempts,
while a non-match costs an attempt. Until 2026-09-22 the check returned the
"nothing to score" sentinel, so an artefact that tripped it could be
presented again and again for free. It now scores 0. The distinction costs
a genuine user nothing on the reference session (no press loses its accept
to the check at all) and turns one cross-session press from a retry into a
failed attempt. Confirmed on the live driver after the change: `fprintd-verify`
matched at 0.85 / 0.85, confirmed by the second frame, on a template enrolled
before it.

`EM_FQ_MIN_NBLK` is not a texture criterion: `nblk` is arithmetically the
number of period blocks geometrically eligible over the overlap (correlation
0.9996 with the eligibility count), so the clause is an overlap gate, and at
20 it sat at ~1200–1400 px — above the matcher's own `EG_MIN_OVERLAP` of 800
and above the genuine 5th percentile of 12–14. Every value from 8 to 20 gives
**bit-identical** attack numbers in all six families: the clause stops no
synthetic pattern that another clause does not stop. It only ever cost
genuine presses, so it is now 12.

Not closed by anything shipped: the oracle attacker (0.91–0.98). A second
check that corroborates the fine structure the Gabor filter smooths away
(pores, ridge-width modulation) raised that attacker's cost materially in the
review's measurement but costs 5–7 % of genuine presses on the kit protocol,
and is therefore not shipped.

**Across sessions (2026-09-18, second capture on the reference unit five days
after the first):** templates from the first session, probes from the second,
twelve templates per finger as the driver enrols — medians 0.94 / 0.92 / 0.83
on three fingers, **0.55 and 0.15** on the other two, ~~because those landed on
skin the first session's twelve presses never covered (the two sessions' thumb
regions do not overlap at all)~~ *(withdrawn, see below: they came back
rotated)*. The reverse direction, enrolling from the second session whose
presses were deliberately spread out, lifts the cross-session *minimum* on the
three overlapping fingers from 0.52–0.59 to 0.82–0.92. That, and tsteppy's
identical finding on his unit, is why the clean-room adapter now steers
enrolment: from the third stored frame on, a press that lands where a stored
frame already is (NCC ≥ 0.90 within 6 px) is refused with the "adjust your
finger" hint instead of spending a stage, at most twice in a row. On the first
session's twelve first-frames the rule would have refused 3–5 presses per
finger; on the spread-out session 1–3. The second session also measured a
same-session **impostor pair at 0.90** (right thumb vs right index, 12 of 24
pairings ≥ 0.78) that no other pair on either session comes near;
cross-session, that "thumb" recording resembles the first session's *index*
finger (0.72) far more than its thumb (0.15). The recorder reports using the
correct fingers. ~~Until that is understood the second session is used for the
coverage question only, not for FAR.~~

**Withdrawn, 2026-09-26: both readings in the paragraph above were wrong, and
the second one was wrong on the evidence it printed itself.**

- *Much of the "uncovered skin" was rotation.* A structure-tensor orientation
  estimate on each session's own flat-fielded frames puts the thumb of the
  second session at about 50–60° and R-Mittel (right middle finger) at about
  30° from where they were in the first — far outside the ±10° rotation search.
  Widening only the rotation search (a rebuild for the measurement) brings them
  back: at ±90° the thumb goes from 0 of 24 cross-session presses accepted to
  21 of 24 (press maximum; 19 of 24 under the driver's rule), at ±30° R-Mittel
  from 0 of 24 to 21 of 24 under either rule. Placement is part of it — on the
  spread-out second session 10 of the kit's 12 press-maximum genuine rejects
  never reach 0.60 against their fold's templates — but the premise the
  enrolment steering was built on, "skin never covered", was partly rotation,
  and steering does nothing about rotation. (Widening the search is no fix
  either; see the last section.)
- *The 0.90 thumb/index pair was this matcher failing, not a mislabel.* The
  vendor matcher settles the label: on the identical frames it matches the
  second session's R-Daumen only to the first session's R-Daumen (9 of 12
  presses against first-session templates, 12 of 12 the other way) and scores
  all 960 impostor comparisons of the combined two-session set 0; in-session it
  accepts none of 480 impostors, R-Daumen against R-Zeige included, and rejects
  2 of 60 genuine presses. What changed between the sessions is the thumb's
  angle: its dominant ridge orientation moved from about 31° to about 162°,
  1.3° from R-Zeige's (on 2026-09-13 it lay 42–48° from both index fingers).
  Two near-parallel ridge fields at the same angle and period are exactly what
  the Gabor front-end cannot tell apart. Setting the session aside "for the
  coverage question only, not for FAR" therefore removed from the FAR record
  the one measurement in this file that showed the reference unit's 0 / 0 was
  not a property of the unit — and the 0.90 stood two sentences above the
  sentence that set it aside. Under the driver's own rule that session gives
  **18 false accepts of 480**. It is used for FAR from now on.

Cost, for the driver's every-frame scoring loop: `em_frame_compute` 1.6 ms
(his 0.17 ms), `em_match` 4.0 ms against 0.85 ms at ±6 and 6.4 ms at ±19 —
the rotation search costs less than the wider translation search alone,
because its coarse pass is decimated and one NCC pass accumulates raw sums.

## Why not minutiae (the number behind the driver design)

libfprint's own way to support an image sensor is `FpImageDevice`: the driver
hands over images, libfprint's bundled NBIS (`mindtct`) extracts minutiae and
`bozorth3` matches them. That is not an option on this sensor, and the reason
is one measurement, reproducible with the kit's `nbis_count` (libfprint's own
`mindtct` sources plus the driver's documented flat-field step; only counts
leave the machine):

```
make -C tools/accuracy nbis-count LIBFPRINT_SRC=/path/to/libfprint
tools/accuracy/nbis-count <dataset>/baseline.npy <dataset>/*.npy
```

Reference dataset, 714 frames (2026-09-16, libfprint 1.94.100,
`g_lfsparms_V2`, 12.8 px/mm from the measured 6.4 px ridge period — the sensor's physical size is not in any datasheet this project has; the two figures the repository used to carry (3.5 × 2.9 mm, i.e. 508 dpi, and 12.8 px/mm from the measured ridge period, i.e. 5.5 × 4.5 mm) cannot both be right. The measurement is the ridge period: 6.4 px, which at 508 dpi would be a 0.32 mm ridge spacing and at 12.8 px/mm a 0.50 mm one. Human ridge spacing is 0.4–0.5 mm, so 5.5 × 4.5 mm (~325 dpi) is the supported figure and 3.5 × 2.9 mm was an assumed 508 dpi. Both are inferences from one measurement, not a specification.):

| configuration | minutiae per frame, min / median / mean / max | frames with 0 | frames with ≥ 8 |
|---|---|---|---|
| mindtct defaults (perimeter points removed) | 0 / **1** / 1.52 / 7 | 236 (33 %) | 0 |
| perimeter points kept (`NBIS_KEEP_PERIM=1`) | 0 / 2 / 2.31 / 9 | 164 | 6 |

`bozorth3` needs on the order of a dozen paired minutiae for a decision; a
70×57 px frame (about 5.5 × 4.5 mm of skin, see below) yields a median of
one. Upscaling
(`NBIS_SCALE`), contrast normalisation (`NBIS_NORM`) and other `ppmm` values
were tried in the same session; the best of the variants is the second row.
An own extractor tuned to the frame size found ~7 per frame, of which only ~50 %
repeated between adjacent frames of the same press, so a minutiae matcher was
not viable either; mosaicking several presses was measured useless because
presses land on the same spot (about 1.2× the frame area in total — on the
2026-09-13 session, whose presses were concentrated; not re-measured on the
deliberately spread 2026-09-18 one). What the
sensor's size leaves is correlation on the ridge texture, which is what the
Gabor front-end does.

## What the three runs agree on, and where they do not

**Agree.** The vendor matcher beats the clean-room matcher on false rejects on
every unit, by a wide margin (0 % vs 35 %, 3.33 % vs 73.3 %, 61.7 % vs 93.3 %).
And no vendor impostor comparison in any of these runs produced a score above
0: 1440 comparisons across the three units, every one exactly 0. (That is not
the same as "never": the threshold comment in `driver/egis0576/egis_engine.h`
records an earlier development measurement with impostor scores up to 4657.)

**Disagree.** Everything about genuine acceptance. Vendor FRR 0 % / 3.33 % /
61.7 %; clean-room FRR 35 % / 73.3 % / 93.3 %; clean-room EER 15 % / 35 % / 45 %;
clean-room FAR 0 % / 0 % / 1.04 %. One run rejects nothing, one rejects three
genuine presses in five.

**Not explained by anything measured here.** Two of the three per-unit variables
order the runs the wrong way: irvingpop's exposure calibration landed nearest
the target (87.36 against a target of 88, versus sam-dant's 97.54), and his
templates are the fullest of the three while the reference run's are the
thinnest. The third — how often the engine found an enrolment frame already
covered (`-8`: 19, 10, none) — does track the FRR order, but it is one number
over three runs with one person each, so unit and person are perfectly
confounded and none of these runs can separate hardware from skin, ridge
structure or press technique.

## What this does and does not show

It shows that the vendor matcher's *genuine acceptance* is strongly
run-dependent — between 38 % and 100 % of presses accepted at the shipped
threshold across three units, one person each — and that the clean-room matcher,
on identical input, is worse on every one of them. It shows that no impostor
comparison has yet produced a vendor score above 0 (1440 comparisons, three
units), and that the clean-room flavour has now produced five false accepts on
one unit.

It does not show a population error rate, in either direction. Each run is one
person, one session, 60 genuine presses and 60 distinct impostor presses of the
same person's own fingers; there is not one cross-person impostor trial in the
corpus. It does not measure the shipped driver either: the kit enrols 6 presses
per fold where the driver enrols 12 accepted stages and asks the user to repeat
a rejected press, it allows a press no retry, and it drives the sensor itself
instead of going through the installed driver. And it cannot explain any of the
differences it measures: the frames that would be needed to do that stay on the
testers' machines by design.

The measurements still missing are the same ones as before, now with one added:
more people, a second session after a reboot on the same unit (the per-boot flat
field is the thing most likely to move scores), and the same person on two
different units — the only way to separate the unit from the finger.
*(2026-09-26: a second session on the reference unit exists now, 2026-09-18.
The vendor matcher barely moves on it — 2 / 60, 0 / 480 — while the Gabor
front-end falls from 0 / 0 to 13 / 60 and 18 / 480. What moved the scores
was where the fingers landed, not the flat-field: the two sessions' baselines
correlate at 0.997. See the last section.)*

## Reproducing on your own hardware

The capture and scoring harness ships in `tools/accuracy/` (see its README):
`capture.py` records a dataset of your own fingers with the driver's exact
finger-on/off gating and flat-field baseline, `make` builds a scorer against
each matcher flavour straight from `driver/egis0576/`, and `evaluate.py` runs
the two-fold protocol above and prints these tables. It writes a
`results.json` of scores, counts and rates only — that file, and nothing else,
is what to post in an issue. The frames are your biometric data: they stay in
a private directory under your home and must never be sent to anyone,
including the maintainers. Given how far the three runs above disagree, more
runs — especially ones that come out badly — are the most useful thing this
measurement can get.

## 2026-09-23: the third unit on the Gabor front-end, and what the kit was measuring

irvingpop re-ran the kit on his 2026-09-13 captures with the v0.5.1 tree — no
new presses, the shipped default. On his Yoga 6 13ALC6, at threshold 5000:

| | reference unit, 2026-09-13 | tsteppy's second unit | irvingpop |
|---|---:|---:|---:|
| genuine min (NCC) | 5153 (0.804) | 0.600 | 1958 (0.305) |
| impostor max (NCC) | 4426 (0.691) | **0.815** | **5788 (0.903)** |
| FRR at 5000, any frame | 0 % | 6.9 % ¹ | 60.0 % |
| FAR at 5000, any frame | 0 % | 1.1 % ¹ | **12.1 %** (58 / 480) |
| FRR / FAR under the driver's rule | **0 % / 0 %** | not measured | not measured |

¹ his own harness, his protocol, reported 2026-09-17.

Two things follow, and they are separate.

**The clean gap is a property of this unit, not of the matcher.** Both reported
foreign units score same-person impostor presses *above* the 0.78 accept point
— 0.815 there since 2026-09-17, 0.903 here. The reference unit's 0.12 of
margin is the exception in the sample, not the rule, and the README said
otherwise until today.

*(Corrected 2026-09-26: not of this unit either — of **one session** of it.
The bold sentence above replaced one over-generalisation with another. The same
unit's 2026-09-18 session scores a same-person impostor at 0.896 under the
driver's own rule, and it had been in this file since that day, set aside as
a possible mislabel. See the last section.)*

**And no false-accept figure this project published was the driver's.** The kit
applied the driver's two-frame rule to genuine presses and the press maximum to
impostors, and never applied the raw-frame corroboration at all. `kit_version
3` fixes both sides (`far_confirmed`, and a third column in `score.c`); the
reference dataset (2026-09-13) re-measured under the true rule is unchanged
at 0 % / 0 %, 60 of 60 confirmed. What that rule does to a 12.1 % needs his
re-run, not a guess: an isolated lucky frame is exactly what the two-frame
rule exists to kill, and 58 of his false accepts are press maxima.

**Tested and rejected: the overlap floor.** The returned NCC is a maximum over
~7,600 coarse and 225 fine alignments admitted from 800 px of masked overlap,
i.e. 20 % of the frame, with probes accepted from coverage 0.35 where a
well-placed frame sits at 0.70–0.75. Maximising over many small-overlap
hypotheses inflates an impostor's best score by construction, so raising the
floor was the obvious lever — tsteppy had also measured 1400 as better on his
unit. On the reference dataset it is not:

| overlap floor | genuine min | impostor max | FRR (driver's rule) | FAR |
|---:|---:|---:|---:|---:|
| **800 (shipped)** | **5153** | **4426** | **0 %** | 0 % |
| 1200 | 4841 | 4426 | 1.7 % | 0 % |
| 1400 | 4810 | 4426 | 3.3 % | 0 % |
| 1600 | 2532 | 4166 | 5.0 % | 0 % |
| 2000 | 1740 | 3928 | 18.3 % | 0 % |

It costs genuine presses and barely moves the impostor ceiling: the margin
narrows. Here the strongest impostor pairs win at 940–1491 px of overlap, not
at the floor, so the mechanism is real but the constant is not where this unit
loses. Whether it is where a *failing* unit loses is exactly what `pairdiag`
(`tools/accuracy/`) now reports, in scalars a reporter can paste.
*(2026-09-26: on the reference unit's own failing session, 2026-09-18, it is
not: the colliding pairs win at 1100–1900 px, and rebuilds at 1200 and
1600 px still accept 17 and 11 of 480 impostors under the driver's rule.)*

**What is NOT the explanation**, each checked: the flat-field (the kit's
arithmetic is character-for-character the driver's, on the reporter's own
baseline); the exposure (his calibrated frame mean lands 2 grey levels from
target, the best-centred of the three units); the local-ridge-period check (a
rejection scores 0, and not one of his 540 Gabor presses scored 0); and
tsteppy's gain-0 regime — our init blob writes `reg 0x12 = 0x05`
([`egis_init.h:63`](../driver/egis0576/egis_init.h)), so the community init
sequence does set the gain and his low-contrast finding is a different problem
from this one.

### 2026-09-26: the first foreign unit under `kit_version 3`

sam-dant captured a fresh session on his IdeaPad Flex 5 14ITL05 (now Ubuntu
26.04.1; the Zorin captures of 13/20 September no longer exist) and evaluated
it with the kit at 68456ff, default build, no overrides:

| matcher | FRR any frame | FAR any frame | FRR driver's rule | FAR driver's rule |
|---|---:|---:|---:|---:|
| vendor | 0 / 60 | 0 / 480 | 0 / 60 | 0 / 480 |
| cleanroom | 20 / 60 | 0 / 480 | 20 / 60 | 0 / 480 |
| **gabor** | 2 / 60 | 3 / 480 | 2 / 60 | **2 / 480** |

Gabor press maxima: genuine 3252 / 6194 / 6374, impostor 223 / 2366.5 / 5417
(max NCC 0.845). The two-frame rule removes one of three false accepts; it
does not close the overlap.

His `pairdiag` is what makes the run informative. Coverage p10 / median / max
0.683 / 0.717 / 0.747, genuine NCC median 0.841, impostor NCC median 0.243 —
the bulk of both populations looks like the reference unit's (0.713, 0.903,
0.270). So on this unit the false accepts are the matcher's tail, not presses
carrying too little ridge area. And his overlap-floor table places the top
impostor pair in a small-overlap alignment: 0.845 at the 800 px floor, 0.741
above 1200 px. That is the opposite of the reference unit, where the top
impostor stayed at 0.666 until 1600 px. Filtering winning alignments is not
the same experiment as rebuilding with a higher floor (a higher floor changes
which alignment wins); the rebuilt run on his frames has been asked for.
*(2026-09-26: "the reference unit" here is its 2026-09-13 session. On its
2026-09-18 session the same `pairdiag` puts the top impostor at 0.873 / 0.865 /
0.829 at the 800 / 1200 / 1600 px floors, and a rebuild at 1200 px still
accepts 17 of 480 impostors — so the rebuilt run asked of sam-dant can tell
whether his tail is small-overlap, but not deliver a universal 0 / 0. And his
0.741 above 1200 px, as a filter of winners, is a lower bound on what a
rebuild would find, not its result.)*

**Where the matchers stand, on every dataset where both were measured under
the same protocol:**

| dataset | vendor FRR / FAR | gabor FRR / FAR (driver's rule unless noted) |
|---|---:|---:|
| reference unit, 2026-09-13 ¹ | 0 % / 0 % | 0 % / 0 % |
| reference unit, 2026-09-18 ¹ | 3.3 % / 0 % (2 / 60, 0 / 480) | 21.7 % / 3.75 % (13 / 60, 18 / 480) |
| reference unit, enrol one session → probe the other ¹ ² | 5.0 % / 0 % (6 / 120, 0 / 960; any frame) | 45.0 % / 4.0 % (54 / 120, 38 / 960; any frame) |
| sam-dant, 2026-09-25 | 0 % / 0 % | 3.3 % / 0.42 % |
| irvingpop, 2026-09-13 | 61.7 % / 0 % | 63.3 % / 12.1 % (FAR any-frame; driver's-rule FAR not yet measured) |

¹ The first row was the only reference-unit row until 2026-09-26; the other
two were added with the last section, which says how they were measured.
² The kit on one dataset holding both sessions, every frame flat-fielded
against its own session's baseline. Its two folds are then the two sessions:
each twelve-press template is probed with the other session's presses of the
same finger, and with every other-finger press of both sessions. Under the
driver's rule, with twelve-press templates and impostor probes from the other
session only, Gabor gives 32 / 60 and 9 / 240 (enrolled on 09-13) and 24 / 60
and 15 / 240 (enrolled on 09-18).

On each of them the vendor matcher rejects no more genuine presses than Gabor
and accepts no impostor. *(2026-09-26: that holds under the driver's rule.
Under the kit's press-maximum rule irvingpop's run is the exception on the
genuine side — vendor 37, Gabor 36 of 60.)* The Gabor front-end's real
gains were measured against the *original clean-room* front-end
(Stepanovich's unit, 51.7 % → 6.9 %), not against vendor.

## 2026-09-26: what prevents a universal 0/0

After sam-dant's run the question was whether the Gabor front-end could be
brought to zero false accepts under the driver's own rule on every unit —
which constant, which unit property, which foreign-capture problem stood in
the way. It was investigated from five directions (how stable the reference
unit's result is, what the score measures, the anatomy of the foreign tails,
the alignment search's multiplicity, person versus sensor), every finding was
re-checked by an independent reviewer, and the key numbers were re-measured
with the unmodified kit. For the reference unit and sam-dant's, the answer is
none of those (irvingpop's unit is a separate, unexplained mode, §8). **What
prevents a universal 0/0 is what the matcher measures**, and it already fails
on the reference unit.

### How it was measured

So that each number can be redone. The kit and `pairdiag` are in this
repository; the other harnesses are described here, not committed. The
frames never left the maintainer's machine; everything below is aggregates.

- **Datasets.** The reference dataset this file was written around
  (2026-09-13). A second capture on the reference unit five days later
  (2026-09-18): same person, same five fingers, 12 presses each, placements
  deliberately spread, its own no-finger baseline — the session "Across
  sessions" above describes. The `results.json` files sam-dant (2026-09-25)
  and irvingpop (his 2026-09-13 captures) posted.
- **The kit, unmodified**, as of 68456ff (`kit_version 3`), default build,
  no overrides: `evaluate.py <dataset> --flavour gabor` and `--flavour
  vendor` (the prebuilt `score-vendor`), `pairdiag` for first-frame pair
  statistics. Which finger pair each impostor score belongs to is
  reconstructed from `evaluate.py`'s loop order (the fold loop in
  `evaluate()`).
- **Across sessions**: the kit on one dataset holding both sessions, each
  frame flat-fielded against its own session's baseline (its two folds are
  then the two sessions); and, for the driver's rule with twelve-press
  templates and impostor probes from the other session only, a scorer that is
  `score.c` with separate enrolment and probe baselines, linked against the
  unmodified adapter and front-end. An independent harness that `#include`s
  the unmodified `egis_match_gabor.c` reproduces all 540 press scores of each
  session exactly.
- **Rebuilds** for the constants in the table further down:
  `-DEG_MIN_OVERLAP`, `-DEG_ROT_MAX`, `-DEG_SRCH`, everything else
  unmodified. **Enrolment splits**: all 924 six-of-twelve splits per finger,
  steering emulated.
- **Orientation**: the coherence-weighted dominant angle of each flat-fielded
  frame's structure tensor. **Phase singularities**: Larkin–Fletcher phase
  residues.
- **Flow-only reconstruction**: a synthetic frame whose ridge phase is
  integrated, by least squares, from the matcher's own orientation field and
  period map of a real frame, with that frame's mask imposed — the frame's
  flow and period with its minutiae removed (0 phase singularities over 120
  frames). **Gabor residual**: the locally contrast-normalised frame minus
  its least-squares multiple of the Gabor output, i.e. the fine structure the
  front-end throws away; compared as a masked NCC at the alignment
  `em_match_ex` reports.

### 1. The score measures ridge flow and ridge period, not identity

The front-end steers a Gabor filter along the local orientation with one
ridge period per frame, then maximises a masked NCC over ±19 px and ±10°. In
the Gabor-filtered image the front-end correlates, a patch with no minutia in
it is essentially described by its orientation field and its period (the
finer structure that also identifies skin is what the filter removes, §6), so
two different fingers that put near-parallel ridges at a similar angle and
period onto the 70×57 px window score like one finger — what the LIMITS note
in `egis_match_gabor.c` already said about synthetic gratings, now seen
between real fingers.

- A **flow-only reconstruction** of a press, with no minutiae, is accepted
  as its own finger on **34 of 60** presses (2026-09-13) and **31 of 60**
  (2026-09-18), period check on (kit fold protocol, templates from the other
  fold). Its own-finger median is 0.824 / 0.856 against the real frame's
  0.965 / 0.940.
- Over the 2880 first-frame impostor pairs of each session, agreement of
  orientation at the winning alignment alone explains R² 0.67 (09-13) and
  0.53 (09-18) of the impostor NCC; agreement of period explains 0.010 and
  0.098, because all five of the owner's fingers have periods of 5.1–6.2 px.
- Impostor pairs whose flow agrees within 4°: **0 of 2880** on 2026-09-13,
  **19 of 2880** on 2026-09-18 (same person, unit and fingers); 14 of those
  19 reach 0.78.

How often such coincidences occur depends on how a person's fingers land in
a session; how often one is accepted is a property of the matcher.

### 2. It fails on the reference unit itself

| reference unit, Gabor, default build | rule | FRR | FAR | impostor max (NCC) |
|---|---|---:|---:|---:|
| 2026-09-13 | driver's | 0 / 60 | 0 / 480 | 0.690 |
| 2026-09-18 | press maximum | 12 / 60 | 21 / 480 | 0.896 |
| 2026-09-18 | driver's | **13 / 60** | **18 / 480** | **0.896** (score 5745) |
| enrol 09-13, probe 09-18, twelve-press templates | driver's | 32 / 60 | 9 / 240 | 0.895 |
| enrol 09-18, probe 09-13, twelve-press templates | driver's | 24 / 60 | 15 / 240 | 0.886 |
| *vendor, 2026-09-18, same frames* | both | *2 / 60* | *0 / 480* | *every impostor 0* |

20 of the 21 press-maximum accepts on 2026-09-18 are R-Daumen (right thumb)
against R-Zeige (right index) — 11 thumb templates against index probes, 9
the other way; the 21st is one L-Zeige press. That finger pair has an impostor median of 0.753 with
20 of its 48 comparisons at or above 0.78; the next pair's median is 0.58,
and on 2026-09-13 the same pair's was 0.399. The accepted impostors pass
every gate the driver has — the period check, the raw-frame corroboration,
the two-frame rule — and none of them wins at zero shift. The genuine
minimum on 2026-09-18 is 0.192: the spread placements cost genuine presses
too (10 of the 12 press-maximum rejects never reach 0.60), which the vendor
matcher, at 2 of 60, largely survives.

### 3. The 2026-09-18 labels are right

The vendor matcher on the identical frames matches the 2026-09-18 R-Daumen
only to the 2026-09-13 R-Daumen — 9 of 12 presses against the first
session's templates, 12 of 12 the other way — and scores all 960 impostor
comparisons of the combined two-session set 0. What changed between the
sessions is the thumb's angle on the sensor: its dominant ridge orientation
moved from about 31° to about 162°, which put it 1.3° from R-Zeige's; on
2026-09-13 it lay 42–48° from both index fingers, outside the ±10° search.
The reading earlier in this file that set this session aside for FAR until
the thumb/index collision was "understood" ("Across sessions", above) is
withdrawn there: the collision was the matcher's failure mode.

### 4. The 2026-09-13 0 / 0 is one favourable, in-sample session

Every Gabor constant was chosen on that dataset ("What this run does **not**
show", above), its presses were concentrated, and that day the most
parallel-ridged finger lay far from both index fingers in angle. Over the 924
enrolment splits per finger, the probability of zero impostor accepts is
**1.000** for 09-13 against itself and **0.000** for 09-18 against itself and
for both cross-session directions. Even inside 09-13 the genuine side is a
draw: 69.4 % of the splits keep the genuine minimum at or above its published
0.804. How much of the clean result is tuning and how much placement luck
the data cannot split; the conclusion does not depend on it.

### 5. No constant measured reaches 0 / 0, even on the reference unit

| what | measured | result | why it cannot work |
|---|---|---|---|
| accept threshold | lowest NCC with FAR 0 under the driver's rule | 09-18: 0.885 at **FRR 15 / 60**; enrol 09-13 → 09-18: 0.892 at **42 / 60**; enrol 09-18 → 09-13: 0.853 at **32 / 60** (09-13 alone: any threshold from about 0.69 to 0.80 gives 0 / 0; at 0.885 that session rejects 2 of 60) | the colliding impostors score where genuine presses do |
| overlap floor (`EG_MIN_OVERLAP`) | rebuilt at 1200 and 1600 px | 09-18: FAR **17 / 480** (max unchanged at 0.896) and **11 / 480** (max 0.868); across sessions at 1600: 7 / 240 and 3 / 240; on 09-13 1600 costs 3 of 60 genuine presses | the colliding pairs win at 1100–1900 px of overlap, not at the floor |
| search width | rebuilt at ±5° / ±12 px, the shipped ±10° / ±19 px, ±30°, ±90° | 09-18 FRR / FAR: ±5° / ±12 px 17 / 60 and 5 / 480 (max 0.821); shipped 13 / 60 and 18 / 480; ±30° 12 / 60 and 32 / 480; 09-13 at ±30°: FAR 1 / 480 (max 0.798); enrol 09-18 → 09-13 at ±90°: FRR 3 / 60 at FAR 32 / 240 under the driver's rule (press maximum 1 / 60 and 35 / 240), max 0.907; ±27 px: 09-18 FRR 7 / 60 (press maximum), FAR not measured | genuine re-placements need the wider search, and every alignment it admits is another chance for a flow coincidence |
| number of alignment hypotheses | random nested subsets, from 1/128 of the 13,689 alignments to all of them | **+0.014 NCC per doubling**; removing hypotheses costs genuine presses about four times as much as impostors | the tail is the amplitude of one pair's correlation field, not extreme-value statistics over many hypotheses |
| image side, simulated on 09-13 | white noise sd up to 15, contrast × 0.3, blur σ up to 1.6 px, no flat-field at all | checked first-frame impostor max at most 0.693 (unmodified 0.666), genuine median 0.87–0.91 throughout | the tail is not an image artefact of this unit: impostor NCC at zero shift is +0.0001 (SE 0.0015, n = 2700), so no fixed pattern survives the flat-field |
| Gabor front-end (σ, coherence threshold, erosion), period-check constants | **not swept** | — | argued, not measured: the colliding pairs' periods agree, and a flow-only reconstruction scores like the real frame |

Every constant tested moves the operating point along a trade-off between
false rejects and false accepts; none leaves both at zero outside the
session it was tuned on. This also answers the rebuild at 1200 px asked of
sam-dant: it can tell whether his tail is a small-overlap one, but it cannot
deliver universality, because on the reference unit's own second session it
leaves 17 of 480.

### 6. The identity is in the frames

- The **vendor matcher** separates the same captures: it scores all 1440
  impostor comparisons of 2026-09-18 (480) and of the combined two-session
  set (960) exactly 0, at 2 of 60 and 6 of 120 false rejects (any frame; 9
  of 120 under the driver's rule).
- **The fine structure the Gabor filter discards carries it.** Of the 54
  impostor pairs `em_match` accepts over all four session combinations
  (first frames), none shares a matched phase singularity and none reaches a
  Gabor-residual NCC of 0.20 (maximum 0.169); 78.8 % of the 1050 accepted
  genuine pairs do, with medians 0.60 / 0.54 / 0.48 / 0.44 for 13→13 /
  18→18 / 13→18 / 18→13. A crude gate on it — accept only at a residual NCC
  of at least 0.10 — gives **FAR 0 / 480 on both sessions** at FRR 2 / 60
  (09-13) and **18 / 60** (09-18), scoring one first frame per press on the
  kit's folds (without the gate that protocol gives 0 / 60, 0 / 480 and
  12 / 60, 11 / 480). **That is a prototype measurement: in-sample
  threshold, first frames, one person** — a candidate direction, not a
  finished gate: on 2026-09-18 its 0 / 480 costs 18 of 60 genuine presses,
  and there the colliding right thumb reproduces its own residual poorly
  (its accepted genuine pairs, ordered, first frames: residual median 0.034
  over 37 pairs, against 0.332 over 93 on 2026-09-13). It is not a matcher,
  and nothing about it is shipped. It is the same idea as the fine-structure
  check measured against synthetic artefacts under "The period check closes
  half of what was claimed" (5–7 % genuine cost there), now measured against
  real impostors.

Minutiae alone will not carry it: a 70×57 frame holds a median of one
("Why not minutiae"), so fine structure or several frames per decision have
to.

### 7. The enrolment steering's premise was partly rotation

Across the two sessions the thumb came back rotated by about 50–60° and
R-Mittel by about 30°; at ±90° the thumb's cross-session accepts go from
0 of 24 to 21 of 24 (press maximum; 19 of 24 under the driver's rule), at
±30° R-Mittel's from 0 of 24 to 21 of 24 under either rule. "Skin never
covered", the premise the steering in `egis_engine_cleanroom.c` was built
on, was partly rotation, and steering does nothing about rotation. Corrected
in "Across sessions" above and in the adapter's comment.

### 8. The foreign units

- **sam-dant** (2026-09-25, `kit_version 3`): Gabor 2 / 60 false rejects and
  2 / 480 false accepts under the driver's rule, impostor max 0.845. **15 of
  his 16 impostor comparisons at or above 0.70 are R-thumb against L-index**
  (that pair's median 0.690, against 0.555 for the same pair on 2026-09-13),
  and the same pair is his top impostor pair in two capture sessions and
  under two front-ends — the one-pair signature of the reference unit's
  2026-09-18 session. Whether his 0.845 is a flow coincidence or a
  small-overlap win is not settled (his 0.741 above 1200 px is a filter of
  winners, a lower bound).
- **irvingpop** (2026-09-13 captures, kit v2) is a **separate, unexplained
  mode**: his whole distribution is compressed. Gabor impostor median 0.647
  (2026-09-13 reference: 0.410, sam-dant 0.369), genuine median 0.733
  (0.968), AUC 0.667 (1.000, 0.9945), own finger ranked first for 22 of 60
  probes; all ten finger pairs are shifted up alike. The vendor matcher
  rejects 37 of 60 genuine presses (33 of them exactly 0) at FAR 0 / 480, and
  29 of 60 presses fail both matchers. None of the image degradations
  simulated on the reference frames reproduces it. It may be an information
  limit of his captures rather than a matcher problem; he has never run
  `pairdiag`.
- **tsteppy's unit** cannot be classified: only its extremes are known
  (genuine min 0.600, impostor max 0.815).

### The new target

The maintainer's decision, 2026-09-26: **an own matcher — not vendor code,
upstreamable — at least as good as vendor:**

- **0 false accepts under the driver's rule on every dataset**: both
  reference-unit sessions, both cross-session directions, sam-dant's,
  tsteppy's;
- **no more false rejects than vendor** on the same data;
- **tuned on one dataset and certified on the others — never on the same
  session.**

Tuning the Gabor constants is no longer the path. The upstream merge request
([`upstream-mr.md`](upstream-mr.md)) is on hold until a matcher meets that
bar.

What the evidence points at, as direction and not as a result: an accept has
to require positive identity evidence at the winning alignment that a flow
coincidence cannot produce (the discarded fine structure, matched
singularities or minutiae), and an overlap without such evidence should be no
decision rather than a flow score; alignment should come from rotation-
tolerant registration over the full angle range, with acceptance decided
separately, instead of a wider or narrower texture search; templates should
cover more area (several frames or a mosaic, with coverage measured
including rotation, and steering rebuilt on that); and every figure should
be reported per finger pair under the driver's rule, on sessions it was not
tuned on.

### What remains undecided

- **irvingpop's mechanism.** Candidates: a fixed pattern that survives the
  flat-field, frames with little detail (blur, contact, gain), placement, or
  flow coincidences in every finger pair. With one person per unit, unit and
  person cannot be separated — and whether his dataset can meet any matcher's
  0 / 0 is open, given that the vendor matcher fails it too.
- **Population FAR.** Every impostor measured anywhere in this file is the
  same person's other finger; impostors from other people have never been
  measured.
- **Whether the identity evidence of §6 separates foreign frames.** The
  residual result is one person, first frames, threshold chosen in-sample,
  and it cost 18 of 60 genuine presses on 2026-09-18, a session on which the
  colliding thumb reproduces its own residual poorly (§6).
- **sam-dant's 0.845**: flow coincidence or small overlap.
- **Constants not measured**: the Gabor front-end (σ, coherence threshold,
  erosion), the period-check constants, FAR at ±27 px. The mechanism argues
  against them helping; that is an argument, not a measurement.
- **How much of 2026-09-13 is tuning and how much luck** (§4).

What would decide the foreign cases, in aggregates a reporter can paste
without any frame leaving the machine: the Gabor-residual NCC of accepted
genuine and accepted impostor pairs, the impostor NCC at zero shift, the flow
agreement of accepted impostors, a per-finger-pair table, and overlap-floor
maxima from a re-search rather than a filter of winners — which `pairdiag`
now prints (blocks `identity`, `zero_shift`, `flow`, `by_finger_pair`,
`impostor_accepted_list`, `research_floor`); neither foreign reporter has
run that version yet.

**Corrected alongside this section** (each marked where it stood): the
README's accuracy row and matcher paragraph; "Across sessions", the
2026-09-23 section and the matcher table in this file; the merge-request
draft, now on hold ([`upstream-mr.md`](upstream-mr.md)); the upstream gap
list ([`upstream-gaps.md`](upstream-gaps.md)); the `gabor` sentence in
[`PROVENANCE.md`](../PROVENANCE.md) (the clean-room paragraph under "Why the
reverse-engineered matcher"); and the comments in
`egis_engine_cleanroom.c`, `gabor/egis_cr_tuning_gabor.h`,
`gabor/egis_match_gabor.c` and `gabor/egis_match_check.h`, which presented
the 2026-09-13 session as the unit's result, cited a worst live impostor of
0.71 for 2026-09-18 where the kit measures 0.896, and gave "skin never
covered" as the steering's premise.

### 2026-09-26: the vendor numbers were measured with a memory the driver does not have

The vendor engine adapts while it verifies: scored against one loaded
gallery, a press it rejects on its own (all twelve frames 0) is accepted
(up to 8449) once three genuine presses of the same finger have been scored
before it. Reloading the gallery before the press — which is what the driver
does at the start of every action — removes the effect entirely. The kit
loaded the gallery once per fold up to `kit_version 3`, so **every vendor
false-reject rate in this file and in the reports on issue #5 is optimistic**.
Re-measured with `kit_version 4` (gallery reloaded before every press) on the
reference unit, driver's rule:

| session | vendor FRR, kit ≤ 3 | vendor FRR, kit 4 | vendor FAR | Gabor (unchanged) |
|---|---:|---:|---:|---:|
| 2026-09-13 | 0 / 60 | 0 / 60 | 0 / 480 | 0 / 60, 0 / 480 |
| 2026-09-18 | 2 / 60 | **4 / 60** | 0 / 480 | 13 / 60, 18 / 480 |

The foreign vendor figures (sam-dant 0 / 60, irvingpop 37 / 60) need a
`kit_version 4` re-run before they can be compared with anything; the true
numbers can only be the same or worse. The bar "no more false rejects than
vendor on the same data" is read against `kit_version 4` from here on.
