/* egis_match_check.h -- local-ridge-period consistency check for the Gabor front-end.
 * Own header next to egis_match_gabor.c; tsteppy/egis_match.h is untouched. */
#pragma once
#include "egis_match.h"

#ifndef EM_FB
#define EM_FB 4              /* period-map cell size in px (window = 3x3 cells
                              * when EM_FQ_SMOOTH, i.e. 12x12 px every 4 px) */
#endif
#define EM_FBX ((EM_W + EM_FB - 1) / EM_FB)   /* 18 */
#define EM_FBY ((EM_H + EM_FB - 1) / EM_FB)   /* 15 */
#define EM_NB (EM_FBX * EM_FBY)

typedef struct
{
  double ncc;        /* plain masked NCC (what em_match used to return)    */
  int    dx, dy;     /* winning shift (template coordinates)               */
  double rot_deg;    /* winning rotation of the probe                      */
  int    overlap;    /* masked overlap pixels at the winning alignment     */
  int    nblk;       /* blocks with a valid period in BOTH frames          */
  double t_spread;   /* sd of the template's block period over its blocks  */
  double p_spread;   /* sd of the probe's block period (aligned)           */
  double mad;        /* mean |Tperiod - Pperiod| over common blocks        */
  double corr;       /* corr of the two period-deviation maps              */
  double dmean;      /* mean(Tperiod) - mean(Pperiod) over common blocks   */
} EmMatchInfo;

/* Decision at the winning alignment (em_match returns -1 when it fails):
 *   nblk     >= EM_FQ_MIN_NBLK      enough cells with a period in BOTH frames
 *   mad      <= EM_FQ_MAX_MAD       template and probe period agree (px)
 *   p_spread >= EM_FQ_MIN_PSPREAD   the probe's period VARIES over the overlap
 *   corr     >= EM_FQ_MIN_CORR      (unused at -2: a smooth chirp satisfies it)
 * Provenance (reference dataset, kit protocol 60/480, all pairs dumped):
 * genuine pairs with NCC >= 0.78 have mad p50 0.08 / p95 0.31, p_spread p5
 * 0.21 / p50 0.55, nblk p5 12 / p50 47; the review's synthetic accepts sit
 * at p_spread < 0.25 (sine 0.03, arc 0.05, loop 0.17, ridge noise 0.19,
 * wavy 0.18, hill-climb model 0.38 whole-frame but < 0.25 over the overlap
 * that wins). 0.25 is the largest value that costs no genuine press in the
 * kit (0.30 costs 4/60). See em_check.c for what this does NOT stop. */
#ifndef EM_FQ_ENABLE
#define EM_FQ_ENABLE 1
#endif
#ifndef EM_FQ_MIN_NBLK
#define EM_FQ_MIN_NBLK 20
#endif
#ifndef EM_FQ_MAX_MAD
#define EM_FQ_MAX_MAD 0.5
#endif
#ifndef EM_FQ_MIN_PSPREAD
#define EM_FQ_MIN_PSPREAD 0.25
#endif
#ifndef EM_FQ_MIN_CORR
#define EM_FQ_MIN_CORR -2.0
#endif
#ifndef EM_FQ_GATE_FROM
#define EM_FQ_GATE_FROM 0.70 /* run the check only from this NCC upwards */
#endif


/* em_match() with the diagnostics of the winning alignment. */
double em_match_ex (const EmFrame *a, const EmFrame *b, EmMatchInfo *info);

/* block period map of one frame (in its own coordinates); returns #valid */
int em_period_map (const double *img, const uint8_t *mask, double *pmap, double *conf);
