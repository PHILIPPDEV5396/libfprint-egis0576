/*
 * egis_match_evidence.h -- identity evidence for the Gabor front-end
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
 *
 * What the decision rests on, and why, is in egis_match_evidence.c. Every
 * constant below is guarded by #ifndef so the accuracy kit can measure a
 * change without editing this file.
 */
#pragma once
#include <stdint.h>
#include "egis_match.h"

/* ---- the Gabor residual (computed by egis_match_gabor.c) ------------------ */
/* Band limit of the residual, px (Gaussian sigma of the normalised
 * convolution). Set on 2026-09-18: 0.7 keeps the ridge-edge detail that
 * reproduces between presses of one finger and drops single-pixel noise. */
#ifndef EV_RES_SIGMA
#define EV_RES_SIGMA 0.7
#endif

/* ---- pose hypotheses --------------------------------------------------------
 * Where a constant says "2026-09-18", it was set on the reference unit's
 * session of that day (tools/accuracy of the out-of-tree repository), the
 * only data the constants of this file were tuned on until v2; the
 * rotation re-centring's were set on the nine development sets of
 * 2026-09-27 and v4's on the 16 of 2026-09-28, as each says.
 *
 * Rotation half-range and coarse step of the envelope search, deg. +-90 was
 * chosen as the full practical placement range, not measured on 2026-09-18
 * (rotations there are <= 12.5 deg); the decision below it was checked on
 * that session at +-10, +-30 and +-60 with the same impostor ceiling. */
#ifndef EV_ROT_MAX
#define EV_ROT_MAX 90                 /* deg, a multiple of EV_ROT_COARSE */
#endif
#ifndef EV_ROT_COARSE
#define EV_ROT_COARSE 10              /* deg */
#endif
/* Translation half-range, px: about the largest shift that still leaves
 * EV_MIN_OVERLAP (a 70 x 15 px strip at the masks' typical 0.7 x 0.7
 * coverage). 2026-09-18: 30 -> 42 recovers three genuine presses (left
 * index 1, 7, 8) whose poses sit at 35-41 px; at 38 one of them (left index
 * 1) is lost again, at 46 nothing changes. */
#ifndef EV_SRCH
#define EV_SRCH 42
#endif
/* Coarse shift grid of the envelope, px: the envelope does not oscillate
 * with the ridge phase, so a grid coarser than the ridge period samples it. */
#ifndef EV_CSTEP
#define EV_CSTEP 3
#endif
/* Distinct envelope peaks refined and scored, in order of envelope.
 * 2026-09-18 (first identity-gate build): at the wider search, 4 lost one
 * genuine press without the orientation-field proposals that build had
 * (more candidates compete for the slots), and 4-7 gave the same press
 * decisions with them; 6 kept that margin either way. Unchanged since;
 * the significance proposals (EV_NSIG) come on top. */
#ifndef EV_NHYP
#define EV_NHYP 6
#endif
/* Significance proposals (ev_hyps): how many envelope peaks are added in
 * order of E sqrt(n) -- the envelope weighted by the square root of the
 * overlap it was computed over -- beyond the EV_NHYP taken in order of E.
 * 2 = the two orientation-field proposals of the first identity-gate build,
 * which they replace, so the hypothesis count stays at 8; not tuned.
 * Measured on the four development sets (egis_match_evidence.c, MEASURED;
 * an integer, so +-1 for +-10 %): 1 and 3 give results identical to 2 on
 * all four -- the same decisions, strongest impostor presses and weakest
 * genuine presses; 0 gives the same decisions, but 18 -> 13's weakest
 * genuine press falls from 2.89 x to 1.69 x the accept point and no
 * impostor press's level changes. */
#ifndef EV_NSIG
#define EV_NSIG 2
#endif
/* A coarse envelope below this is not a hypothesis (inactive on 2026-09-18:
 * the top peaks always exceed it). */
#ifndef EV_HYP_MIN
#define EV_HYP_MIN 0.30
#endif
/* Full-resolution refinement half-width around a coarse peak, px. */
#ifndef EV_REF
#define EV_REF 3
#endif
/* Refined ridge-image NCC a hypothesis must reach before its fine structure
 * is looked at (2026-09-18). Not an accept criterion: it only keeps the
 * block search off poses where not even the ridge flow agrees. */
#ifndef EV_GFLOOR
#define EV_GFLOOR 0.50
#endif
/* A pose with less masked overlap, px, is not a hypothesis. Not the
 * front-end's 800 (EG_MIN_OVERLAP in egis_match_gabor.c): about the least
 * area in which W can reach the accept point at a genuine press's block NCC
 * (three blocks at the 8 px step, 16 x 32 px). 2026-09-18: 800 -> 500
 * recovers three genuine presses at 510-700 px; at 550 one of them is lost
 * again, at 450 nothing changes. Until v4 this said the evidence needs no
 * floor of its own; since v4 the same floor applies to the area a set
 * actually scores too (EV_MIN_SCORED), and two of those three presses are
 * rejected again (egis_match_evidence.c, "What these three cost"). */
#ifndef EV_MIN_OVERLAP
#define EV_MIN_OVERLAP 500
#endif

/* Rotation step of the full-resolution refinement, deg, within half a coarse
 * step either side. The step-3 prototype's 1.25 costs a third more time;
 * blocks shift independently and adjacent ones may disagree by EV_EPS, so a
 * pose 1.25 deg off moves adjacent blocks against each other by 0.2 px.
 * 2026-09-18: 2.5 gives the same press decisions as 1.25. */
#ifndef EV_REF_ROT
#define EV_REF_ROT 2.5
#endif

/* ---- block evidence ---------------------------------------------------------
 * The frame is cut into EV_BS x EV_BS blocks every EV_STEP px; each block
 * with at least half its pixels in the overlap searches its own shift
 * (+-EV_RS px) for its best residual NCC; blocks at or above EV_TAU whose
 * shifts agree with a neighbour's within EV_EPS px form 8-connected sets,
 * and a set's evidence is the sum of its blocks' Fisher-z excess over
 * EV_TAU. All set on 2026-09-18 by the step-3 prototype. */
#ifndef EV_BS
#define EV_BS 16
#endif
#ifndef EV_STEP
#define EV_STEP 8
#endif
#ifndef EV_RS
#define EV_RS 6
#endif
#ifndef EV_TAU
#define EV_TAU 0.60
#endif
#ifndef EV_EPS
#define EV_EPS 1.5
#endif

/* Re-centring of the block search (ev_hyp_evidence): a best set of at
 * least EV_RC_N blocks whose mean shift is at least EV_RC_MIN px moves the
 * pose once by that shift, and the blocks search again; the higher W is
 * kept. EV_RC_N 0 disables it. Chosen, not tuned: three blocks is the least
 * set in which W reaches the accept point at a genuine press's block NCC
 * (the argument of EV_MIN_OVERLAP), and EV_RC_MIN is half of EV_RS, the
 * shift at which a block's search window has lost half its reach on one
 * side. Measured on the four development sets (09-18, 09-13, 13 -> 18,
 * 18 -> 13, the driver's rule): EV_RC_MIN 2.7 and 3.3 (-+10 %) give
 * results identical to 3.0 on all four; EV_RC_N 4 gives the same decisions,
 * 13 -> 18's weakest genuine press at 1.01 x instead of 1.08 x, and lower
 * impostor figures (09-13's strongest press 0.00 x instead of 0.06 x, the
 * one-frame maxima of 09-13 and 13 -> 18 0.55 x / 0.60 x instead of
 * 0.60 x / 0.99 x); EV_RC_N 2 loses 09-18's right middle 0 (4 / 60): a
 * two-block set moves its second frame to a pose where the flat-fielded W
 * is higher but the raw frame does not corroborate; off, 13 -> 18 rejects
 * 8 / 60 (left index 9 and right thumb 9 lost). No setting accepts an
 * impostor press. */
#ifndef EV_RC_N
#define EV_RC_N 3
#endif
#ifndef EV_RC_MIN
#define EV_RC_MIN 3.0
#endif

/* Rotation re-centring (ev_hyp_evidence), after the translation one: the
 * best set's block shifts, fitted by least squares as a small turn about
 * the set plus a common shift, say by how much the probe is still turned
 * against the template; when the set has at least EV_RR_N blocks and the
 * turn is EV_RR_MIN..EV_RR_MAX deg, the probe is rotated by that much more
 * (the set's centroid kept in place) and the blocks search again; the higher
 * W is kept. EV_RR_N 0 disables it. Chosen on the nine development sets of
 * 2026-09-27 (the three sessions of the reference unit in session and in all
 * six directions across them, the driver's rule and setting):
 *   EV_RR_N 6    a turn needs a set that spans an area, two rows of three.
 *                Over the nine sets' 1,352,411 scored hypotheses the step is
 *                taken on 50,226 genuine and 26 impostor ones, and raises
 *                the evidence of 22,780 of 53,028 genuine frame pairs (14,816
 *                by more than W 0.5) and of 14 of 258,930 impostor pairs (by
 *                at most 0.5, to at most W 0.86). At 3 (= EV_RC_N) and 4
 *                the decisions are the same, but the step is taken on 2,024
 *                and 690 impostor hypotheses and lifts impostors: at 3 the
 *                strongest impostor press in session on 2026-09-13 goes from
 *                0.06 to 0.32 x and on 2026-09-26 from 0.27 to 0.41 x; at 4
 *                the strongest single impostor frames rise (2026-09-18 in
 *                session 0.65 -> 0.74 x). 5 and 7 (+-10 %, the integer
 *                neighbours): the same decisions on all nine sets and no
 *                impostor figure higher (at 5 the strongest impostor press
 *                of 2026-09-26 -> 2026-09-18 falls from 0.39 x to 0.14 x).
 *   EV_RR_MIN 1.25 deg, half of EV_REF_ROT: a smaller turn is within the
 *                refinement's own step. 1.125 and 1.375: the same
 *                decisions and impostor figures on all nine sets.
 *   EV_RR_MAX 10 deg, EV_ROT_COARSE: a larger turn is not a residual of the
 *                refinement but another coarse hypothesis. 9 and 11: the
 *                same decisions and impostor figures on all nine sets.
 * What it does there is in egis_match_evidence.c, "Why the rotation
 * re-centring". */
#ifndef EV_RR_N
#define EV_RR_N 6
#endif
#ifndef EV_RR_MIN
#define EV_RR_MIN 1.25
#endif
#ifndef EV_RR_MAX
#define EV_RR_MAX 10.0
#endif

/* ---- v4: independent area, the area rule, along-ridge hypotheses ---------
 * Why each exists: egis_match_evidence.c, "Why independent area", "Why the
 * floor on the area scored", "Why the area rule", "Why along-ridge
 * hypotheses". "The 16 development sets" are the reference unit's four
 * sessions (2026-09-13, -18, -26, -27) in session and across them in all
 * twelve directions, the driver's rule and setting; every value below was
 * chosen or checked on them, so every figure is IN-SAMPLE. Sensitivity
 * (+-10 %, all 16 sets): egis_match_evidence.c, MEASURED.
 *
 * EV_AREA: independent blocks (the sum of a set's block weights,
 * ev_block_weight) from which on the accept point is W 2
 * (EGIS_CR_ACCEPT_EVIDENCE). A set of less independent area a counts
 * w a / EV_AREA, i.e. its accept point rises as 2 EV_AREA / a. 8 = one block
 * above the largest set any impostor pair formed on the 16 development sets:
 * 7 blocks, under v3's full search of all 464,760 impostor frame pairs
 * (W >= 1) and under this search on the 308 impostor presses whose frames
 * reached W 0.6 under v3 (independent area under 7). So no set of an
 * impostor's size reaches the accept point at the per-block NCC that
 * suffices from 8 blocks on. Not tuned. v4.1, whose weights count a border
 * block in full where its set lacks the regular neighbour: on those 308
 * presses, every probe frame against every template frame at the pose of a
 * search without the floor and the area rule (the strongest evidence there
 * is), the 3,710 sets hold at most 8 blocks and at most 6.25 independent
 * blocks, under v4's weights and v4.1's alike (317 sets count more under
 * v4.1, none reaches 7). EV_AREA_HARD 1 makes it a minimum area instead (a
 * smaller set counts nothing): see "Why the area rule" for why not. */
#ifndef EV_AREA
#define EV_AREA 8.0
#endif
#ifndef EV_AREA_HARD
#define EV_AREA_HARD 0
#endif
/* EV_MIN_SCORED: template pixels a set must actually score -- the union
 * over its blocks of the pixels where the template's mask and the probe's,
 * at the block's own shift, overlap -- for its evidence to count. It is
 * EV_MIN_OVERLAP, applied where the evidence is computed instead of at the
 * proposed pose (the same argument: about the least area of three blocks at
 * the 8 px step, 16 x 32 px); not tuned. */
#ifndef EV_MIN_SCORED
#define EV_MIN_SCORED EV_MIN_OVERLAP
#endif
/* Along-ridge hypotheses: once every hypothesis has been scored and none
 * cleared the accept point, the EV_AR_H best ones (by refined ridge-image
 * NCC) are moved by +-EV_AR_STEP px along the ridges of their overlap and
 * scored again; what they find counts only from EV_AR_N independent blocks
 * on. EV_AR_N 0 disables them.
 *   EV_AR_N 10     midway between the largest impostor set (7 blocks) and
 *                  the smallest set of the genuine search failures they are
 *                  for (12 blocks; 15-27 at the fine structure's pose).
 *   EV_AR_STEP 12  with the blocks' own +-EV_RS (6) px search, the moved
 *                  pose covers 6-18 px along the ridges on either side and
 *                  the hypothesis itself 0-6: the fine structure of the
 *                  genuine presses this is for sits 10-14 px along the
 *                  ridges from the hypothesis. +-6 as well (the first
 *                  design, +-6 and +-12) re-searches half of what the
 *                  hypothesis already searched and costs twice the time.
 *   EV_AR_H 4      the fourth-ranked hypothesis is the one that lands 13 px
 *                  off on the second of those presses (egis_match_evidence.c,
 *                  "Why along-ridge hypotheses"); what they cost is in
 *                  COST there. */
#ifndef EV_AR_N
#define EV_AR_N 10
#endif
#ifndef EV_AR_H
#define EV_AR_H 4
#endif
#ifndef EV_AR_STEP
#define EV_AR_STEP 12.0
#endif

/* The blocks tile the WHOLE frame: every EV_STEP px from the top-left corner,
 * plus one column / row anchored at the right / bottom border where the step
 * does not end on it. Without that last column and row (70 = 6 x 8 + 16 + 6,
 * 57 = 5 x 8 + 16 + 1) the frame's last 6 columns and last row belonged to no
 * block, and an overlap at the template's right or bottom border -- a
 * placement shifted by 25-40 px, which leaves a strip of 30-45 px -- lost up
 * to a fifth of its width. Measured on 2026-09-18 (driver's rule, with the
 * rest of this file as it is): without the last column and row two genuine
 * presses are rejected that are accepted with them (5 / 60 -> 3 / 60), and
 * no impostor press is accepted either way. */
#define EV_NBX ((EM_W - EV_BS) / EV_STEP + 1 + ((EM_W - EV_BS) % EV_STEP != 0))
#define EV_NBY ((EM_H - EV_BS) / EV_STEP + 1 + ((EM_H - EV_BS) % EV_STEP != 0))
#define EV_NB (EV_NBX * EV_NBY)

/* A frame as the evidence stage sees it: the front-end's ridge image and
 * mask, and the residual. 51,880 bytes. */
typedef struct
{
  EmFrame f;
  float   res[EM_N];       /* band-limited Gabor residual, 0 outside mask */
} EvFrame;

/* A probe: an EvFrame plus the quadrature of its ridge image (-grad, scaled
 * to the ridge image's energy over the mask), which the envelope search
 * correlates against so that its landscape does not oscillate with the ridge
 * phase. Only the probe needs it. 83,800 bytes. */
typedef struct
{
  EvFrame e;
  float   q1[EM_N], q2[EM_N];
} EvProbe;

/* The pose the evidence was found at: the probe rotated by rot_deg about the
 * frame centre, template (x, y) <-> probe (x - dx, y - dy). */
typedef struct
{
  double rot_deg;
  int    dx, dy;
  int    valid;
} EvPose;

/* Implemented in egis_match_gabor.c: em_frame_compute() plus the residual. */
void em_frame_residual (const uint8_t *raw,
                        EmFrame       *f,
                        float         *res,
                        double         sigma);

void ev_frame_compute (const uint8_t *raw,
                       EvFrame       *F);
void ev_probe_compute (const uint8_t *raw,
                       EvProbe       *P);

/* Identity evidence of probe p against template frame t, in milli-units of
 * W (see egis_match_evidence.c), >= 0; -1 when no alignment reaches the
 * overlap floor at all ("nothing to score"). Hypotheses are scored in order
 * of their ridge-image NCC; with stop_at > 0 the search stops once the
 * evidence reaches it. *pose (may be NULL) receives the pose of the best
 * evidence. */
int ev_match (const EvFrame *t,
              const EvProbe *p,
              int            stop_at,
              EvPose        *pose);

/* The evidence at one given pose (same statistic, no hypothesis search), in
 * milli-W; -1 if the pose has no usable overlap. */
int ev_evidence_at (const EvFrame *t,
                    const EvProbe *p,
                    const EvPose  *pose);
