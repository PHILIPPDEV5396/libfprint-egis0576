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
 *   make pairdiag && ./pairdiag <dataset>/baseline.npy <list>
 *
 * where <list> has one line per press, "<finger-label> <path to its first
 * frame>" -- evaluate.py --pair-list writes it for you. Output is a JSON
 * object of counts and rates: no image data, nothing a frame can be rebuilt
 * from, safe to paste into an issue. The frames stay on your machine.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "egis_match.h"
#include "egis_match_check.h"
#include "egis_cr_tuning_gabor.h"

#define N 3990
#define MAXP 256

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

/* One population's shape, printed as JSON. */
static void
report (const char *name, double *ncc, double *ovl, int n, int last)
{
  qsort (ncc, n, sizeof *ncc, cmp_d);
  qsort (ovl, n, sizeof *ovl, cmp_d);
  printf ("  \"%s\": {\"n\": %d, \"ncc_p5\": %.4f, \"ncc_median\": %.4f, "
          "\"ncc_p95\": %.4f, \"ncc_max\": %.4f, \"overlap_p10\": %.0f, "
          "\"overlap_median\": %.0f, \"overlap_max\": %.0f}%s\n",
          name, n, pct_of (ncc, n, 0.05), pct_of (ncc, n, 0.5),
          pct_of (ncc, n, 0.95), n ? ncc[n - 1] : 0,
          pct_of (ovl, n, 0.10), pct_of (ovl, n, 0.5), n ? ovl[n - 1] : 0,
          last ? "" : ",");
}

int
main (int argc, char **argv)
{
  static uint8_t base[N], raw[N], cor[N];
  static EmFrame F[MAXP];
  static char lab[MAXP][32];
  static double gn[MAXP * MAXP / 2], go[MAXP * MAXP / 2];
  static double in_[MAXP * MAXP / 2], io[MAXP * MAXP / 2];
  static double cov[MAXP];
  static const int floors[] = { 800, 1200, 1400, 1600, 2000 };
  int nf = 0, ng = 0, ni = 0, unscored = 0, i, j, k;
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
  while (fgets (line, sizeof line, fp) && nf < MAXP)
    {
      if (sscanf (line, "%31s %3999[^\n]", l, p) != 2) continue;
      if (rd (p, raw)) continue;
      /* driver/egis0576.c flat_field() */
      for (i = 0; i < N; i++)
        {
          int v = (int) raw[i] - (int) base[i] + (int) mean;
          cor[i] = (uint8_t) (v < 0 ? 0 : (v > 255 ? 255 : v));
        }
      em_frame_compute (cor, &F[nf]);
      cov[nf] = F[nf].coverage;
      snprintf (lab[nf], sizeof lab[0], "%s", l);
      nf++;
    }
  fclose (fp);
  if (nf < 2) { fprintf (stderr, "need at least two presses\n"); return 2; }

  for (i = 0; i < nf; i++)
    for (j = i + 1; j < nf; j++)
      {
        EmMatchInfo m;
        double ncc = em_match_ex (&F[i], &F[j], &m);
        if (ncc < 0) { unscored++; continue; }   /* nothing comparable in the pair */
        if (strcmp (lab[i], lab[j]) == 0) { gn[ng] = ncc; go[ng] = m.overlap; ng++; }
        else { in_[ni] = ncc; io[ni] = m.overlap; ni++; }
      }

  printf ("{\n  \"tool\": \"pairdiag\", \"presses\": %d, \"pairs_unscorable\": %d,\n", nf, unscored);
  qsort (cov, nf, sizeof *cov, cmp_d);
  printf ("  \"coverage\": {\"p10\": %.3f, \"median\": %.3f, \"max\": %.3f, \"probe_gate\": %.2f, \"enrol_gate\": %.2f},\n",
          pct_of (cov, nf, 0.10), pct_of (cov, nf, 0.5), cov[nf - 1],
          em_min_coverage, (double) EGIS_CR_MIN_ENROL_COVERAGE);
  /* The floor sweep is the cheap version of rebuilding with -DEG_MIN_OVERLAP:
   * it cannot move a winning alignment (that needs the search itself), but it
   * says how much of each population lives above a floor at all, and what the
   * worst genuine / best impostor pair up there scores. */
  printf ("  \"by_overlap_floor\": [");
  for (k = 0; k < (int) (sizeof floors / sizeof floors[0]); k++)
    {
      int gk = 0, ik = 0;
      double gmax = 0, imax = 0;
      for (i = 0; i < ng; i++) if (go[i] >= floors[k]) { gk++; if (gn[i] > gmax) gmax = gn[i]; }
      for (i = 0; i < ni; i++) if (io[i] >= floors[k]) { ik++; if (in_[i] > imax) imax = in_[i]; }
      printf ("%s\n    {\"floor\": %d, \"genuine_pairs\": %d, \"genuine_max\": %.4f, "
              "\"impostor_pairs\": %d, \"impostor_max\": %.4f}",
              k ? "," : "", floors[k], gk, gmax, ik, imax);
    }
  printf ("\n  ],\n");
  report ("genuine", gn, go, ng, 0);
  report ("impostor", in_, io, ni, 1);
  printf ("}\n");
  return 0;
}
