/* objcheck.c -- is a candidate object usable as the "finger" of the umockdev
 * driver test? Reads two files of concatenated flat-fielded 3990-byte frames
 * (one press each) and prints, per frame, the matcher's coverage and, across
 * the two presses, the best masked NCC -- the number the driver's threshold
 * (0.78) is compared against. Numbers only, frames stay where they are. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "egis_match.h"

#define N 3990

static int rd (const char *p, uint8_t **out)
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

int main (int argc, char **argv)
{
  uint8_t *a, *b;
  int na, nb, i, j, blank_a = 0, blank_b = 0;
  static EmFrame A[64], B[64];
  double best = -2, cov_a = 0, cov_b = 0;

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
        double v = em_match (&A[i], &B[j]);
        if (v > best) best = v;
      }
  printf ("best coverage   %.2f / %.2f   (probe gate %.2f, enrolment gate 0.60)\n",
          cov_a, cov_b, em_min_coverage);
  printf ("best cross NCC  %.3f            (accept threshold %.2f)\n", best, em_match_threshold);
  /* A frame whose coverage is exactly 0 did not clear the ridge-evidence
   * gate: there was contact, but no ridge field to speak of -- almost always
   * a press too light for this object rather than the wrong object. */
  if (blank_a + blank_b > 0)
    printf ("blank frames   %d of %d / %d of %d  (no ridge evidence: too light a press)\n",
            blank_a, na, blank_b, nb);

  if (cov_a >= 0.60 && cov_b >= 0.60 && best >= em_match_threshold)
    printf ("VERDICT: yes -- enrol and verify with this object\n");
  else if (cov_a >= 0.60 && cov_b >= 0.60)
    printf ("VERDICT: not yet -- the structure is there, the presses do not reproduce each other\n"
            "         (best %.3f < %.2f). Same spot, same rotation, same force, and try again.\n",
            best, em_match_threshold);
  else if (cov_a >= 0.55 || cov_b >= 0.55)
    printf ("VERDICT: not yet -- this object DOES have ridge-like structure (best coverage %.2f),\n"
            "         but %s press was too light to show it. Press harder (aim for a variance\n"
            "         over 400, both presses alike) and try again -- best cross NCC so far %.3f.\n",
            cov_a > cov_b ? cov_a : cov_b,
            cov_a >= 0.55 && cov_b >= 0.55 ? "neither" : (cov_a > cov_b ? "the second" : "the first"),
            best);
  else
    printf ("VERDICT: no -- too little ridge-like structure on either press (%.2f / %.2f);\n"
            "         this object's texture is not a ridge field. Try a softer one with\n"
            "         irregular grooves 0.2-0.5 mm apart.\n", cov_a, cov_b);
  return 0;
}
