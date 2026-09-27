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
 * each) and reports what the driver would do with them, press 1 taken as the
 * enrolment and press 2 as the probe. Three things are printed:
 *
 *   the ridge-image correlation (em_match_ex), which says whether the two
 *   presses look alike at all;
 *   the front-end's old decision (em_match): that correlation, or
 *   EM_FQ_REJECT when the ridge-period consistency check rejects the pair.
 *   Since 2026-09-26 NEITHER decides verify or identify any more -- the
 *   period check takes no part in the driver's decision -- and both are
 *   printed only to say how an object looks to the front-end;
 *   the DECISION: the identity evidence (egis_match_evidence.c), which is
 *   what egis_verify / egis_identify score, mapped as the adapter maps it
 *   (score = W * EGIS_THRESHOLD / EGIS_CR_ACCEPT_EVIDENCE), best over the
 *   press-1 frames for each press-2 frame, and whether two consecutive
 *   press-2 frames clear the threshold (the driver's rule, less its raw-frame
 *   corroboration, which needs the un-flat-fielded frames this tool does not
 *   get).
 *
 * Build (tools/upstream/objprobe.py does it): the front-end, the evidence
 * stage and this file, with -I driver/egis0576 -I .../gabor -I .../tsteppy.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "egis_engine.h"
#include "egis_match.h"
#include "egis_match_check.h"
#include "egis_match_evidence.h"
#include "egis_cr_tuning_gabor.h"

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

/* The pixels-per-mm the driver's own measurement implies (6.4 px ridge
 * period against a 0.5 mm human ridge spacing). Only used to print a
 * groove size someone can act on; nothing decides on it. */
#define PX_PER_MM 12.8

/* What the ridge-period estimator makes of a frame: how many of its blocks
 * get a period at all, and where those periods sit inside the window it can
 * see (EM_FQ_K0..EM_FQ_K1 px). A texture finer or coarser than the window
 * pins them against an edge and leaves almost no valid block, which is the
 * one failure the coverage and the correlation do not show. */
static void
report_period (const EmFrame *f, const char *label)
{
  static double map[EM_NB], conf[EM_NB];
  int nv = em_period_map (f->img, f->mask, map, conf);
  double mn = 1e9, mx = -1e9, sum = 0;
  int k = 0, at_lo = 0, at_hi = 0;

  for (int i = 0; i < EM_NB; i++)
    if (map[i] > 0)
      {
        if (map[i] < mn) mn = map[i];
        if (map[i] > mx) mx = map[i];
        if (map[i] <= EM_FQ_K0 + 0.01) at_lo++;
        if (map[i] >= EM_FQ_K1 - 0.01) at_hi++;
        sum += map[i];
        k++;
      }
  printf ("%s period: %d of %d blocks have one", label, nv, EM_NB);
  if (k == 0)
    {
      printf (" -- none at all: the texture is outside the %.1f-%.1f px the\n"
              "         estimator can see (%.2f-%.2f mm grooves)\n",
              (double) EM_FQ_K0, (double) EM_FQ_K1,
              EM_FQ_K0 / PX_PER_MM, EM_FQ_K1 / PX_PER_MM);
      return;
    }
  printf (", %.1f px mean (%.2f mm), range %.1f-%.1f px\n", sum / k, sum / k / PX_PER_MM, mn, mx);
  if (at_lo > k / 2)
    printf ("         -- pinned at the bottom of the %.1f-%.1f px window: this texture is\n"
            "            TOO FINE for the sensor. Grooves need to be about %.2f mm, not less.\n",
            (double) EM_FQ_K0, (double) EM_FQ_K1, 6.4 / PX_PER_MM);
  else if (at_hi > k / 2)
    printf ("         -- pinned at the top of the %.1f-%.1f px window: this texture is\n"
            "            TOO COARSE. Grooves need to be about %.2f mm, not more.\n",
            (double) EM_FQ_K0, (double) EM_FQ_K1, 6.4 / PX_PER_MM);
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
  int ev_score[64], ev_best = -1, ev_two = 0;

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
        if (raw >= EM_FQ_GATE_FROM && dec != raw)
          {
            const char *w = blocked_by (&in);
            if (!strcmp (w, "nblk")) n_nblk++;
            else if (!strcmp (w, "flat")) n_flat++;
            else n_other++;
          }
      }

  /* the decision: identity evidence, press 1 as the template (EvFrame),
   * each press-2 frame as a probe (EvProbe), as egis_verify scores it */
  {
    EvFrame *T = malloc ((size_t) na * sizeof *T);
    EvProbe *P = malloc (sizeof *P);
    int prev = 0;

    if (!T || !P) { fprintf (stderr, "out of memory\n"); return 2; }
    for (i = 0; i < na; i++)
      ev_frame_compute (a + i * N, &T[i]);
    for (j = 0; j < nb; j++)
      {
        int best = -1;
        ev_probe_compute (b + j * N, P);
        if (P->e.f.coverage >= em_min_coverage)
          for (i = 0; i < na; i++)
            {
              int w = ev_match (&T[i], P, 0, NULL);
              if (w > best) best = w;
            }
        ev_score[j] = best < 0 ? -1
                      : (int) lround (best * ((double) EGIS_THRESHOLD / EGIS_CR_ACCEPT_EVIDENCE));
        if (ev_score[j] > ev_best) ev_best = ev_score[j];
        if (ev_score[j] >= EGIS_THRESHOLD)
          {
            if (prev) ev_two = 1;
            prev = 1;
          }
        else
          prev = 0;
      }
    free (T);
    free (P);
  }

  {
    int ba = 0, bb = 0;
    for (int i = 1; i < na; i++) if (A[i].coverage > A[ba].coverage) ba = i;
    for (int i = 1; i < nb; i++) if (B[i].coverage > B[bb].coverage) bb = i;
    report_period (&A[ba], "press 1");
    report_period (&B[bb], "press 2");
  }
  printf ("best coverage      %.2f / %.2f   (probe gate %.2f, enrolment gate 0.60)\n",
          cov_a, cov_b, em_min_coverage);
  printf ("best correlation   %.3f          (how alike the two presses' ridge images are)\n", raw_best);
  printf ("old NCC decision   %.3f          (front-end NCC with the period check; decides nothing\n"
          "                                  since 2026-09-26, shown for comparison)\n", dec_best);
  printf ("evidence per frame");
  for (j = 0; j < nb; j++)
    printf (" %d", ev_score[j]);
  printf ("\nbest evidence      %d (%.2f x the threshold %d)   <- THE DECISION\n",
          ev_best, ev_best / (double) EGIS_THRESHOLD, EGIS_THRESHOLD);
  printf ("driver's rule      %s (two consecutive press-2 frames >= %d; raw-frame\n"
          "                   corroboration not checked here)\n",
          ev_two ? "ACCEPT" : "reject", EGIS_THRESHOLD);
  if (blank_a + blank_b > 0)
    printf ("blank frames       %d of %d / %d of %d  (no ridge evidence: too light a press)\n",
            blank_a, na, blank_b, nb);
  if (n_nblk + n_flat + n_other > 0)
    printf ("old period check would reject: %d pairs (%d too few period blocks, %d flat map, %d other)\n",
            n_nblk + n_flat + n_other, n_nblk, n_flat, n_other);

  if (cov_a >= 0.60 && cov_b >= 0.60 && ev_two)
    printf ("VERDICT: yes -- the identity evidence accepts press 2 against press 1: this\n"
            "         object MATCHES its own enrolment (a recording made with it pins the\n"
            "         successful-match path, not a refusal)\n");
  else if (cov_a >= 0.60 && cov_b >= 0.60 && raw_best >= em_match_threshold)
    printf ("VERDICT: no match -- the ridge images reproduce (%.3f), but the object's fine\n"
            "         structure does not reproduce over enough of the overlap for the\n"
            "         identity evidence (best %.2f x the threshold): the driver refuses it.\n",
            raw_best, ev_best / (double) EGIS_THRESHOLD);
  else if (cov_a >= 0.60 && cov_b >= 0.60)
    printf ("VERDICT: no match yet -- the structure is there, but neither the fine structure\n"
            "         (evidence %.2f x the threshold) nor the ridge images (correlation %.3f)\n"
            "         reproduce between the presses. Same spot, same rotation, same force,\n"
            "         try again.\n",
            ev_best / (double) EGIS_THRESHOLD, raw_best);
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
            "         irregular grooves about 0.5 mm apart.\n", cov_a, cov_b);
  return 0;
}
