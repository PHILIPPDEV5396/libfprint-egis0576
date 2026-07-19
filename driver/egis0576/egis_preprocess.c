/* ============================================================================
 * egis_preprocess.c
 *
 * EgisTec EH576 per-frame image preprocessing, byte-exact translated from the
 * Windows driver (via Ghidra decompilation of EgisTouchFP0576.dll).
 *
 * Public entry point:
 *     void egis_preprocess(const uint8_t *raw, uint8_t *out);
 *
 * Geometry (hard-coded for this reader): W=70 (0x46), H=57 (0x39),
 *                                        N=3990 (0xf96), 1 byte/pixel, row-major.
 *
 * Pipeline applied, in order (confirmed by prior RE):
 *     raw
 *       -> (1) per-frame min-subtract      FUN_180066e40 + FUN_180066600
 *       -> (2) invert (255 - x)            trivial
 *       -> (3) auto-brightness (mode 0)    FUN_1800657b0  (byte-exact gate)
 *       -> (4) Otsu contrast stretch(0x8c) FUN_18002a2f0  (+ FUN_18002a610)
 *       -> (5) vertical row-flip           trivial
 *   (bad-pixel substitution is intentionally OMITTED, per task.)
 *
 * Compiles standalone against only <stdint.h> and <string.h>.
 *
 * ----------------------------------------------------------------------------
 * NOTES ON FIDELITY
 * ----------------------------------------------------------------------------
 * Auto-brightness corner gate -- BYTE-EXACT.
 *   FUN_1800657b0's per-corner "qualification" is driven by a 10x5 ridge-
 *   COHERENCE map produced by FUN_18006a730 (structure-tensor estimator,
 *   FUN_180069410 + FUN_180069320), which converts Gxx/Gyy/Gxy sums to bytes via
 *   six sqrt/arctan lookup tables held in the DLL .rdata. Those tables were
 *   extracted byte-exact and the estimator translated literally -- see the
 *   companion egis_coherence_map.c, called via the extern below. The rest of the
 *   auto-brightness stage (resize, corner 8x8 sums, dark/bright binning, offset
 *   math, add-offset+clamp) is likewise byte-exact. For full-contact fingerprint
 *   frames the gate is effectively a no-op, matching the Windows behaviour; the
 *   machinery is exact for the cases where it does fire.
 *
 * UNRESOLVED CONSTANT -- bad-pixel calibration (only relevant if you re-enable
 *   bad-pixel handling, which this file does NOT):
 *       DAT_18009b085 : uint8_t bad_pixel[0xf96] (1==bad), runtime config blob
 *       DAT_18009b084 : uint8_t bad_replace,                runtime config
 *   These are device-stored config, not compile-time constants. Omitted here.
 *
 * Resolved (NOT image data, dropped from the translation):
 *   DAT_180092040 = /GS stack-canary cookie (__security_cookie).
 *   DAT_1800920bc = runtime SIMD lane count; selects a vectorized path that is
 *                   numerically identical to the scalar code (and never even
 *                   activates for this upscale), so it is irrelevant.
 *   FUN_18004d980 / FUN_18004d200 = memset / memcpy.
 * ==========================================================================*/

#include <stdint.h>
#include <string.h>

/* ---- geometry ---------------------------------------------------------- */
#define EGIS_W 70     /* 0x46 */
#define EGIS_H 57     /* 0x39 */
#define EGIS_N 3990   /* 0xf96 = EGIS_W * EGIS_H */

/* ---- tunables that ARE genuine driver constants ------------------------ */
#define EGIS_OTSU_TARGET   0x8c  /* target black level, 4th arg at call sites  */
#define EGIS_RESIZE_W      0xa0  /* 160: auto-brightness working width          */
#define EGIS_RESIZE_H      0x50  /* 80 : auto-brightness working height         */


/* ============================================================================
 * (1) PER-FRAME MIN-SUBTRACT
 *     FUN_180066e40 (min stat, bad-pixels excluded) + FUN_180066600 (subtract).
 *     Bad-pixel path omitted => min is taken over ALL pixels.
 * ==========================================================================*/

/* FUN_180066e40, min-only, no bad-pixel map: min over all N pixels. */
static uint8_t egis_frame_min(const uint8_t *img, int n)
{
    uint8_t vmin = 0xFF;              /* local_68 init 0xff */
    for (int i = 0; i < n; i++)
        if (img[i] < vmin) vmin = img[i];
    return vmin;
}

/* FUN_180066600: subtract per-frame min from every pixel.
 * Original is signed-char arithmetic (`*p = *p - (char)min`); the low 8 bits are
 * identical to plain (uint8_t)(p - min). This is 8-bit WRAP, not a clamp -- but
 * since min is the frame minimum, real pixels never underflow. */
static void egis_min_subtract(uint8_t *img, int n, uint8_t vmin)
{
    for (int i = 0; i < n; i++)
        img[i] = (uint8_t)((int8_t)img[i] - (int8_t)vmin);
}


/* ============================================================================
 * (3) AUTO-BRIGHTNESS  (mode 0)  -- FUN_1800657b0
 *     Helpers: FUN_18002a870 (add-offset+clamp), FUN_18002b2b0 (area resize),
 *              FUN_18006a730 coherence map  (byte-exact, egis_coherence_map.c).
 * ==========================================================================*/

/* FUN_18002a870 -- add signed offset to every pixel, clamp to [0,255]. EXACT. */
static void egis_add_offset(uint8_t *img, int w, int h, int off)
{
    long n = (long)w * h;
    for (long i = 0; i < n; ++i) {
        int v = (int)img[i] + off;
        if (v < 0xff) { if (v < 1) v = 0; img[i] = (uint8_t)v; }
        else          { img[i] = 0xff; }
    }
}

/* FUN_18002b2b0 -- in-place separable area-weighted resample. EXACT.
 * Two passes: vertical (Hsrc->Hdst) then horizontal (Wsrc->Wdst). Each output
 * sample distributes a total weight of `scale = (S<<10)/D` across consecutive
 * source samples (first capped by its coverage, rest by 1024), then
 * out = (sum(w_i*src_i) + scale/2) / scale.
 *
 * The DLL's SSE variant (guarded by DAT_1800920bc) is numerically identical and
 * never activates for an upscale (needs >7 taps; upscaling yields <=2), so this
 * scalar form is byte-exact. malloc() in the original replaced by a static
 * scratch sized for this driver's fixed EGIS_RESIZE_W x EGIS_RESIZE_H target
 * (NOTE: not reentrant, which matches the single-frame driver usage). */
#define EGIS_RS_MAXTAPS 8
static int egis_rs_weights(int scale, int pos, int *w /*[EGIS_RS_MAXTAPS]*/)
{
    int s0  = pos >> 10;                 /* pos>=0 -> floor */
    int cap = (s0 + 1) * 1024 - pos;     /* coverage of first source sample */
    int rem = scale, k = 0;
    do {                                 /* runs >=1 (orig do/while) */
        int take = (rem < cap) ? rem : cap;
        if (k < EGIS_RS_MAXTAPS) w[k] = take;
        ++k;
        rem -= take;
        cap = 1024;
    } while (rem > 0);
    return k;                            /* tap count */
}

static void egis_resize(uint8_t *img, int wsrc, int hsrc, int wdst, int hdst)
{
    static uint8_t tmp[EGIS_RESIZE_H * EGIS_W]; /* [hdst][wsrc], fixed geometry */
    int w[EGIS_RS_MAXTAPS];

    /* pass 1: vertical, Hsrc -> Hdst, width stays wsrc */
    int scaleY = (hsrc << 10) / hdst;
    for (int r = 0; r < hdst; ++r) {
        int pos = r * scaleY;
        int s0  = pos >> 10;
        int nt  = egis_rs_weights(scaleY, pos, w);
        for (int x = 0; x < wsrc; ++x) {
            int acc = 0;
            for (int i = 0; i < nt; ++i)
                acc += w[i] * (int)img[(size_t)(s0 + i) * wsrc + x];
            tmp[(size_t)r * wsrc + x] = (uint8_t)((acc + scaleY / 2) / scaleY);
        }
    }

    /* pass 2: horizontal, Wsrc -> Wdst, height stays hdst */
    int scaleX = (wsrc << 10) / wdst;
    for (int r = 0; r < hdst; ++r) {
        for (int c = 0; c < wdst; ++c) {
            int pos = c * scaleX;
            int s0  = pos >> 10;
            int nt  = egis_rs_weights(scaleX, pos, w);
            int acc = 0;
            for (int i = 0; i < nt; ++i)
                acc += w[i] * (int)tmp[(size_t)r * wsrc + (s0 + i)];
            img[(size_t)r * wdst + c] = (uint8_t)((acc + scaleX / 2) / scaleX);
        }
    }
}

extern void egis_coherence_map(uint8_t *buf, int w, int h, uint8_t *map_out); /* real Windows estimator, egis_coherence_map.c */

/* sum an 8x8 block of `img` anchored at (r0,c0). */
static long egis_block_sum8x8(const uint8_t *img, int w, int r0, int c0)
{
    long s = 0;
    for (int r = 0; r < 8; ++r)
        for (int c = 0; c < 8; ++c)
            s += img[(size_t)(r0 + r) * w + (c0 + c)];
    return s;
}

/* FUN_1800657b0 (mode 0) -- DC brightness leveling. Top-level math EXACT;
 * corner gate depends on egis_coherence_map (byte-exact). Returns applied offset. */
static uint32_t egis_auto_brightness(uint8_t *img, int w, int h)
{
    /* resized working copy + coherence corner metrics */
    static uint8_t work[EGIS_RESIZE_W * EGIS_RESIZE_H];
    memcpy(work, img, (size_t)w * h);
    egis_resize(work, w, h, EGIS_RESIZE_W, EGIS_RESIZE_H);   /* FUN_18002b2b0 */

    uint8_t map[(EGIS_RESIZE_W / 16) * (EGIS_RESIZE_H / 16)]; /* 10x5 = 50 */
    egis_coherence_map(work, EGIS_RESIZE_W, EGIS_RESIZE_H, map);

    /* corner metrics: indices 0, 9, 40, 49 of the 10x5 map (TL,TR,BL,BR) */
    const uint8_t m[4]  = { map[0], map[9], map[40], map[49] };
    const int     r0[4] = { 0, 0, h - 8, h - 8 };
    const int     c0[4] = { 0, w - 8, 0, w - 8 };

    long sum[2] = { 0, 0 };   /* local_3258 */
    int  cnt[2] = { 0, 0 };   /* local_3250 */

    for (int k = 0; k < 4; ++k) {
        int bin;
        if (m[k] < 0x3c)      bin = 0;   /* dark   */
        else if (m[k] > 0x78) bin = 1;   /* bright */
        else                  continue;  /* 0x3c..0x78 -> not counted */
        sum[bin] += egis_block_sum8x8(img, w, r0[k], c0[k]);
        cnt[bin] += 0x40;                /* 64 pixels per 8x8 block */
    }

    /* offset decision (param_4 == 0 branch) */
    if (cnt[0] > 0) {
        int dark   = (int)(sum[0] / cnt[0]);
        int bright = (cnt[1] > 0) ? (int)(sum[1] / cnt[1]) : 0;
        uint32_t off = (uint32_t)(0xff - dark);
        if ((int)off <= 0xff - bright) {                 /* dark >= bright */
            egis_add_offset(img, w, h, (int)off);        /* FUN_18002a870 */
            return off;
        }
    }
    return 0;   /* no adjustment applied */
}


/* ============================================================================
 * (4) OTSU CONTRAST STRETCH  -- FUN_18002a2f0 (+ threshold FUN_18002a610). EXACT.
 * ==========================================================================*/

/* FUN_18002a610 -- Otsu threshold over pixels < 0xf0. Pixels >= 0xf0 are
 * excluded from the histogram AND removed from the foreground total. The
 * between-class variance uses 64-bit SIGNED products but UNSIGNED 64-bit
 * divisions/accumulate/compare (matching `ulonglong` + `uVar12 < uVar7`); this
 * wrap/compare behaviour is load-bearing and reproduced verbatim. */
static int egis_otsu_threshold(const uint8_t *img, int n)
{
    int hist[256];
    for (int i = 0; i < 256; i++) hist[i] = 0;

    int sum_all    = 0;   /* iVar13 */
    int high_count = 0;   /* iVar6  */
    for (int i = 0; i < n; i++) {
        uint8_t p = img[i];
        if (p < 0xf0) { sum_all += (unsigned int)p; hist[p] += 1; }
        else          { high_count += 1; }
    }

    uint32_t best_thr = 0;   /* uVar14 */
    uint64_t best_var = 0;   /* uVar12 */
    int cum_count = 0;       /* iVar5 */
    int cum_wsum  = 0;       /* iVar8 */

    for (uint32_t t = 0; t < 0x100; t++) {
        int hcnt = hist[t];
        if (hcnt != 0) {
            cum_count += hcnt;
            cum_wsum  += hcnt * (int)t;
            int fg_rest = (n - high_count) - cum_count;   /* iVar2 */
            if (cum_count != 0 && fg_rest != 0) {
                int64_t below = (int64_t)cum_wsum;               /* lVar10 */
                int64_t above = (int64_t)(sum_all - cum_wsum);   /* lVar4  */
                uint64_t var =
                      (uint64_t)(below * below * (int64_t)fg_rest)
                          / (uint64_t)(int64_t)cum_count
                    + (uint64_t)((int64_t)cum_count * above * above)
                          / (uint64_t)(int64_t)fg_rest
                    + (uint64_t)(below * above * (int64_t)-2);
                if (best_var < var) { best_var = var; best_thr = t; }
            }
        }
    }
    return (int)(best_thr & 0xff);
}

/* FUN_18002a2f0 -- Otsu contrast stretch. gain is Q4 (16x). Per pixel: invert,
 * scale, round (t = (0xff-p)*gain16 + 8; signed >>4 with round-toward-zero for
 * negatives), then clamp/re-invert (t>=255 -> 0; t<0 -> 255; else 255-t). */
static void egis_otsu_stretch(uint8_t *img, int w, int h, int target)
{
    int thr = egis_otsu_threshold(img, w * h) & 0xff;

    int gain16;
    if (thr == 0xff) gain16 = 0x10;
    else             gain16 = ((0xff - target) * 0x10) / (0xff - thr);

    int n = w * h;
    for (int i = 0; i < n; i++) {
        int t = (0xff - (unsigned int)img[i]) * gain16 + 8;
        t = (t + ((t >> 31) & 0xf)) >> 4;   /* divide-by-16, round toward zero */
        if (t < 0xff) {
            if (t < 0) img[i] = 0xff;
            else       img[i] = (uint8_t)(0xff - t);
        } else {
            img[i] = 0;
        }
    }
}


/* ============================================================================
 * (5) VERTICAL ROW-FLIP  -- reverse row order, 70-byte rows.
 * ==========================================================================*/
static void egis_row_flip(uint8_t *img, int w, int h)
{
    uint8_t rowbuf[EGIS_W];
    for (int r = 0; r < h / 2; ++r) {
        uint8_t *a = img + (size_t)r * w;
        uint8_t *b = img + (size_t)(h - 1 - r) * w;
        memcpy(rowbuf, a, w);
        memcpy(a, b, w);
        memcpy(b, rowbuf, w);
    }
}


/* ============================================================================
 * PUBLIC ENTRY POINT
 * ==========================================================================*/
void egis_preprocess(const uint8_t *raw, uint8_t *out)
{
    /* work on a private copy in `out` */
    memcpy(out, raw, EGIS_N);

    /* (1) per-frame min-subtract */
    uint8_t vmin = egis_frame_min(out, EGIS_N);
    egis_min_subtract(out, EGIS_N, vmin);

    /* (2) invert */
    for (int i = 0; i < EGIS_N; i++)
        out[i] = (uint8_t)(0xff - out[i]);

    /* (3) auto-brightness (mode 0) -- byte-exact coherence-gated global shift */
    egis_auto_brightness(out, EGIS_W, EGIS_H);

    /* (4) Otsu contrast stretch, target 0x8c */
    egis_otsu_stretch(out, EGIS_W, EGIS_H, EGIS_OTSU_TARGET);

    /* (5) vertical row-flip */
    egis_row_flip(out, EGIS_W, EGIS_H);
}
