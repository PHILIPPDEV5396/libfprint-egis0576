/* egis_engine_cleanroom.c -- clean-room matcher adapter for the egis0576 driver
 *
 * Copyright (C) 2026 Philipp Oster (adapter)
 * Matcher: Copyright (C) 2026 Thaddeus Stepanovich, see tsteppy/egis_match.c
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * ---------------------------------------------------------------------------
 * WHAT THIS FILE IS
 *
 * The egis0576 driver (driver/egis0576.c) talks to its host matcher only
 * through the small contract in egis_engine.h. Three flavours exist:
 *
 *   egis_engine.c            "vendor"    Egis' own extractor/matcher, machine-
 *                                        translated from the Windows driver
 *                                        (egis_funcs.c). Default. Not upstreamable.
 *   egis_engine_cleanroom.c  "cleanroom" THIS FILE: the same contract on top of
 *                                        Thaddeus Stepanovich's LGPL-2.1-or-later
 *                                        correlation matcher em_frame_compute()
 *                                        / em_match() (tsteppy/egis_match.{c,h}).
 *   egis_engine_cleanroom.c  "gabor"     THIS FILE again, on the same two-function
 *     + gabor/                           contract implemented by gabor/
 *                                        egis_match_gabor.c (own work, LGPL):
 *                                        orientation-selective front-end and a
 *                                        rotation search. Its three operating-
 *                                        point constants come from
 *                                        gabor/egis_cr_tuning_gabor.h, force-
 *                                        included by the build (see below).
 *
 * The flavour is chosen at BUILD time with the meson option
 *
 *     -Degis0576_matcher=vendor      (default, today's behaviour, byte-identical)
 *     -Degis0576_matcher=cleanroom   (this file + tsteppy/egis_match.c)
 *     -Degis0576_matcher=gabor       (this file + gabor/egis_match_gabor.c)
 *
 * Exactly one flavour is compiled into libfprint; the other flavours' sources
 * are not built at all (see the 'egis0576' block in libfprint/meson.build).
 * The driver source and egis_engine.h are identical in all flavours. Numbers
 * quoted in this header (thresholds, stack, timings, template counts) are
 * for tsteppy's front-end unless they say otherwise; the Gabor front-end's
 * are in its own two files.
 *
 * PROVENANCE OF tsteppy/egis_match.{c,h}
 *
 *   Upstream: https://github.com/tsteppy/egistec-eh576-libfprint.git
 *   Commit:   56ac424fdd84906b5128ca306c67ccf2768327a1  (driver/egis_match.{c,h})
 *   Copied byte-for-byte (license header intact); re-sync with plain `cp`.
 *   Its `#ifdef EGIS_MATCH_MAIN` evaluator is inert in this build.
 *
 * INPUT FRAMES / PREPROCESSING POLICY
 *
 *   em_frame_compute() is specified for RAW 8-bit 70x57 sensor bytes and does
 *   its own high-pass enhancement, coherence mask and standardisation. In this
 *   flavour egis_preprocess() is therefore an identity copy: the Windows-style
 *   per-frame preprocessing (egis_preprocess.c: min-subtract, invert, auto-
 *   brightness, Otsu stretch, row flip) is treated as part of the VENDOR
 *   matcher's input contract and is not compiled here.
 *
 *   What the engine still receives from the driver is the per-boot flat-field
 *   corrected frame (driver/egis0576.c: flat_field() runs before
 *   egis_preprocess() on every frame and cannot be bypassed from the engine
 *   side). "Same captures" for the A/B comparison therefore means: same sensor
 *   bytes, same flat-field correction, same finger on/off gating and the same
 *   every-frame scoring policy of THIS driver; only the matcher differs.
 *   Thaddeus' published numbers (0 % FAR / ~10 % FRR at NCC 0.53) were measured
 *   on un-flat-fielded raw frames with ONE settled, ghost-checked frame per
 *   press; this driver scores EVERY frame while the finger is down and accepts
 *   on the first one over threshold. Measured on THIS pipeline (identical
 *   captures for both flavours, 60 presses per run, impostors = the same
 *   person's other fingers) on THREE units, one person each
 *   (docs/matcher-comparison.md). At the shipped threshold 5000 (NCC 0.53),
 *   this flavour against the vendor flavour on the same captures:
 *
 *     reference unit (714 frames)  FRR 35.0 % / FAR 0.0 %   vendor  0 / 60, 0 / 480
 *       genuine 1220 / 6961 / 9135, impostor 710 / 1638 / 4470; EER 15 % (NCC 0.24)
 *     sam-dant       (720 frames)  FRR 73.3 % / FAR 0.0 %   vendor  2 / 60, 0 / 480
 *       genuine 965 / 2425.5 / 8994, impostor 698 / 1697.5 / 4232; EER 35 %
 *     irvingpop      (720 frames)  FRR 93.3 % / FAR 1.04 %  vendor 37 / 60, 0 / 480
 *       genuine 1248 / 2865 / 7532, impostor 770 / 2517 / 5481; EER 45 %
 *
 *   So the 35 % / 0 % above is the BEST of the three runs, not this flavour's
 *   error rate: on irvingpop's unit five of the 480 impostor comparisons reach
 *   the threshold (5238..5481) while 57 of the 60 genuine presses score below
 *   the largest impostor, i.e. the two populations are not separated there at
 *   any threshold. The vendor flavour's impostor scores were 0 in all 1440
 *   comparisons across the three units; its genuine side is run-dependent too
 *   (0 / 2 / 37 rejects of 60). Each run is one person, one unit, one session,
 *   and the kit enrols 6 presses per fold against the driver's 12 stages, so
 *   none of these is a population rate or the shipped driver's rate. Reproduce
 *   with tools/accuracy/ (score-cleanroom / score-vendor + evaluate.py).
 *
 * SCORE MAPPING
 *
 *   The contract is an int score, higher is better, accept iff
 *   score >= EGIS_THRESHOLD (5000), < 0 means "nothing to score". em_match()
 *   returns an NCC in [-1, 1] (or exactly -1.0 when the masked overlap is too
 *   small). It is scaled so that Thaddeus' operating point lands exactly on
 *   the driver's threshold:
 *
 *       score = lround(ncc * EGIS_THRESHOLD / EGIS_CR_ACCEPT_NCC)
 *             = lround(ncc * 9433.96...)         (EGIS_CR_ACCEPT_NCC = 0.53)
 *
 *       ncc 0.530 -> 5000 (accept)     ncc 1.000 -> 9434
 *       ncc 0.528 -> 4981 (reject; his best impostor)
 *       ncc 0.421 -> 3972
 *
 *   To convert a logged score back to an NCC divide by 9433.96. The Gabor
 *   flavour uses the same formula with its own EGIS_CR_ACCEPT_NCC (0.78, see
 *   gabor/egis_cr_tuning_gabor.h): 5000 / 0.78 = 6410.26 per unit NCC, so a
 *   perfect 1.0 logs as 6410 there and scores are NOT comparable across the
 *   two flavours except through the threshold. A negative
 *   result (-1) is returned verbatim, never scaled, for: em_match's overlap
 *   sentinel, a probe whose coverage is below EGIS_CR_MIN_COVERAGE (his probe
 *   quality gate), an empty gallery slot or a bad index. The driver already
 *   treats < 0 as a plain non-match.
 *
 * TEMPLATE BLOB FORMAT (what the driver stores as fpi-data 'ay')
 *
 *   16-byte little-endian header followed by the raw frames of every accepted
 *   enrolment press, exactly as handed to egis_enroll_add():
 *
 *       offset  size  field
 *            0     8  magic   "EH576CR\0"
 *            8     2  version 1
 *           10     2  width   70
 *           12     2  height  57
 *           14     2  nframes 1..EGIS_CR_MAX_FRAMES
 *           16   nframes * 3990   frames
 *
 *   A full 12-press print is 16 + 12 * 3990 = 47,896 bytes. Storing raw frames
 *   (rather than EmFrames) keeps the blob 9x smaller, independent of future
 *   changes to em_frame_compute(), and in the same representation Thaddeus'
 *   own driver stores, so his offline tools can re-evaluate these enrolments.
 *   EmFrames are recomputed once per action in egis_gallery_load() (~0.2 ms
 *   each with tsteppy's front-end, ~1.6 ms with the Gabor one), never per
 *   probe.
 *
 *   Cross-flavour: a vendor-flavour blob fails the magic/size check here,
 *   egis_gallery_load() returns -1 and the driver maps that to
 *   FP_DEVICE_ERROR_DATA_INVALID without touching the sensor; the user
 *   re-enrols. The reverse direction (a clean-room blob fed to a vendor build)
 *   is NOT guarded on the vendor side. The cleanroom and gabor flavours store
 *   the SAME blob (raw frames, no front-end state), so a template enrolled
 *   under one loads silently under the other -- and scores on the other's
 *   scale, against the other's threshold. Re-enrol after switching flavours,
 *   in every direction.
 *
 * ENROLMENT MAPPING (egis_enroll_add return codes, see egis_engine.h)
 *
 *   The driver counts accepted presses itself (EGIS0576_ENROLL_STAGES = 12) and
 *   treats 1 and 2 both as "accepted"; anything else becomes a CENTER_FINGER
 *   retry. This adapter returns:
 *     -2  probe coverage < EGIS_CR_MIN_COVERAGE (0.55, his enrolment gate)
 *      4  redundant: NCC >= EGIS_CR_REDUNDANT_NCC (0.95) against a frame already
 *         in the session (same-press duplicate; his best genuine cross-press
 *         score is 0.923, same-press agreement 0.997-0.998)
 *      1  stored, more wanted        2  stored, session full (12 frames)
 *   Once the session holds EGIS_CR_MAX_FRAMES frames every further add returns
 *   2 without storing, so the driver's counter and ours can never deadlock.
 *
 * THREADING
 *
 *   Same as the vendor engine: process-global state, no locking. The driver
 *   calls egis_engine_init / egis_enroll_begin / egis_gallery_load on the main
 *   thread before the capture worker starts, and egis_enroll_add / egis_verify
 *   / egis_identify / egis_enroll_finish from the worker; actions never
 *   overlap. All EmFrames live on the heap (35,920 bytes each; tsteppy's
 *   em_frame_compute uses ~64 KB of stack transiently, the Gabor one < 8 KB
 *   and a malloc'd scratch block).
 * ---------------------------------------------------------------------------
 */

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "egis_engine.h"
#include "egis_match.h"

/* Must equal EGIS0576_ENROLL_STAGES in driver/egis0576.c. If the driver asks
 * for more presses than this, enrolment still terminates (extra adds return 2)
 * but the extra frames are not stored. */
#define EGIS_CR_MAX_FRAMES 12
/* Maximum gallery entries per egis_gallery_load (driver passes at most 5). */
#define EGIS_CR_MAX_GALLERY 5

/* Thaddeus' operating points (his driver: EGIS0576_MATCH_THRESHOLD and
 * EGIS0576_MIN_COVERAGE). */
/* Overridable at compile time (-D...) so the accuracy kit can measure a
 * different matcher front-end behind the same adapter without editing this
 * file: all three are calibrated against the NCC distribution of
 * tsteppy/egis_match.c, and a front-end with a different distribution needs
 * them moved or the gates fire on the wrong things. The defaults are the
 * shipped driver's and are unchanged. */
#ifndef EGIS_CR_ACCEPT_NCC
#define EGIS_CR_ACCEPT_NCC 0.53      /* maps to EGIS_THRESHOLD exactly */
#endif
#ifndef EGIS_CR_MIN_COVERAGE
#define EGIS_CR_MIN_COVERAGE 0.55    /* enrol + probe quality gate */
#endif
#ifndef EGIS_CR_REDUNDANT_NCC
#define EGIS_CR_REDUNDANT_NCC 0.95   /* enrol: reject same-press duplicate */
#endif
/* egis_verify() stops scoring template frames at the first one over the
 * accept NCC (the driver only compares the result with the threshold). The
 * accuracy kit builds with this at 0 so its score distributions show the
 * true best-of-template maximum, not the first frame that cleared it. */
#ifndef EGIS_CR_VERIFY_EARLY_EXIT
#define EGIS_CR_VERIFY_EARLY_EXIT 1
#endif

#define EGIS_CR_MAGIC "EH576CR"      /* 7 chars + NUL = 8 bytes */
#define EGIS_CR_VERSION 1
#define EGIS_CR_HDR_SIZE 16

/* ---- state ---------------------------------------------------------------- */

typedef struct {
    int nframes;
    EmFrame *frames;                              /* nframes EmFrames */
} CrEntry;

static struct {
    int active;
    int count;
    uint8_t raw[EGIS_CR_MAX_FRAMES][EGIS_IMG_SIZE];
    EmFrame frames[EGIS_CR_MAX_FRAMES];
    EmFrame scratch;
} *enrol;                                          /* heap, ~479 KB, once */

static CrEntry gallery[EGIS_CR_MAX_GALLERY];
static int gallery_n;
static EmFrame *probe;                             /* heap scratch for verify/identify */

/* ---- helpers -------------------------------------------------------------- */

static void
gallery_free (void)
{
    for (int i = 0; i < EGIS_CR_MAX_GALLERY; i++) {
        free (gallery[i].frames);
        gallery[i].frames = NULL;
        gallery[i].nframes = 0;
    }
    gallery_n = 0;
}

static int
ncc_to_score (double ncc)
{
    if (ncc < 0.0)
        return -1;                                 /* sentinel / no overlap */
    return (int) lround (ncc * ((double) EGIS_THRESHOLD / EGIS_CR_ACCEPT_NCC));
}

/* best-of-N over one gallery entry; -1 if nothing could be scored */
static int
score_entry (const EmFrame *p, const CrEntry *e, int stop_at_accept)
{
    double best = -1.0;
    for (int i = 0; i < e->nframes; i++) {
        /* template first, probe second: tsteppy's em_match is symmetric, but
         * a front-end that resamples the probe (rotation search) is not, and
         * this is the order the accuracy kit measures. */
        double s = em_match (&e->frames[i], p);
        if (s > best)
            best = s;
        /* verify only needs "over the threshold or not", so it stops at the
         * first template frame that clears it: a genuine press then costs one
         * to a few em_match calls instead of all twelve. identify keeps the
         * full maximum, because it ranks prints against each other. */
        if (stop_at_accept && best >= EGIS_CR_ACCEPT_NCC)
            break;
    }
    return ncc_to_score (best);
}

static void
put_u16 (uint8_t *p, unsigned v)
{
    p[0] = (uint8_t) (v & 0xff);
    p[1] = (uint8_t) ((v >> 8) & 0xff);
}

static unsigned
get_u16 (const uint8_t *p)
{
    return (unsigned) p[0] | ((unsigned) p[1] << 8);
}

/* Parse one blob header; returns nframes (>= 1) or -1 if it is not a valid
 * clean-room template of exactly the expected size. */
static int
blob_nframes (const uint8_t *blob, int size)
{
    if (!blob || size < EGIS_CR_HDR_SIZE)
        return -1;
    if (memcmp (blob, EGIS_CR_MAGIC, 8) != 0)
        return -1;
    if (get_u16 (blob + 8) != EGIS_CR_VERSION ||
        get_u16 (blob + 10) != EGIS_IMG_W ||
        get_u16 (blob + 12) != EGIS_IMG_H)
        return -1;
    unsigned n = get_u16 (blob + 14);
    if (n < 1 || n > EGIS_CR_MAX_FRAMES)
        return -1;
    if ((long) size != (long) EGIS_CR_HDR_SIZE + (long) n * EGIS_IMG_SIZE)
        return -1;
    return (int) n;
}

/* ---- contract ------------------------------------------------------------- */

int
egis_engine_init (void)
{
    if (!enrol) {
        enrol = calloc (1, sizeof (*enrol));
        if (!enrol)
            return -1;
    }
    if (!probe) {
        probe = calloc (1, sizeof (*probe));
        if (!probe)
            return -1;
    }
    enrol->active = 0;
    enrol->count = 0;
    gallery_free ();
    return 0;
}

int
egis_enroll_begin (void)
{
    if (!enrol)
        return -1;
    enrol->active = 1;
    enrol->count = 0;
    return 0;
}

int
egis_enroll_add (const uint8_t *raw, int *progress)
{
    if (!enrol || !enrol->active || !raw)
        return -1;
    if (progress)
        *progress = enrol->count * 100 / EGIS_CR_MAX_FRAMES;

    /* (a) session full: idempotent "done", nothing stored */
    if (enrol->count >= EGIS_CR_MAX_FRAMES) {
        if (progress)
            *progress = 100;
        return 2;
    }

    /* (b) quality gate */
    em_frame_compute (raw, &enrol->scratch);
    if (enrol->scratch.coverage < EGIS_CR_MIN_COVERAGE)
        return -2;

    /* (c) same-press duplicate */
    for (int i = 0; i < enrol->count; i++)
        if (em_match (&enrol->frames[i], &enrol->scratch) >= EGIS_CR_REDUNDANT_NCC)
            return 4;

    /* (d) store */
    memcpy (enrol->raw[enrol->count], raw, EGIS_IMG_SIZE);
    enrol->frames[enrol->count] = enrol->scratch;
    enrol->count++;
    if (progress)
        *progress = enrol->count * 100 / EGIS_CR_MAX_FRAMES;
    return enrol->count >= EGIS_CR_MAX_FRAMES ? 2 : 1;
}

int
egis_enroll_finish (uint8_t **out)
{
    if (!enrol || !enrol->active || !out)
        return -1;
    enrol->active = 0;
    if (enrol->count < 1)
        return -1;

    int size = EGIS_CR_HDR_SIZE + enrol->count * EGIS_IMG_SIZE;
    uint8_t *blob = malloc ((size_t) size);      /* libc malloc: driver frees with g_free */
    if (!blob)
        return -1;
    memset (blob, 0, EGIS_CR_HDR_SIZE);
    memcpy (blob, EGIS_CR_MAGIC, 8);
    put_u16 (blob + 8, EGIS_CR_VERSION);
    put_u16 (blob + 10, EGIS_IMG_W);
    put_u16 (blob + 12, EGIS_IMG_H);
    put_u16 (blob + 14, (unsigned) enrol->count);
    for (int i = 0; i < enrol->count; i++)
        memcpy (blob + EGIS_CR_HDR_SIZE + i * EGIS_IMG_SIZE, enrol->raw[i], EGIS_IMG_SIZE);

    enrol->count = 0;
    *out = blob;
    return size;
}

int
egis_gallery_load (const uint8_t *const *blobs, const int *sizes, int n)
{
    gallery_free ();
    if (!blobs || !sizes || n <= 0 || n > EGIS_CR_MAX_GALLERY)
        return -1;

    /* validate everything first so a bad blob loads nothing */
    int nframes[EGIS_CR_MAX_GALLERY];
    for (int i = 0; i < n; i++) {
        nframes[i] = blob_nframes (blobs[i], sizes[i]);
        if (nframes[i] < 1)
            return -1;
    }

    for (int i = 0; i < n; i++) {
        int nf = nframes[i];
        gallery[i].frames = calloc ((size_t) nf, sizeof (EmFrame));
        if (!gallery[i].frames) {
            gallery_free ();
            return -1;
        }
        gallery[i].nframes = nf;
        for (int k = 0; k < nf; k++)
            em_frame_compute (blobs[i] + EGIS_CR_HDR_SIZE + k * EGIS_IMG_SIZE,
                              &gallery[i].frames[k]);
    }
    gallery_n = n;
    return 0;
}

int
egis_verify (const uint8_t *raw, int idx)
{
    if (!probe || !raw || idx < 0 || idx >= gallery_n || !gallery[idx].frames)
        return -1;
    em_frame_compute (raw, probe);
    if (probe->coverage < EGIS_CR_MIN_COVERAGE)
        return -1;
    return score_entry (probe, &gallery[idx], EGIS_CR_VERIFY_EARLY_EXIT);
}

void
egis_preprocess (const uint8_t *raw, uint8_t *out)
{
    /* identity: em_frame_compute() does its own enhancement (see header) */
    if (raw != out)
        memcpy (out, raw, EGIS_IMG_SIZE);
}

int
egis_identify (const uint8_t *raw, int *out_idx)
{
    if (out_idx)
        *out_idx = -1;
    if (!probe || !raw || gallery_n <= 0)
        return -1;
    em_frame_compute (raw, probe);
    if (probe->coverage < EGIS_CR_MIN_COVERAGE)
        return -1;

    int best = -1, besti = -1;
    for (int i = 0; i < gallery_n; i++) {
        if (!gallery[i].frames)
            continue;
        int s = score_entry (probe, &gallery[i], 0);
        if (s > best) {                            /* strict: ties keep lower idx */
            best = s;
            besti = i;
        }
    }
    if (out_idx)
        *out_idx = (best >= EGIS_THRESHOLD) ? besti : -1;
    return best;
}
