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
 * exactly as drivers/egis0576.c does it:
 *   flat_field(raw, baseline) -> egis_preprocess() -> engine
 * (the clean-room adapter's egis_preprocess is an identity copy, so calling it
 * unconditionally reproduces BOTH real pipelines.)
 *
 * Enrolment feeds the listed frames in order, one per line (the driver uses
 * the first finger-on frame of each press). Every probe file is then scored
 * against the enrolled template. Output, one line per probe on stdout:
 *   <probe-line-index> <score>      (score -1 = engine had nothing to score)
 * Enrolment diagnostics go to stderr in a fixed format evaluate.py parses:
 *   enroll <i> -> <code> (progress <p>)     code 1=need more 2=done 4=redundant <0 rejected
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

/* drivers/egis0576.c flat_field(): out = clamp(raw - baseline + mean(baseline)) */
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
  uint8_t ff[N];
  flat_field(raw, base, ff);
  egis_preprocess(ff, out);
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
  if (egis_engine_init() != 0) { fprintf(stderr, "engine init failed\n"); return 3; }
  if (egis_enroll_begin() != 0) { fprintf(stderr, "enroll_begin failed\n"); return 3; }

  el = fopen(argv[2], "r");
  if (!el) { fprintf(stderr, "cannot open enroll list\n"); return 2; }
  idx = 0;
  while (fgets(line, sizeof line, el)) {
    int prog = 0;
    chomp(line);
    if (!*line) continue;
    if (rd(line, raw)) { fprintf(stderr, "enroll %d -> unreadable\n", idx); idx++; continue; }
    prep(raw, base, cor);
    r = egis_enroll_add(cor, &prog);
    fprintf(stderr, "enroll %d -> %d (progress %d)\n", idx, r, prog);
    idx++;
    if (r == 1 || r == 2) added++;
    if (r == 2) break;
  }
  fclose(el);

  sz = egis_enroll_finish(&blob);
  fprintf(stderr, "enrolled %d frames, template %d bytes\n", added, sz);
  if (sz <= 0) { fprintf(stderr, "enroll_finish failed %d\n", sz); return 4; }
  blobs[0] = blob;
  sizes[0] = sz;
  if (egis_gallery_load(blobs, sizes, 1) != 0) { fprintf(stderr, "gallery_load failed\n"); return 4; }

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
      printf("%d %d\n", idx, egis_verify(cor, 0));
    }
    idx++;
  }
  fclose(pl);
  free(blob);
  return 0;
}
