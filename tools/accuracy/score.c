/* score.c -- offline scorer for the egis0576 accuracy kit.
 *
 * Built twice by the Makefile, once per matcher flavour, against the repo's
 * own driver/egis0576 sources behind the same egis_engine.h contract:
 *   score-vendor     egis_engine.c + egis_funcs.c + egis_rt.c + egis_preprocess.c + egis_coherence_map.c
 *   score-cleanroom  egis_engine_cleanroom.c + tsteppy/egis_match.c
 *
 *   score <baseline> <enroll.lst> <probe.lst>
 *
 * Frames are raw 70x57 = 3990-byte uint8 images, either as numpy .npy files
 * (what capture.py writes) or as bare 3990-byte files. Pipeline per frame,
 * exactly as driver/egis0576.c does it:
 *   flat_field(raw, baseline) -> engine
 * (the vendor engine applies its own Windows-style per-frame preprocessing
 * internally; the clean-room engine needs none.)
 *
 * Enrolment feeds the listed frames in order, one per line (the driver uses
 * the first finger-on frame of each press). Every probe file is then scored
 * against the enrolled template. Output, one line per probe on stdout:
 *   <probe-line-index> <score>      (score -1 = engine had nothing to score)
 * Enrolment diagnostics go to stderr in a fixed format evaluate.py parses:
 *   enroll <i> -> <code> (progress <p>)
 *     vendor flavour (egis_funcs.c): 1 = need more, 2 = done, 4 and -8 = the
 *     frame REGISTERED against the template but added nothing new ("redundant",
 *     egis_funcs.c:9661/9698/9727; 4 vs -8 is only the stage counter), -1 = the
 *     frame was rejected outright, fewer than 11 minutiae ("bad image",
 *     egis_funcs.c:9621-9626). So -8 is a success code, not a rejection.
 *     clean-room flavour (egis_engine_cleanroom.c:135): -2 = coverage below the
 *     enrolment gate.
 *   enrolled <n> frames, template <bytes> bytes
 * No frame content ever reaches stdout or stderr. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "egis_engine.h"

#define N EGIS_IMG_SIZE

/* Read one frame: .npy (uint8, 3990 elements, C order) or bare 3990 bytes. */
static int rd(const char *path, uint8_t *b)
{
  FILE *f = fopen(path, "rb");
  unsigned char hdr[12];
  size_t n, hlen, off;
  int ok = 0;

  if (!f) return -1;
  n = fread(hdr, 1, 10, f);
  if (n == 10 && memcmp(hdr, "\x93NUMPY", 6) == 0) {
    char *h;
    if (hdr[6] == 1) {
      hlen = hdr[8] | (hdr[9] << 8);
      off = 10 + hlen;
    } else {
      if (fread(hdr + 10, 1, 2, f) != 2) { fclose(f); return -1; }
      hlen = (size_t)hdr[8] | ((size_t)hdr[9] << 8) | ((size_t)hdr[10] << 16) | ((size_t)hdr[11] << 24);
      off = 12 + hlen;
    }
    if (hlen > 65536) { fclose(f); return -1; }
    h = malloc(hlen + 1);
    if (!h) { fclose(f); return -1; }
    if (fread(h, 1, hlen, f) != hlen) { free(h); fclose(f); return -1; }
    h[hlen] = 0;
    /* uint8, C order, 3990 elements -- anything else is not a sensor frame */
    ok = (strstr(h, "|u1") != NULL) && (strstr(h, "'fortran_order': False") != NULL) &&
         (strstr(h, "(3990,)") != NULL || strstr(h, "(57, 70)") != NULL);
    free(h);
    if (!ok) { fclose(f); return -1; }
    if (fseek(f, (long)off, SEEK_SET) != 0) { fclose(f); return -1; }
  } else {
    rewind(f);
  }
  n = fread(b, 1, N, f);
  ok = (n == N) && (fgetc(f) == EOF);
  fclose(f);
  return ok ? 0 : -1;
}

/* driver/egis0576.c flat_field(): out = clamp(raw - baseline + mean(baseline)) */
static void flat_field(const uint8_t *raw, const uint8_t *base, uint8_t *out)
{
  long s = 0;
  int i, m;
  for (i = 0; i < N; i++) s += base[i];
  m = (int)(s / N);
  for (i = 0; i < N; i++) {
    int v = (int)raw[i] - (int)base[i] + m;
    out[i] = v < 0 ? 0 : (v > 255 ? 255 : v);
  }
}

static void prep(const uint8_t *raw, const uint8_t *base, uint8_t *out)
{
  flat_field(raw, base, out);
}

static char *chomp(char *line)
{
  line[strcspn(line, "\r\n")] = 0;
  return line;
}

int main(int argc, char **argv)
{
  uint8_t base[N], raw[N], cor[N];
  char line[4096];
  FILE *el, *pl;
  int added = 0, r, sz, idx;
  uint8_t *blob = NULL;
  const uint8_t *blobs[1];
  int sizes[1];

  if (argc != 4) {
    fprintf(stderr, "usage: %s <baseline.npy|bin> <enroll.lst> <probe.lst>\n", argv[0]);
    return 2;
  }
  if (rd(argv[1], base)) { fprintf(stderr, "bad baseline file\n"); return 2; }
  EgisEngine *eng = egis_engine_new();
  if (!eng) { fprintf(stderr, "engine init failed\n"); return 3; }
  if (egis_enroll_begin(eng) != 0) { fprintf(stderr, "enroll_begin failed\n"); return 3; }

  el = fopen(argv[2], "r");
  if (!el) { fprintf(stderr, "cannot open enroll list\n"); return 2; }
  /* One line per frame. Frames of one PRESS may be grouped with a "--" line
   * after the group: the press is then tried frame by frame until the engine
   * returns anything but -2 (quality under the gate), which is what the
   * driver's settle loop does; without groups every line is its own press. */
  idx = 0;
  {
    int settled = 0;                       /* this press already produced a verdict */
    int last = -2;
    while (fgets(line, sizeof line, el)) {
      int prog = 0;
      chomp(line);
      if (!*line) continue;
      if (strcmp(line, "--") == 0) {
        if (!settled)                      /* every frame of the press was refused */
          fprintf(stderr, "enroll %d -> %d (progress 0)\n", idx, last), idx++;
        settled = 0; last = -2;
        continue;
      }
      if (settled) continue;               /* rest of a press that already settled */
      if (rd(line, raw)) { fprintf(stderr, "enroll %d -> unreadable\n", idx); idx++; settled = 1; continue; }
      prep(raw, base, cor);
      r = egis_enroll_add(eng, cor, &prog);
      last = r;
      if (r == -2) continue;               /* let the press settle: next frame */
      fprintf(stderr, "enroll %d -> %d (progress %d)\n", idx, r, prog);
      idx++;
      settled = 1;
      if (r == 1 || r == 2) added++;
      if (r == 2) break;
    }
  }
  fclose(el);

  sz = egis_enroll_finish(eng, NULL, 0);
  if (sz > 0) { blob = malloc((size_t)sz); sz = blob ? egis_enroll_finish(eng, blob, sz) : -1; }
  fprintf(stderr, "enrolled %d frames, template %d bytes\n", added, sz);
  if (sz <= 0) { fprintf(stderr, "enroll_finish failed %d\n", sz); return 4; }
  blobs[0] = blob;
  sizes[0] = sz;
  if (egis_gallery_load(eng, blobs, sizes, 1) != 0) { fprintf(stderr, "gallery_load failed\n"); return 4; }

  pl = fopen(argv[3], "r");
  if (!pl) { fprintf(stderr, "cannot open probe list\n"); return 2; }
  idx = 0;
  while (fgets(line, sizeof line, pl)) {
    chomp(line);
    if (!*line) continue;
    if (rd(line, raw)) {
      printf("%d unreadable\n", idx);
    } else {
      prep(raw, base, cor);
      printf("%d %d\n", idx, egis_verify(eng, cor, 0));
    }
    idx++;
  }
  fclose(pl);
  free(blob);
  return 0;
}
