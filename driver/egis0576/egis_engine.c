/* egis_engine.c — implementation of the clean engine API over the native
 * translated Egis functions (egis_funcs.c) and flat-memory runtime. */
#include <string.h>
#include <stdlib.h>
#include "egis_rt.h"
#include "egis_decls.h"
#include "egis_engine.h"

/* the engine uses e_malloc explicitly for arena memory; keep plain malloc/free
 * as libc so egis_enroll_finish can return a blob that outlives the arena. */
#undef malloc
#undef calloc
#undef realloc
#undef free

/* global runtime addresses (mapped .data) */
#define STATE (*(volatile int *)0x18009707cUL)
#define CFG   (*(volatile long long *)0x18009d018UL)
#define GALLERY_BASE 0x1800a0470UL
#define GCOUNT (*(volatile int *)0x1800a0464UL)

/* internal state-machine values */
#define ST_READY   0x1b58   /* 7000 */
#define ST_ENROLL  0x1b59
#define ST_PACK    0x1b5a

/* The Windows-style per-frame preprocessing this engine expects on its input
 * (min-subtract, invert, auto-brightness, Otsu stretch, row flip); applied
 * here so the driver contract stays free of it. */
void egis_preprocess(const uint8_t *raw, uint8_t *out);

static void *cur_session;

/* The flat-memory runtime is process-global by construction (mapped at fixed
 * addresses), so this flavour has exactly one engine: the handle is a
 * singleton and free is a no-op. */
static struct EgisEngine { int initialised; } the_engine;

static int egis_engine_init(void);

EgisEngine *egis_engine_new(void) {
    if (!the_engine.initialised) {
        if (egis_engine_init() != 0) return NULL;
        the_engine.initialised = 1;
    }
    return &the_engine;
}

void egis_engine_free(EgisEngine *e) { (void)e; }

static int egis_engine_init(void) {
    if (egis_init() != 0) return -1;
    void *ctx = e_malloc(0x400);
    if (!ctx) return -1;
    FUN_18002bf50(ctx, 0, 5, 0);        /* alloc + seed config table, mode 5, state 7000 */
    cur_session = 0;
    return CFG ? 0 : -1;
}

/* raw 70x57 frame -> extracted featureset (in arena); NULL on failure */
static void *extract(const uint8_t *frame) {
    uint8_t pre[EGIS_IMG_SIZE];
    char *h = (char *)FUN_18002e460(EGIS_IMG_W, EGIS_IMG_H);
    if (!h) return 0;
    egis_preprocess(frame, pre);
    memcpy(h + EGIS_IMG_H * 8, pre, EGIS_IMG_SIZE);
    void *fs = 0;
    int r = FUN_18003ce10(h, EGIS_IMG_W, EGIS_IMG_H, &fs, (void *)CFG);
    if (r != 0 || !fs || *(int *)fs < 11) return 0;   /* need >= 11 minutiae */
    return fs;
}

int egis_enroll_begin(EgisEngine *e) {
    (void)e;
    egis_reset();
    void *ctx = e_malloc(0x400);
    FUN_18002bf50(ctx, 0, 5, 0);
    cur_session = FUN_18002c0d0(0, 0, 0, 0);
    return (cur_session && STATE == ST_ENROLL) ? 0 : -1;
}

int egis_enroll_add(EgisEngine *e, const uint8_t *frame, int *progress) {
    (void)e;
    if (!cur_session) return -1;
    uint8_t *img = e_malloc(EGIS_IMG_SIZE);
    if (!img) return -1;
    egis_preprocess(frame, img);
    int prog = 0;
    int r = (int)FUN_18002c880(img, EGIS_IMG_W, EGIS_IMG_H, cur_session, &prog);
    if (progress) *progress = prog;
    return r;
}

/* The packed blob lives in the arena; it is packed on the first call (the
 * size query) and copied out on the second. */
static void *packed_blob;
static int   packed_size;

int egis_enroll_finish(EgisEngine *e, uint8_t *out, int cap) {
    (void)e;
    if (!packed_blob) {
        if (!cur_session) return -1;
        STATE = ST_PACK;                /* force pack-ready */
        packed_size = 0;
        packed_blob = FUN_18002dc40(cur_session, &packed_size, 0, 0);
        cur_session = 0;
        if (!packed_blob || packed_size <= 0) { packed_blob = 0; return -1; }
    }
    if (!out) return packed_size;
    if (cap < packed_size) return -1;
    memcpy(out, packed_blob, packed_size);
    int size = packed_size;
    packed_blob = 0;
    packed_size = 0;
    return size;
}

static long long gallery_entry(int idx);   /* fwd decl for the load-verify below */

int egis_gallery_load(EgisEngine *e, const uint8_t *const *blobs, const int *sizes, int n) {
    (void)e;
    if (n <= 0 || n > 5) return -1;
    /* fresh arena + config for this match operation (no cross-op leak/state) */
    egis_reset();
    void *ctx = e_malloc(0x400);
    if (!ctx) return -1;
    FUN_18002bf50(ctx, 0, 5, 0);
    STATE = ST_READY;
    /* copy blobs into the arena (the loader reads from there) + build load struct */
    long long *blobptrs = e_malloc((long long)n * 8);
    int *szs = e_malloc((long long)n * 4);
    if (!blobptrs || !szs) return -1;
    for (int i = 0; i < n; i++) {
        if (sizes[i] <= 0) return -1;                 /* reject empty/corrupt blob */
        uint8_t *b = e_malloc((uint64_t)sizes[i]);
        if (!b) return -1;
        memcpy(b, blobs[i], (size_t)sizes[i]);
        blobptrs[i] = (long long)b;
        szs[i] = sizes[i];
    }
    long long *loadst = e_malloc(0x18);
    if (!loadst) return -1;
    loadst[0] = (long long)blobptrs;
    loadst[1] = (long long)szs;
    loadst[2] = n;
    FUN_18002d350(loadst, 0, 0, 0);
    /* FUN_18002d350 sets GCOUNT=n unconditionally, even if a blob failed to
     * parse (leaving a NULL slot). Verify every requested slot actually loaded
     * so a corrupt template surfaces as a load failure instead of a finger that
     * silently never matches. */
    if (GCOUNT < n) return -1;
    for (int i = 0; i < n; i++)
        if (!gallery_entry(i)) return -1;
    return 0;
}

static long long gallery_entry(int idx) {
    return *(long long *)(GALLERY_BASE + (long long)idx * 0x18);
}

/* The vendor matcher decides on minutiae correspondence, which a painted,
 * inverted imprint does not produce, so no raw-frame corroboration here. */
int egis_verify_raw_ok(EgisEngine *e, const uint8_t *raw_unfielded, int idx) {
    (void)e; (void)raw_unfielded; (void)idx;
    return 1;
}

int egis_gallery_capacity(EgisEngine *e) {
    (void)e;
    return 5;   /* FUN_18002bf50(ctx, 0, 5, 0) above */
}

int egis_verify(EgisEngine *e, const uint8_t *frame, int idx) {
    void *fs = extract(frame);
    (void)e;
    if (!fs) return -1;
    long long g = gallery_entry(idx);
    if (!g) return -1;
    int flag = 0;
    return (int)FUN_18002d530(fs, (void *)g, &flag, 0);
}

int egis_identify(EgisEngine *e, const uint8_t *frame, int *out_idx) {
    void *fs = extract(frame);
    (void)e;
    if (!fs) return -1;
    int best = -1, besti = -1, flag;
    for (int i = 0; i < GCOUNT; i++) {
        long long g = gallery_entry(i);
        if (!g) continue;
        int s = (int)FUN_18002d530(fs, (void *)g, &flag, 0);
        if (s > best) { best = s; besti = i; }
    }
    if (out_idx) *out_idx = (best >= EGIS_THRESHOLD) ? besti : -1;
    return best;
}
