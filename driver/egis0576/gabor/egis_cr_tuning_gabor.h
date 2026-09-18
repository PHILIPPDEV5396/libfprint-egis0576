/* egis_cr_tuning_gabor.h -- the clean-room adapter's three operating-point
 * constants for the Gabor front-end (egis_match_gabor.c).
 *
 * Copyright (C) 2026 Philipp Oster
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * egis_engine_cleanroom.c calibrates these to the NCC distribution of
 * tsteppy/egis_match.c. The Gabor front-end produces a different distribution
 * (both populations sit higher), so linking it behind the adapter's defaults
 * would accept impostors: at the shipped EGIS_CR_ACCEPT_NCC 0.53 it measured
 * 14 % false accepts on the reference dataset. This header is force-included
 * (-include) ahead of the adapter when the Gabor flavour is built, so the
 * operating point travels with the front-end instead of hiding in a build
 * flag. Values and provenance:
 *
 *   EGIS_CR_ACCEPT_NCC     placed in the gap between the lowest genuine press
 *                          (0.81, flat-fielded and raw alike) and the highest
 *                          impostor press (0.69) on the reference dataset
 *                          (tools/accuracy, 60 / 480) -- deliberately NOT at
 *                          its mid-point (0.75, where the two folds agree to
 *                          0.001) but 0.03 above it, on the genuine side. A
 *                          false reject costs a retry; a false accept costs
 *                          the login, so the margin belongs on the impostor
 *                          side: 0.09 against the reference dataset's worst
 *                          impostor, 0.07 against the worst live one seen so
 *                          far (0.71, 2026-09-18), 0.03 under the worst
 *                          genuine press. Maps to EGIS_THRESHOLD (5000)
 *                          exactly. What this margin is NOT: a defence
 *                          against synthetic ridge textures -- a curved
 *                          grating at the right period scores 0.88-0.96
 *                          against real templates, above the worst genuine
 *                          press, so no value of this constant separates
 *                          them (see egis_match_gabor.c, LIMITS).
 *   EGIS_CR_MIN_COVERAGE   the front-end's per-pixel mask after erosion covers
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

#define EGIS_CR_ACCEPT_NCC 0.78
#define EGIS_CR_MIN_COVERAGE 0.35
#define EGIS_CR_REDUNDANT_NCC 1.0
/* Raw-frame corroboration of an accept (egis_verify_raw_ok, see
 * egis_engine.h for the hole it closes). Genuine un-flat-fielded probes
 * against flat-fielded templates: min 0.79 over 66 frames on the reference
 * unit; a featureless contact that reached 0.93 flat-fielded through a
 * poisoned baseline reaches ~0.2 raw. 0.5 sits between with margin both ways. */
#define EGIS_CR_RAW_CORROBORATE_NCC 0.5
