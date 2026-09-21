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
 * TEMPLATE BYTES (what egis_enroll_finish() hands the driver)
 *
 *   The stored frames of every accepted enrolment press, back to back,
 *   exactly as handed to egis_enroll_add(): nframes x 3990 bytes, nothing
 *   else. A full 12-press print is 47,880 bytes. Versioning and validation
 *   of what fprintd hands back are the driver's (it wraps these bytes in a
 *   (qay) GVariant with a format version); here a blob is valid iff it is a
 *   whole number of frames, 1..EGIS_CR_MAX_FRAMES of them. Storing frames
 *   (rather than EmFrames) keeps the blob 9x smaller and independent of the
 *   front-end: the two clean-room front-ends read the same bytes -- and score
 *   them on different scales, which is why the driver's version guards a
 *   flavour switch and the user re-enrols. EmFrames are recomputed once per
 *   action in egis_gallery_load() (~0.2 ms each with tsteppy's front-end,
 *   ~1.6 ms with the Gabor one), never per probe.
 *
 *   PRIVACY: these bytes are sensor images of the fingertip, flat-fielded.
 *   Minutiae-based drivers store coordinates that cannot be turned back into
 *   an image; a correlation matcher has nothing but the image to match
 *   against. The store is fprintd's (root-only, mode 0700); the argument for
 *   accepting that trade-off is in docs/matcher-comparison.md.
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
 *      5  same placement as a stored frame (from the 3rd frame on): not
 *         stored, the driver asks for an adjusted press; after two refusals in
 *         a row the next such press is stored anyway (see EGIS_CR_STEER_*)
 *      1  stored, more wanted        2  stored, session full (12 frames)
 *   Once the session holds EGIS_CR_MAX_FRAMES frames every further add returns
 *   2 without storing, so the driver's counter and ours can never deadlock.
 *
 * THREADING
 *
 *   All state lives in the EgisEngine the driver's device instance owns
 *   (egis_engine_new / egis_engine_free); nothing is static. The driver
 *   calls egis_enroll_begin / egis_gallery_load on the main thread before
 *   the capture worker starts, and egis_enroll_add / egis_verify /
 *   egis_identify / egis_enroll_finish from the worker; actions never
 *   overlap, so no locking. All EmFrames live on the heap (35,920 bytes each; tsteppy's
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
#ifdef __has_include
#if __has_include("egis_match_check.h")
#include "egis_match_check.h"      /* Gabor front-end: em_match_ex() with the alignment */
#define EGIS_CR_HAVE_MATCH_EX 1
#endif
#endif

/* Must equal EGIS0576_ENROLL_STAGES in driver/egis0576.c. If the driver asks
 * for more presses than this, enrolment still terminates (extra adds return 2)
 * but the extra frames are not stored. */
#define EGIS_CR_MAX_FRAMES EGIS_ENROLL_STAGES
/* Maximum gallery entries per egis_gallery_load. fprintd names at most 10
 * fingers per user and passes them all to identify; its pre-enrol duplicate
 * check can pass more than one user's prints. The driver asks
 * egis_gallery_capacity() and refuses loudly beyond it instead of matching
 * a prefix. */
#define EGIS_CR_MAX_GALLERY 16

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
/* Enrolment frames are held to a higher coverage than probes: a probe only
 * has to match, a template has to be worth matching against for years. The
 * reference unit's first steered enrolment stored eight of twelve frames at
 * coverage 0.37-0.53 -- first frames of edge placements, partial contact --
 * and a normal press then matched them at only 0.78. The driver keeps
 * sampling a press until a frame clears this gate (~300 ms), so the finger
 * settles instead of the user pressing again. */
#ifndef EGIS_CR_MIN_ENROL_COVERAGE
#define EGIS_CR_MIN_ENROL_COVERAGE EGIS_CR_MIN_COVERAGE
#endif
/* Enrolment steering. Coverage decides the genuine floor: on the reference
 * unit a second session five days later matched the first session's
 * templates at a median of 0.94 / 0.92 / 0.83 on three fingers and 0.55 /
 * 0.15 on two, because those two landed on skin the twelve enrolment presses
 * never covered -- and enrolling from a session whose presses were spread
 * out lifted the cross-session minimum from 0.52-0.59 to 0.82-0.92 on the
 * fingers that overlap at all. tsteppy saw the same on his unit. The driver
 * cannot say "a bit to the left", but it can refuse to spend a stage on a
 * press that lands where one already is: from the EGIS_CR_STEER_FROM-th
 * stored frame on, a candidate whose best template match is at NCC >=
 * EGIS_CR_STEER_NCC AND within EGIS_CR_STEER_SHIFT px of that template is
 * returned as 5 (same placement, move the finger) instead of stored; the
 * driver shows the "adjust your finger" retry. After EGIS_CR_STEER_MAX_RETRY
 * consecutive refusals the press is stored anyway, so enrolment cannot
 * stall on a user who keeps landing in the same place. Only with a
 * front-end that reports the alignment (em_match_ex). */
#ifndef EGIS_CR_STEER_FROM
#define EGIS_CR_STEER_FROM 3
#endif
#ifndef EGIS_CR_STEER_NCC
#define EGIS_CR_STEER_NCC 0.90
#endif
#ifndef EGIS_CR_STEER_SHIFT
#define EGIS_CR_STEER_SHIFT 6
#endif
#ifndef EGIS_CR_STEER_MAX_RETRY
#define EGIS_CR_STEER_MAX_RETRY 2
#endif
/* Raw-frame corroboration (egis_verify_raw_ok): the un-flat-fielded confirming
 * frame must reach this NCC against the accepted gallery entry, else the
 * accept is dropped. 0 disables the check (returns 1 always). tsteppy's
 * front-end has not been measured on raw-vs-flat-fielded pairs, so it ships
 * disabled; the Gabor tuning header sets it. */
#ifndef EGIS_CR_RAW_CORROBORATE_NCC
#define EGIS_CR_RAW_CORROBORATE_NCC 0.0
#endif
/* egis_verify() stops scoring template frames at the first one over the
 * accept NCC (the driver only compares the result with the threshold). The
 * accuracy kit builds with this at 0 so its score distributions show the
 * true best-of-template maximum, not the first frame that cleared it. */
#ifndef EGIS_CR_VERIFY_EARLY_EXIT
#define EGIS_CR_VERIFY_EARLY_EXIT 1
#endif


/* ---- state ---------------------------------------------------------------- */

typedef struct {
    int nframes;
    EmFrame *frames;                              /* nframes EmFrames */
} CrEntry;

typedef struct {
    int active;
    int count;
    int steer_refusals;                           /* consecutive code-5 returns */
    uint8_t raw[EGIS_CR_MAX_FRAMES][EGIS_IMG_SIZE];
    EmFrame frames[EGIS_CR_MAX_FRAMES];
    EmFrame scratch;
} CrEnrol;                                         /* ~479 KB */

/* One matcher instance: everything the adapter remembers between calls.
 * Allocated by egis_engine_new(), owned by the driver's device instance. */
struct EgisEngine {
    CrEnrol  enrol;
    CrEntry  gallery[EGIS_CR_MAX_GALLERY];
    int      gallery_n;
    EmFrame  probe;                                /* scratch for verify/identify */
};

/* ---- helpers -------------------------------------------------------------- */

static void
gallery_free (EgisEngine *e)
{
    for (int i = 0; i < EGIS_CR_MAX_GALLERY; i++) {
        free (e->gallery[i].frames);
        e->gallery[i].frames = NULL;
        e->gallery[i].nframes = 0;
    }
    e->gallery_n = 0;
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

static int
blob_nframes (const uint8_t *blob, int size)
{
    if (!blob || size <= 0 || size % EGIS_IMG_SIZE != 0)
        return -1;
    if (size / EGIS_IMG_SIZE > EGIS_CR_MAX_FRAMES)
        return -1;
    return size / EGIS_IMG_SIZE;
}

/* ---- contract ------------------------------------------------------------- */

EgisEngine *
egis_engine_new (void)
{
    EgisEngine *e = calloc (1, sizeof (*e));       /* ~1.2 MB, once per device */
    return e;
}

void
egis_engine_free (EgisEngine *e)
{
    if (!e)
        return;
    gallery_free (e);
    free (e);
}

int
egis_enroll_begin (EgisEngine *e)
{
    if (!e)
        return -1;
    e->enrol.active = 1;
    e->enrol.count = 0;
    e->enrol.steer_refusals = 0;
    return 0;
}

int
egis_enroll_add (EgisEngine *e, const uint8_t *raw, int *progress)
{
    CrEnrol *enrol = e ? &e->enrol : NULL;

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

    /* (b) quality gate (stricter than the probe gate, see above) */
    em_frame_compute (raw, &enrol->scratch);
    if (enrol->scratch.coverage < EGIS_CR_MIN_ENROL_COVERAGE)
        return -2;

    /* (c) same-press duplicate */
    for (int i = 0; i < enrol->count; i++)
        if (em_match (&enrol->frames[i], &enrol->scratch) >= EGIS_CR_REDUNDANT_NCC)
            return 4;

#ifdef EGIS_CR_HAVE_MATCH_EX
    /* (c2) same placement as a stored frame: ask for a shifted press */
    if (enrol->count >= EGIS_CR_STEER_FROM
        && enrol->steer_refusals < EGIS_CR_STEER_MAX_RETRY) {
        for (int i = 0; i < enrol->count; i++) {
            EmMatchInfo in;
            double v = em_match_ex (&enrol->frames[i], &enrol->scratch, &in);
            if (v >= EGIS_CR_STEER_NCC
                && abs (in.dx) <= EGIS_CR_STEER_SHIFT
                && abs (in.dy) <= EGIS_CR_STEER_SHIFT) {
                enrol->steer_refusals++;
                return 5;
            }
        }
    }
    enrol->steer_refusals = 0;
#endif

    /* (d) store */
    memcpy (enrol->raw[enrol->count], raw, EGIS_IMG_SIZE);
    enrol->frames[enrol->count] = enrol->scratch;
    enrol->count++;
    if (progress)
        *progress = enrol->count * 100 / EGIS_CR_MAX_FRAMES;
    return enrol->count >= EGIS_CR_MAX_FRAMES ? 2 : 1;
}

int
egis_enroll_finish (EgisEngine *e, uint8_t **out)
{
    CrEnrol *enrol = e ? &e->enrol : NULL;

    if (!enrol || !enrol->active || !out)
        return -1;
    enrol->active = 0;
    if (enrol->count < 1)
        return -1;

    /* The template is the stored frames, nothing else: count x EGIS_IMG_SIZE
     * bytes. Versioning and validation of what fprintd hands back belong to
     * the driver (it wraps this in a typed GVariant); here a blob is valid
     * iff it is a whole number of frames, 1..EGIS_CR_MAX_FRAMES of them. */
    int size = enrol->count * EGIS_IMG_SIZE;
    uint8_t *blob = malloc ((size_t) size);      /* libc malloc: driver frees with free() */
    if (!blob)
        return -1;
    for (int i = 0; i < enrol->count; i++)
        memcpy (blob + i * EGIS_IMG_SIZE, enrol->raw[i], EGIS_IMG_SIZE);

    enrol->count = 0;
    *out = blob;
    return size;
}

int
egis_gallery_load (EgisEngine *e, const uint8_t *const *blobs, const int *sizes, int n)
{
    if (!e)
        return -1;
    gallery_free (e);
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
        e->gallery[i].frames = calloc ((size_t) nf, sizeof (EmFrame));
        if (!e->gallery[i].frames) {
            gallery_free (e);
            return -1;
        }
        e->gallery[i].nframes = nf;
        for (int k = 0; k < nf; k++)
            em_frame_compute (blobs[i] + k * EGIS_IMG_SIZE, &e->gallery[i].frames[k]);
    }
    e->gallery_n = n;
    return 0;
}

int
egis_verify (EgisEngine *e, const uint8_t *raw, int idx)
{
    if (!e || !raw || idx < 0 || idx >= e->gallery_n || !e->gallery[idx].frames)
        return -1;
    em_frame_compute (raw, &e->probe);
    if (e->probe.coverage < EGIS_CR_MIN_COVERAGE)
        return -1;
    return score_entry (&e->probe, &e->gallery[idx], EGIS_CR_VERIFY_EARLY_EXIT);
}

int
egis_gallery_capacity (EgisEngine *e)
{
    (void) e;
    return EGIS_CR_MAX_GALLERY;
}

int
egis_verify_raw_ok (EgisEngine *e, const uint8_t *raw_unfielded, int idx)
{
    double best = -1.0;

    if (EGIS_CR_RAW_CORROBORATE_NCC <= 0.0)
        return 1;
    if (!e || !raw_unfielded || idx < 0 || idx >= e->gallery_n || !e->gallery[idx].frames)
        return 0;
    /* The raw frame still carries the sensor's fixed pattern (that is the
     * point: nothing has been subtracted from it, so nothing can have been
     * painted into it). Genuine raw probes against flat-fielded templates
     * measured NCC >= 0.79 on the reference unit; a featureless contact whose
     * flat-fielded twin scored 0.93 through a poisoned baseline scores like a
     * blank here. */
    em_frame_compute (raw_unfielded, &e->probe);
    for (int i = 0; i < e->gallery[idx].nframes; i++) {
        double v = em_match (&e->gallery[idx].frames[i], &e->probe);
        if (v > best)
            best = v;
        if (best >= EGIS_CR_RAW_CORROBORATE_NCC)
            return 1;
    }
    return 0;
}

int
egis_identify (EgisEngine *e, const uint8_t *raw, int *out_idx)
{
    if (out_idx)
        *out_idx = -1;
    if (!e || !raw || e->gallery_n <= 0)
        return -1;
    em_frame_compute (raw, &e->probe);
    if (e->probe.coverage < EGIS_CR_MIN_COVERAGE)
        return -1;

    int best = -1, besti = -1;
    for (int i = 0; i < e->gallery_n; i++) {
        if (!e->gallery[i].frames)
            continue;
        int s = score_entry (&e->probe, &e->gallery[i], 0);
        if (s > best) {                            /* strict: ties keep lower idx */
            best = s;
            besti = i;
        }
    }
    if (out_idx)
        *out_idx = (best >= EGIS_THRESHOLD) ? besti : -1;
    return best;
}
