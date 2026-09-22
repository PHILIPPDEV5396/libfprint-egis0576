/* nbis_count.c -- how many minutiae does the PUBLIC-DOMAIN NBIS extractor
 * (NIST mindtct, as bundled in libfprint) find on a 70x57 EH576 frame?
 *
 * The answer is the reason the egis0576 driver is not an FpImageDevice: see
 * docs/matcher-comparison.md ("Why not minutiae"). No vendor code, no vendor
 * blob: libfprint's own nbis/mindtct sources (make nbis-count
 * LIBFPRINT_SRC=<checkout>) plus the driver's documented flat-field step.
 * Frames stay local; only counts are printed.
 *
 *   nbis_count <baseline.npy> <frame.npy>...
 * prints one header line with the effective configuration, then
 *   <path> <n_minutiae> <rc>
 * per frame. Knobs, all off by default: NBIS_SCALE=<1..6> (bilinear
 * upscale before extraction), NBIS_PPMM=<x> (pixels per mm handed to
 * mindtct; default 12.8 = the measured 6.4 px ridge period over 0.5 mm,
 * times the scale), NBIS_NORM=1 (2 % percentile contrast stretch),
 * NBIS_KEEP_PERIM=1 (keep the perimeter minutiae mindtct removes by
 * default).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <nbis.h>

#define W 70
#define H 57
#define N (W * H)

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
    if (hdr[6] == 1) { hlen = hdr[8] | (hdr[9] << 8); off = 10 + hlen; }
    else {
      if (fread(hdr + 10, 1, 2, f) != 2) { fclose(f); return -1; }
      hlen = (size_t)hdr[8] | ((size_t)hdr[9] << 8) | ((size_t)hdr[10] << 16) | ((size_t)hdr[11] << 24);
      off = 12 + hlen;
    }
    if (hlen > 65536) { fclose(f); return -1; }
    h = malloc(hlen + 1);
    if (!h) { fclose(f); return -1; }
    if (fread(h, 1, hlen, f) != hlen) { free(h); fclose(f); return -1; }
    h[hlen] = 0;
    ok = strstr(h, "|u1") && strstr(h, "'fortran_order': False") &&
         (strstr(h, "(3990,)") || strstr(h, "(57, 70)"));
    free(h);
    if (!ok) { fclose(f); return -1; }
    if (fseek(f, (long)off, SEEK_SET) != 0) { fclose(f); return -1; }
  } else rewind(f);
  n = fread(b, 1, N, f);
  ok = (n == N) && (fgetc(f) == EOF);
  fclose(f);
  return ok ? 0 : -1;
}

static void flat_field(const uint8_t *raw, const uint8_t *base, uint8_t *out)
{
  long s = 0; int i, m;
  for (i = 0; i < N; i++) s += base[i];
  m = (int)(s / N);
  for (i = 0; i < N; i++) {
    int v = (int)raw[i] - (int)base[i] + m;
    out[i] = v < 0 ? 0 : (v > 255 ? 255 : v);
  }
}

/* bilinear upscale by integer factor */
static void upscale(const uint8_t *in, int w, int h, int f, uint8_t *out)
{
  int W2 = w * f, H2 = h * f, x, y;
  for (y = 0; y < H2; y++) {
    double sy = (y + 0.5) / f - 0.5;
    int y0 = (int)floor(sy); double fy = sy - y0;
    int y1 = y0 + 1;
    if (y0 < 0) y0 = 0; if (y0 > h-1) y0 = h-1;
    if (y1 < 0) y1 = 0; if (y1 > h-1) y1 = h-1;
    for (x = 0; x < W2; x++) {
      double sx = (x + 0.5) / f - 0.5;
      int x0 = (int)floor(sx); double fx = sx - x0;
      int x1 = x0 + 1;
      double v;
      if (x0 < 0) x0 = 0; if (x0 > w-1) x0 = w-1;
      if (x1 < 0) x1 = 0; if (x1 > w-1) x1 = w-1;
      v = in[y0*w+x0]*(1-fx)*(1-fy) + in[y0*w+x1]*fx*(1-fy)
        + in[y1*w+x0]*(1-fx)*fy     + in[y1*w+x1]*fx*fy;
      out[y*W2+x] = (uint8_t)(v < 0 ? 0 : (v > 255 ? 255 : v + 0.5));
    }
  }
}

/* percentile contrast stretch to full 0..255 */
static void stretch(uint8_t *b, int n)
{
  int hist[256] = {0}, i, lo = 0, hi = 255; long c = 0;
  for (i = 0; i < n; i++) hist[b[i]]++;
  for (i = 0; i < 256; i++) { c += hist[i]; if (c >= (long)(n * 0.02)) { lo = i; break; } }
  c = 0;
  for (i = 255; i >= 0; i--) { c += hist[i]; if (c >= (long)(n * 0.02)) { hi = i; break; } }
  if (hi <= lo) return;
  for (i = 0; i < n; i++) {
    int v = ((int)b[i] - lo) * 255 / (hi - lo);
    b[i] = v < 0 ? 0 : (v > 255 ? 255 : v);
  }
}

int main(int argc, char **argv)
{
  uint8_t base[N], raw[N], img[N];
  static uint8_t big[N * 36];
  double ppmm = 12.8;     /* measured: 6.4 px ridge period / 0.5 mm */
  int i, sc = 1, norm = 0;
  const char *e = getenv("NBIS_PPMM");

  if (getenv("NBIS_SCALE")) sc = atoi(getenv("NBIS_SCALE"));
  if (sc < 1 || sc > 6) sc = 1;
  norm = getenv("NBIS_NORM") != NULL;
  ppmm *= sc;
  if (e) ppmm = atof(e);
  if (argc < 3) { fprintf(stderr, "usage: %s <baseline> <frame>...\n", argv[0]); return 2; }
  if (rd(argv[1], base)) { fprintf(stderr, "bad baseline\n"); return 2; }
  printf("# nbis_count libfprint=%s scale=%d ppmm=%.2f norm=%d keep_perim=%d lfsparms=g_lfsparms_V2\n",
#ifdef NBIS_LIBFPRINT_VERSION
         NBIS_LIBFPRINT_VERSION,
#else
         "?",
#endif
         sc, ppmm, norm, getenv("NBIS_KEEP_PERIM") ? 1 : 0);

  for (i = 2; i < argc; i++) {
    MINUTIAE *minutiae = NULL;
    int *qmap = NULL, *dmap = NULL, *lcmap = NULL, *lfmap = NULL, *hcmap = NULL;
    unsigned char *bin = NULL;
    int mw, mh, bw, bh, bd, rc;
    LFSPARMS parms;

    uint8_t *use = img; int uw = W, uh = H;

    if (rd(argv[i], raw)) { printf("%s -1 read\n", argv[i]); continue; }
    flat_field(raw, base, img);
    if (norm) stretch(img, N);
    if (sc > 1) { upscale(img, W, H, sc, big); use = big; uw = W * sc; uh = H * sc; }

    parms = g_lfsparms_V2;
    parms.remove_perimeter_pts = getenv("NBIS_KEEP_PERIM") ? FALSE : TRUE;

    rc = get_minutiae(&minutiae, &qmap, &dmap, &lcmap, &lfmap, &hcmap,
                      &mw, &mh, &bin, &bw, &bh, &bd,
                      use, uw, uh, 8, ppmm, &parms);
    if (rc) printf("%s -1 rc=%d\n", argv[i], rc);
    else printf("%s %d ok\n", argv[i], minutiae ? minutiae->num : 0);

    if (minutiae) free_minutiae(minutiae);
    free(qmap); free(dmap); free(lcmap); free(lfmap); free(hcmap); free(bin);
  }
  return 0;
}
