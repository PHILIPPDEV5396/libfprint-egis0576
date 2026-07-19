/* =========================================================================
 * EgisTec EH576 ridge-coherence estimator  (self-contained)
 *
 * Byte-exact Linux port of the Windows DLL coherence gate used by the mode-0
 * auto-brightness stage (FUN_1800657b0). It turns a resized 160x80 (0xa0 x 0x50)
 * working image into a 10x5 (=50-byte) coherence map. Auto-brightness reads the
 * four CORNER cells (indices 0, 9, 40, 49).
 *
 * This replaces the previous egis_coherence_map() STUB (which returned a neutral
 * 0x60 for every cell).
 *
 * SOURCE OF TRUTH
 *   Ghidra decompilation of the Egis Windows driver (EgisTouchFP0576.dll)
 *       FUN_18006a730 (top; param4=NULL, param5=3)  -> coherence map copied back
 *       FUN_18002a2c0 (global mean; early-out mean > 0xf4)
 *       FUN_180069e50 (3x3 box mean, rad 1)
 *       FUN_18006a010 (box mean, win=4  -> rad 4)
 *       FUN_18006a2d0 (0x40 local-contrast normalize)
 *       FUN_180069410 (per-16x16 structure tensor + orientation + coherence)
 *       FUN_180069320 (arctan orientation)
 *       FUN_180069cc0 (thr 0x50; 3x3 majority filter on coherence -> orientation)
 *
 * LOOKUP TABLES (extracted byte-exact from the DLL's .rdata section, 150732 bytes;
 * base VA 0x18006d000, file_off = VA - 0x18006d000). All are uint8_t elements.
 *
 *   EGIS_ARCTAN   VA 0x1800771c0  off 0xa1c0  len 129  idx 0..128
 *   EGIS_ISQRT_T0 VA 0x180077250  off 0xa250  len 256  v<0x100 : idx=v
 *   EGIS_ISQRT_T1 VA 0x180077340  off 0xa340  len 64   v<0x400 : idx=v>>4
 *   EGIS_ISQRT_T2 VA 0x180077360  off 0xa360  len 128  v<0x1000: idx=v>>5
 *   EGIS_ISQRT_T3 VA 0x1800773a0  off 0xa3a0  len 256  v<0x4000: idx=v>>6
 *   EGIS_ISQRT_T4 VA 0x180077420  off 0xa420  len 509  v<0xfe01: idx=v>>7
 *
 * The five sqrt tables are one physically-contiguous run in .rdata (0xa250..0xa61c);
 * each table's base pointer is placed so base+min_used_index continues the previous
 * table's accessed range, so emitting each as an independent full array is faithful
 * to the original base-pointer+shift indexing.
 *
 * REMAINING APPROXIMATION
 *   None affecting the returned coherence map. The orientation map produced by
 *   FUN_180069320 / FUN_180069410 and consumed by the FUN_180069cc0 majority
 *   filter is computed but discarded (mode 3 returns coherence only), so any
 *   residual imprecision in egis_orientation() cannot reach map_out[].
 *   Build with -fwrapv (two's-complement wrap) so the 32-bit structure-tensor
 *   accumulators wrap identically to the DLL on pathological (non-fingerprint)
 *   input; real smoothed frames never overflow.
 *
 * Compiles standalone against only <stdint.h> / <string.h> (no libc math).
 * ========================================================================= */

#include <stdint.h>
#include <string.h>

/* ---- fixed EH576 auto-brightness working geometry ---------------------- */
#define EGIS_RESIZE_W   0xa0    /* 160 */
#define EGIS_RESIZE_H   0x50    /*  80 */
#define EGIS_BLK        16
#define EGIS_BLKW       (EGIS_RESIZE_W / EGIS_BLK)   /* 10 */
#define EGIS_BLKH       (EGIS_RESIZE_H / EGIS_BLK)   /*  5 */

/* ============================ LOOKUP TABLES ============================== */

/* arctan/orientation LUT (FUN_180069320): mn=min(|dx|,|dy|), mx=max(...);
 * index = (mn/2 + mx*0x80)/mx in [0,128]. Reflected branch uses 0x1e0-LUT[i]. */
static const uint8_t EGIS_ARCTAN[129] = {
    0,2,5,7,10,12,14,17,19,21,24,26,29,31,33,36,
    38,40,43,45,47,50,52,54,57,59,61,64,66,68,70,73,
    75,77,79,82,84,86,88,90,93,95,97,99,101,103,105,108,
    110,112,114,116,118,120,122,124,126,128,130,132,134,136,138,140,
    142,144,145,147,149,151,153,155,157,158,160,162,164,165,167,169,
    171,172,174,176,177,179,181,182,184,186,187,189,190,192,194,195,
    197,198,200,201,203,204,206,207,209,210,211,213,214,216,217,218,
    220,221,222,224,225,226,228,229,230,231,233,234,235,236,238,239,
    240
};

static const uint8_t EGIS_ISQRT_T0[256] = {
    0,1,1,2,2,2,2,3,3,3,3,3,3,4,4,4,
    4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,
    6,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,
    9,9,9,9,9,9,9,9,9,9,9,10,10,10,10,10,
    10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,11,
    11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
    11,11,11,11,11,12,12,12,12,12,12,12,12,12,12,12,
    12,12,12,12,12,12,12,12,12,12,12,12,12,13,13,13,
    13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
    13,13,13,13,13,13,13,14,14,14,14,14,14,14,14,14,
    14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,
    14,14,14,15,15,15,15,15,15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
    15,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16
};

static const uint8_t EGIS_ISQRT_T1[64] = {
    15,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
    16,16,17,17,18,18,19,19,20,20,20,21,21,22,22,22,
    23,23,23,24,24,24,25,25,25,26,26,26,27,27,27,27,
    28,28,28,29,29,29,29,30,30,30,30,31,31,31,31,32
};

static const uint8_t EGIS_ISQRT_T2[128] = {
    23,23,23,24,24,24,25,25,25,26,26,26,27,27,27,27,
    28,28,28,29,29,29,29,30,30,30,30,31,31,31,31,32,
    32,32,33,33,34,34,35,35,36,36,37,37,38,38,38,39,
    39,40,40,40,41,41,42,42,42,43,43,43,44,44,45,45,
    45,46,46,46,47,47,47,48,48,48,49,49,49,50,50,50,
    51,51,51,52,52,52,52,53,53,53,54,54,54,55,55,55,
    55,56,56,56,57,57,57,57,58,58,58,59,59,59,59,60,
    60,60,60,61,61,61,61,62,62,62,62,63,63,63,63,64
};

static const uint8_t EGIS_ISQRT_T3[256] = {
    45,46,46,46,47,47,47,48,48,48,49,49,49,50,50,50,
    51,51,51,52,52,52,52,53,53,53,54,54,54,55,55,55,
    55,56,56,56,57,57,57,57,58,58,58,59,59,59,59,60,
    60,60,60,61,61,61,61,62,62,62,62,63,63,63,63,64,
    64,64,65,65,66,66,67,67,68,68,69,69,70,70,71,71,
    72,72,72,73,73,74,74,75,75,75,76,76,77,77,78,78,
    78,79,79,80,80,80,81,81,82,82,82,83,83,84,84,84,
    85,85,85,86,86,87,87,87,88,88,88,89,89,89,90,90,
    91,91,91,92,92,92,93,93,93,94,94,94,95,95,95,96,
    96,96,97,97,97,98,98,98,99,99,99,100,100,100,101,101,
    101,102,102,102,102,103,103,103,104,104,104,105,105,105,106,106,
    106,106,107,107,107,108,108,108,109,109,109,109,110,110,110,111,
    111,111,111,112,112,112,113,113,113,113,114,114,114,115,115,115,
    115,116,116,116,116,117,117,117,118,118,118,118,119,119,119,119,
    120,120,120,121,121,121,121,122,122,122,122,123,123,123,123,124,
    124,124,124,125,125,125,125,126,126,126,126,127,127,127,127,128
};

static const uint8_t EGIS_ISQRT_T4[509] = {
    91,91,91,92,92,92,93,93,93,94,94,94,95,95,95,96,
    96,96,97,97,97,98,98,98,99,99,99,100,100,100,101,101,
    101,102,102,102,102,103,103,103,104,104,104,105,105,105,106,106,
    106,106,107,107,107,108,108,108,109,109,109,109,110,110,110,111,
    111,111,111,112,112,112,113,113,113,113,114,114,114,115,115,115,
    115,116,116,116,116,117,117,117,118,118,118,118,119,119,119,119,
    120,120,120,121,121,121,121,122,122,122,122,123,123,123,123,124,
    124,124,124,125,125,125,125,126,126,126,126,127,127,127,127,128,
    128,128,129,129,130,130,131,131,132,132,133,133,134,134,135,135,
    136,136,137,137,138,138,139,139,139,140,140,141,141,142,142,143,
    143,144,144,144,145,145,146,146,147,147,148,148,148,149,149,150,
    150,151,151,151,152,152,153,153,153,154,154,155,155,156,156,156,
    157,157,158,158,158,159,159,160,160,160,161,161,162,162,162,163,
    163,164,164,164,165,165,166,166,166,167,167,167,168,168,169,169,
    169,170,170,170,171,171,172,172,172,173,173,173,174,174,175,175,
    175,176,176,176,177,177,177,178,178,179,179,179,180,180,180,181,
    181,181,182,182,182,183,183,183,184,184,185,185,185,186,186,186,
    187,187,187,188,188,188,189,189,189,190,190,190,191,191,191,192,
    192,192,193,193,193,194,194,194,195,195,195,196,196,196,197,197,
    197,198,198,198,199,199,199,200,200,200,200,201,201,201,202,202,
    202,203,203,203,204,204,204,205,205,205,206,206,206,206,207,207,
    207,208,208,208,209,209,209,210,210,210,210,211,211,211,212,212,
    212,213,213,213,213,214,214,214,215,215,215,216,216,216,216,217,
    217,217,218,218,218,219,219,219,219,220,220,220,221,221,221,221,
    222,222,222,223,223,223,223,224,224,224,225,225,225,225,226,226,
    226,227,227,227,227,228,228,228,229,229,229,229,230,230,230,230,
    231,231,231,232,232,232,232,233,233,233,234,234,234,234,235,235,
    235,235,236,236,236,237,237,237,237,238,238,238,238,239,239,239,
    239,240,240,240,241,241,241,241,242,242,242,242,243,243,243,243,
    244,244,244,244,245,245,245,246,246,246,246,247,247,247,247,248,
    248,248,248,249,249,249,249,250,250,250,250,251,251,251,251,252,
    252,252,252,253,253,253,253,254,254,254,254,255,255
};

/* Piecewise integer-sqrt, byte-exact reproduction of the block inside
 * FUN_180069410 (LAB_180069b4f / LAB_180069b62): table select by magnitude
 * band, then a single +/-1 nearest-integer refinement, then saturate.
 * `v` is the coherence numerator (== iVar30 in the decomp). Returns 0..255.
 *
 * NOTE (assembly fix): both halves of the DLL's lookup -- the table read AND
 * the +/-1 refinement -- live here, so callers must NOT refine again. */
static int egis_isqrt_lut(unsigned v)
{
    unsigned r, r1;

    if (v >= 0xfe01u)          /* saturate: uVar28 = 0xff (no LUT) */
        return 0xff;

    if      (v < 0x100u)   r = EGIS_ISQRT_T0[v];        /* idx = v    */
    else if (v < 0x400u)   r = EGIS_ISQRT_T1[v >> 4];   /* idx = v>>4 */
    else if (v < 0x1000u)  r = EGIS_ISQRT_T2[v >> 5];   /* idx = v>>5 */
    else if (v < 0x4000u)  r = EGIS_ISQRT_T3[v >> 6];   /* idx = v>>6 */
    else                   r = EGIS_ISQRT_T4[v >> 7];   /* idx = v>>7 */

    /* refine: (r+1)^2 - v < v - r^2  ->  pick r+1  (decomp LAB_180069b62) */
    r1 = r + 1u;
    if ((int)(r1 * r1 - v) < (int)(v - r * r))
        r = r1;

    return (int)(r & 0xff);
}

/* ========================= ESTIMATOR HELPERS ============================ */

/* FUN_18002a2c0(img,W,H,W,0): global mean = sum(all px)/(W*H). */
static int egis_global_mean(const uint8_t *img, int w, int h)
{
    long sum = 0, n = (long)w * h;
    for (long i = 0; i < n; ++i) sum += img[i];
    return (int)(sum / n);
}

/* FUN_180069e50 (rad 1) and FUN_18006a010 (rad 4): 2-D clamped box mean with a
 * single truncating division by the in-window pixel count. dst must not alias src. */
static void egis_box_mean(uint8_t *dst, const uint8_t *src, int w, int h, int rad)
{
    for (int r = 0; r < h; ++r) {
        int r0 = (r - rad < 0) ? 0 : r - rad;
        int r1 = (r + rad > h - 1) ? h - 1 : r + rad;
        for (int c = 0; c < w; ++c) {
            int c0 = (c - rad < 0) ? 0 : c - rad;
            int c1 = (c + rad > w - 1) ? w - 1 : c + rad;
            int s = 0, n = 0;
            for (int y = r0; y <= r1; ++y)
                for (int x = c0; x <= c1; ++x) { s += src[(size_t)y * w + x]; ++n; }
            dst[(size_t)r * w + c] = (uint8_t)(s / n);
        }
    }
}

/* FUN_18006a2d0 (param5 = 0x40): adaptive local-contrast normalize.
 * p3 = 3x3 mean (pixel to stretch), p4 = box-9 mean (centre); avgD = rad-64
 * box mean of |p4-p3|. lower = max(0,centre-avgD), upper = min(255,centre+avgD);
 * stretch p3 from [lower,upper] to [0,255]; upper==lower -> pass p3 through. */
static void egis_combine(uint8_t *out, const uint8_t *p3, const uint8_t *p4, int w, int h)
{
    const int rad = 0x40;
    for (int r = 0; r < h; ++r) {
        int r0 = (r - rad < 0) ? 0 : r - rad, r1 = (r + rad > h - 1) ? h - 1 : r + rad;
        for (int c = 0; c < w; ++c) {
            int c0 = (c - rad < 0) ? 0 : c - rad, c1 = (c + rad > w - 1) ? w - 1 : c + rad;
            int sumD = 0, cnt = 0;
            for (int y = r0; y <= r1; ++y)
                for (int x = c0; x <= c1; ++x) {
                    int d = (int)p4[(size_t)y*w+x] - (int)p3[(size_t)y*w+x];
                    if (d < 0) d = -d;
                    sumD += d; ++cnt;
                }
            int avgD = sumD / cnt;
            uint8_t avg = (uint8_t)avgD, centre = p4[(size_t)r*w+c];
            uint8_t lower = (centre < avg) ? 0 : (uint8_t)(centre - avg);
            uint8_t upper = ((int)(0xff - (avgD & 0xff)) < (int)centre)
                          ? 0xff : (uint8_t)(centre + avg);
            uint8_t inp = p3[(size_t)r*w+c], res = inp;
            if ((uint8_t)(upper - lower) != 0) {
                if (lower < inp)
                    res = (inp < upper)
                        ? (uint8_t)(((int)((uint32_t)inp - (uint32_t)lower) * 0xff)
                                    / (int)(uint32_t)(uint8_t)(upper - lower))
                        : 0xff;
                else res = 0;
            }
            out[(size_t)r*w+c] = res;
        }
    }
}

/* FUN_180069320: orientation from (Sxx-Syy, 2*Sxy) via arctan LUT. */
static uint32_t egis_orientation(uint32_t p1, uint32_t p2)
{
    if (p1 == 0) return (0 < (int)p2) ? 0x5a : 0x1e;
    int a1 = (int)p1; if (a1 < 0) a1 = -a1;
    int a2 = (int)p2; if (a2 < 0) a2 = -a2;
    uint32_t u = (a1 < a2) ? 0x1e0 - (uint32_t)EGIS_ARCTAN[(a2/2 + a1*0x80)/a2]
                           :         (uint32_t)EGIS_ARCTAN[(a1/2 + a2*0x80)/a1];
    int t = (int)(u + 8);
    u = (uint32_t)((t + ((t >> 31) & 0xf)) >> 4);
    if ((int)p1 < 0) {
        if ((int)p2 >= 0) { if ((uint8_t)u == 0) return u; return 0x78 - (u & 0xff); }
        return u;
    }
    if ((int)p2 < 0) return 0x3c - (u & 0xff);
    return (u & 0xff) + 0x3c;
}

/* FUN_180069cc0 (thr 0x50): 3x3 block majority; reads coherence, writes 0xff into
 * the ORIENTATION map only -> no effect on the mode-3 output (kept for fidelity). */
static void egis_majority(uint8_t *orient, const uint8_t *coh, int bw, int bh, uint8_t thr)
{
    for (int r = 0; r < bh; ++r) {
        int r0 = (r < 1) ? 0 : r - 1, r1 = (r <= bh - 2) ? r + 1 : bh - 1;
        for (int c = 0; c < bw; ++c) {
            int c0 = (c < 1) ? 0 : c - 1, c1 = (c <= bw - 2) ? c + 1 : bw - 1;
            int cnt = 0, hi = 0;
            for (int y = r0; y <= r1; ++y)
                for (int x = c0; x <= c1; ++x) { ++cnt; if (thr < coh[(size_t)y*bw+x]) ++hi; }
            if (hi <= cnt / 2) orient[(size_t)r*bw+c] = 0xff;
        }
    }
}

/* ============================ PUBLIC ENTRY ============================== */

/* FUN_18006a730 (param4 == NULL, param5 == 3): writes the 10x5 coherence map to
 * map_out. On the mean > 0xf4 early-out the DLL leaves the resized image
 * untouched and the caller reads its flat bytes 0/9/40/49 as the "map". */
void egis_coherence_map(uint8_t *buf /*160x80*/, int w, int h, uint8_t *map_out)
{
    const int blkW = w / EGIS_BLK, blkH = h / EGIS_BLK;

    if (egis_global_mean(buf, w, h) > 0xf4) {          /* mean early-out */
        for (int i = 0; i < blkW * blkH; ++i) map_out[i] = buf[i];
        return;
    }

    static uint8_t b3[EGIS_RESIZE_W * EGIS_RESIZE_H];
    static uint8_t b4[EGIS_RESIZE_W * EGIS_RESIZE_H];
    static uint8_t bn[EGIS_RESIZE_W * EGIS_RESIZE_H];

    egis_box_mean(b3, buf, w, h, 1);     /* FUN_180069e50 */
    egis_box_mean(b4, b3,  w, h, 4);     /* FUN_18006a010 */
    egis_combine (bn, b3, b4, w, h);     /* FUN_18006a2d0 */
    egis_box_mean(b3, bn,  w, h, 1);     /* FUN_180069e50 */

    int gxx[EGIS_BLKH][EGIS_BLKW], gyy[EGIS_BLKH][EGIS_BLKW], gxy[EGIS_BLKH][EGIS_BLKW];
    for (int br = 0; br < blkH; ++br) {
        int ys = (br == 0) ? 1 : EGIS_BLK * br;
        int ye = (EGIS_BLK * (br + 1) == h) ? h - 1 : EGIS_BLK * (br + 1);
        for (int bc = 0; bc < blkW; ++bc) {
            int xs = (bc == 0) ? 1 : EGIS_BLK * bc;
            int xe = (EGIS_BLK * (bc + 1) == w) ? w - 1 : EGIS_BLK * (bc + 1);
            int axx = 0, ayy = 0, axy = 0;
            for (int y = ys; y < ye; ++y) {
                const uint8_t *rm1 = b3 + (size_t)(y-1)*w, *r0 = b3 + (size_t)y*w,
                              *rp1 = b3 + (size_t)(y+1)*w;
                for (int x = xs; x < xe; ++x) {
                    int tl=rm1[x-1],tc=rm1[x],tr=rm1[x+1], ml=r0[x-1],mr=r0[x+1],
                        bl=rp1[x-1],bcp=rp1[x],brp=rp1[x+1];
                    int gx = (brp-tl) + 4*(mr-ml) + (tr-bl);   /* Sobel [1,4,1] */
                    int gy = (brp-tl) + 4*(bcp-tc) - (tr-bl);
                    axx += gx*gx; ayy += gy*gy; axy += gx*gy;
                }
            }
            gxx[br][bc]=axx; gyy[br][bc]=ayy; gxy[br][bc]=axy;
        }
    }

    uint8_t orient[EGIS_BLKW * EGIS_BLKH];
    for (int br = 0; br < blkH; ++br) {
        int R0 = (br < 1) ? 0 : br - 1, R1 = (br <= blkH - 2) ? br + 1 : blkH - 1;
        for (int bc = 0; bc < blkW; ++bc) {
            int C0 = (bc < 1) ? 0 : bc - 1, C1 = (bc <= blkW - 2) ? bc + 1 : blkW - 1;
            int sxx=0, syy=0, sxy=0;
            for (int rr = R0; rr <= R1; ++rr)
                for (int cc = C0; cc <= C1; ++cc) { sxx+=gxx[rr][cc]; syy+=gyy[rr][cc]; sxy+=gxy[rr][cc]; }
            int Sxx=(sxx+0x480)/0x900, Syy=(syy+0x480)/0x900, Sxy=(sxy+0x480)/0x900;
            orient[br*blkW+bc] = (uint8_t)egis_orientation((uint32_t)(Sxx-Syy),(uint32_t)(Sxy*2));

            int A=Sxx/0x24, B=Syy/0x24, q1=A>>2, q2=B>>2;
            int diff=q1-q2, ssum=q1+q2;
            int b2=((Sxy/0x24)>>2)*2, s2=(ssum*ssum)/0xfe01;
            unsigned ratio;
            if (A + B < 0x32*0x32 || s2 == 0)      /* low-energy gate -> ratio 0 */
                ratio = 0;
            else
                ratio = (unsigned)((diff*diff + b2*b2)/s2);
            map_out[br*blkW+bc] = (uint8_t)egis_isqrt_lut(ratio);  /* lookup+refine+sat */
        }
    }
    egis_majority(orient, map_out, blkW, blkH, 0x50);  /* orientation-only, discarded */
}

/* ============================ SELF-TEST ================================= */
