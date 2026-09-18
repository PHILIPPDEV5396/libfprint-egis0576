/* egis_match_gabor.c -- orientation-selective front-end for the EH576
 * correlation matcher, with a rotation-aware coarse-to-fine search.
 *
 * Copyright (C) 2026 Philipp Oster
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * ---------------------------------------------------------------------------
 * WHAT THIS IS
 *
 * An alternative implementation of the em_frame_compute() / em_match() pair
 * declared in tsteppy/egis_match.h: same EmFrame, same NCC-in-[-1,1] return,
 * same -1 sentinel, so egis_engine_cleanroom.c and the accuracy kit build
 * against either. Thaddeus Stepanovich's file is not touched. It is NOT a
 * drop-in at the operating point: both score populations sit higher than
 * with his front-end, so the adapter's three constants (accept NCC, coverage
 * gate, duplicate gate) must come from gabor/egis_cr_tuning_gabor.h -- at his
 * 0.53 accept threshold this front-end false-accepts 14 % of same-person
 * impostors. Also not symmetric: em_match(a, b) resamples b (the probe) and
 * leaves a (the stored template) alone; calling it the other way round costs
 * up to 0.13 NCC on individual pairs and turns 0 % FRR into 1.7 % on the
 * reference dataset. The adapter and the kit call it template-first.
 *
 * Own work from public-domain building blocks (Hong, Wan & Jain 1998 for the
 * Gabor enhancement; structure-tensor orientation; masked NCC as in his
 * file). No vendor code and no vendor-derived constants were read or used.
 *
 * WHAT IT DOES DIFFERENTLY
 *
 *   1. Local contrast normalisation instead of his +128 high-pass.
 *   2. Structure-tensor orientation and coherence per pixel; the ridge period
 *      from a 1-D autocorrelation along the ridge normal (one value per frame,
 *      no FFT).
 *   3. A 16-direction 11x11 Gabor bank, one kernel per pixel chosen by that
 *      pixel's orientation, so a frame costs one 121-tap pass, not sixteen.
 *      This keeps what is ridge-like at the local orientation and period and
 *      suppresses the rest.
 *   4. A per-pixel mask (smoothed coherence > 0.28, energy above its 20th
 *      percentile, one erosion) instead of his 16x16-block mask.
 *   5. Translation search +-EG_SRCH (19) px AND rotation search
 *      +-EG_ROT_MAX (10) deg in EG_ROT_STEP (2.5) deg steps, coarse-to-fine:
 *      every 2nd angle on a 3-px pixel grid first, then the best EG_REFINE (3)
 *      candidates at full resolution over +-2 px and the neighbouring angles.
 *      The NCC peak is about 1.5 px and 2.5 deg wide, which is why the shift
 *      grid is every pixel and the angle step is what it is: at 5 deg the
 *      worst genuine press scores 0.78, at 2.5 deg 0.81.
 *
 * MEASURED (tools/accuracy protocol, driver order, one person, one unit, one
 * session: 5 fingers x 12 presses, block split, 60 genuine / 480 impostor
 * press comparisons, every frame of a press scored; adapter-free C harness)
 *
 *                                genuine     impostor    FRR at
 *                                min         max         FAR 0
 *   tsteppy as shipped (+-6)     0.129       0.474       26.7 %
 *   tsteppy at +-19              0.271       0.649        5.0 %
 *   this file, defaults          0.812       0.690        0.0 %
 *
 * The last line is in-sample: the angle step, erosion and search size were
 * chosen on this dataset (14 configurations tried; every one with the 2.5 deg
 * step and +-10 deg range separated the populations, +-15 deg and +-5 deg did
 * not). Cross-fold check: a threshold placed mid-gap on one fold, applied to
 * the other, gives 0 / 30 false rejects and 0 / 240 false accepts both ways,
 * and the two folds put that threshold at 0.751 and 0.752. The stricter
 * rule -- threshold AT the other fold's highest impostor -- gives 0 % FRR at
 * 1.7 % FAR, i.e. the impostor tail is still only known to +-0.05 from 480
 * samples. What the numbers do NOT say: anything about other units or other
 * people (issue #5 is where that gets measured). The +0.12 NCC gap between
 * the populations is 4x what his front-end reaches at +-19 (-0.38, they
 * overlap) and is the whole point.
 *
 * SECOND UNIT (Thaddeus Stepanovich, Yoga 7 16IRL8, Intel; 29 genuine / 88
 * impostor decisions, cross-fold with the strictest zero-false-accept
 * threshold; his docs/gabor-frontend-second-unit.md): his front-end as
 * shipped 51.7 % FRR / 4.5 % FAR; this file on raw frames 17.2 % / 1.1 %;
 * this file on flat-fielded frames 6.9 % / 1.1 %, one of the two folds at
 * 0 / 0. The 0 % / 0 % above did NOT reproduce there (genuine min 0.600
 * against impostor max 0.815, flat-fielded), so across two units the honest
 * summary is "a large improvement", not "a clean gap". His retuned old
 * front-end (+-26 px, 1400 px floor) ties this file on RAW frames pooled
 * (17.2 % each) and loses to it flat-fielded; the ablation above was
 * measured against the 800 px floor, which on my data was inert at +-19
 * (800..2000 changed nothing) but on his binds hard, so "what each piece
 * contributes" is still one dataset's answer.
 *
 * THE FLAT-FIELD IS HALF OF IT. The per-boot flat-field this driver already
 * applies (driver/egis0576.c) is what makes the threshold portable: on raw
 * frames his unit wants ~0.85 where mine wants 0.80, flat-fielded his two
 * folds agree to within 0.001 (0.815 / 0.813) and mine to 0.001 (0.751 /
 * 0.752). It must be SUBTRACTIVE (raw - baseline + mean(baseline)); the
 * multiplicative form clips and destroys frames. And it helps this front-end
 * while hurting his (51.7 % -> 65.5 % on his unit), because his +128
 * high-pass has no local normalisation to absorb the changed contrast. The
 * threshold it lands on is still unit-dependent at this sample size (the
 * mid-gap is 0.75 here, 0.81 there; the shipped 0.78 sits on the genuine side
 * of this unit's gap on purpose, see egis_cr_tuning_gabor.h); a third unit
 * decides. One caveat on the comparison itself: his numbers come from HIS
 * capture path (gain switching between reg 0x12 = 0 and 6, one settled frame
 * per press, 3-frame reference), not this driver's (calibrated exposure at a
 * fixed gain, 8-frame baseline, every frame scored), so his 0.815 impostor
 * ceiling is a property of that pipeline as much as of his unit, and it is
 * not an argument for where this driver's constant should sit.
 *
 * LIMITS (from an adversarial review, 2026-09-18, all measured):
 *   - Synthetic ridge textures. A curved grating (arc) at the sensor's ridge
 *     period scores 0.909 against real templates, a hill-climbed ridge model
 *     0.960, filtered ridge-frequency noise 0.849 -- above the worst genuine
 *     press (0.812). This is what a masked texture correlation on 70x57 px
 *     IS: any patch of locally parallel ridges at the right period and angle
 *     matches. No threshold fixes it, the probe self-similarity gate does
 *     not stop the curved ones, and this sensor offers no liveness signal.
 *     To exploit it an attacker must present the pattern to the sensor
 *     (physical access, a fabricated artefact), which also defeats every
 *     other matcher without presentation-attack detection -- but a
 *     minutiae-based matcher would reject a featureless grating, and this
 *     one does not. Countermeasures under measurement.
 *   - Poisoned flat-field baseline. If the driver's per-boot baseline is
 *     captured with the enrolled finger resting lightly on the sensor, the
 *     subtraction paints that finger's ridges, inverted, into every later
 *     frame, and a correlation matcher cannot tell an inverted half-period-
 *     shifted copy from the real thing: a featureless smudge then scored
 *     0.92-0.94. Closed in the adapter by corroborating every accept on the
 *     un-flat-fielded frame (egis_verify_raw_ok): the smudge's raw frame has
 *     no ridges (-1), genuine raw probes score >= 0.83 against flat-fielded
 *     templates on the reference dataset.
 *   - Partial presses. A small well-aligned patch of one finger can match a
 *     different finger's template over the 800 px minimum overlap. Under
 *     evaluation (raising the floor to 1200 costs one genuine press in 60).
 *
 * WHERE THE GAIN COMES FROM (numpy prototype of this pipeline, exhaustive
 * search, 240 impostors; the C reproduces its per-frame features
 * bit-for-bit, verified on synthetic frames, and its numbers to +-0.01)
 *
 *   his front-end, +-6, no rotation      FRR 26.7 %   gap -0.31
 *   + search +-19                        FRR  3.3 %   gap -0.22
 *   + per-pixel mask                     FRR  3.3 %   gap -0.26
 *   + rotation +-10 deg                  FRR  1.7 %   gap -0.03
 *   + Gabor front-end                    FRR  0.0 %   gap +0.13
 *   Gabor alone, his mask and +-6        FRR 23.3 %   gap -0.28
 *
 * Most of the rise in BOTH populations comes from the larger alignment space
 * (search, mask, rotation take the worst impostor 0.44 -> 0.67 and the worst
 * genuine press 0.13 -> 0.64); on this dataset the Gabor step is what
 * finally lifts the genuine floor clear of the impostor ceiling, and none of
 * the pieces does it alone. See SECOND UNIT for why "on this dataset" is
 * load-bearing.
 *
 * COST (synthetic frames, idle laptop, gcc -O3): em_frame_compute 1.6 ms
 * against his 0.17 ms, once per frame; em_match 4.0 ms against his 0.85 ms
 * at +-6 and 6.4 ms at +-19 -- the rotation search costs LESS than the wider
 * translation search alone, because the coarse pass is decimated and one
 * NCC pass accumulates raw sums. With 12 gallery frames that is ~50 ms per
 * probe frame. The exhaustive equivalent (every angle, every shift, every
 * pixel) is 37 ms per em_match, 9x slower; a run with every angle in the
 * coarse pass and 2-px instead of 3-px decimation found the same extremes to
 * four decimals on all 540 press comparisons.
 *
 * MEMORY: per-frame scratch (12 x 32 kB) is malloc'd inside em_frame_compute
 * and freed before it returns, so the function needs < 8 kB of stack and is
 * reentrant; em_match keeps one rotated copy (36 kB) on the stack. Nothing
 * static, nothing global.
 * ---------------------------------------------------------------------------
 */
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "egis_match.h"

#define EG_PI 3.14159265358979323846   /* M_PI is not C99 */

#ifndef EG_SRCH
#define EG_SRCH 19           /* translation search half-width, full res     */
#endif
#ifndef EG_ROT_MAX
#define EG_ROT_MAX 10.0      /* rotation search half-width in degrees       */
#endif
#ifndef EG_ROT_STEP
#define EG_ROT_STEP 2.5
#endif
/* Symmetric set: -k*step .. +k*step including 0, k = floor(max/step). */
#define EG_ROT_K ((int) (EG_ROT_MAX / EG_ROT_STEP + 1e-9))
#define EG_NROT (2 * EG_ROT_K + 1)
#ifndef EG_MIN_OVERLAP
#define EG_MIN_OVERLAP 800   /* his gate, unchanged                          */
#endif
#ifndef EG_COH_TH
#define EG_COH_TH 0.28
#endif
#ifndef EG_ERODE
#define EG_ERODE 1
#endif
#ifndef EG_MIN_RIDGE_SD
#define EG_MIN_RIDGE_SD 5.0  /* ridge evidence: sd of the Gabor response over
                              * the mask, in the units eg_normalise leaves
                              * (local sd floored at 6). Measured on the
                              * reference dataset: real frames min 6.65 /
                              * p1 12.2 / median 16.4; a smooth ramp 0.02, a
                              * smooth bump 0.42, white noise 5.6. Without
                              * this the mask -- a coherence ratio -- gives a
                              * ridge-free gradient full coverage (0.74) and
                              * it gets enrolled. */
#endif
#ifndef EG_GSX
#define EG_GSX 2.6           /* Gabor sigma across the ridges                */
#endif
#ifndef EG_GSY
#define EG_GSY 2.6           /* Gabor sigma along the ridges                 */
#endif
#define EG_NDIR 16
#define EG_KR 5              /* Gabor kernel radius -> 11x11                 */
#define EG_KS (2 * EG_KR + 1)
#ifndef EG_REFINE
#define EG_REFINE 3          /* coarse candidates refined at full resolution */
#endif
#ifndef EG_COARSE
#define EG_COARSE 1          /* 0 = exhaustive full-resolution search        */
#endif
#ifndef EG_SHIFT_STEP
#define EG_SHIFT_STEP 1      /* coarse pass: shift grid (peak is ~1.5 px wide)*/
#endif
#ifndef EG_ROT_COARSE
#define EG_ROT_COARSE 2      /* coarse pass: every n-th angle of the fine grid;
                              * the fine pass fills in the neighbours        */
#endif
#ifndef EG_PIXEL_STEP
#define EG_PIXEL_STEP 3      /* coarse pass: pixel decimation                */
#endif

/* ---- small separable Gaussian ------------------------------------------- */
/* All per-frame scratch lives in one heap block so em_frame_compute() needs
 * a few hundred bytes of stack, not ~450 kB (the capture worker is a GLib
 * thread; tsteppy's own em_frame_compute peaks at ~64 kB and the adapter
 * documents that figure). Allocated and freed inside em_frame_compute(), so
 * the function stays reentrant. */
typedef struct
{
  double img[EM_N], norm[EM_N], ridge[EM_N], coh[EM_N], energy[EM_N];
  double a[EM_N], b[EM_N], c[EM_N], d[EM_N], e[EM_N];   /* general purpose */
  double tmp[EM_N];                                      /* eg_blur only    */
} EgScratch;

/* numpy 'reflect' padding: index -i maps to i, n-1+i maps to n-1-i. Used for
 * every filter so the C matches the numpy reference at the borders. */
static inline int
eg_refl (int i, int n)
{
  if (i < 0)
    i = -i;
  if (i >= n)
    i = 2 * n - 2 - i;
  return i;
}

static void
eg_blur (const double *src, double *dst, double sigma, double *tmp)
{
  int r = (int) (3 * sigma);
  double k[64];
  double s = 0;

  if (r < 1) r = 1;
  if (r > 31) r = 31;
  for (int i = -r; i <= r; i++)
    {
      k[i + r] = exp (-(double) i * i / (2 * sigma * sigma));
      s += k[i + r];
    }
  for (int i = 0; i <= 2 * r; i++)
    k[i] /= s;

  for (int y = 0; y < EM_H; y++)
    for (int x = 0; x < EM_W; x++)
      {
        double a = 0;
        for (int i = -r; i <= r; i++)
          {
            a += k[i + r] * src[y * EM_W + eg_refl (x + i, EM_W)];
          }
        tmp[y * EM_W + x] = a;
      }
  for (int y = 0; y < EM_H; y++)
    for (int x = 0; x < EM_W; x++)
      {
        double a = 0;
        for (int i = -r; i <= r; i++)
          {
            a += k[i + r] * tmp[eg_refl (y + i, EM_H) * EM_W + x];
          }
        dst[y * EM_W + x] = a;
      }
}

/* ---- local contrast normalisation --------------------------------------- */
static void
eg_normalise (EgScratch *S)
{
  double *lm = S->a, *d = S->b, *ls = S->c;

  eg_blur (S->img, lm, 3.0, S->tmp);
  for (int i = 0; i < EM_N; i++)
    d[i] = S->img[i] - lm[i];
  for (int i = 0; i < EM_N; i++)
    lm[i] = d[i] * d[i];
  eg_blur (lm, ls, 3.0, S->tmp);
  for (int i = 0; i < EM_N; i++)
    {
      double s = sqrt (ls[i] > 1e-9 ? ls[i] : 1e-9);
      S->norm[i] = d[i] / (s < 6.0 ? 6.0 : s);
    }
}

/* ---- structure tensor: ridge angle, coherence, energy -------------------- */
static void
eg_orientation (EgScratch *S)
{
  const double *img = S->img;
  double *gx = S->a, *gy = S->b, *sxx = S->c, *syy = S->d, *sxy = S->e;

  for (int y = 0; y < EM_H; y++)
    {
      const double *r0 = img + eg_refl (y - 1, EM_H) * EM_W;
      const double *r1 = img + y * EM_W;
      const double *r2 = img + eg_refl (y + 1, EM_H) * EM_W;
      for (int x = 0; x < EM_W; x++)
        {
          int xl = eg_refl (x - 1, EM_W), xr = eg_refl (x + 1, EM_W);
          gx[y * EM_W + x] = (r1[xr] - r1[xl]) * 2 + r0[xr] - r0[xl] + r2[xr] - r2[xl];
          gy[y * EM_W + x] = (r2[x] - r0[x]) * 2 + r2[xl] - r0[xl] + r2[xr] - r0[xr];
        }
    }
  for (int i = 0; i < EM_N; i++)
    {
      sxx[i] = gx[i] * gx[i];
      syy[i] = gy[i] * gy[i];
      sxy[i] = gx[i] * gy[i];
    }
  /* eg_blur may run in place: it reads src into tmp, then tmp into dst */
  eg_blur (sxx, sxx, 3.0, S->tmp);
  eg_blur (syy, syy, 3.0, S->tmp);
  eg_blur (sxy, sxy, 3.0, S->tmp);
  for (int i = 0; i < EM_N; i++)
    {
      double num = 2.0 * sxy[i], den = sxx[i] - syy[i];
      double tr = sxx[i] + syy[i];
      S->ridge[i] = 0.5 * atan2 (num, den) + EG_PI / 2.0;
      S->coh[i] = tr > 1e-9 ? hypot (num, den) / tr : 0.0;
      S->energy[i] = tr;
    }
}

/* ---- ridge period from a 1-D autocorrelation along the ridge normal ------ */
static double
eg_bilinear (const double *a, double sx, double sy)
{
  int x0 = (int) floor (sx), y0 = (int) floor (sy);
  double fx = sx - x0, fy = sy - y0;

  if (sx < 0 || sx > EM_W - 1 || sy < 0 || sy > EM_H - 1)
    return 0.0;
  if (x0 >= EM_W - 1) { x0 = EM_W - 2; fx = 1.0; }   /* sx == EM_W-1 exactly */
  if (y0 >= EM_H - 1) { y0 = EM_H - 2; fy = 1.0; }
  return a[y0 * EM_W + x0] * (1 - fx) * (1 - fy)
         + a[y0 * EM_W + x0 + 1] * fx * (1 - fy)
         + a[(y0 + 1) * EM_W + x0] * (1 - fx) * fy
         + a[(y0 + 1) * EM_W + x0 + 1] * fx * fy;
}

static double
eg_period (const double *norm, const double *ridge, double *nx, double *ny)
{
  double best = -1e30, bestk = 6.0;

  for (int i = 0; i < EM_N; i++)
    {
      ny[i] = cos (ridge[i]);       /* unit normal to the ridge */
      nx[i] = -sin (ridge[i]);
    }
  /* Lags 4 .. 9.5 px: the sensor's ridge period is 5-7 px, and letting the
   * search run to 11 would admit the second harmonic (2T) for periods <= 5.5. */
  for (double k = 4.0; k <= 9.501; k += 0.5)
    {
      double acc = 0;
      int n = 0;
      for (int y = 0; y < EM_H; y++)
        for (int x = 0; x < EM_W; x++)
          {
            int i = y * EM_W + x;
            double sx = x + k * nx[i], sy = y + k * ny[i];
            if (sx < 0 || sx > EM_W - 1 || sy < 0 || sy > EM_H - 1)
              continue;
            acc += norm[i] * eg_bilinear (norm, sx, sy);
            n++;
          }
      if (n == 0)
        continue;
      acc /= n;
      if (acc > best)
        {
          best = acc;
          bestk = k;
        }
    }
  return bestk;
}

/* ---- Gabor bank, one kernel per pixel ----------------------------------- */
static void
eg_gabor (const double *norm, const double *ridge, double period, double *out)
{
  double bank[EG_NDIR][EG_KS * EG_KS];   /* 15 kB, built per frame: 1936 exp/cos,
                                          * nothing against the 480k MACs below,
                                          * and no process-global state */
  const double sx = EG_GSX, sy = EG_GSY;

    {
      for (int d = 0; d < EG_NDIR; d++)
        {
          double th = EG_PI * d / EG_NDIR;   /* direction the filter is steered
                                                * ACROSS (ridge normal); the
                                                * lookup below adds 90 deg */
          double ct = cos (th), st = sin (th);
          double mean = 0;
          for (int j = -EG_KR; j <= EG_KR; j++)
            for (int i = -EG_KR; i <= EG_KR; i++)
              {
                double xr = i * ct + j * st;
                double yr = -i * st + j * ct;
                double g = exp (-(xr * xr / (2 * sx * sx) + yr * yr / (2 * sy * sy)))
                           * cos (2 * EG_PI * xr / period);
                bank[d][(j + EG_KR) * EG_KS + (i + EG_KR)] = g;
                mean += g;
              }
          mean /= EG_KS * EG_KS;
          for (int i = 0; i < EG_KS * EG_KS; i++)
            bank[d][i] -= mean;
        }
    }

  for (int y = 0; y < EM_H; y++)
    for (int x = 0; x < EM_W; x++)
      {
        int i = y * EM_W + x;
        /* the kernel is indexed by the direction the filter is steered
         * across, i.e. ridge + 90 deg, matching the bank's construction */
        int d = (int) lround ((ridge[i] + EG_PI / 2.0) / (EG_PI / EG_NDIR));
        double acc = 0;
        const double *k;

        d %= EG_NDIR;
        if (d < 0)
          d += EG_NDIR;
        k = bank[d];
        for (int j = -EG_KR; j <= EG_KR; j++)
          {
            int yy = y + j;
            yy = yy < 0 ? -yy : (yy >= EM_H ? 2 * EM_H - 2 - yy : yy);
            for (int ii = -EG_KR; ii <= EG_KR; ii++)
              {
                int xx = x + ii;
                xx = xx < 0 ? -xx : (xx >= EM_W ? 2 * EM_W - 2 - xx : xx);
                acc += k[(j + EG_KR) * EG_KS + (ii + EG_KR)] * norm[yy * EM_W + xx];
              }
          }
        out[i] = acc;
      }
}

/* ---- mask: coherent, textured, eroded ----------------------------------- */
static int
eg_cmp_double (const void *a, const void *b)
{
  double x = *(const double *) a, y = *(const double *) b;
  return x < y ? -1 : (x > y ? 1 : 0);
}

static double
eg_mask (EgScratch *S, uint8_t *mask)
{
  double *sc = S->a, *sorted = S->b;
  uint8_t tmp[EM_N];
  double thr;
  int kept = 0;

  eg_blur (S->coh, sc, 2.0, S->tmp);
  memcpy (sorted, S->energy, EM_N * sizeof (double));
  qsort (sorted, EM_N, sizeof (double), eg_cmp_double);
  {
    /* numpy.percentile(x, 20): linear interpolation at 0.2 * (n - 1) */
    double pos = 0.2 * (EM_N - 1);
    int lo = (int) pos;
    thr = sorted[lo] + (pos - lo) * (sorted[lo + 1] - sorted[lo]);
  }

  for (int i = 0; i < EM_N; i++)
    mask[i] = (sc[i] > EG_COH_TH && S->energy[i] > thr) ? 1 : 0;

  for (int pass = 0; pass < EG_ERODE; pass++)
    {
      memcpy (tmp, mask, sizeof tmp);
      for (int y = 0; y < EM_H; y++)
        for (int x = 0; x < EM_W; x++)
          {
            int i = y * EM_W + x;
            int ok = tmp[i]
                     && y > 0 && tmp[i - EM_W]
                     && y < EM_H - 1 && tmp[i + EM_W]
                     && x > 0 && tmp[i - 1]
                     && x < EM_W - 1 && tmp[i + 1];
            mask[i] = ok ? 1 : 0;
          }
    }
  for (int i = 0; i < EM_N; i++)
    kept += mask[i];
  return (double) kept / EM_N;
}

/* ---- public: frame -------------------------------------------------------*/
void
em_frame_compute (const uint8_t *raw, EmFrame *f)
{
  EgScratch *S = malloc (sizeof *S);
  double mean = 0, var = 0, sd;

  if (!S)
    {
      /* Out of memory: hand back an empty frame. coverage 0 fails every
       * gate downstream (adapter: enrol -2 / verify -1), nothing crashes. */
      memset (f, 0, sizeof *f);
      return;
    }
  for (int i = 0; i < EM_N; i++)
    S->img[i] = raw[i];
  eg_normalise (S);
  eg_orientation (S);
  eg_gabor (S->norm, S->ridge, eg_period (S->norm, S->ridge, S->a, S->b), f->img);
  f->coverage = eg_mask (S, f->mask);
  free (S);

  /* Ridge evidence on an ABSOLUTE scale, before the standardisation below
   * erases it: a frame whose Gabor response over the mask has no amplitude
   * is not a fingerprint, whatever its coherence says. Zero its coverage so
   * the adapter's gate rejects it in enrol (-2) and verify (-1). */
  {
    double m = 0, v = 0;
    int n = 0;
    for (int i = 0; i < EM_N; i++)
      if (f->mask[i])
        {
          m += f->img[i];
          n++;
        }
    if (n > 0)
      {
        m /= n;
        for (int i = 0; i < EM_N; i++)
          if (f->mask[i])
            v += (f->img[i] - m) * (f->img[i] - m);
        if (sqrt (v / n) < EG_MIN_RIDGE_SD)
          {
            memset (f->mask, 0, sizeof f->mask);
            f->coverage = 0.0;
          }
      }
  }

  for (int i = 0; i < EM_N; i++)
    mean += f->img[i];
  mean /= EM_N;
  for (int i = 0; i < EM_N; i++)
    {
      f->img[i] -= mean;
      var += f->img[i] * f->img[i];
    }
  sd = sqrt (var / EM_N) + 1e-6;
  for (int i = 0; i < EM_N; i++)
    f->img[i] = f->mask[i] ? f->img[i] / sd : 0.0;
  /* Zeroing outside the mask matters for exactly one thing, but it matters:
   * the rotation search resamples img bilinearly, and without this a pixel
   * just inside the mask borrows intensity from a neighbour the mask threw
   * away. The masked NCC itself never reads these pixels. */
}

/* ---- rotation ------------------------------------------------------------*/
static double
eg_bilinear_u8 (const uint8_t *a, double sx, double sy)
{
  int x0 = (int) floor (sx), y0 = (int) floor (sy);
  double fx = sx - x0, fy = sy - y0;

  if (sx < 0 || sx > EM_W - 1 || sy < 0 || sy > EM_H - 1)
    return 0.0;
  if (x0 >= EM_W - 1) { x0 = EM_W - 2; fx = 1.0; }
  if (y0 >= EM_H - 1) { y0 = EM_H - 2; fy = 1.0; }
  return a[y0 * EM_W + x0] * (1 - fx) * (1 - fy)
         + a[y0 * EM_W + x0 + 1] * fx * (1 - fy)
         + a[(y0 + 1) * EM_W + x0] * (1 - fx) * fy
         + a[(y0 + 1) * EM_W + x0 + 1] * fx * fy;
}

static void
eg_rotate (const EmFrame *b, double th, double *img, uint8_t *mask)
{
  double cy = (EM_H - 1) / 2.0, cx = (EM_W - 1) / 2.0;
  double ct = cos (th), st = sin (th);

  for (int y = 0; y < EM_H; y++)
    for (int x = 0; x < EM_W; x++)
      {
        int i = y * EM_W + x;
        double dx = x - cx, dy = y - cy;
        double sx = cx + dx * ct - dy * st;
        double sy = cy + dx * st + dy * ct;
        if (sx < 0 || sx > EM_W - 1 || sy < 0 || sy > EM_H - 1)
          {
            img[i] = 0;
            mask[i] = 0;
            continue;
          }
        mask[i] = eg_bilinear_u8 (b->mask, sx, sy) > 0.6 ? 1 : 0;
        img[i] = mask[i] ? eg_bilinear (b->img, sx, sy) : 0.0;
      }
}

/* ---- masked NCC at one shift, on a decimation grid ----------------------- */
static double
eg_ncc (const double *ai, const uint8_t *am, const double *bi, const uint8_t *bm,
        int dx, int dy, int step, int min_overlap)
{
  int ay0 = dy > 0 ? dy : 0, ay1 = EM_H + (dy < 0 ? dy : 0);
  int ax0 = dx > 0 ? dx : 0, ax1 = EM_W + (dx < 0 ? dx : 0);
  double sa = 0, sb = 0, saa = 0, sbb = 0, sab = 0, cov, va, vb;
  int n = 0;

  /* One pass with raw sums; the centred sums follow algebraically
   * (cov = sum(ab) - sum(a) sum(b) / n, etc.). Same result as the two-pass
   * form to ~1e-12 -- the inputs are standardised to unit variance, so there
   * is no cancellation to fear -- at half the memory traffic, which is what
   * this function is bound by. */
  for (int y = ay0; y < ay1; y += step)
    {
      const double *ra = ai + y * EM_W, *rb = bi + (y - dy) * EM_W - dx;
      const uint8_t *ma = am + y * EM_W, *mb = bm + (y - dy) * EM_W - dx;
      for (int x = ax0; x < ax1; x += step)
        if (ma[x] & mb[x])
          {
            double a = ra[x], b = rb[x];
            sa += a;
            sb += b;
            saa += a * a;
            sbb += b * b;
            sab += a * b;
            n++;
          }
    }
  if (n == 0 || n * step * step < min_overlap)
    return -1.0;
  cov = sab - sa * sb / n;
  va = saa - sa * sa / n;
  vb = sbb - sb * sb / n;
  if (va <= 0 || vb <= 0)
    return -1.0;
  return cov / (sqrt (va * vb) + 1e-6);
}

/* ---- public: match -------------------------------------------------------*/
/* a = the stored template frame, b = the probe. Only b is resampled (rotated),
 * so em_match(a, b) and em_match(b, a) differ slightly at the mask edge; the
 * adapter and the bench both call it template-first. */
double
em_match (const EmFrame *a, const EmFrame *b)
{
  double cand_s[EG_REFINE];
  int cand_dx[EG_REFINE], cand_dy[EG_REFINE], cand_r[EG_REFINE];
  double rimg[EM_N];
  uint8_t rmask[EM_N];
  double best = -1.0;

  for (int i = 0; i < EG_REFINE; i++)
    {
      cand_s[i] = -2.0;
      cand_dx[i] = cand_dy[i] = cand_r[i] = 0;
    }

#if !EG_COARSE
  /* exhaustive: every rotation, every shift, full resolution. Slow; this is
   * the reference the coarse-to-fine path is measured against. */
  for (int r = 0; r < EG_NROT; r++)
    {
      double th = (r - EG_ROT_K) * EG_ROT_STEP * EG_PI / 180.0;

      eg_rotate (b, th, rimg, rmask);
      for (int dy = -EG_SRCH; dy <= EG_SRCH; dy++)
        for (int dx = -EG_SRCH; dx <= EG_SRCH; dx++)
          {
            double s = eg_ncc (a->img, a->mask, rimg, rmask, dx, dy, 1,
                               EG_MIN_OVERLAP);
            if (s > best)
              best = s;
          }
    }
  (void) cand_s; (void) cand_dx; (void) cand_dy; (void) cand_r;
#else
  /* coarse pass: every 2nd pixel and every 2nd shift, one rotation at a time
   * (keeping all EG_NROT rotated copies would put 180 kB on the stack of a
   * capture worker thread for no gain). */
  for (int r = EG_ROT_K % EG_ROT_COARSE; r < EG_NROT; r += EG_ROT_COARSE)
    {
      /* r walks the fine grid in steps of EG_ROT_COARSE, phased so that 0 deg
       * (r == EG_ROT_K) is always visited */
      double th = (r - EG_ROT_K) * EG_ROT_STEP * EG_PI / 180.0;

      eg_rotate (b, th, rimg, rmask);
      for (int dy = -EG_SRCH; dy <= EG_SRCH; dy += EG_SHIFT_STEP)
        for (int dx = -EG_SRCH; dx <= EG_SRCH; dx += EG_SHIFT_STEP)
          {
            double s = eg_ncc (a->img, a->mask, rimg, rmask, dx, dy,
                               EG_PIXEL_STEP, EG_MIN_OVERLAP);
            if (s <= -1.0 || s <= cand_s[EG_REFINE - 1])
              continue;
            for (int i = 0; i < EG_REFINE; i++)
              if (s > cand_s[i])
                {
                  for (int j = EG_REFINE - 1; j > i; j--)
                    {
                      cand_s[j] = cand_s[j - 1];
                      cand_dx[j] = cand_dx[j - 1];
                      cand_dy[j] = cand_dy[j - 1];
                      cand_r[j] = cand_r[j - 1];
                    }
                  cand_s[i] = s;
                  cand_dx[i] = dx;
                  cand_dy[i] = dy;
                  cand_r[i] = r;
                  break;
                }
          }
    }

  /* fine pass: full resolution around each surviving candidate, at its
   * rotation and the two neighbouring ones (the NCC is as peaky in angle as
   * it is in shift) */
  for (int c = 0; c < EG_REFINE; c++)
    {
      if (cand_s[c] <= -1.0)
        continue;
      for (int r = cand_r[c] - (EG_ROT_COARSE - 1); r <= cand_r[c] + (EG_ROT_COARSE - 1); r++)
        {
          double th;

          if (r < 0 || r >= EG_NROT)
            continue;
          th = (r - EG_ROT_K) * EG_ROT_STEP * EG_PI / 180.0;
          eg_rotate (b, th, rimg, rmask);
          for (int dy = cand_dy[c] - 2; dy <= cand_dy[c] + 2; dy++)
            for (int dx = cand_dx[c] - 2; dx <= cand_dx[c] + 2; dx++)
              {
                double s;
                if (dy < -EG_SRCH || dy > EG_SRCH || dx < -EG_SRCH || dx > EG_SRCH)
                  continue;
                s = eg_ncc (a->img, a->mask, rimg, rmask, dx, dy, 1,
                            EG_MIN_OVERLAP);
                if (s > best)
                  best = s;
              }
        }
    }
#endif
  return best;
}
