/* egis_rt.h — runtime for the flat-memory native reimplementation of the
 * EgisTec EH576 feature extractor + matcher, translated verbatim from the
 * decompiled Windows driver. The .rdata/.data sections are mapped at their
 * original virtual addresses so Ghidra's absolute pointer arithmetic compiles
 * and runs natively. No Windows, no emulator. */
#ifndef EGIS_RT_H
#define EGIS_RT_H
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* Ghidra type spellings -> C */
typedef uint64_t undefined8;
typedef uint32_t undefined4;
typedef uint16_t undefined2;
typedef uint8_t  undefined1;
typedef uint8_t  undefined;
typedef int64_t  longlong;
typedef uint64_t ulonglong;
typedef uint32_t uint;
typedef uint16_t ushort;
typedef uint8_t  byte;
typedef uint8_t  code;
typedef uint8_t  IMAGE_DOS_HEADER;   /* only ever used as a byte pointer */
typedef int      bool_t;

/* runtime */
int   egis_init(void);                 /* map sections (once), reset state; 0 = ok */
void  egis_reset(void);                /* rewind arena + restore pristine .data */
void *e_malloc(uint64_t sz);
void *e_calloc(uint64_t n, uint64_t sz);
void *e_realloc(void *p, uint64_t sz);
void  e_free(void *p);

/* FUN_18002b720 (logger) is defined by the translated batch itself.
 * FUN_180063a10 (another logger) is NOT in the translated set — stub it. */
void  FUN_180063a10();

/* memcpy/memset the driver calls by address (FUN_18004d200 / FUN_18004d980) */
void *FUN_18004d200(void *dst, const void *src, uint64_t n);   /* memcpy */
void *FUN_18004d980(void *dst, int val, uint64_t n);           /* memset */

/* CRT the translated code uses directly */
#define malloc(n)      e_malloc(n)
#define calloc(a,b)    e_calloc(a,b)
#define realloc(p,n)   e_realloc(p,n)
#define free(p)        e_free(p)

#endif
