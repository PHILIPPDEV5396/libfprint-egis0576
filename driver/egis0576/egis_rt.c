/* egis_rt.c — flat-memory runtime for the native Egis extractor/matcher.
 * Maps the DLL's .rdata (RO) and .data (RW) at their original VAs (contents
 * embedded, no external files) + a bump arena. egis_reset() rewinds the arena
 * and restores pristine .data so a long-running daemon (fprintd) does not leak
 * or carry state between operations. */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/mman.h>
#include "egis_rt.h"
#include "egis_blobs.h"

#undef malloc
#undef calloc
#undef realloc
#undef free

#define RDATA_VA 0x18006d000UL
#define DATA_VA  0x180092000UL
#define DATA_VSIZE 0xe500UL
#define RDATA_VSIZE 0x24cccUL
#define HEAP_VA  0x100000000000UL
#define HEAP_SZ  0x40000000UL

static uint8_t *heap_base, *heap_ptr, *heap_end;

/* bump-arena allocation registry (declared here so egis_reset can clear it) */
#define MAXALLOC (1 << 20)
static struct { uint64_t p, sz; } atab[MAXALLOC];
static int an;
static uint64_t asize(uint64_t p) {
    for (int i = an - 1; i >= 0; i--) if (atab[i].p == p) return atab[i].sz;
    return 0;
}

static void *map_blob(uint64_t va, uint64_t vsize, const unsigned char *blob,
                      uint64_t bloblen, int ro) {
    uint64_t msz = ((vsize > bloblen ? vsize : bloblen) + 0xfff) & ~0xfffUL;
    void *p = mmap((void *)va, msz, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    if (p == MAP_FAILED) {
        fprintf(stderr, "egis: mmap @ %#lx failed (address in use?)\n", (unsigned long)va);
        return NULL;
    }
    memcpy(p, blob, bloblen);
    if (ro) mprotect(p, msz, PROT_READ);
    return p;
}

int egis_init(void) {
    static int done = 0;
    if (!done) {
        if (!map_blob(RDATA_VA, RDATA_VSIZE, egis_rdata_blob, EGIS_RDATA_LEN, 1)) return -1;
        if (!map_blob(DATA_VA,  DATA_VSIZE,  egis_data_blob,  EGIS_DATA_LEN, 0)) return -1;
        void *h = mmap((void *)HEAP_VA, HEAP_SZ, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
        if (h == MAP_FAILED) { fprintf(stderr, "egis: heap mmap failed\n"); return -1; }
        heap_base = (uint8_t *)HEAP_VA; heap_end = heap_base + HEAP_SZ;
        done = 1;
    }
    egis_reset();
    return 0;
}

/* rewind the arena and restore pristine .data (fresh state per operation) */
void egis_reset(void) {
    heap_ptr = heap_base;
    an = 0;                 /* arena rewound: drop the stale alloc registry too */
    memcpy((void *)DATA_VA, egis_data_blob, EGIS_DATA_LEN);
    if (DATA_VSIZE > EGIS_DATA_LEN)
        memset((void *)(DATA_VA + EGIS_DATA_LEN), 0, DATA_VSIZE - EGIS_DATA_LEN);
}

void *e_malloc(uint64_t sz) {
    if (sz > HEAP_SZ) { fprintf(stderr, "egis: alloc %#lx too large\n", (unsigned long)sz); return NULL; }
    sz = (sz + 15) & ~15UL; if (!sz) sz = 16;
    if (heap_ptr + sz > heap_end) { fprintf(stderr, "egis: heap exhausted\n"); return NULL; }
    uint8_t *p = heap_ptr; heap_ptr += sz;
    if (an < MAXALLOC) { atab[an].p = (uint64_t)p; atab[an].sz = sz; an++; }
    return p;
}
void *e_calloc(uint64_t n, uint64_t sz) {
    if (n && sz && n > HEAP_SZ / sz) return NULL;   /* multiply-overflow guard */
    uint64_t t = n * sz; void *p = e_malloc(t);
    if (p) memset(p, 0, (t + 15) & ~15UL);
    return p;
}
void *e_realloc(void *op, uint64_t sz) {
    uint64_t old = (uint64_t)op;
    if (old && asize(old) >= ((sz + 15) & ~15UL)) return op;
    void *np = e_malloc(sz); if (!np) return NULL;
    memset(np, 0, (sz + 15) & ~15UL);
    if (old) { uint64_t o = asize(old); memcpy(np, op, sz < o ? sz : o); }
    return np;
}
void e_free(void *p) { (void)p; }

void *FUN_18004d200(void *d, const void *s, uint64_t n) { return memcpy(d, s, n); }
void *FUN_18004d980(void *d, int v, uint64_t n) { return memset(d, v, n); }
void FUN_180063a10() { }
