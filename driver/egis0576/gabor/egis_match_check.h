/*
 * egis_match_check.h -- local-ridge-period consistency check for the Gabor front-end
 * Copyright (C) 2026 Philipp Oster
 *
 * Own header next to egis_match_gabor.c; tsteppy/egis_match.h is untouched.
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
 */
#pragma once
#include "egis_match.h"

#ifndef EM_FB
#define EM_FB 4              /* period-map cell size in px (window = 3x3 cells
                             * when EM_FQ_SMOOTH, i.e. 12x12 px every 4 px) */
#endif
#define EM_FBX ((EM_W + EM_FB - 1) / EM_FB)   /* 18 */
#define EM_FBY ((EM_H + EM_FB - 1) / EM_FB)   /* 15 */
#define EM_NB (EM_FBX * EM_FBY)

typedef struct
{
  double ncc;        /* plain masked NCC (what em_match used to return)    */
  int    dx, dy;     /* winning shift (template coordinates)               */
  double rot_deg;    /* winning rotation of the probe                      */
  int    overlap;    /* masked overlap pixels at the winning alignment     */
  int    nblk;       /* blocks with a valid period in BOTH frames          */
  double t_spread;   /* sd of the template's block period over its blocks  */
  double p_spread;   /* sd of the probe's block period (aligned)           */
  double mad;        /* mean |Tperiod - Pperiod| over common blocks        */
  double corr;       /* corr of the two period-deviation maps              */
  double dmean;      /* mean(Tperiod) - mean(Pperiod) over common blocks   */
} EmMatchInfo;

/* Decision at the winning alignment (em_match returns -1 when it fails):
 *   nblk     >= EM_FQ_MIN_NBLK      enough cells with a period in BOTH frames
 *   mad      <= EM_FQ_MAX_MAD       template and probe period agree (px)
 *   p_spread >= EM_FQ_MIN_PSPREAD   the probe's period VARIES over the overlap
 *   corr     >= EM_FQ_MIN_CORR      (unused at -2: a smooth chirp satisfies it)
 * Provenance (reference dataset, kit protocol 60/480, all pairs dumped):
 * genuine pairs with NCC >= 0.78 have mad p50 0.08 / p95 0.31, p_spread p5
 * 0.21 / p50 0.55, nblk p5 12 / p50 47; the review's synthetic accepts sit
 * at p_spread < 0.25 (sine 0.03, arc 0.05, loop 0.17, ridge noise 0.19,
 * wavy 0.18, hill-climb model 0.38 whole-frame but < 0.25 over the overlap
 * that wins). 0.25 is the largest value that costs no genuine press in the
 * kit (0.30 costs 4/60).
 *
 * WHAT THIS CLOSES, MEASURED, AND WHAT IT DOES NOT (2026-09-22, six blind
 * families over widened parameter grids against the 30 real enrolment
 * templates of the reference dataset; best score reachable with the check
 * passing, accept threshold 0.78):
 *
 *     family   check off   check on
 *     sine       0.865       0.700   closed
 *     arc        0.909       0.699   closed
 *     chirp      0.784       0.768   closed
 *     noise      0.875       0.795   PASSES
 *     loop       0.907       0.892   PASSES
 *     jitter     0.914       0.895   PASSES
 *
 * So it closes families whose period is CONSTANT, and an attacker who
 * modulates the period -- two extra sine terms, no score oracle, no
 * knowledge of the victim's print -- goes straight through. Against a
 * score-oracle attacker it is worth 0.000-0.002 NCC (independently
 * re-measured). Treat it as what it is: a cheap filter for the most naive
 * generated textures, not the driver's answer to a fabricated artefact.
 * That answer does not exist in this driver; see docs/matcher-comparison.md.
 *
 * EM_FQ_MIN_NBLK is NOT a texture criterion. nblk is arithmetically the
 * number of blocks geometrically eligible over the overlap (r = 0.9996
 * against the eligibility count; it equals it outright in 82 % of pairs),
 * so the clause is an overlap gate, and at 20 it sat at ~1200-1400 px --
 * well above the matcher's own EG_MIN_OVERLAP of 800 -- i.e. above the
 * genuine 5th percentile above. Measured over all four blind families and
 * both sessions: every value from 8 to 20 gives BIT-IDENTICAL attack
 * numbers (no synthetic pair is stopped by this clause that another clause
 * does not stop), while 20 costs one genuine press of 60 in-session and
 * three of the cross-session presses under the driver's two-consecutive-
 * frame rule. 12 costs none in-session and one across sessions, and still
 * refuses to judge a pair with almost no eroded overlap. */
#ifndef EM_FQ_ENABLE
#define EM_FQ_ENABLE 1
#endif
#ifndef EM_FQ_MIN_NBLK
#define EM_FQ_MIN_NBLK 12
#endif
#ifndef EM_FQ_MAX_MAD
#define EM_FQ_MAX_MAD 0.5
#endif
#ifndef EM_FQ_MIN_PSPREAD
#define EM_FQ_MIN_PSPREAD 0.25
#endif
#ifndef EM_FQ_MIN_CORR
#define EM_FQ_MIN_CORR -2.0
#endif
/* A probe whose period map is flat (p_spread < EM_FQ_MIN_PSPREAD) still
 * passes when it reproduces the template's map: corr >= RESCUE_CORR and
 * mad <= RESCUE_MAD, with at least RESCUE_PSPREAD of variation so that the
 * correlation is not computed on noise. Measured genuine flat pairs on the
 * reference unit: corr 0.58-0.97, mad 0.03-0.12; blind synthetic families
 * have nothing to correlate. */
#ifndef EM_FQ_RESCUE_PSPREAD
#define EM_FQ_RESCUE_PSPREAD 0.06
#endif
#ifndef EM_FQ_RESCUE_CORR
#define EM_FQ_RESCUE_CORR 0.70
#endif
#ifndef EM_FQ_RESCUE_MAD
#define EM_FQ_RESCUE_MAD 0.10
#endif
#ifndef EM_FQ_GATE_FROM
#define EM_FQ_GATE_FROM 0.70 /* run the check only from this NCC upwards */
#endif


/* em_match() with the diagnostics of the winning alignment. */
double em_match_ex (const EmFrame *a,
                    const EmFrame *b,
                    EmMatchInfo   *info);

/* block period map of one frame (in its own coordinates); returns #valid */
int em_period_map (const double  *img,
                   const uint8_t *mask,
                   double        *pmap,
                   double        *conf);
