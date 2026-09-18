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

## Gabor front-end (`-Degis0576_matcher=gabor`), reference unit only

The third flavour keeps Thaddeus Stepanovich's adapter and masked NCC and
replaces what is fed into it: an orientation-selective Gabor enhancement
instead of the isotropic high-pass, a per-pixel coherence mask instead of the
16×16-block one, and a rotation search (±10° in 2.5° steps, coarse-to-fine)
on top of the ±19 px translation search
(`driver/egis0576/gabor/egis_match_gabor.c`; the file's header carries the
full ablation, cost and caveats). It ships with its own operating point,
NCC 0.75 → score 5000 (score = NCC × 5000 / 0.75, so a perfect 1.0 logs as
6667), because both populations sit higher than with his front-end and his
0.53 would false-accept 14 % of same-person impostors.

Same captures, same protocol, same kit (`score-gabor`), 2026-09-17:

| | genuine (n = 60) | impostor (n = 480) |
|---|---:|---:|
| score min / median / max | 5411 / 6464 / 6604 | 407 / 2769 / 4603 |
| in NCC terms | 0.81 / 0.97 / 0.99 | 0.06 / 0.42 / 0.69 |
| at threshold 5000 (NCC 0.75) | **FRR 0.0 %** | **FAR 0.0 %** |

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
— 0.75 here, 0.81 there — which is the open problem a third unit has to
inform. He also showed that his own front-end, retuned jointly (±26 px,
1400 px overlap floor), ties the Gabor one on *raw* frames pooled and loses
to it flat-fielded, so the ablation in the file's header, measured against a
fixed 800 px floor, is one dataset's answer to "what each piece contributes".

Cost, for the driver's every-frame scoring loop: `em_frame_compute` 1.6 ms
(his 0.17 ms), `em_match` 4.0 ms against 0.85 ms at ±6 and 6.4 ms at ±19 —
the rotation search costs less than the wider translation search alone,
because its coarse pass is decimated and one NCC pass accumulates raw sums.

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
