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
 * only data any constant of this file was tuned on.
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
 * front-end's 800 (EG_MIN_OVERLAP in egis_match_gabor.c): the evidence
 * needs no floor of its own -- a smaller overlap holds fewer blocks -- and
 * this one only keeps the pose ranking meaningful, at about the least area
 * in which W can reach the accept point at a genuine press's block NCC
 * (three blocks at the 8 px step, 16 x 32 px). 2026-09-18: 800 -> 500
 * recovers three genuine presses at 510-700 px; at 550 one of them is lost
 * again, at 450 nothing changes. */
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
