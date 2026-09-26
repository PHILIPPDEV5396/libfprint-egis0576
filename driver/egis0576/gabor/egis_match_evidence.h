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
 * Rotation half-range and coarse step of the envelope search, deg. +-90 was
 * chosen as the full practical placement range, not measured on 2026-09-18
 * (rotations there are <= 12.5 deg); the decision below it was checked on
 * that session at +-10, +-30 and +-60 with the same impostor ceiling. */
#ifndef EV_ROT_MAX
#define EV_ROT_MAX 90.0
#endif
#ifndef EV_ROT_COARSE
#define EV_ROT_COARSE 10.0
#endif
/* Translation half-range, px (2026-09-18: two presses needed more than 25). */
#ifndef EV_SRCH
#define EV_SRCH 30
#endif
/* Coarse shift grid of the envelope, px: the envelope does not oscillate
 * with the ridge phase, so a grid coarser than the ridge period samples it. */
#ifndef EV_CSTEP
#define EV_CSTEP 3
#endif
/* Distinct envelope peaks refined and scored (2026-09-18). */
#ifndef EV_NHYP
#define EV_NHYP 4
#endif
/* A coarse envelope below this is not a hypothesis (inactive on 2026-09-18:
 * the top four peaks always exceed it). */
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
/* The front-end's own overlap floor (EG_MIN_OVERLAP in egis_match_gabor.c):
 * a pose with less masked overlap is not a hypothesis. */
#ifndef EV_MIN_OVERLAP
#define EV_MIN_OVERLAP 800
#endif

/* ---- block evidence ---------------------------------------------------------
 * The overlap is cut into EV_BS x EV_BS blocks every EV_STEP px; each block
 * searches its own shift (+-EV_RS px) for its best residual NCC; blocks at or
 * above EV_TAU whose shifts agree with a neighbour's within EV_EPS px form
 * 8-connected sets, and a set's evidence is the sum of its blocks' Fisher-z
 * excess over EV_TAU. All set on 2026-09-18. */
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

#define EV_NBX ((EM_W - EV_BS) / EV_STEP + 1)
#define EV_NBY ((EM_H - EV_BS) / EV_STEP + 1)
#define EV_NB (EV_NBX * EV_NBY)

/* A frame as the evidence stage sees it: the front-end's ridge image and
 * mask, plus the residual. ~52 kB. */
typedef struct
{
  EmFrame f;
  float   res[EM_N];       /* band-limited Gabor residual, 0 outside mask */
} EvFrame;

/* A probe: an EvFrame plus the quadrature of its ridge image (-grad, scaled
 * to the ridge image's energy over the mask), which the envelope search
 * correlates against so that its landscape does not oscillate with the ridge
 * phase. Only the probe needs it. ~84 kB. */
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

/* PORT STAGE ONLY: best refined ridge-image NCC over the hypothesis search. */
double ev_ridge_best (const EvFrame *t,
                      const EvProbe *p);
