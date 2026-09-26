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
 *      The EV_NHYP best distinct peaks, plus the best peak at each of the
 *      EV_NOF rotations the ORIENTATION FIELD ranks highest, are refined at
 *      full resolution; a hypothesis whose refined ridge-image NCC is under
 *      EV_GFLOOR is dropped. The ridge image only PROPOSES poses.
 *   2. At each hypothesis the frame is cut into blocks that tile it
 *      completely; each block with at least half its pixels in the overlap
 *      searches its own shift for the best masked NCC of the Gabor RESIDUAL
 *      (em_frame_residual), and blocks that reproduce the residual under one
 *      smoothly varying displacement form connected sets.
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
 * Why a flow coincidence cannot produce it: the residual is by construction
 * what the ridge image (flow and period) does not explain, so two fingers
 * with matching flow and period have uncorrelated residuals.
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
 * Why the orientation-field proposals: on near-parallel ridges the envelope
 * peaks of a genuine pair are broad and several can outrank the true pose;
 * the orientation field's rotation profile depends on how the flow turns
 * across the overlap, not on the ridge phase, and so ranks rotations
 * independently. Measured on 2026-09-18: about one extra hypothesis per
 * pair; 52 genuine frame pairs reach W 2 that did not, no impostor pair
 * gains above W 0.81, and no press decision changes on that session.
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
 * the driver's two-frame + raw rule; reference unit, session 2026-09-18,
 * one person, impostors = the same person's other fingers). IN-SAMPLE:
 * every constant here and in the adapter was set on this session.
 *
 *     60 genuine / 480 impostor presses: 3 / 60 rejects, 0 / 480 accepts;
 *     strongest impostor 0.45 x the accept point (press maximum 0.62 x),
 *     weakest accepted genuine 1.38 x; the vendor matcher on the same
 *     frames 4 / 60 and 0 / 480. Of the three rejects, two (left index 0,
 *     right thumb 2) show no block evidence against any stored frame at any
 *     rotation and any shift within +-42 px (W <= 0.83), and the third
 *     (right index 9) reaches W 2.1-2.2 only at a rotation of 110-115 deg,
 *     outside the +-90 deg search; the vendor matcher rejects all three.
 *
 * COST (egis_verify, impostor probes, early exit on, DEV frames, gcc,
 * 10-frame templates): 10.3 ms per template frame at -O2 and 12.0 ms at -O3
 * against the front-end's 4.0 / 4.5 ms (2.6-2.7x). About half of it is the
 * envelope search, a quarter the refinement, a sixth the block evidence.
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
static void
ev_lattice (EvFrame *F, const float *ofr, const float *ofi)
{
  for (int j = 0; j < EV_OLY; j++)
    for (int i = 0; i < EV_OLX; i++)
      {
        int q = (j * EV_OF_LAT) * EM_W + i * EV_OF_LAT;
        F->olr[j * EV_OLX + i] = ofr[q];
        F->oli[j * EV_OLX + i] = ofi[q];
      }
}

void
ev_frame_compute (const uint8_t *raw, EvFrame *F)
{
  float *of = malloc (2 * EM_N * sizeof (float));

  em_frame_residual (raw, &F->f, F->res, EV_RES_SIGMA, of, of ? of + EM_N : NULL);
  if (of)
    ev_lattice (F, of, of + EM_N);
  else
    {
      memset (F->olr, 0, sizeof F->olr);
      memset (F->oli, 0, sizeof F->oli);
    }
  free (of);
}

/* Quadrature of the ridge image: for a locally plane ridge wave g = A cos(k.x)
 * the Riesz transform is -grad(g)/|k|; the scale is fixed by energy balance
 * over the mask instead of by |k|. */
void
ev_probe_compute (const uint8_t *raw, EvProbe *P)
{
  const double *g = P->e.f.img;
  double eg = 0, eq = 0, sc;

  em_frame_residual (raw, &P->e.f, P->e.res, EV_RES_SIGMA, P->ofr, P->ofi);
  ev_lattice (&P->e, P->ofr, P->ofi);
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
  int dx, dy;
} EvHyp;

typedef struct
{
  float ta[EV_GN], tm[EV_GN], ta2[EV_GN];
  float pm[EV_GN], pg[EV_GN], pg2[EV_GN], p1[EV_GN], p2[EV_GN];
  float fa[EM_N], fm[EM_N], fa2[EM_N];          /* template, full resolution */
  float qa[EM_N], qm[EM_N], qa2[EM_N];          /* rotated probe, full res.  */
  float land[EV_S2 * EV_S2];
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
ev_grid_env (const EvWork *W, int dx, int dy)
{
  int ox = -dx / EV_CSTEP, oy = -dy / EV_CSTEP;
  int j0 = oy < 0 ? -oy : 0, j1 = EV_GY - (oy > 0 ? oy : 0);
  int i0 = ox < 0 ? -ox : 0, i1 = EV_GX - (ox > 0 ? ox : 0);
  double n = 0, sa = 0, sg = 0, saa = 0, sgg = 0, sag = 0, s1 = 0, sa1 = 0, s2 = 0, sa2 = 0;
  double va, vg, c0, c1, c2;

  for (int j = j0; j < j1; j++)
    {
      const float *ta = W->ta + j * EV_GX, *tm = W->tm + j * EV_GX, *ta2 = W->ta2 + j * EV_GX;
      int o = (j + oy) * EV_GX + ox;
      const float *pm = W->pm + o, *pg = W->pg + o, *pg2 = W->pg2 + o, *p1 = W->p1 + o, *p2 = W->p2 + o;
      float L[10][4] = { { 0 } };
      float rn, ra, rg, raa, rgg, rag, r1, ra1, r2, ra2;
      int i = i0;
      /* four interleaved partial sums per quantity, summed in a fixed order:
       * the lanes are independent, so the compiler can keep them in vector
       * registers (a third of this search's time at -O2) */
      for (; i + 4 <= i1; i += 4)
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
      for (; i < i1; i++)
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
      const float *ta = W->fa + y * EM_W, *tm = W->fm + y * EM_W, *ta2 = W->fa2 + y * EM_W;
      int o = (y - dy) * EM_W - dx;
      const float *pa = W->qa + o, *pm = W->qm + o, *pa2 = W->qa2 + o;
      float rn = 0, rsa = 0, rsb = 0, rsaa = 0, rsbb = 0, rsab = 0;
      for (int x = xa; x < xb; x++)
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

/* The envelope landscape at one rotation: its local maxima (3x3 on the
 * coarse grid) above EV_HYP_MIN go into cand[], kept sorted by envelope,
 * descending, at most ncap; *rbest receives the highest of them (s = -2 if
 * none). Returns 1 if any shift reached the overlap floor. */
static int
ev_land_peaks (const EvProbe *B, double rot, EvWork *W, EvHyp *cand, int *nc, int ncap, EvHyp *rbest)
{
  int any = 0;

  rbest->s = -2.0;

  ev_grid_probe (B, rot * EV_PI / 180.0, W);
  for (int y = 0; y < EV_S2; y++)
    for (int x = 0; x < EV_S2; x++)
      {
        double v = ev_grid_env (W, (x - EV_S2 / 2) * EV_CSTEP, (y - EV_S2 / 2) * EV_CSTEP);
        W->land[y * EV_S2 + x] = (float) v;
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
        if (v > rbest->s)
          *rbest = (EvHyp) { v, rot, (x - EV_S2 / 2) * EV_CSTEP, (y - EV_S2 / 2) * EV_CSTEP };
        if (*nc < ncap || v > cand[*nc - 1].s)
          {
            int k = *nc < ncap ? (*nc)++ : ncap - 1;
            while (k > 0 && cand[k - 1].s < v)
              {
                cand[k] = cand[k - 1];
                k--;
              }
            cand[k] = (EvHyp) { v, rot, (x - EV_S2 / 2) * EV_CSTEP, (y - EV_S2 / 2) * EV_CSTEP };
          }
      }
  return any;
}

#if EV_NOF > 0
/* Rotation proposals from the orientation field: the coherence-weighted
 * doubled ridge angle z = coh e^{2i theta} of the template, on an
 * EV_OF_LAT-px lattice, against the probe's rotated by phi (positions
 * rotated, phase turned by e^{-2i phi}), maximised over lattice shifts within
 * +-EV_SRCH: P(phi) = max sum Re(zT conj zP) / sum |zT||zP|. It depends on how
 * the ridge FLOW turns across the overlap, not on the ridge phase or period,
 * so it ranks rotations independently of the envelope. Evaluated on the
 * envelope's own coarse rotations (an orientation field cannot tell phi from
 * phi + 180, so -EV_ROT_MAX stands for +EV_ROT_MAX too); the EV_NOF highest
 * peaks at least EV_OF_SEP deg apart, as rotation indices -nr .. nr - 1.
 * Returns how many (<= EV_NOF). */
static int
ev_of_rots (const EvFrame *A, const EvProbe *B, int nr, int *out)
{
  enum { SL = EV_SRCH / EV_OF_LAT };
  int na = 2 * nr, nout = 0;
  double prof[2 * (EV_ROT_MAX / EV_ROT_COARSE)];
  float pr[EV_OLN], pi[EV_OLN], pm[EV_OLN], tm[EV_OLN];
  double cy = (EM_H - 1) / 2.0, cx = (EM_W - 1) / 2.0;
  int minn = EV_MIN_OVERLAP / (EV_OF_LAT * EV_OF_LAT);

  for (int k = 0; k < EV_OLN; k++)
    tm[k] = (float) sqrt ((double) A->olr[k] * A->olr[k] + (double) A->oli[k] * A->oli[k]);
  for (int a = 0; a < na; a++)
    {
      double th = (a - nr) * EV_ROT_COARSE * EV_PI / 180.0;
      double ct = cos (th), st = sin (th), c2 = cos (-2 * th), s2 = sin (-2 * th);
      double best = -2.0;
      for (int j = 0; j < EV_OLY; j++)
        for (int i = 0; i < EV_OLX; i++)
          {
            int k = j * EV_OLX + i, ix, iy, q;
            double ddx = i * EV_OF_LAT - cx, ddy = j * EV_OF_LAT - cy;
            ix = (int) lround (cx + ddx * ct - ddy * st);
            iy = (int) lround (cy + ddx * st + ddy * ct);
            pr[k] = pi[k] = pm[k] = 0.0f;
            if (ix < 0 || ix >= EM_W || iy < 0 || iy >= EM_H || !B->e.f.mask[iy * EM_W + ix])
              continue;
            q = iy * EM_W + ix;
            pr[k] = (float) (B->ofr[q] * c2 - B->ofi[q] * s2);
            pi[k] = (float) (B->ofr[q] * s2 + B->ofi[q] * c2);
            pm[k] = (float) sqrt ((double) B->ofr[q] * B->ofr[q] + (double) B->ofi[q] * B->ofi[q]);
          }
      for (int sy = -SL; sy <= SL; sy++)
        for (int sx = -SL; sx <= SL; sx++)
          {
            double num = 0, den = 0;
            int n = 0;
            for (int j = 0; j < EV_OLY; j++)
              {
                int jj = j - sy;
                if (jj < 0 || jj >= EV_OLY)
                  continue;
                for (int i = 0; i < EV_OLX; i++)
                  {
                    int ii = i - sx, k = j * EV_OLX + i, kk;
                    if (ii < 0 || ii >= EV_OLX || tm[k] <= 0)
                      continue;
                    kk = jj * EV_OLX + ii;
                    if (pm[kk] <= 0)
                      continue;
                    num += A->olr[k] * pr[kk] + A->oli[k] * pi[kk];
                    den += tm[k] * pm[kk];
                    n++;
                  }
              }
            if (n >= minn && den > 0 && num / den > best)
              best = num / den;
          }
      prof[a] = best;
    }
  /* circular local maxima (the profile has period 180 deg), highest first */
  while (nout < EV_NOF)
    {
      int bi = -1;
      for (int a = 0; a < na; a++)
        {
          double v = prof[a];
          int ok = v > -1.0 && v >= prof[(a + na - 1) % na] && v >= prof[(a + 1) % na];
          for (int q = 0; q < nout && ok; q++)
            {
              int d = abs (a - (out[q] + nr));
              if (d > na / 2)
                d = na - d;
              if (d * EV_ROT_COARSE < EV_OF_SEP)
                ok = 0;
            }
          if (ok && (bi < 0 || v > prof[bi]))
            bi = a;
        }
      if (bi < 0)
        break;
      out[nout++] = bi - nr;
    }
  return nout;
}
#endif

static int
ev_distinct (const EvHyp *c, const EvHyp *H, int nh)
{
  for (int k = 0; k < nh; k++)
    if (fabs (c->rot - H[k].rot) <= 5.0 &&
        hypot (c->dx - H[k].dx, c->dy - H[k].dy) <= EV_CSTEP + 0.5)
      return 0;
  return 1;
}

/* The best distinct hypotheses -- the EV_NHYP highest envelope peaks over the
 * coarse rotation grid, plus up to EV_NOF more from the orientation field's
 * rotations -- refined at full resolution and sorted by refined NCC,
 * descending. H must hold EV_NHYP + EV_NOF. Returns how many, or -1 if no
 * alignment reaches the overlap floor at all. */
static int
ev_hyps (const EvFrame *A, const EvProbe *B, EvHyp *H, EvWork *W)
{
  enum { NCAND = 64, NROT = 2 * (EV_ROT_MAX / EV_ROT_COARSE) + 1 };
  EvHyp cand[NCAND], rbest[NROT];
  int nc = 0, any = 0, nr = EV_ROT_MAX / EV_ROT_COARSE, nh = 0;

  ev_grid_template (A, W);
  for (int r = -nr; r <= nr; r++)
    any |= ev_land_peaks (B, r * EV_ROT_COARSE, W, cand, &nc, NCAND, &rbest[r + nr]);
  if (!any)
    return -1;
  /* distinct: another candidate within 5 deg AND one grid step is the same peak */
  for (int c = 0; c < nc && nh < EV_NHYP; c++)
    if (ev_distinct (&cand[c], H, nh))
      H[nh++] = cand[c];
#if EV_NOF > 0
  /* the best envelope peak at each rotation the orientation field proposes,
   * whatever its rank among all peaks */
  {
    int ro[EV_NOF], no = ev_of_rots (A, B, nr, ro);
    for (int q = 0; q < no; q++)
      {
        EvHyp c = rbest[ro[q] + nr];
        if (ro[q] == -nr && rbest[2 * nr].s > c.s)
          c = rbest[2 * nr];                     /* -90 and +90 deg: one orientation */
        if (c.s > EV_HYP_MIN && ev_distinct (&c, H, nh))
          H[nh++] = c;
      }
  }
#endif
  /* refine at full resolution: +-EV_REF px at the coarse rotation, then the
   * 1.25-deg rotations within half a coarse step at +-1 px */
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
      const float *ta = E->ta + y * EM_W, *tm = E->tm + y * EM_W, *ta2 = E->ta2 + y * EM_W;
      int off = (y - sy) * EM_W - sx;
      const float *pa = E->pa + off, *pm = E->pm + off, *pa2 = E->pa2 + off;
      float rn = 0, rsa = 0, rsb = 0, rsaa = 0, rsbb = 0, rsab = 0;
      for (int x = xa; x < xb; x++)
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
static double
ev_evidence (const EvEvWork *E, int dx, int dy)
{
  double best[EV_NB];
  int ax[EV_NB], ay[EV_NB], ok[EV_NB], lab[EV_NB], stack[EV_NB];
  int need = (EV_BS * EV_BS) / 2;
  double wbest = 0;

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
      int sp = 0;
      double w = 0;
      if (!ok[k0] || best[k0] < EV_TAU || lab[k0] >= 0)
        continue;
      lab[k0] = k0;
      stack[sp++] = k0;
      while (sp)
        {
          int k = stack[--sp], y = k / EV_NBX, x = k % EV_NBX;
          w += atanh (best[k] < 0.999 ? best[k] : 0.999) - atanh (EV_TAU);
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
        wbest = w;
    }
  return wbest;
}

/* ---- public ------------------------------------------------------------------ */
int
ev_match (const EvFrame *t, const EvProbe *p, int stop_at, EvPose *pose)
{
  EvWork *W = malloc (sizeof *W);
  EvEvWork *E = malloc (sizeof *E);
  EvHyp H[EV_NHYP + EV_NOF];
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
          int ev;
          if (H[k].s < EV_GFLOOR)
            break;                               /* sorted: the rest are lower */
          ev_rotate_res (p, H[k].rot * EV_PI / 180.0, E);
          {
            double w = ev_evidence (E, H[k].dx, H[k].dy);
            if (w > wmax)
              wmax = w;
          }
          ev = (int) lround (wmax * 1000.0);
          if (ev > best || !bp.valid)
            {
              if (ev > best)
                best = ev;
              bp.rot_deg = H[k].rot;
              bp.dx = H[k].dx;
              bp.dy = H[k].dy;
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
  w = ev_evidence (E, pose->dx, pose->dy);
  free (E);
  return (int) lround (w * 1000.0);
}
