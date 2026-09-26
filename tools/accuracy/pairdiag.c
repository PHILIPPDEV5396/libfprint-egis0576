/*
 * pairdiag.c -- why does this unit separate, or not?
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
 * evaluate.py says WHETHER a unit separates. This says what the alignment
 * search was working with when it did or did not: the coverage of the frames,
 * the masked overlap at the winning alignment, and the correlation as a
 * function of an overlap floor. Those three are what distinguishes "the
 * presses carry little ridge area" from "the matcher is too permissive on
 * this skin", and no amount of press-level scores can tell them apart.
 *
 * It also says what a match was MADE of. The Gabor NCC measures ridge flow
 * and ridge period, not identity: two different fingers that put
 * near-parallel ridges of one angle and period on the window score like one
 * finger (root-cause investigation, 2026-09-26; on the reference unit's
 * 2026-09-18 session it accepts 18 of 480 impostor presses under the
 * driver's rule, all 18 right thumb against right index). The blocks after
 * "impostor" are meant to help tell, on any unit, whether its failures are
 * that mechanism, a capture without fine detail, or a pattern fixed to the
 * sensor:
 *
 *   identity        the fine structure the Gabor filter discards,
 *                   correlated at the alignment the search picked. On the
 *                   reference unit's frames the same skin usually
 *                   reproduced it and flow coincidences did not (one
 *                   person, first frames); whether that holds elsewhere is
 *                   what this block measures.
 *   zero_shift      the Gabor NCC between different fingers with no shift
 *                   and no rotation: ~0 unless something fixed to the sensor
 *                   survives the flat-field.
 *   flow            how parallel each frame's ridges are, and how closely
 *                   two frames' ridge angles agree at the winning alignment.
 *   by_finger_pair  which fingers collide.
 *   research_floor  the search RE-RUN with an overlap floor (1200, 1400,
 *                   1600 and 2000 px), which by_overlap_floor (a filter of
 *                   the winners) is not.
 *
 *   make pairdiag && ./pairdiag <dataset>/baseline.npy <list>
 *
 * where <list> has one line per press, "<finger-label> <path to its first
 * frame>" -- evaluate.py --pair-list writes it for you. Output is a JSON
 * object of counts, rates and per-pair scalars with finger labels: no image
 * data, nothing a frame can be rebuilt from, safe to paste into an issue. The
 * frames stay on your machine. A run over the kit's 60 presses takes about a
 * minute and a half on a current laptop (over two on battery), most of it in
 * research_floor. List lines that give no press are counted on stderr and in
 * "presses_skipped", never dropped silently.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
/* The front-end is compiled INTO this file, unmodified, instead of being
 * linked: the identity and flow blocks need its normalised image, its
 * ridge-angle field, its rotation and its NCC, which are static there.
 * Including the source keeps them the matcher's own arithmetic rather than a
 * copy that could drift. em_frame_compute, em_match_ex and em_match are the
 * functions the adapter calls, and GABOR_EXTRA reaches them as before. */
#include "egis_match_gabor.c"
#include "egis_cr_tuning_gabor.h"

#define N EM_N
#define MAXP 256
#define MAXPP (MAXP * (MAXP - 1))   /* ordered pairs */
#define MAXLIST 50           /* impostor_accepted_list: highest NCC first */
#define PD_HIGH 0.70         /* the "_ge_070" fields: an NCC worth a look. The
                              * same value as EM_FQ_GATE_FROM, but a field name
                              * must not move with a GABOR_EXTRA override */
#define PD_ZS_OVERLAP 800    /* zero_shift: fixed, like PD_HIGH, so the field
                              * does not move with GABOR_EXTRA */

/* Read one frame: .npy (uint8, 3990 elements, C order) or bare 3990 bytes --
 * the same two shapes score.c accepts. */
static int
rd (const char *path, uint8_t *b)
{
  FILE *f = fopen (path, "rb");
  unsigned char hdr[12];
  size_t n, hlen, off;
  int ok = 0;

  if (!f) return -1;
  n = fread (hdr, 1, 10, f);
  if (n == 10 && memcmp (hdr, "\x93NUMPY", 6) == 0)
    {
      char *h;
      if (hdr[6] == 1) { hlen = hdr[8] | (hdr[9] << 8); off = 10 + hlen; }
      else
        {
          if (fread (hdr + 10, 1, 2, f) != 2) { fclose (f); return -1; }
          hlen = (size_t) hdr[8] | ((size_t) hdr[9] << 8) |
                 ((size_t) hdr[10] << 16) | ((size_t) hdr[11] << 24);
          off = 12 + hlen;
        }
      if (hlen > 65536) { fclose (f); return -1; }
      h = malloc (hlen + 1);
      if (!h) { fclose (f); return -1; }
      if (fread (h, 1, hlen, f) != hlen) { free (h); fclose (f); return -1; }
      h[hlen] = 0;
      ok = (strstr (h, "|u1") != NULL) && (strstr (h, "'fortran_order': False") != NULL) &&
           (strstr (h, "(3990,)") != NULL || strstr (h, "(57, 70)") != NULL);
      free (h);
      if (!ok) { fclose (f); return -1; }
      if (fseek (f, (long) off, SEEK_SET) != 0) { fclose (f); return -1; }
    }
  else
    rewind (f);
  n = fread (b, 1, N, f);
  ok = (n == N) && (fgetc (f) == EOF);
  fclose (f);
  return ok ? 0 : -1;
}

static int cmp_d (const void *a, const void *b)
{
  double x = *(const double *) a, y = *(const double *) b;
  return x < y ? -1 : x > y;
}

static double pct_of (double *v, int n, double q)
{
  if (n <= 0) return 0;
  return v[(int) (q * (n - 1) + 0.5)];
}

/* A number, or null where the population is empty (JSON has no NaN). */
static void
jnum (double v, int prec)
{
  if (isfinite (v))
    printf ("%.*f", prec, v);
  else
    fputs ("null", stdout);
}

/* One population's shape, printed as JSON; an empty one prints null, not a
 * 0 that would read as measured. */
static void
report (const char *name, double *ncc, double *ovl, int n, int last)
{
  qsort (ncc, n, sizeof *ncc, cmp_d);
  qsort (ovl, n, sizeof *ovl, cmp_d);
  printf ("  \"%s\": {\"n\": %d, \"ncc_p5\": ", name, n);
  jnum (n ? pct_of (ncc, n, 0.05) : NAN, 4);
  printf (", \"ncc_median\": ");
  jnum (n ? pct_of (ncc, n, 0.5) : NAN, 4);
  printf (", \"ncc_p95\": ");
  jnum (n ? pct_of (ncc, n, 0.95) : NAN, 4);
  printf (", \"ncc_max\": ");
  jnum (n ? ncc[n - 1] : NAN, 4);
  printf (", \"overlap_p10\": ");
  jnum (n ? pct_of (ovl, n, 0.10) : NAN, 0);
  printf (", \"overlap_median\": ");
  jnum (n ? pct_of (ovl, n, 0.5) : NAN, 0);
  printf (", \"overlap_max\": ");
  jnum (n ? ovl[n - 1] : NAN, 0);
  printf ("}%s\n", last ? "" : ",");
}

/* ---- what the identity and flow blocks need of a frame ------------------ */

typedef struct
{
  EmFrame res;          /* Gabor residual on the frame's own mask, shaped as an
                         * EmFrame so that eg_rotate resamples it exactly as
                         * the search resamples the Gabor image */
  double  ridge[N];     /* ridge angle, eg_orientation of the flat-fielded frame */
  double  c2[N], s2[N]; /* cos / sin of twice that angle, for resampling */
  double  coherence;    /* ridge parallelism over the whole frame, 0..1 */
} PdFrame;

/* The residual is what the Gabor filter throws away: the local-contrast-
 * normalised frame (eg_normalise, the filter's own input) minus its
 * least-squares multiple of the Gabor output, over the mask -- pores,
 * ridge-width modulation, ridge endings, the detail a field of ridge angle
 * and period cannot reproduce. The coherence is that of the structure tensor
 * summed over the mask's interior on the Gabor image: 1 when every ridge in
 * the frame is parallel, 0 when there is no common direction. */
static int
pd_frame (const uint8_t *cor, const EmFrame *f, PdFrame *p)
{
  EgScratch *S = malloc (sizeof *S);
  const double *I = f->img;
  const uint8_t *M = f->mask;
  double m = 0, ng = 0, gg = 0, beta, sxx = 0, syy = 0, sxy = 0;
  int n = 0;

  if (!S) return -1;
  for (int i = 0; i < N; i++)
    S->img[i] = cor[i];
  eg_normalise (S);
  eg_orientation (S);
  for (int i = 0; i < N; i++)
    if (M[i]) { m += S->norm[i]; n++; }
  m = n ? m / n : 0;
  for (int i = 0; i < N; i++)
    if (M[i])
      {
        ng += (S->norm[i] - m) * I[i];
        gg += I[i] * I[i];
      }
  beta = ng / (gg > 1e-12 ? gg : 1e-12);
  for (int i = 0; i < N; i++)
    {
      p->res.img[i] = M[i] ? (S->norm[i] - m) - beta * I[i] : 0.0;
      p->ridge[i] = S->ridge[i];
      p->c2[i] = cos (2 * S->ridge[i]);
      p->s2[i] = sin (2 * S->ridge[i]);
    }
  memcpy (p->res.mask, M, sizeof p->res.mask);
  p->res.coverage = f->coverage;
  free (S);

  for (int y = 1; y < EM_H - 1; y++)
    for (int x = 1; x < EM_W - 1; x++)
      {
        int i = y * EM_W + x, ok = 1;
        double gx, gy;
        for (int j = -1; j <= 1 && ok; j++)
          for (int k = -1; k <= 1; k++)
            if (!M[i + j * EM_W + k]) { ok = 0; break; }
        if (!ok) continue;
        gx = (I[i + 1] - I[i - 1]) * 2 + I[i - EM_W + 1] - I[i - EM_W - 1] +
             I[i + EM_W + 1] - I[i + EM_W - 1];
        gy = (I[i + EM_W] - I[i - EM_W]) * 2 + I[i + EM_W - 1] - I[i - EM_W - 1] +
             I[i + EM_W + 1] - I[i - EM_W + 1];
        sxx += gx * gx;
        syy += gy * gy;
        sxy += gx * gy;
      }
  p->coherence = sxx + syy > 0 ? hypot (sxx - syy, 2 * sxy) / (sxx + syy) : 0.0;
  return 0;
}

/* Masked NCC of the two residuals at the alignment the search picked (probe
 * rotated by th, then shifted by dx, dy into template coordinates, exactly as
 * eg_match_core scored the Gabor images there). NAN if nothing overlaps. */
static double
pd_resid_at (const PdFrame *a, const PdFrame *b, int dx, int dy, double th)
{
  double rimg[N], s;
  uint8_t rmask[N];

  eg_rotate (&b->res, th, rimg, rmask);
  s = eg_ncc (a->res.img, a->res.mask, rimg, rmask, dx, dy, 1, 1);
  return s <= -1.0 ? NAN : s;
}

/* RMS difference of the two ridge-angle fields over the overlap at that
 * alignment, in degrees. Genuine pairs the driver accepts sit at a median of
 * 2.3 / 2.2 deg on the reference unit's two sessions (399 / 239 pairs). */
static double
pd_orient_rms (const EmFrame *fa, const PdFrame *a, const EmFrame *fb, const PdFrame *b,
               int dx, int dy, double th)
{
  double rimg[N], acc = 0;
  double cy = (EM_H - 1) / 2.0, cx = (EM_W - 1) / 2.0, ct = cos (th), st = sin (th);
  uint8_t rmask[N];
  int n = 0;

  eg_rotate (fb, th, rimg, rmask);
  for (int y = 0; y < EM_H; y++)
    for (int x = 0; x < EM_W; x++)
      {
        int i = y * EM_W + x, px = x - dx, py = y - dy;
        double ux, uy, sx, sy, d;
        if (px < 0 || px >= EM_W || py < 0 || py >= EM_H ||
            !rmask[py * EM_W + px] || !fa->mask[i])
          continue;
        /* the probe pixel eg_rotate sampled for (px, py); its ridge angle,
         * resampled as a doubled-angle vector, turns with the frame */
        ux = px - cx;
        uy = py - cy;
        sx = cx + ux * ct - uy * st;
        sy = cy + ux * st + uy * ct;
        d = cos (2 * (a->ridge[i] -
                      (0.5 * atan2 (eg_bilinear (b->s2, sx, sy), eg_bilinear (b->c2, sx, sy)) - th)));
        d = 0.5 * acos (d > 1 ? 1 : (d < -1 ? -1 : d));
        acc += d * d;
        n++;
      }
  return n < 10 ? NAN : sqrt (acc / n) * 180.0 / EG_PI;
}

static int
pd_overlap (const uint8_t *am, const uint8_t *bm, int dx, int dy)
{
  int n = 0;

  for (int y = dy > 0 ? dy : 0; y < EM_H + (dy < 0 ? dy : 0); y++)
    for (int x = dx > 0 ? dx : 0; x < EM_W + (dx < 0 ? dx : 0); x++)
      n += am[y * EM_W + x] & bm[(y - dy) * EM_W + x - dx];
  return n;
}

/* The alignment search re-run exhaustively -- every rotation of the fine
 * grid, every shift within +-EG_SRCH, every pixel -- keeping for each floor
 * the best NCC among the alignments whose masked overlap reaches it. That is
 * the space a rebuild with -DEG_MIN_OVERLAP=<floor> searches (its
 * coarse-to-fine pass visits part of it), so a higher floor can move the
 * winner to another alignment; a filter of the default search's winners
 * cannot. best[k] = -1 when no alignment reaches floors[k]. floors ascend. */
static void
pd_research (const EmFrame *a, const EmFrame *b, const int *floors, int nfl, double *best)
{
  double rimg[N];
  uint8_t rmask[N];

  for (int k = 0; k < nfl; k++)
    best[k] = -1.0;
  for (int r = 0; r < EG_NROT; r++)
    {
      double th = (r - EG_ROT_K) * EG_ROT_STEP * EG_PI / 180.0;

      eg_rotate (b, th, rimg, rmask);
      for (int dy = -EG_SRCH; dy <= EG_SRCH; dy++)
        for (int dx = -EG_SRCH; dx <= EG_SRCH; dx++)
          {
            /* one NCC at the lowest floor; the overlap is counted only when
             * that NCC would raise a higher floor's best, and then once for
             * all of them */
            double s = eg_ncc (a->img, a->mask, rimg, rmask, dx, dy, 1, floors[0]);
            int ov = -1;
            if (s <= -1.0)
              continue;
            if (s > best[0])
              best[0] = s;
            for (int k = 1; k < nfl; k++)
              if (s > best[k])
                {
                  if (ov < 0)
                    ov = pd_overlap (a->mask, rmask, dx, dy);
                  if (ov < floors[k])
                    break;          /* floors ascend: no higher one is reached */
                  best[k] = s;
                }
          }
    }
}

/* ---- aggregation helpers ------------------------------------------------ */

/* A finger label as a JSON string: the manifest's labels are plain words, but
 * the two characters JSON reserves are escaped rather than trusted. */
static void
jstr (const char *s)
{
  putchar ('"');
  for (; *s; s++)
    {
      if (*s == '"' || *s == '\\') putchar ('\\');
      if ((unsigned char) *s >= 0x20) putchar (*s);
    }
  putchar ('"');
}

/* Sorts v[0..n) in place; the callers pass finite values only. */
static double
q_of (double *v, int n, double q)
{
  if (n <= 0) return NAN;
  qsort (v, n, sizeof *v, cmp_d);
  return pct_of (v, n, q);
}

static double
max_of (const double *v, int n)
{
  double m = NAN;
  for (int i = 0; i < n; i++)
    if (!(v[i] <= m)) m = v[i];
  return m;
}

static int
n_ge (const double *v, int n, double t)
{
  int k = 0;
  for (int i = 0; i < n; i++) k += v[i] >= t;
  return k;
}

/* P(genuine value > impostor value), ties counted half: the area under the
 * ROC curve of that one scalar. NAN when either side is empty. */
static double
auc (const double *g, int ng, const double *im, int ni)
{
  double s = 0;
  if (ng <= 0 || ni <= 0) return NAN;
  for (int i = 0; i < ng; i++)
    for (int j = 0; j < ni; j++)
      s += g[i] > im[j] ? 1.0 : (g[i] == im[j] ? 0.5 : 0.0);
  return s / ((double) ng * ni);
}

/* One compared press pair, template a against probe b. */
typedef struct
{
  int    a, b;                /* presses: template, probe               */
  double ncc;                 /* em_match_ex: plain NCC at the winner   */
  double chk;                 /* em_match: what the driver compares     */
  int    dx, dy, overlap;
  double rot, resid, orms;
} PdPair;

/* research_floor's floors, ascending (pd_research counts the NCC at the
 * first); by_overlap_floor filters at these and at the default 800 px */
static const int rfloors[] = { 1200, 1400, 1600, 2000 };
#define NRF ((int) (sizeof rfloors / sizeof rfloors[0]))

int
main (int argc, char **argv)
{
  static uint8_t base[N], raw[N], cor[N];
  static EmFrame F[MAXP];
  static PdFrame X[MAXP];
  static PdPair P[MAXPP];
  static char lab[MAXP][32];
  static double gn[MAXP * MAXP / 2], go[MAXP * MAXP / 2];
  static double in_[MAXP * MAXP / 2], io[MAXP * MAXP / 2];
  static double zs[MAXP * MAXP / 2], rf[MAXP * MAXP / 2][NRF];
  static int rf_gen[MAXP * MAXP / 2];
  static double va[MAXPP], vb[MAXPP], vc[MAXPP], vd[MAXPP];
  static int ord[MAXPP];
  static double cov[MAXP], coh[MAXP];
  static int fid[MAXP];
  static const int floors[] = { 800, 1200, 1400, 1600, 2000 };
  const double thr = em_match_threshold;
  int nf = 0, ng = 0, ni = 0, np = 0, nz = 0, nrf = 0, nfing = 0, unscored = 0, i, j, k;
  int skipped = 0;
  long sum = 0;
  double mean;
  char line[4096], l[32], p[4000];
  FILE *fp;

  if (argc < 3)
    {
      fprintf (stderr, "usage: %s <baseline.npy> <list: '<label> <frame path>' per press>\n", argv[0]);
      return 2;
    }
  if (rd (argv[1], base)) { fprintf (stderr, "bad baseline file\n"); return 2; }
  for (i = 0; i < N; i++) sum += base[i];
  mean = (double) (sum / N);

  fp = fopen (argv[2], "r");
  if (!fp) { fprintf (stderr, "cannot open list\n"); return 2; }
  /* A line that does not give a press is counted, not silently dropped: the
   * reporter must see that presses were lost. Blank lines do not count. */
  while (nf < MAXP && fgets (line, sizeof line, fp))
    {
      if (sscanf (line, "%31s %3999[^\n]", l, p) != 2)
        {
          if (strspn (line, " \t\r\n") != strlen (line)) skipped++;
          continue;
        }
      if (rd (p, raw)) { skipped++; continue; }
      /* driver/egis0576.c flat_field() */
      for (i = 0; i < N; i++)
        {
          int v = (int) raw[i] - (int) base[i] + (int) mean;
          cor[i] = (uint8_t) (v < 0 ? 0 : (v > 255 ? 255 : v));
        }
      em_frame_compute (cor, &F[nf]);
      if (pd_frame (cor, &F[nf], &X[nf])) { fprintf (stderr, "out of memory\n"); return 2; }
      cov[nf] = F[nf].coverage;
      snprintf (lab[nf], sizeof lab[0], "%s", l);
      for (k = 0; k < nf; k++)       /* finger index, in order of first line */
        if (strcmp (lab[k], l) == 0) break;
      fid[nf] = k < nf ? fid[k] : nfing++;
      nf++;
    }
  if (nf == MAXP)
    while (fgets (line, sizeof line, fp))
      if (strspn (line, " \t\r\n") != strlen (line))
        {
          fprintf (stderr, "pairdiag: only the first %d presses are used\n", MAXP);
          break;
        }
  fclose (fp);
  /* no paths: the count says enough, and stderr may be pasted too */
  if (skipped)
    fprintf (stderr, "pairdiag: skipped %d list line(s): unreadable frame, bad label or "
             "malformed line\n", skipped);
  if (nf < 2) { fprintf (stderr, "need at least two presses\n"); return 2; }
  fprintf (stderr, "pairdiag: %d presses: %d searches, then %d exhaustive re-searches\n",
           nf, nf * (nf - 1), nf * (nf - 1) / 2);

  /* Every ordered pair: the matcher is not symmetric (it resamples the probe,
   * never the template), and either press of a pair can be the enrolled one.
   * The i < j half -- template = the earlier line -- is the pair set the
   * blocks up to "impostor" have always used, and they still do. */
  for (i = 0; i < nf; i++)
    for (j = 0; j < nf; j++)
      {
        EmMatchInfo m;
        PdPair *q;
        double ncc, th;

        if (i == j) continue;
        ncc = em_match_ex (&F[i], &F[j], &m);
        if (i < j)
          {
            if (ncc < 0) unscored++;        /* nothing comparable in the pair */
            else if (fid[i] == fid[j]) { gn[ng] = ncc; go[ng] = m.overlap; ng++; }
            else { in_[ni] = ncc; io[ni] = m.overlap; ni++; }
            if (fid[i] != fid[j])
              {
                /* the frames as they are, no shift, no rotation: eg_rotate
                 * at 0 deg reproduces a frame exactly, so this is the value
                 * the search itself sees at (0, 0, 0) */
                double z = eg_ncc (F[i].img, F[i].mask, F[j].img, F[j].mask, 0, 0, 1, PD_ZS_OVERLAP);
                if (z > -1.0) zs[nz++] = z;
              }
          }
        if (ncc < 0) continue;
        q = &P[np++];
        q->a = i;
        q->b = j;
        q->ncc = ncc;
        /* em_match returns em_match_ex's NCC unchanged below the period
         * check's gate (egis_match_check.h); from the gate up, ask it */
        q->chk = ncc >= EM_FQ_GATE_FROM ? em_match (&F[i], &F[j]) : ncc;
        q->dx = m.dx;
        q->dy = m.dy;
        q->rot = m.rot_deg;
        q->overlap = m.overlap;
        th = m.rot_deg * EG_PI / 180.0;
        q->resid = pd_resid_at (&X[i], &X[j], m.dx, m.dy, th);
        q->orms = pd_orient_rms (&F[i], &X[i], &F[j], &X[j], m.dx, m.dy, th);
      }

  /* research_floor runs on every i < j pair: the ones by_overlap_floor
   * filters, plus the pairs_unscorable ones the default search found no
   * alignment for. */
  for (i = 0; i < nf; i++)
    for (j = i + 1; j < nf; j++)
      {
        pd_research (&F[i], &F[j], rfloors, NRF, rf[nrf]);
        rf_gen[nrf++] = fid[i] == fid[j];
      }

  printf ("{\n  \"tool\": \"pairdiag\", \"presses\": %d, \"presses_skipped\": %d, \"pairs_unscorable\": %d,\n",
          nf, skipped, unscored);
  qsort (cov, nf, sizeof *cov, cmp_d);
  printf ("  \"coverage\": {\"p10\": %.3f, \"median\": %.3f, \"max\": %.3f, \"probe_gate\": %.2f, \"enrol_gate\": %.2f},\n",
          pct_of (cov, nf, 0.10), pct_of (cov, nf, 0.5), cov[nf - 1],
          em_min_coverage, (double) EGIS_CR_MIN_ENROL_COVERAGE);
  /* A FILTER of the winners, not a rebuild with -DEG_MIN_OVERLAP: it drops
   * the pairs whose winning alignment lies below a floor and cannot move a
   * winner to another alignment (that needs the search itself, which
   * research_floor at the end re-runs). So its maxima above a floor are a
   * lower bound on what the exhaustive re-search (research_floor) finds, and
   * in practice on a rebuild with -DEG_MIN_OVERLAP (equal to it at 1200 and
   * 1600 px on both reference sessions), not that rebuild's result; it says
   * how much of each population lives above a floor at all. */
  printf ("  \"by_overlap_floor\": [");
  for (k = 0; k < (int) (sizeof floors / sizeof floors[0]); k++)
    {
      int gk = 0, ik = 0;
      double gmax = 0, imax = 0;
      for (i = 0; i < ng; i++) if (go[i] >= floors[k]) { gk++; if (gn[i] > gmax) gmax = gn[i]; }
      for (i = 0; i < ni; i++) if (io[i] >= floors[k]) { ik++; if (in_[i] > imax) imax = in_[i]; }
      printf ("%s\n    {\"floor\": %d, \"genuine_pairs\": %d, \"genuine_max\": ",
              k ? "," : "", floors[k], gk);
      jnum (gk ? gmax : NAN, 4);
      printf (", \"impostor_pairs\": %d, \"impostor_max\": ", ik);
      jnum (ik ? imax : NAN, 4);
      printf ("}");
    }
  printf ("\n  ],\n");
  report ("genuine", gn, go, ng, 0);
  report ("impostor", in_, io, ni, 0);

  /* identity: "accepted" = em_match >= em_match_threshold, the driver's
   * comparison; resid = pd_resid_at at em_match_ex's winning alignment */
  {
    int n, nga = 0, ngi = 0, nia;
    double *ga = va, *g7 = vb, t;

    printf ("  \"identity\": {\n");
    for (k = 0, n = 0; k < np; k++)
      if (fid[P[k].a] == fid[P[k].b] && P[k].chk >= thr && isfinite (P[k].resid))
        ga[n++] = P[k].resid;
    nga = n;
    printf ("    \"genuine_accepted\": {\"n\": %d, \"resid_p10\": ", nga);
    jnum (q_of (ga, nga, 0.10), 4);
    printf (", \"resid_median\": ");
    jnum (q_of (ga, nga, 0.5), 4);
    printf (", \"resid_p90\": ");
    jnum (q_of (ga, nga, 0.90), 4);
    printf (", \"frac_resid_ge_010\": ");
    jnum (nga ? (double) n_ge (ga, nga, 0.10) / nga : NAN, 3);
    printf (", \"frac_resid_ge_020\": ");
    jnum (nga ? (double) n_ge (ga, nga, 0.20) / nga : NAN, 3);
    printf ("},\n");

    for (k = 0, n = 0; k < np; k++)
      if (fid[P[k].a] != fid[P[k].b] && P[k].ncc >= PD_HIGH && isfinite (P[k].resid))
        g7[n++] = P[k].resid;
    ngi = n;
    t = max_of (g7, ngi);
    printf ("    \"impostor_ge_070\": {\"n\": %d, \"resid_median\": ", ngi);
    jnum (q_of (g7, ngi, 0.5), 4);
    printf (", \"resid_p90\": ");
    jnum (q_of (g7, ngi, 0.90), 4);
    printf (", \"resid_max\": ");
    jnum (t, 4);
    printf ("},\n");

    for (k = 0, n = 0, nia = 0; k < np; k++)
      if (fid[P[k].a] != fid[P[k].b] && P[k].chk >= thr)
        {
          nia++;
          if (isfinite (P[k].resid)) g7[n++] = P[k].resid;
        }
    printf ("    \"impostor_accepted\": {\"n\": %d, \"n_resid_ge_010\": %d, \"n_resid_ge_020\": %d, \"resid_max\": ",
            nia, n_ge (g7, n, 0.10), n_ge (g7, n, 0.20));
    jnum (max_of (g7, n), 4);
    printf ("},\n");

    /* the AUCs: genuine against impostor pairs, both at NCC >= 0.70, on the
     * residual and on the Gabor NCC itself */
    {
      int a = 0, b = 0, c = 0, d = 0;
      double *gr = va, *ir = vb, *gc = vc, *ic = vd;
      for (k = 0; k < np; k++)
        {
          if (P[k].ncc < PD_HIGH) continue;
          if (fid[P[k].a] == fid[P[k].b])
            {
              gc[c++] = P[k].ncc;
              if (isfinite (P[k].resid)) gr[a++] = P[k].resid;
            }
          else
            {
              ic[d++] = P[k].ncc;
              if (isfinite (P[k].resid)) ir[b++] = P[k].resid;
            }
        }
      printf ("    \"genuine_ge_070\": {\"n\": %d},\n    \"auc_resid_ge_070\": ", c);
      jnum (auc (gr, a, ir, b), 3);
      printf (", \"auc_ncc_ge_070\": ");
      jnum (auc (gc, c, ic, d), 3);
      printf ("\n  },\n");
    }
  }

  /* zero_shift, on the i < j impostor pairs: at (0, 0, 0) the NCC is
   * symmetric, so the other half would only count every pair twice */
  {
    double zm = 0, zv = 0;
    for (k = 0; k < nz; k++) zm += zs[k];
    zm = nz ? zm / nz : NAN;
    for (k = 0; k < nz; k++) zv += (zs[k] - zm) * (zs[k] - zm);
    printf ("  \"zero_shift\": {\"n\": %d, \"impostor_ncc_mean\": ", nz);
    jnum (zm, 4);
    printf (", \"impostor_ncc_se\": ");
    jnum (nz > 1 ? sqrt (zv / (nz - 1) / nz) : NAN, 4);
    printf (", \"impostor_ncc_median\": ");
    jnum (q_of (zs, nz, 0.5), 4);
    printf ("},\n");
  }

  /* flow: coherence over the frames the probe gate would pass at all; the
   * accepted genuine pairs' orientation RMS is the unit's own yardstick */
  {
    int n = 0, ni_ = 0, l4 = 0, l6 = 0, n7 = 0, nga = 0;
    for (i = 0; i < nf; i++)
      if (F[i].coverage >= em_min_coverage) coh[n++] = X[i].coherence;
    for (k = 0; k < np; k++)
      {
        if (!isfinite (P[k].orms)) continue;
        if (fid[P[k].a] == fid[P[k].b])
          {
            if (P[k].chk >= thr) vb[nga++] = P[k].orms;
            continue;
          }
        ni_++;
        l4 += P[k].orms < 4.0;
        l6 += P[k].orms < 6.0;
        if (P[k].ncc >= PD_HIGH) va[n7++] = P[k].orms;
      }
    printf ("  \"flow\": {\"frames\": %d, \"coherence_p10\": ", n);
    jnum (q_of (coh, n, 0.10), 3);
    printf (", \"coherence_median\": ");
    jnum (q_of (coh, n, 0.5), 3);
    printf (", \"coherence_p90\": ");
    jnum (q_of (coh, n, 0.90), 3);
    printf (",\n    \"impostor_pairs\": %d, \"impostor_orient_rms_lt4_frac\": ", ni_);
    jnum (ni_ ? (double) l4 / ni_ : NAN, 4);
    printf (", \"impostor_orient_rms_lt6_frac\": ");
    jnum (ni_ ? (double) l6 / ni_ : NAN, 4);
    printf (", \"impostor_ge_070_orient_rms_median\": ");
    jnum (q_of (va, n7, 0.5), 1);
    printf (", \"genuine_accepted_orient_rms_median\": ");
    jnum (q_of (vb, nga, 0.5), 1);
    printf ("},\n");
  }

  /* by_finger_pair: impostor pairs of each two fingers, both directions */
  printf ("  \"by_finger_pair\": [");
  {
    int first = 1;
    for (i = 0; i < nfing; i++)
      for (j = i + 1; j < nfing; j++)
        {
          int n = 0, n7 = 0, nacc = 0;
          const char *la = NULL, *lb = NULL;
          for (k = 0; k < nf; k++)
            {
              if (!la && fid[k] == i) la = lab[k];
              if (!lb && fid[k] == j) lb = lab[k];
            }
          for (k = 0; k < np; k++)
            {
              int fa = fid[P[k].a], fb = fid[P[k].b];
              if (!((fa == i && fb == j) || (fa == j && fb == i))) continue;
              va[n++] = P[k].ncc;
              n7 += P[k].ncc >= PD_HIGH;
              nacc += P[k].chk >= thr;
            }
          printf ("%s\n    {\"a\": ", first ? "" : ",");
          jstr (la);
          printf (", \"b\": ");
          jstr (lb);
          printf (", \"n\": %d, \"ncc_median\": ", n);
          jnum (q_of (va, n, 0.5), 4);
          printf (", \"ncc_max\": ");
          jnum (n ? va[n - 1] : NAN, 4);
          printf (", \"n_ge_070\": %d, \"n_accepted\": %d}", n7, nacc);
          first = 0;
        }
  }
  printf ("\n  ],\n");

  /* impostor_accepted_list: every impostor pair em_match accepts, highest
   * NCC first, at most MAXLIST of them. em_match's value is not printed: it
   * is em_match_ex's NCC unchanged or EM_FQ_REJECT, and a rejected pair is
   * not listed, so here it always equals "ncc". */
  {
    int n = 0, shown;
    for (k = 0; k < np; k++)
      if (fid[P[k].a] != fid[P[k].b] && P[k].chk >= thr)
        ord[n++] = k;
    for (i = 1; i < n; i++)            /* insertion sort, n is small */
      for (j = i; j > 0 && P[ord[j]].ncc > P[ord[j - 1]].ncc; j--)
        {
          int t = ord[j];
          ord[j] = ord[j - 1];
          ord[j - 1] = t;
        }
    shown = n < MAXLIST ? n : MAXLIST;
    printf ("  \"impostor_accepted_list\": [");
    for (i = 0; i < shown; i++)
      {
        const PdPair *q = &P[ord[i]];
        printf ("%s\n    {\"a\": ", i ? "," : "");
        jstr (lab[q->a]);
        printf (", \"b\": ");
        jstr (lab[q->b]);
        printf (", \"ncc\": %.4f, \"overlap\": %d, \"dx\": %d, \"dy\": %d, "
                "\"rot\": %.1f, \"orient_rms_deg\": ",
                q->ncc, q->overlap, q->dx, q->dy, q->rot);
        jnum (q->orms, 1);
        printf (", \"resid\": ");
        jnum (q->resid, 4);
        printf (", \"coherence_a\": %.3f, \"coherence_b\": %.3f}", X[q->a].coherence, X[q->b].coherence);
      }
    printf ("%s],\n  \"impostor_accepted_list_omitted\": %d,\n", shown ? "\n  " : "", n - shown);
  }

  /* research_floor: every i < j pair, re-searched above each floor */
  printf ("  \"research_floor\": [");
  for (k = 0; k < NRF; k++)
    {
      int gk = 0, ik = 0, n78 = 0, n = 0;
      double imax = -1.0;
      for (i = 0; i < nrf; i++)
        {
          double s = rf[i][k];
          if (rf_gen[i])
            {
              va[n++] = s;                 /* none above the floor: -1 */
              gk += s > -1.0;
            }
          else if (s > -1.0)
            {
              ik++;
              n78 += s >= thr;
              if (s > imax) imax = s;
            }
        }
      printf ("%s\n    {\"floor\": %d, \"genuine_pairs\": %d, \"genuine_median\": ",
              k ? "," : "", rfloors[k], gk);
      jnum (q_of (va, n, 0.5), 4);
      printf (", \"impostor_pairs\": %d, \"impostor_max\": ", ik);
      jnum (ik ? imax : NAN, 4);
      printf (", \"impostor_n_ge_078\": %d}", n78);
    }
  printf ("\n  ]\n}\n");
  return 0;
}
