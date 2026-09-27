/* egis_match_evidence.c -- identity evidence for the Gabor front-end
 *
 * Copyright (C) 2026 Philipp Oster
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * ---------------------------------------------------------------------------
 * WHY THIS EXISTS
 *
 * The Gabor front-end's masked NCC (egis_match_gabor.c) measures whether two
 * patches agree in ridge FLOW and ridge PERIOD, and that is not identity: on
 * the reference unit's 2026-09-18 session two different fingers of one
 * person scored up to 0.896 against a 0.78 threshold (egis_match_gabor.c,
 * LIMITS). What does tell skin apart is the fine structure the steered Gabor
 * filter discards -- ridge breaks, ridge-width and edge detail,
 * minutia-scale features. This file makes the accept decision on that alone.
 *
 * WHAT IT DOES
 *
 *   1. Pose hypotheses: a phase-invariant ENVELOPE correlation of the
 *      template's ridge image against the probe's ridge image and its
 *      quadrature, over +-EV_ROT_MAX deg (EV_ROT_COARSE steps) and
 *      +-EV_SRCH px (EV_CSTEP grid), down to EV_MIN_OVERLAP px of overlap.
 *      The EV_NHYP highest distinct peaks, plus the EV_NSIG most SIGNIFICANT
 *      ones not yet taken (envelope times the square root of the overlap it
 *      was computed over), are refined at full resolution; a hypothesis
 *      whose refined ridge-image NCC is under EV_GFLOOR is dropped. The
 *      ridge image only PROPOSES poses.
 *   2. At each hypothesis the frame is cut into blocks that tile it
 *      completely; each block with at least half its pixels in the overlap
 *      searches its own shift for the best masked NCC of the Gabor RESIDUAL
 *      (em_frame_residual), and blocks that reproduce the residual under one
 *      smoothly varying displacement form connected sets. When the best
 *      set's blocks agree on a mean shift of EV_RC_MIN px or more, the pose
 *      is moved by that shift once and the blocks search again around it
 *      (RE-CENTRING, ev_hyp_evidence).
 *   3. The evidence W of a set is the sum of its blocks' Fisher-z excess over
 *      EV_TAU; the result is the best W over sets and hypotheses, in
 *      milli-W. The adapter accepts at W >= 2 (egis_engine_cleanroom.c) and
 *      corroborates an accept by the evidence of the un-flat-fielded frame
 *      at the same pose (ev_evidence_at).
 *
 * Why the blocks shift independently: on near-parallel ridges the ridge
 * image is nearly blind to a shift along the ridges, so its best alignment
 * lands a few px off, and two presses of one finger are related by a
 * smooth non-rigid displacement that no single rigid pose fits; one rigid
 * residual NCC over the whole overlap then decorrelates (measured on
 * 2026-09-18: the right thumb reproduced its own residual at median 0.04
 * across presses at the front-end's alignment, while blocks allowed their
 * own shift reproduced it at >= 0.6 in 18.5 of 21 blocks, at median offsets
 * of 4 px that vary smoothly from block to block). Why neighbour agreement:
 * each block's local maximum over its shifts finds noise peaks too;
 * requiring the offsets of adjacent blocks to agree, and summing only over a
 * connected set, keeps those from adding up.
 *
 * What the residual does and does not rule out: it is what the ridge image
 * (one local flow and one local period) does not explain, so two fingers
 * whose flow and period merely agree leave residuals that do not correlate
 * -- on the reference unit's sessions no frame of the same person's other
 * fingers reaches the accept point against any stored frame (at most
 * 0.99 x, 2026-09-13 templates against a 2026-09-18 probe). That single
 * frame is what the pose search found, not what the statistic can reach:
 * a dense scan of that pair over every rotation (2.5 deg) and shift (2 px)
 * within +-90 deg / +-48 px finds W 2.19 (1.09 x) at a pose none of the
 * hypotheses comes near. The margin for impostors is carried by the
 * driver's rule -- two consecutive frames and the raw-frame corroboration --
 * not by single frames staying under the accept point. It is NOT
 * proof against a structured texture: a synthetic pattern whose curvature or
 * period departs from the filter's single-period model leaves a structured
 * misfit in the residual, and that misfit can correlate. Measured on
 * 2026-09-27 against this file as of commit 5eb1590 (the verifier's port of
 * the out-of-tree attack harnesses: real twelve-press templates of the
 * 2026-09-13 and 2026-09-18 sessions, three frames per artefact press with
 * the sensor's fixed pattern and noise added, the driver's rule including
 * the raw check): fixed families of synthetic ridge textures -- gratings,
 * arcs and whorls, loops and deltas, chirps, period-modulated, wavy and fine
 * textures, 27,240 artefact-template presses -- stayed at or under 0.61 x
 * the accept point with no frame over it; but a score-ORACLE hill-climb of a
 * smooth 10-parameter synthetic ridge model (curvature plus one
 * period-modulation term, no minutiae), fed the returned score, reached W
 * 2.2-2.4 (1.10-1.19 x, three frames over the accept point, raw check
 * passed) against one real template (2026-09-18, right index) -- an accept
 * under the driver's rule; its other 39 climbs peaked at 0.57 x or less. So
 * an attacker who can query the score is not closed by this decision, as he
 * was not by the correlation it replaced.
 *
 * Why the search reaches small edge overlaps: on the reference unit's
 * 2026-09-18 session three genuine presses (left index 1, 7, 8) sat at
 * 35-41 px shift with 510-700 px of masked overlap, where the ridge image
 * matched at NCC 0.97-0.98 and 6-9 blocks reproduced the residual (W up to
 * 5.9) -- and the step-3 prototype's search (+-30 px, 800 px floor, blocks
 * missing the frame's last 6 columns and last row) never scored them. W
 * needs no overlap floor of its own: a smaller overlap holds fewer blocks,
 * so a chance W shrinks with it. The floor only keeps the pose ranking
 * meaningful, at about the least area in which W can reach the accept point
 * at a genuine press's block NCC: three blocks at the 8 px step, 16 x 32 px.
 * The translation range is about the largest shift that still leaves that
 * much (a 70 x 15 px strip at the masks' typical 0.7 x 0.7 coverage).
 *
 * Why the significance proposals: an NCC over a small overlap reaches a
 * high value by chance far more easily than one over a large overlap, and
 * on near-parallel ridges the envelope's highest peaks are small edge
 * overlaps that the flow happens to fit. A genuine pose with a large
 * overlap, whose rigid correlation the skin's distortion has lowered, then
 * ranks far down. Measured on the two sessions of the reference unit,
 * enrolled on one and probed with the other (the right thumb came back
 * rotated by 50-55 deg): a dense scan over every rotation and shift found
 * W 6-14 (2026-09-18 templates, a 2026-09-13 probe of the right thumb) and
 * W 3-11 (the other way round) at poses of small shift and large overlap
 * that ranked 11th-46th among the distinct envelope peaks, or were no peak
 * at all, while E sqrt(n) ranked some of them first. With the EV_NSIG
 * proposals the weakest accepted genuine press of 2026-09-18 -> 2026-09-13
 * rises from 1.69 x to 2.89 x the accept point (1.11 x in the first
 * identity-gate build), no genuine press's level falls and no impostor
 * press's level under the driver's rule changes on any of the four sets
 * (below). They replace the orientation-field rotation proposals of that
 * first build (commit 5eb1590): added back on top of them, those change no
 * genuine press's level on any of the four sets, raise one impostor press
 * (09-18, to 0.25 x), and make each template frame 13 % slower (11.2
 * against 9.9 ms at -O2).
 *
 * Why the re-centring: on near-parallel ridges the refined pose can sit
 * 6-10 px from where the fine structure reproduces, displaced ALONG the
 * ridges, where the ridge image cannot tell; the blocks' own +-EV_RS px
 * search then ends at its border, and the set they form is short. Their
 * mean shift says where the pose should be. Measured on the hypotheses of
 * the first identity-gate build (2026-09-13 templates, 2026-09-18 probes):
 * three genuine presses were rejected for exactly this -- left index 9 (the
 * hypothesis at +33 px, the fine structure at +22..24 px: all 9 eligible
 * blocks in the set, W 1.6-1.8, every one at or next to the -6 px border of
 * its search; W 2.5-2.6 at the moved pose) and right thumb 5 and 9 -- and
 * all three are accepted with it (right thumb 5 also by the significance
 * proposals alone; left index 9 and right thumb 9 only by this). It is
 * selective:
 * over the four development sets (below) it raises the evidence of 2,821 of
 * 20,980 genuine frame pairs (1,897 by more than W 0.5) and of 194 of
 * 167,970 impostor frame pairs, 4 of them by more than W 0.5 -- one, of the
 * 2026-09-13 templates against a 2026-09-18 probe, from 0.66 to 1.98 (the
 * 0.99 x above).
 *
 * Why a block needs half its pixels, at full weight: weighting each block's
 * atanh(r) by sqrt(n / 256) and letting blocks of >= 48 px count --
 * per-pixel significance -- was measured on 2026-09-18 and not adopted: at
 * the same accept point it rejected 5 of 60 genuine presses instead of 3
 * (the strongest impostor fell from W 0.89 to 0.51, the separation between
 * the weakest accepted genuine press and the strongest impostor stayed about
 * the same), and counting such partial blocks at full weight accepted 3 of
 * 480 impostor presses.
 *
 * MEASURED (tools/accuracy of the out-of-tree repository, kit_version 4,
 * the driver's two-frame + raw rule; the reference unit's sessions
 * 2026-09-13 and 2026-09-18, one person, impostors = the same person's
 * other fingers). In session: the kit's two folds of six-press templates.
 * Across sessions: twelve-press templates of one session, genuine probes of
 * the other, impostor probes of both. IN-SAMPLE: every constant here and in
 * the adapter was set on these four sets (on 2026-09-18 alone until the
 * significance proposals and the re-centring came).
 *
 *     set        rejects  accepts  strongest impostor   weakest    vendor
 *                                  press    one frame   genuine    rejects
 *     09-18      3 / 60   0 / 480  0.44 x   0.65 x      1.41 x     4 / 60
 *     09-13      0 / 60   0 / 480  0.06 x   0.60 x      3.86 x     0 / 60
 *     13 -> 18   6 / 60   0 / 480  0.27 x   0.99 x      1.08 x     7 / 60
 *     18 -> 13   0 / 60   0 / 480  0.44 x   0.65 x      2.89 x     0 / 60
 *
 *     ("press" = the highest threshold at which the driver's rule would
 *     accept an impostor press, as a multiple of the accept point.) The
 *     first identity-gate build (commit 5eb1590) rejected 9 / 60 of 13 -> 18
 *     and accepted the weakest genuine press of 18 -> 13 at 1.11 x. Every
 *     press rejected here is rejected by the vendor matcher too. Of the
 *     three 09-18 rejects, two (left index 0, right thumb 2) show no block
 *     evidence against any stored frame at any rotation and any shift
 *     within +-42 px (W <= 0.83), and the third (right index 9) reaches W
 *     2.1-2.2 only at a rotation of 110-115 deg, outside the +-90 deg
 *     search.
 *
 * COST (egis_verify, impostor probes, early exit on, 09-18 frames, gcc,
 * 10-frame templates, one core): 10.05 ms per template frame at -O2 and
 * 11.3 ms at -O3 against the front-end's 3.71 / 4.12 ms (2.71 x / 2.74 x;
 * the first identity-gate build 9.5 / 10.9 ms). A whole impostor verify
 * takes 101 ms on average at -O2 (p95 135, max 148) against 37 ms. Of an
 * impostor pair's 9.9 ms (-O2), 4.1 go to the envelope search, 3.3 to the
 * refinement of 7.9 hypotheses, 2.5 to the block evidence (4.1 evaluations,
 * re-centring included).
 *
 * Plain C99, libm only, deterministic, no global or static state: every
 * call's scratch is malloc'd and freed before it returns.
 * ---------------------------------------------------------------------------
 */
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "egis_match_evidence.h"

#define EV_PI 3.14159265358979323846   /* M_PI is not C99 */

/* ---- sampling helpers (the front-end's bilinear arithmetic) --------------- */
static double
ev_bil_d (const double *a, double sx, double sy)
{
  int x0 = (int) floor (sx), y0 = (int) floor (sy);
  double fx = sx - x0, fy = sy - y0;

  if (sx < 0 || sx > EM_W - 1 || sy < 0 || sy > EM_H - 1)
    return 0.0;
  if (x0 >= EM_W - 1)
    {
      x0 = EM_W - 2;
      fx = 1.0;
    }
  if (y0 >= EM_H - 1)
    {
      y0 = EM_H - 2;
      fy = 1.0;
    }
  return a[y0 * EM_W + x0] * (1 - fx) * (1 - fy)
         + a[y0 * EM_W + x0 + 1] * fx * (1 - fy)
         + a[(y0 + 1) * EM_W + x0] * (1 - fx) * fy
         + a[(y0 + 1) * EM_W + x0 + 1] * fx * fy;
}

static double
ev_bil_u8 (const uint8_t *a, double sx, double sy)
{
  int x0 = (int) floor (sx), y0 = (int) floor (sy);
  double fx = sx - x0, fy = sy - y0;

  if (sx < 0 || sx > EM_W - 1 || sy < 0 || sy > EM_H - 1)
    return 0.0;
  if (x0 >= EM_W - 1)
    {
      x0 = EM_W - 2;
      fx = 1.0;
    }
  if (y0 >= EM_H - 1)
    {
      y0 = EM_H - 2;
      fy = 1.0;
    }
  return a[y0 * EM_W + x0] * (1 - fx) * (1 - fy)
         + a[y0 * EM_W + x0 + 1] * fx * (1 - fy)
         + a[(y0 + 1) * EM_W + x0] * (1 - fx) * fy
         + a[(y0 + 1) * EM_W + x0 + 1] * fx * fy;
}

/* caller guarantees 0 <= sx <= EM_W-1, 0 <= sy <= EM_H-1 */
static double
ev_bil_f (const float *a, double sx, double sy)
{
  int x0 = (int) floor (sx), y0 = (int) floor (sy);
  double fx = sx - x0, fy = sy - y0;

  if (x0 >= EM_W - 1)
    {
      x0 = EM_W - 2;
      fx = 1.0;
    }
  if (y0 >= EM_H - 1)
    {
      y0 = EM_H - 2;
      fy = 1.0;
    }
  return a[y0 * EM_W + x0] * (1 - fx) * (1 - fy) + a[y0 * EM_W + x0 + 1] * fx * (1 - fy)
         + a[(y0 + 1) * EM_W + x0] * (1 - fx) * fy + a[(y0 + 1) * EM_W + x0 + 1] * fx * fy;
}

/* ---- per-frame features ----------------------------------------------------- */
void
ev_frame_compute (const uint8_t *raw, EvFrame *F)
{
  em_frame_residual (raw, &F->f, F->res, EV_RES_SIGMA);
}

/* Quadrature of the ridge image: for a locally plane ridge wave g = A cos(k.x)
 * the Riesz transform is -grad(g)/|k|; the scale is fixed by energy balance
 * over the mask instead of by |k|. */
void
ev_probe_compute (const uint8_t *raw, EvProbe *P)
{
  const double *g = P->e.f.img;
  double eg = 0, eq = 0, sc;

  em_frame_residual (raw, &P->e.f, P->e.res, EV_RES_SIGMA);
  memset (P->q1, 0, sizeof P->q1);
  memset (P->q2, 0, sizeof P->q2);
  for (int y = 1; y < EM_H - 1; y++)
    for (int x = 1; x < EM_W - 1; x++)
      {
        int i = y * EM_W + x;
        if (!P->e.f.mask[i])
          continue;
        P->q1[i] = (float) (-(g[i + 1] - g[i - 1]) * 0.5);
        P->q2[i] = (float) (-(g[i + EM_W] - g[i - EM_W]) * 0.5);
        eg += g[i] * g[i];
        eq += (double) P->q1[i] * P->q1[i] + (double) P->q2[i] * P->q2[i];
      }
  sc = eq > 1e-12 ? sqrt (eg / eq) : 0.0;
  for (int i = 0; i < EM_N; i++)
    {
      P->q1[i] = (float) (P->q1[i] * sc);
      P->q2[i] = (float) (P->q2[i] * sc);
    }
}

/* ---- coarse hypothesis search ----------------------------------------------
 * Envelope correlation: the template's ridge image a against the probe's
 * (g, q1, q2) on an EV_CSTEP-px grid, E = sqrt(cov(a,g)^2 + cov(a,q1)^2 +
 * cov(a,q2)^2) / sqrt(var a var g). E does not change sign with the ridge
 * phase, so a grid coarser than the ridge period samples it safely; the phase
 * is recovered by the full-resolution refinement. */
#define EV_GX ((EM_W + EV_CSTEP - 1) / EV_CSTEP)   /* 24 */
#define EV_GY ((EM_H + EV_CSTEP - 1) / EV_CSTEP)   /* 19 */
#define EV_GN (EV_GX * EV_GY)
#define EV_S2 (2 * (EV_SRCH / EV_CSTEP) + 1)

typedef struct
{
  double s, rot;
  int    dx, dy;
  double q;          /* coarse significance, env * sqrt (overlap points) */
} EvHyp;
#define EV_NH (EV_NHYP + EV_NSIG)       /* most hypotheses per pair */

typedef struct
{
  float ta[EV_GN], tm[EV_GN], ta2[EV_GN];
  float pm[EV_GN], pg[EV_GN], pg2[EV_GN], p1[EV_GN], p2[EV_GN];
  float fa[EM_N], fm[EM_N], fa2[EM_N];          /* template, full resolution */
  float qa[EM_N], qm[EM_N], qa2[EM_N];          /* rotated probe, full res.  */
  float land[EV_S2 * EV_S2], landq[EV_S2 * EV_S2];
} EvWork;

static void
ev_grid_template (const EvFrame *A, EvWork *W)
{
  for (int j = 0; j < EV_GY; j++)
    for (int i = 0; i < EV_GX; i++)
      {
        int x = EV_CSTEP * i, y = EV_CSTEP * j, k = j * EV_GX + i;
        float v = 0, mk = 0;
        if (x < EM_W && y < EM_H && A->f.mask[y * EM_W + x])
          {
            v = (float) A->f.img[y * EM_W + x];
            mk = 1;
          }
        W->ta[k] = v;
        W->tm[k] = mk;
        W->ta2[k] = v * v;
      }
}

/* probe rotated by th about the centre, sampled at the grid points */
static void
ev_grid_probe (const EvProbe *B, double th, EvWork *W)
{
  double cy = (EM_H - 1) / 2.0, cx = (EM_W - 1) / 2.0, ct = cos (th), st = sin (th);

  for (int j = 0; j < EV_GY; j++)
    for (int i = 0; i < EV_GX; i++)
      {
        int k = j * EV_GX + i;
        double dx = EV_CSTEP * i - cx, dy = EV_CSTEP * j - cy;
        double sx = cx + dx * ct - dy * st, sy = cy + dx * st + dy * ct;
        W->pm[k] = W->pg[k] = W->pg2[k] = W->p1[k] = W->p2[k] = 0.0f;
        if (EV_CSTEP * i >= EM_W || EV_CSTEP * j >= EM_H ||
            sx < 0 || sx > EM_W - 1 || sy < 0 || sy > EM_H - 1)
          continue;
        if (ev_bil_u8 (B->e.f.mask, sx, sy) <= 0.6)
          continue;
        W->pm[k] = 1.0f;
        W->pg[k] = (float) ev_bil_d (B->e.f.img, sx, sy);
        W->pg2[k] = W->pg[k] * W->pg[k];
        W->p1[k] = (float) ev_bil_f (B->q1, sx, sy);
        W->p2[k] = (float) ev_bil_f (B->q2, sx, sy);
      }
}

/* template grid point (i, j) <-> probe grid point (i - dx/step, j - dy/step) */
static double
ev_grid_env (const EvWork *W, int dx, int dy, double *npts)
{
  int ox = -dx / EV_CSTEP, oy = -dy / EV_CSTEP;
  int j0 = oy < 0 ? -oy : 0, j1 = EV_GY - (oy > 0 ? oy : 0);
  int i0 = ox < 0 ? -ox : 0, i1 = EV_GX - (ox > 0 ? ox : 0);
  double n = 0, sa = 0, sg = 0, saa = 0, sgg = 0, sag = 0, s1 = 0, sa1 = 0, s2 = 0, sa2 = 0;
  double va, vg, c0, c1, c2;

  for (int j = j0; j < j1; j++)
    {
      /* row pointers at the first element used (column i0), so none points
       * before its array's start; i below counts from there */
      int t0 = j * EV_GX + i0, o = (j + oy) * EV_GX + ox + i0, ni = i1 - i0;
      const float *ta = W->ta + t0, *tm = W->tm + t0, *ta2 = W->ta2 + t0;
      const float *pm = W->pm + o, *pg = W->pg + o, *pg2 = W->pg2 + o, *p1 = W->p1 + o, *p2 = W->p2 + o;
      float L[10][4] = { { 0 } };
      float rn, ra, rg, raa, rgg, rag, r1, ra1, r2, ra2;
      int i = 0;
      /* four interleaved partial sums per quantity, summed in a fixed order:
       * the lanes are independent, so the compiler can keep them in vector
       * registers (a third of this search's time at -O2) */
      for (; i + 4 <= ni; i += 4)
        for (int l = 0; l < 4; l++)
          {
            float a = ta[i + l], m = tm[i + l], q = pm[i + l], g = pg[i + l];
            L[0][l] += m * q;
            L[1][l] += a * q;
            L[2][l] += ta2[i + l] * q;
            L[3][l] += m * g;
            L[4][l] += m * pg2[i + l];
            L[5][l] += a * g;
            L[6][l] += m * p1[i + l];
            L[7][l] += a * p1[i + l];
            L[8][l] += m * p2[i + l];
            L[9][l] += a * p2[i + l];
          }
      for (; i < ni; i++)
        {
          float a = ta[i], m = tm[i], q = pm[i], g = pg[i];
          L[0][0] += m * q;
          L[1][0] += a * q;
          L[2][0] += ta2[i] * q;
          L[3][0] += m * g;
          L[4][0] += m * pg2[i];
          L[5][0] += a * g;
          L[6][0] += m * p1[i];
          L[7][0] += a * p1[i];
          L[8][0] += m * p2[i];
          L[9][0] += a * p2[i];
        }
      rn = (L[0][0] + L[0][1]) + (L[0][2] + L[0][3]);
      ra = (L[1][0] + L[1][1]) + (L[1][2] + L[1][3]);
      raa = (L[2][0] + L[2][1]) + (L[2][2] + L[2][3]);
      rg = (L[3][0] + L[3][1]) + (L[3][2] + L[3][3]);
      rgg = (L[4][0] + L[4][1]) + (L[4][2] + L[4][3]);
      rag = (L[5][0] + L[5][1]) + (L[5][2] + L[5][3]);
      r1 = (L[6][0] + L[6][1]) + (L[6][2] + L[6][3]);
      ra1 = (L[7][0] + L[7][1]) + (L[7][2] + L[7][3]);
      r2 = (L[8][0] + L[8][1]) + (L[8][2] + L[8][3]);
      ra2 = (L[9][0] + L[9][1]) + (L[9][2] + L[9][3]);
      n += rn;
      sa += ra;
      saa += raa;
      sg += rg;
      sgg += rgg;
      sag += rag;
      s1 += r1;
      sa1 += ra1;
      s2 += r2;
      sa2 += ra2;
    }
  *npts = n;
  if (n < 1 || n * EV_CSTEP * EV_CSTEP < EV_MIN_OVERLAP)
    return -1.0;
  va = saa - sa * sa / n;
  vg = sgg - sg * sg / n;
  if (va <= 1e-9 || vg <= 1e-9)
    return -1.0;
  c0 = sag - sa * sg / n;
  c1 = sa1 - sa * s1 / n;
  c2 = sa2 - sa * s2 / n;
  return sqrt (c0 * c0 + c1 * c1 + c2 * c2) / sqrt (va * vg);
}

/* full-resolution masked NCC of the ridge images, float, contiguous rows:
 * template (x,y) <-> rotated probe (x - dx, y - dy) */
static void
ev_full_template (const EvFrame *A, EvWork *W)
{
  for (int i = 0; i < EM_N; i++)
    {
      float m = A->f.mask[i] ? 1.0f : 0.0f;
      W->fa[i] = (float) A->f.img[i] * m;
      W->fm[i] = m;
      W->fa2[i] = W->fa[i] * W->fa[i];
    }
}

static void
ev_full_probe (const EvProbe *B, double th, EvWork *W)
{
  double cy = (EM_H - 1) / 2.0, cx = (EM_W - 1) / 2.0, ct = cos (th), st = sin (th);

  for (int y = 0; y < EM_H; y++)
    for (int x = 0; x < EM_W; x++)
      {
        int i = y * EM_W + x;
        double dx = x - cx, dy = y - cy;
        double sx = cx + dx * ct - dy * st, sy = cy + dx * st + dy * ct;
        W->qa[i] = W->qm[i] = W->qa2[i] = 0.0f;
        if (sx < 0 || sx > EM_W - 1 || sy < 0 || sy > EM_H - 1)
          continue;
        if (ev_bil_u8 (B->e.f.mask, sx, sy) > 0.6)
          {
            float v = (float) ev_bil_d (B->e.f.img, sx, sy);
            W->qa[i] = v;
            W->qm[i] = 1.0f;
            W->qa2[i] = v * v;
          }
      }
}

static double
ev_full_ncc (const EvWork *W, int dx, int dy)
{
  int ya = dy > 0 ? dy : 0, yb = EM_H + (dy < 0 ? dy : 0);
  int xa = dx > 0 ? dx : 0, xb = EM_W + (dx < 0 ? dx : 0);
  double n = 0, sa = 0, sb = 0, saa = 0, sbb = 0, sab = 0, cov, va, vb;

  for (int y = ya; y < yb; y++)
    {
      /* row pointers at the first pixel used (column xa), none before its
       * array's start */
      int t0 = y * EM_W + xa, o = (y - dy) * EM_W - dx + xa, nx = xb - xa;
      const float *ta = W->fa + t0, *tm = W->fm + t0, *ta2 = W->fa2 + t0;
      const float *pa = W->qa + o, *pm = W->qm + o, *pa2 = W->qa2 + o;
      float rn = 0, rsa = 0, rsb = 0, rsaa = 0, rsbb = 0, rsab = 0;
      for (int x = 0; x < nx; x++)
        {
          rn += tm[x] * pm[x];
          rsa += ta[x] * pm[x];
          rsb += tm[x] * pa[x];
          rsaa += ta2[x] * pm[x];
          rsbb += tm[x] * pa2[x];
          rsab += ta[x] * pa[x];
        }
      n += rn;
      sa += rsa;
      sb += rsb;
      saa += rsaa;
      sbb += rsbb;
      sab += rsab;
    }
  if (n < 1 || n < EV_MIN_OVERLAP)
    return -1.0;
  cov = sab - sa * sb / n;
  va = saa - sa * sa / n;
  vb = sbb - sb * sb / n;
  if (va <= 0 || vb <= 0)
    return -1.0;
  return cov / (sqrt (va * vb) + 1e-6);
}

/* Insert c into list[] (at most ncap, *n used), kept sorted descending by
 * the key selected: envelope (s) or significance (q). */
static void
ev_cand_insert (EvHyp *list, int *n, int ncap, EvHyp c, int by_q)
{
  double v = by_q ? c.q : c.s;
  int k;

  if (*n >= ncap && v <= (by_q ? list[*n - 1].q : list[*n - 1].s))
    return;
  k = *n < ncap ? (*n)++ : ncap - 1;
  while (k > 0 && (by_q ? list[k - 1].q : list[k - 1].s) < v)
    {
      list[k] = list[k - 1];
      k--;
    }
  list[k] = c;
}

/* The envelope landscape at one rotation: its local maxima (3x3 on the
 * coarse grid) above EV_HYP_MIN go into cand[], kept sorted by envelope,
 * descending, at most ncap, and into candq[] sorted by significance (below).
 * Returns 1 if any shift reached the overlap floor. */
static int
ev_land_peaks (const EvProbe *B, double rot, EvWork *W, EvHyp *cand, int *nc,
               EvHyp *candq, int *nq, int ncap)
{
  int any = 0;

  ev_grid_probe (B, rot * EV_PI / 180.0, W);
  for (int y = 0; y < EV_S2; y++)
    for (int x = 0; x < EV_S2; x++)
      {
        double npts = 0;
        double v = ev_grid_env (W, (x - EV_S2 / 2) * EV_CSTEP, (y - EV_S2 / 2) * EV_CSTEP, &npts);
        W->land[y * EV_S2 + x] = (float) v;
        /* significance of the envelope: E^2 is about chi^2 with 3 degrees
         * of freedom over npts under no relation, so E sqrt(npts) ranks
         * peaks by how unlikely they are, not by how high a small overlap
         * can push a correlation */
        W->landq[y * EV_S2 + x] = (float) (v > 0 ? v * sqrt (npts) : 0.0);
        if (v > -1.0)
          any = 1;
      }
  for (int y = 0; y < EV_S2; y++)
    for (int x = 0; x < EV_S2; x++)
      {
        float v = W->land[y * EV_S2 + x];
        int lm = v > EV_HYP_MIN;
        for (int j = -1; j <= 1 && lm; j++)
          for (int i = -1; i <= 1 && lm; i++)
            {
              int yy = y + j, xx = x + i;
              if ((i || j) && yy >= 0 && yy < EV_S2 && xx >= 0 && xx < EV_S2 &&
                  W->land[yy * EV_S2 + xx] > v)
                lm = 0;
            }
        if (!lm)
          continue;
        {
          EvHyp c = { v, rot, (x - EV_S2 / 2) * EV_CSTEP, (y - EV_S2 / 2) * EV_CSTEP,
                      W->landq[y * EV_S2 + x] };
          ev_cand_insert (cand, nc, ncap, c, 0);
          ev_cand_insert (candq, nq, ncap, c, 1);
        }
      }
  return any;
}

static int
ev_distinct (const EvHyp *c, const EvHyp *H, int nh)
{
  for (int k = 0; k < nh; k++)
    if (fabs (c->rot - H[k].rot) <= EV_ROT_COARSE / 2.0 &&
        hypot (c->dx - H[k].dx, c->dy - H[k].dy) <= EV_CSTEP + 0.5)
      return 0;
  return 1;
}

/* The best distinct hypotheses -- the EV_NHYP highest envelope peaks over the
 * coarse rotation grid, plus the EV_NSIG most significant ones not yet taken
 * -- refined at full resolution and sorted by refined NCC, descending. H must
 * hold EV_NH. Returns how many, or -1 if no alignment reaches the overlap
 * floor at all. */
static int
ev_hyps (const EvFrame *A, const EvProbe *B, EvHyp *H, EvWork *W)
{
  enum { NCAND = 64 };
  EvHyp cand[NCAND], candq[NCAND];
  int nc = 0, nq = 0, any = 0, nr = EV_ROT_MAX / EV_ROT_COARSE, nh = 0;

  ev_grid_template (A, W);
  for (int r = -nr; r <= nr; r++)
    any |= ev_land_peaks (B, r * EV_ROT_COARSE, W, cand, &nc, candq, &nq, NCAND);
  if (!any)
    return -1;
  /* distinct: another candidate within half a coarse rotation step AND one
   * grid step is the same peak */
  for (int c = 0; c < nc && nh < EV_NHYP; c++)
    if (ev_distinct (&cand[c], H, nh))
      H[nh++] = cand[c];
#if EV_NSIG > 0
  /* the most significant envelope peaks not yet taken, whatever their
   * envelope rank */
  {
    int added = 0;
    for (int c = 0; c < nq && added < EV_NSIG; c++)
      if (ev_distinct (&candq[c], H, nh))
        {
          H[nh++] = candq[c];
          added++;
        }
  }
#endif
  /* refine at full resolution: +-EV_REF px at the coarse rotation, then the
   * rotations EV_REF_ROT deg apart within half a coarse step either side, at
   * +-1 px around that shift */
  ev_full_template (A, W);
  for (int k = 0; k < nh; k++)
    {
      EvHyp b = H[k];
      int cx0, cy0;
      b.s = -2;
      ev_full_probe (B, H[k].rot * EV_PI / 180.0, W);
      for (int dy = H[k].dy - EV_REF; dy <= H[k].dy + EV_REF; dy++)
        for (int dx = H[k].dx - EV_REF; dx <= H[k].dx + EV_REF; dx++)
          {
            double v = ev_full_ncc (W, dx, dy);
            if (v > b.s)
              {
                b.s = v;
                b.dx = dx;
                b.dy = dy;
              }
          }
      cx0 = b.dx;
      cy0 = b.dy;
      for (int st = 1; st * EV_REF_ROT <= EV_ROT_COARSE / 2.0 + 1e-6; st++)
        for (int sg = -1; sg <= 1; sg += 2)
          {
            double rot = H[k].rot + sg * st * EV_REF_ROT;
            ev_full_probe (B, rot * EV_PI / 180.0, W);
            for (int dy = cy0 - 1; dy <= cy0 + 1; dy++)
              for (int dx = cx0 - 1; dx <= cx0 + 1; dx++)
                {
                  double v = ev_full_ncc (W, dx, dy);
                  if (v > b.s)
                    {
                      b.s = v;
                      b.rot = rot;
                      b.dx = dx;
                      b.dy = dy;
                    }
                }
          }
      H[k] = b;
    }
  /* sort by refined NCC, drop exact duplicates */
  for (int i = 1; i < nh; i++)
    {
      EvHyp x = H[i];
      int j = i;
      while (j > 0 && H[j - 1].s < x.s)
        {
          H[j] = H[j - 1];
          j--;
        }
      H[j] = x;
    }
  {
    int m = 0;
    for (int i = 0; i < nh; i++)
      {
        int dup = 0;
        for (int k = 0; k < m && !dup; k++)
          if (H[k].rot == H[i].rot && H[k].dx == H[i].dx && H[k].dy == H[i].dy)
            dup = 1;
        if (!dup)
          H[m++] = H[i];
      }
    nh = m;
  }
  return nh;
}

/* ---- block residual evidence ------------------------------------------------ */
typedef struct
{
  float ta[EM_N], tm[EM_N], ta2[EM_N];          /* template residual          */
  float pa[EM_N], pm[EM_N], pa2[EM_N];          /* probe residual, rotated    */
} EvEvWork;

static void
ev_template_res (const EvFrame *A, EvEvWork *E)
{
  for (int i = 0; i < EM_N; i++)
    {
      E->ta[i] = A->res[i];
      E->tm[i] = A->f.mask[i] ? 1.0f : 0.0f;
      E->ta2[i] = A->res[i] * A->res[i];
    }
}

static void
ev_rotate_res (const EvProbe *B, double th, EvEvWork *E)
{
  double cy = (EM_H - 1) / 2.0, cx = (EM_W - 1) / 2.0, ct = cos (th), st = sin (th);

  for (int y = 0; y < EM_H; y++)
    for (int x = 0; x < EM_W; x++)
      {
        int i = y * EM_W + x;
        double dx = x - cx, dy = y - cy;
        double sx = cx + dx * ct - dy * st, sy = cy + dx * st + dy * ct;
        E->pa[i] = E->pm[i] = E->pa2[i] = 0.0f;
        if (sx < 0 || sx > EM_W - 1 || sy < 0 || sy > EM_H - 1)
          continue;
        if (ev_bil_u8 (B->e.f.mask, sx, sy) > 0.6)
          {
            double v = ev_bil_f (B->e.res, sx, sy);
            E->pa[i] = (float) v;
            E->pm[i] = 1.0f;
            E->pa2[i] = (float) (v * v);
          }
      }
}

/* masked NCC of template block [x0,x0+BS)x[y0,y0+BS) against the probe
 * shifted by (sx, sy): template (x,y) <-> probe (x - sx, y - sy); *on = the
 * masked pixel count it was computed over. NAN if under 20 px or flat. */
static double
ev_block_ncc (const EvEvWork *E, int x0, int y0, int sx, int sy, int *on)
{
  int xa = x0, xb = x0 + EV_BS, ya = y0, yb = y0 + EV_BS;
  double n = 0, sa = 0, sb = 0, saa = 0, sbb = 0, sab = 0, cov, va, vb;

  if (xa < sx)
    xa = sx;
  if (xb > EM_W + sx)
    xb = EM_W + sx;
  if (ya < sy)
    ya = sy;
  if (yb > EM_H + sy)
    yb = EM_H + sy;
  if (xb > EM_W)
    xb = EM_W;
  if (yb > EM_H)
    yb = EM_H;
  *on = 0;
  if (xa >= xb || ya >= yb)
    return NAN;
  for (int y = ya; y < yb; y++)
    {
      /* row pointers at the first pixel used (column xa), none before its
       * array's start */
      int t0 = y * EM_W + xa, off = (y - sy) * EM_W - sx + xa, nx = xb - xa;
      const float *ta = E->ta + t0, *tm = E->tm + t0, *ta2 = E->ta2 + t0;
      const float *pa = E->pa + off, *pm = E->pm + off, *pa2 = E->pa2 + off;
      float rn = 0, rsa = 0, rsb = 0, rsaa = 0, rsbb = 0, rsab = 0;
      for (int x = 0; x < nx; x++)
        {
          rn += tm[x] * pm[x];
          rsa += ta[x] * pm[x];
          rsb += tm[x] * pa[x];
          rsaa += ta2[x] * pm[x];
          rsbb += tm[x] * pa2[x];
          rsab += ta[x] * pa[x];
        }
      n += rn;
      sa += rsa;
      sb += rsb;
      saa += rsaa;
      sbb += rsbb;
      sab += rsab;
    }
  *on = (int) (n + 0.5);
  if (n < 20)
    return NAN;
  cov = sab - sa * sb / n;
  va = saa - sa * sa / n;
  vb = sbb - sb * sb / n;
  if (va <= 1e-12 || vb <= 1e-12)
    return NAN;
  return cov / sqrt (va * vb);
}

/* The evidence W at one pose (the probe residual already rotated into E):
 * every block that has at least half its pixels in the overlap searches its
 * own shift; the best 8-connected set of blocks at or above EV_TAU whose
 * shifts agree with a neighbour's within EV_EPS px scores the sum of
 * atanh(best) - atanh(EV_TAU) over its blocks. */
typedef struct
{
  double mx, my;     /* mean block shift of the best set, px */
  int    n;          /* its blocks */
} EvSet;

static double
ev_evidence (const EvEvWork *E, int dx, int dy, EvSet *set)
{
  double best[EV_NB];
  int ax[EV_NB], ay[EV_NB], ok[EV_NB], lab[EV_NB], stack[EV_NB];
  int need = (EV_BS * EV_BS) / 2;
  double wbest = 0;
  EvSet bs = { 0.0, 0.0, 0 };

  for (int j = 0; j < EV_NBY; j++)
    for (int i = 0; i < EV_NBX; i++)
      {
        int k = j * EV_NBX + i, on;
        int x0 = i * EV_STEP < EM_W - EV_BS ? i * EV_STEP : EM_W - EV_BS;
        int y0 = j * EV_STEP < EM_H - EV_BS ? j * EV_STEP : EM_H - EV_BS;
        ok[k] = 0;
        best[k] = -2;
        ax[k] = ay[k] = 0;
        ev_block_ncc (E, x0, y0, dx, dy, &on);
        if (on < need)
          continue;
        for (int yy = -EV_RS; yy <= EV_RS; yy++)
          for (int xx = -EV_RS; xx <= EV_RS; xx++)
            {
              int on2;
              double v = ev_block_ncc (E, x0, y0, dx + xx, dy + yy, &on2);
              if (on2 >= need && v == v && v > best[k])
                {
                  best[k] = v;
                  ax[k] = xx;
                  ay[k] = yy;
                }
            }
        ok[k] = 1;
      }
  for (int k = 0; k < EV_NB; k++)
    lab[k] = -1;
  for (int k0 = 0; k0 < EV_NB; k0++)
    {
      int sp = 0, cnt = 0, sx = 0, sy = 0;
      double w = 0;
      if (!ok[k0] || best[k0] < EV_TAU || lab[k0] >= 0)
        continue;
      lab[k0] = k0;
      stack[sp++] = k0;
      while (sp)
        {
          int k = stack[--sp], y = k / EV_NBX, x = k % EV_NBX;
          w += atanh (best[k] < 0.999 ? best[k] : 0.999) - atanh (EV_TAU);
          cnt++;
          sx += ax[k];
          sy += ay[k];
          for (int dj = -1; dj <= 1; dj++)
            for (int di = -1; di <= 1; di++)
              {
                int yy = y + dj, xx = x + di, kk;
                if ((!di && !dj) || yy < 0 || yy >= EV_NBY || xx < 0 || xx >= EV_NBX)
                  continue;
                kk = yy * EV_NBX + xx;
                if (!ok[kk] || best[kk] < EV_TAU || lab[kk] >= 0)
                  continue;
                if (hypot (ax[kk] - ax[k], ay[kk] - ay[k]) <= EV_EPS)
                  {
                    lab[kk] = k0;
                    stack[sp++] = kk;
                  }
              }
        }
      if (w > wbest)
        {
          wbest = w;
          bs.n = cnt;
          bs.mx = (double) sx / cnt;
          bs.my = (double) sy / cnt;
        }
    }
  if (set)
    *set = bs;
  return wbest;
}

/* The evidence at a hypothesis pose, re-centred once: when the best set's
 * blocks agree on a mean shift of EV_RC_MIN px or more (and it has at least
 * EV_RC_N blocks), the evidence is looked for again with the pose moved by
 * that shift, and the higher of the two is kept. *odx, *ody receive the pose
 * it was found at. */
static double
ev_hyp_evidence (const EvEvWork *E, int dx, int dy, int *odx, int *ody)
{
  EvSet st;
  double w = ev_evidence (E, dx, dy, &st);

  *odx = dx;
  *ody = dy;
#if EV_RC_N > 0
  if (st.n >= EV_RC_N && hypot (st.mx, st.my) >= EV_RC_MIN)
    {
      int rx = dx + (int) lround (st.mx), ry = dy + (int) lround (st.my);
      double w2 = ev_evidence (E, rx, ry, NULL);
      if (w2 > w)
        {
          w = w2;
          *odx = rx;
          *ody = ry;
        }
    }
#endif
  return w;
}

/* ---- public ------------------------------------------------------------------ */
int
ev_match (const EvFrame *t, const EvProbe *p, int stop_at, EvPose *pose)
{
  EvWork *W = malloc (sizeof *W);
  EvEvWork *E = malloc (sizeof *E);
  EvHyp H[EV_NH];
  EvPose bp = { 0.0, 0, 0, 0 };
  int nh, best = -1;
  double wmax = 0;

  if (!W || !E)
    {
      free (W);
      free (E);
      if (pose)
        *pose = bp;
      return -1;
    }
  nh = ev_hyps (t, p, H, W);
  if (nh >= 0)
    {
      best = 0;
      ev_template_res (t, E);
      for (int k = 0; k < nh; k++)
        {
          int ev, odx, ody;
          if (H[k].s < EV_GFLOOR)
            break;                               /* sorted: the rest are lower */
          ev_rotate_res (p, H[k].rot * EV_PI / 180.0, E);
          {
            double w = ev_hyp_evidence (E, H[k].dx, H[k].dy, &odx, &ody);
            if (w > wmax)
              wmax = w;
          }
          ev = (int) lround (wmax * 1000.0);
          if (ev > best || !bp.valid)
            {
              if (ev > best)
                best = ev;
              bp.rot_deg = H[k].rot;
              bp.dx = odx;
              bp.dy = ody;
              bp.valid = 1;
            }
          if (stop_at > 0 && best >= stop_at)
            break;
        }
    }
  free (W);
  free (E);
  if (pose)
    *pose = bp;
  return best;
}

int
ev_evidence_at (const EvFrame *t, const EvProbe *p, const EvPose *pose)
{
  EvEvWork *E;
  double w;

  if (!pose || !pose->valid)
    return -1;
  E = malloc (sizeof *E);
  if (!E)
    return -1;
  ev_template_res (t, E);
  ev_rotate_res (p, pose->rot_deg * EV_PI / 180.0, E);
  w = ev_evidence (E, pose->dx, pose->dy, NULL);
  free (E);
  return (int) lround (w * 1000.0);
}
