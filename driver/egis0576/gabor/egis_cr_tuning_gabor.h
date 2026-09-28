/* egis_cr_tuning_gabor.h -- the clean-room adapter's three operating-point
 * constants for the Gabor front-end (egis_match_gabor.c).
 *
 * Copyright (C) 2026 Philipp Oster
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * The adapter's defaults are calibrated to the NCC distribution of
 * tsteppy/egis_match.c. The Gabor front-end produces a different distribution
 * (both populations sit higher), so linking it behind those defaults would
 * accept impostors: at the original 0.53 accept point it measured 14 % false
 * accepts on the reference dataset. The two constants the front-end owns
 * outright -- the accept NCC (em_match_threshold) and the probe coverage gate
 * (em_min_coverage) -- are defined in egis_match_gabor.c itself, as
 * egis_match.h prescribes; this header carries the adapter-side policy that
 * depends on them, and the adapter includes it directly when it is built
 * against this front-end. Values and provenance:
 *
 *   em_match_threshold     (egis_match_gabor.c, 0.78) placed in the gap between the lowest genuine press
 *                          (0.81, flat-fielded and raw alike) and the highest
 *                          impostor press (0.69) of ONE session of the
 *                          reference unit (2026-09-13, tools/accuracy, 60 /
 *                          480 -- the session every front-end constant was
 *                          tuned on) -- deliberately NOT at
 *                          its mid-point (0.75, where the two folds agree to
 *                          0.001) but 0.03 above it, on the genuine side. A
 *                          false reject costs a retry; a false accept costs
 *                          the login, so the margin belongs on the impostor
 *                          side: 0.09 against that session's worst
 *                          impostor, 0.03 under its worst genuine press.
 *                          That gap exists on no other dataset. Until
 *                          2026-09-26 this line also claimed 0.07 of margin
 *                          against "the worst live impostor seen so far
 *                          (0.71, 2026-09-18)": wrong -- the kit on the
 *                          2026-09-18 session of the same unit, person and
 *                          fingers measures 0.896 under the driver's own rule
 *                          (right thumb against right index; 18 of 480
 *                          impostor presses accepted, where the vendor
 *                          matcher on the same frames accepts none). No
 *                          value of this constant gives 0 / 0 outside the
 *                          tuning session: FAR 0 on 2026-09-18 needs 0.885
 *                          and rejects 15 of 60 genuine presses; enrolled on
 *                          one session and probed with the other it needs
 *                          0.892 / 0.853 and rejects 42 / 32 of 60. Why:
 *                          egis_match_gabor.c, LIMITS. Maps to EGIS_THRESHOLD
 *                          (5000) exactly. What this margin is NOT: a defence
 *                          against synthetic ridge textures -- a curved
 *                          grating at the right period scores 0.88-0.96
 *                          against real templates, above the worst genuine
 *                          press, so no value of this constant separates
 *                          them (see egis_match_gabor.c, LIMITS).
 *   em_min_coverage        (egis_match_gabor.c, 0.35) the front-end's per-pixel mask after erosion covers
 *                          0.70-0.75 of a well-placed frame and < 0.2 of an
 *                          empty or smeared one; tsteppy's block mask, which
 *                          0.55 was chosen for, is quantised to 16x16 blocks
 *                          and not comparable.
 *   EGIS_CR_REDUNDANT_NCC  disabled (1.0): with this front-end two DIFFERENT
 *                          presses of the same finger reach NCC 0.99, the same
 *                          value as two frames of one press, so the gate cannot
 *                          tell them apart and at 0.95 it threw away one in
 *                          four genuine enrolment presses. The driver already
 *                          waits for finger-off between enrolment stages
 *                          (driver/egis0576.c), so same-press duplicates cannot
 *                          reach the adapter in the first place.
 */
#pragma once

#define EGIS_CR_MIN_ENROL_COVERAGE 0.60   /* well-placed frames: 0.70-0.75 */
#define EGIS_CR_REDUNDANT_NCC 1.0
/* The accept point of the identity evidence (egis_match_evidence.c),
 * milli-W: accept at W >= 2.0, mapped onto EGIS_THRESHOLD. Set on the
 * reference unit's 2026-09-18 session by the step-3 prototype, at about the
 * geometric middle of its gap (strongest impostor press W 1.24, weakest
 * accepted genuine 3.09); not moved since. */
#ifndef EGIS_CR_ACCEPT_EVIDENCE
#define EGIS_CR_ACCEPT_EVIDENCE 2000
#endif
/* Raw-frame corroboration of an accept (egis_verify_raw_ok, see
 * egis_engine.h for the hole it closes): the evidence of the UN-flat-fielded
 * frame at the pose and template frame that produced the accept, milli-W.
 * The raw frame carries the sensor's fixed pattern into the residual, so
 * genuine raw evidence runs at about 0.44 of the flat-fielded (median,
 * 2026-09-18); at 0.75, over that session's on-gate frames, 98.7 % of the
 * genuine frames that clear the accept point pass, 0.16 % of all impostor
 * frames (the step-3 prototype's raw check, the best ridge-image NCC over the
 * whole hypothesis search, passed 94 %). Set on 2026-09-18, between the
 * impostor frames' p99 (0.36) and the accepted genuine frames' p5 (1.39).
 * With the significance proposals and the re-centring (the pose can now be
 * a moved one) the same session gives 98.3 % and 0.16 % (p99 0.40, p5
 * 1.33); not moved.
 * WHICH pose (2026-09-27): those figures were taken at the best pose over
 * the template; the driver's verify stops at the FIRST pose that clears the
 * accept point, often the weakest that does, and until then the raw frame
 * was checked there only. On the nine development sets of that day (the
 * reference unit's three sessions, in session and across them, the
 * driver's setting) that passed 5,568 of 6,009 genuine frames over the
 * accept point (92.7 %) and cost four genuine presses. Since then
 * egis_verify_raw_ok asks the early-exit pose and, only if that fails, the
 * best pose over the template (both found on the flat-fielded frame):
 * 5,970 of 6,046 (98.7 %), with the rotation re-centring. The impostor side
 * is unchanged there: 1 of the nine sets' 34,176 on-gate impostor frames
 * reaches the accept point (2026-09-26 templates, 2026-09-18 probe, W 2.24,
 * raw corroborated at its early-exit pose already, the next frame at 0.39 x)
 * and the second pose is never asked for an impostor frame. Where it would
 * matter -- the raw frame of another finger or a featureless raw frame
 * behind an accepted flat-fielded one -- see the adapter's
 * egis_verify_raw_ok. */
#ifndef EGIS_CR_RAW_EVIDENCE
#define EGIS_CR_RAW_EVIDENCE 750
#endif
/* Which poses the raw frame is asked at (egis_verify_raw_ok), since v4
 * (2026-09-28): verify's early-exit pose; when that fails, ANY template
 * frame whose flat-fielded evidence clears the accept point, at the pose
 * that evidence was found at (until then only the best one over the
 * template, which is one of them). 0 restores v3's two poses. Measured on
 * the 16 development sets (egis_match_evidence.c, MEASURED): the new
 * statistic lowers raw corroboration (genuine frames over the accept point
 * corroborated: v3 10,507 of 10,644 = 98.7 %, v4 with two poses 10,338 of
 * 10,603 = 97.5 %, v4 with any pose 10,390 = 98.0 %), and any pose
 * recovers two genuine presses (S1 -> S2 left index 7, S3 -> S2 right index
 * 10) that the two poses lose. The attacks the check exists for were
 * re-measured behind every one of the 10,603 genuine flat-fielded frames
 * over the accept point, the raw frame substituted by: the probe session's
 * no-finger baseline (featureless), the raw frame of another finger, and
 * the genuine raw frame with its ridges blurred away (two 9 x 9 box
 * filters: a featureless contact of the finger's own shape, what a
 * poisoned baseline leaves in the raw bytes). Each corroborated 0 times,
 * as under v3 (0 of 10,644 each) and v4 with two poses (0 of 10,603).
 * v4.1: the same 10,603 frames, the same 10,390 corroborated, 0 for each
 * substitution (and for that blur with the sensor's fixed pattern added
 * back); the template frames before the one verify stopped at are no
 * longer searched again (the same verdict on every one of the 11,428
 * genuine frames), which takes the path where the raw frame fails from 142
 * to 134 ms on average at -O2, its worst case (206 ms) unchanged. */
#ifndef EGIS_CR_RAW_ANY_POSE
#define EGIS_CR_RAW_ANY_POSE 1
#endif
