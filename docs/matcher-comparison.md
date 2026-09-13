# Matcher comparison and a first FAR/FRR

Two matchers can be built into this driver (`-Degis0576_matcher=vendor|cleanroom`,
see the README). This document compares them **on identical captures**, and in
doing so gives the driver its first measured false-accept / false-reject rates.
Measured 2026-09-13 on the reference unit (Lenovo Yoga 7 14ARB7).

## Dataset

One person, one session, one flat-field baseline (mean of 8 no-finger frames,
as the driver builds it at open). Five fingers — right thumb, right index, left
index, right middle, left middle — 12 presses each. Every frame captured while
the finger was down was kept, exactly as the driver's verify loop sees them
(finger-on at raw variance ≥ 250, finger-off below 215): **715 frames, 60
presses.** Frames were stored raw; flat-fielding and preprocessing were applied
offline, identically for both matchers, by a scorer linked against each
flavour's engine behind the same `egis_engine.h` contract.

The impostor set is deliberately the hard one: the same person's *other*
fingers, including adjacent same-hand fingers — the case that sank every earlier
clean-room attempt (see `PROVENANCE.md`).

## Protocol

Two-fold per finger: enrol on presses 0–5, test on 6–11, then the reverse.
Enrolment used the first finger-on frame of each press, as the driver does. A
probe press counts as **accepted** if any of its frames scores at or above the
shipped threshold (5000) — the driver's policy. Genuine trials: the same
finger's held-out presses (60 total). Impostor trials: every press of every
other finger against each template (480 total).

## Vendor matcher (default build)

| | genuine (n = 60) | impostor (n = 480) |
|---|---:|---:|
| score min / median / max | 5609 / 8572 / 12436 | **0 / 0 / 0** |
| at threshold 5000 | **FRR 0.0 %** | **FAR 0.0 %** |

Every impostor press — 480 of them, adjacent fingers included — scored
exactly 0. The lowest genuine press scored 5609. Any threshold from 1 to 5609
is error-free on this data; the shipped 5000 sits near the top of that window
with a margin of over 5600 points. EER 0 %.

## Clean-room matcher (Thaddeus Stepanovich's, `-Degis0576_matcher=cleanroom`)

Scores are his masked NCC scaled so that his published operating point, 0.53,
lands on the driver's threshold 5000 (score = NCC × 5000 / 0.53).

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

**Consequence.** As it stands the clean-room matcher is not a drop-in
replacement: at his threshold it costs a third of genuine presses, and lowering
the threshold to recover them admits adjacent-finger impostors. Whether that is
fixable — a wider or rotation-aware search, multi-frame templates scored
jointly, a different enhancement — is now a concrete question with a dataset
and a harness to answer it against.

## What this does and does not show

It shows that on this unit, in one session, the vendor matcher separates a
person's fingers from each other perfectly with a wide margin, and that the
clean-room matcher, on identical input, does not yet — the first
measured accuracy figure this driver has had, and a stronger one than the
qualitative "genuine matches, adjacent rejected" the README carried before.

It does not show a population error rate: n is one person and one session. The
two obvious next measurements are a second person (different skin, different
ridge spacing) and a second session after a reboot (the per-boot flat field is
the thing most likely to move scores). The capture and scoring harness lives
outside the repository; the procedure above is enough to reproduce it.
