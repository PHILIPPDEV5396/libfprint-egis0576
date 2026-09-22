/*
 * objcheck.c -- is a candidate object usable as the "finger" of the umockdev
 * driver test?
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
 * Reads two files of concatenated flat-fielded 3990-byte frames (one press
 * each) and reports what the driver would do with them. Two numbers matter
 * and they are not the same:
 *
 *   the RAW correlation (em_match_ex), which says whether the two presses
 *   look alike at all, and
 *   the DECISION (em_match), which is what the driver actually sees -- the
 *   correlation, or -1 when the ridge-period consistency check rejects the
 *   pair as synthetic-looking.
 *
 * A pair the check rejects is reported as -1, the same value as "nothing to
 * score", so a tool that only looks at em_match cannot tell "these presses
 * do not match" from "these presses match and the check threw it away", and
 * its maximum lands just under the check's gate (0.70). Both are printed.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "egis_match.h"
#include "egis_match_check.h"

#define N 3990

static int
rd (const char *p, uint8_t **out)
{
  FILE *f = fopen (p, "rb");
  long n;

  if (!f) return -1;
  fseek (f, 0, SEEK_END); n = ftell (f); rewind (f);
  if (n < N || n % N) { fclose (f); return -1; }
  *out = malloc (n);
  if (fread (*out, 1, n, f) != (size_t) n) { fclose (f); return -1; }
  fclose (f);
  return (int) (n / N);
}

/* Why em_match rejected a pair whose correlation cleared the gate. */
static const char *
blocked_by (const EmMatchInfo *in)
{
  if (in->nblk < EM_FQ_MIN_NBLK) return "nblk";
  if (in->mad > EM_FQ_MAX_MAD) return "mad";
  if (in->corr < EM_FQ_MIN_CORR) return "corr";
  return "flat";
}

int
main (int argc, char **argv)
{
  uint8_t *a, *b;
  int na, nb, i, j, blank_a = 0, blank_b = 0;
  int n_nblk = 0, n_flat = 0, n_other = 0;
  static EmFrame A[64], B[64];
  double raw_best = -2, dec_best = -2, cov_a = 0, cov_b = 0;

  if (argc < 3) { fprintf (stderr, "usage: %s press1.raw press2.raw\n", argv[0]); return 2; }
  na = rd (argv[1], &a); nb = rd (argv[2], &b);
  if (na < 1 || nb < 1) { fprintf (stderr, "cannot read frames\n"); return 2; }
  if (na > 64) na = 64;
  if (nb > 64) nb = 64;

  printf ("press 1: %d frames, coverage", na);
  for (i = 0; i < na; i++)
    {
      em_frame_compute (a + i * N, &A[i]);
      printf (" %.2f", A[i].coverage);
      if (A[i].coverage > cov_a) cov_a = A[i].coverage;
      if (A[i].coverage == 0) blank_a++;
    }
  printf ("\npress 2: %d frames, coverage", nb);
  for (i = 0; i < nb; i++)
    {
      em_frame_compute (b + i * N, &B[i]);
      printf (" %.2f", B[i].coverage);
      if (B[i].coverage > cov_b) cov_b = B[i].coverage;
      if (B[i].coverage == 0) blank_b++;
    }
  printf ("\n");

  for (i = 0; i < na; i++)
    for (j = 0; j < nb; j++)
      {
        EmMatchInfo in;
        double raw = em_match_ex (&A[i], &B[j], &in);
        double dec = em_match (&A[i], &B[j]);

        if (raw > raw_best) raw_best = raw;
        if (dec > dec_best) dec_best = dec;
        if (raw >= EM_FQ_GATE_FROM && dec < 0)
          {
            const char *w = blocked_by (&in);
            if (!strcmp (w, "nblk")) n_nblk++;
            else if (!strcmp (w, "flat")) n_flat++;
            else n_other++;
          }
      }

  printf ("best coverage      %.2f / %.2f   (probe gate %.2f, enrolment gate 0.60)\n",
          cov_a, cov_b, em_min_coverage);
  printf ("best correlation   %.3f          (how alike the two presses are)\n", raw_best);
  printf ("best decision      %.3f          (what the driver sees; accept at %.2f)\n",
          dec_best, em_match_threshold);
  if (blank_a + blank_b > 0)
    printf ("blank frames       %d of %d / %d of %d  (no ridge evidence: too light a press)\n",
            blank_a, na, blank_b, nb);
  if (n_nblk + n_flat + n_other > 0)
    printf ("rejected by the period check: %d pairs (%d too few period blocks, %d flat map, %d other)\n",
            n_nblk + n_flat + n_other, n_nblk, n_flat, n_other);

  if (cov_a >= 0.60 && cov_b >= 0.60 && dec_best >= em_match_threshold)
    printf ("VERDICT: yes -- enrol and verify with this object\n");
  else if (raw_best >= em_match_threshold && dec_best < em_match_threshold)
    printf ("VERDICT: no -- the presses DO reproduce each other (%.3f), but the driver's\n"
            "         ridge-period check rejects them, so a verify can never succeed with\n"
            "         this object. Its texture is too regular, or its contact area too\n"
            "         small for the check's block count. Try a softer object with\n"
            "         IRREGULAR grooves and a larger contact patch.\n", raw_best);
  else if (cov_a >= 0.60 && cov_b >= 0.60)
    printf ("VERDICT: not yet -- the structure is there, the presses do not reproduce each\n"
            "         other (%.3f < %.2f). Same spot, same rotation, same force, try again.\n",
            raw_best, em_match_threshold);
  else if (cov_a >= 0.55 || cov_b >= 0.55)
    printf ("VERDICT: not yet -- this object DOES have ridge-like structure (best coverage\n"
            "         %.2f), but %s press was too light to show it. Press harder (aim for a\n"
            "         variance over 400, both presses alike) and try again -- best\n"
            "         correlation so far %.3f.\n",
            cov_a > cov_b ? cov_a : cov_b,
            cov_a >= 0.55 && cov_b >= 0.55 ? "neither" : (cov_a > cov_b ? "the second" : "the first"),
            raw_best);
  else
    printf ("VERDICT: no -- too little ridge-like structure on either press (%.2f / %.2f);\n"
            "         this object's texture is not a ridge field. Try a softer one with\n"
            "         irregular grooves 0.2-0.5 mm apart.\n", cov_a, cov_b);
  return 0;
}
