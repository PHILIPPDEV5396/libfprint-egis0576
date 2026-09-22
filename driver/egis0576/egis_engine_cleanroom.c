/*
 * egis_engine_cleanroom.c -- the host matcher behind egis_engine.h
 * Copyright (C) 2026 Philipp Oster
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * WHAT THIS FILE IS
 *
 * The egis0576 driver talks to its matcher only through egis_engine.h. This
 * file implements that contract as an image-correlation matcher: each
 * enrolment press is kept as a 70x57 frame, a probe frame is compared with
 * every stored frame by masked normalised cross-correlation over a shift and
 * rotation search, and the best score decides. The frame representation
 * (EmFrame) and the two functions em_frame_compute() / em_match() are the
 * interface Thaddeus Stepanovich designed for his LGPL correlation matcher
 * (egis_match.h, his file, unchanged); the implementation compiled here is
 * the Gabor front-end in egis_match_gabor.c, with its operating-point
 * constants in egis_cr_tuning.h and the alignment report of
 * egis_match_check.h that the enrolment steering below uses.
 *
 * Why not minutiae: on a 70x57 sensor (3.5 x 2.9 mm) NBIS mindtct finds a
 * median of one minutia per frame, and an own extractor about seven with
 * only ~50 % of them repeating between adjacent frames of the same press;
 * there is nothing to match. Correlation on the ridge texture is what the
 * sensor's size leaves.
 *
 * MEASURED (tools/accuracy of the out-of-tree repository, one person per
 * unit, twelve-press enrolment, every frame of a press scored, impostors =
 * the same person's other fingers; scores are NCC):
 *
 *   reference unit, 60 genuine / 480 impostor presses:
 *     genuine 0.80 / 0.97 / 0.99 (min / median / max), impostor 0.06 / 0.41 /
 *     0.69 -> at the shipped 0.78: 0 rejects, 0 accepts; held-out threshold
 *     across folds 0.75 with the same result
 *   second unit (T. Stepanovich, flat-fielded frames): 6.9 % FRR, 0 % FAR
 *   a session five days later on the reference unit: fingers whose presses
 *     covered the enrolled skin matched at 0.83-0.94, two placed on skin the
 *     enrolment never saw did not -- which is what the steering addresses
 *   live through fprintd: genuine presses 0.86-0.95, another finger 0.55
 *
 * The same-person impostor set is the ceiling this could be measured
 * against; nobody else's fingers were available. Physical artefacts were not
 * tried.
 *
 * SCORE MAPPING
 *
 *   The contract is an int score, accept iff score >= EGIS_THRESHOLD (5000),
 *   < 0 means "nothing to score". The NCC in [-1, 1] is scaled so that the
 *   front-end's threshold (em_match_threshold, 0.78) lands exactly on it:
 *   score = lround (ncc * 5000 / 0.78), i.e. 6410 per unit NCC; a perfect
 *   1.0 logs as 6410. -1 is returned verbatim for the front-end's overlap
 *   sentinel, a probe under the coverage gate, an empty slot or a bad index;
 *   the driver treats it as a plain non-match.
 *
 * TEMPLATE BYTES
 *
 *   egis_enroll_finish() hands the driver the stored frames back to back,
 *   nframes x 3990 bytes and nothing else (47,880 bytes for twelve presses);
 *   the driver wraps them in a versioned GVariant. A blob is valid iff it is
 *   a whole number of frames, 1..EGIS_CR_MAX_FRAMES of them. EmFrames are
 *   recomputed from the bytes once per action in egis_gallery_load()
 *   (~1.6 ms each), never per probe.
 *
 *   PRIVACY: these bytes are images of the fingertip, flat-fielded. A
 *   minutiae driver stores coordinates that cannot be turned back into an
 *   image; a correlation matcher has nothing but the image to match against.
 *   They live in fprintd's store (root only, mode 0700), like every other
 *   driver's template.
 *
 * ENROLMENT (egis_enroll_add return codes, see egis_engine.h)
 *
 *   The driver counts accepted presses (EGIS_ENROLL_STAGES = 12), treats 1
 *   and 2 as accepted and everything else as a "adjust your finger" retry:
 *     -2  coverage below EGIS_CR_MIN_ENROL_COVERAGE (the driver keeps
 *         sampling the same press for a few frames first, so a settling
 *         finger gets its chance)
 *      4  same-press duplicate: NCC >= EGIS_CR_REDUNDANT_NCC against a stored
 *         frame
 *      5  same placement as a stored frame (steering, from the third frame
 *         on): not stored; after EGIS_CR_STEER_MAX_RETRY refusals in a row
 *         the press is stored anyway
 *      1  stored, more wanted        2  stored, session full
 *   Once the session is full every further add returns 2 without storing,
 *   so the driver's counter and this one cannot deadlock.
 *
 * THREADING
 *
 *   All state lives in the EgisEngine the device instance owns; nothing is
 *   static. The driver calls egis_enroll_begin / egis_enroll_finish on the
 *   main loop between captures and everything else from one GTask at a
 *   time, so nothing here runs concurrently and no locking is needed.
 *   EmFrames live on the heap (35,920 bytes each); em_frame_compute() uses
 *   < 8 kB of stack and a malloc'd scratch block.
 */

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "egis_engine.h"
#include "egis_match.h"

/* The out-of-tree repository also builds this adapter on the original
 * front-end (tsteppy/egis_match.c, -DEGIS_CR_FRONTEND_TSTEPPY) for the
 * comparison in its docs/matcher-comparison.md; that front-end has no
 * alignment report, so no steering, and the defaults below are its operating
 * point. Only the Gabor build is submitted. */
#ifndef EGIS_CR_FRONTEND_TSTEPPY
#include "gabor/egis_cr_tuning_gabor.h"
#include "gabor/egis_match_check.h"
#define EGIS_CR_HAVE_MATCH_EX 1
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

/* The accept NCC and the probe coverage gate are the front-end's own
 * (egis_match.h): the Gabor file defines 0.78 / 0.35, tsteppy's 0.53 / 0.55.
 * The threshold maps to EGIS_THRESHOLD exactly. The adapter's own gates
 * below are overridable at compile time (-D...) so the accuracy kit can
 * measure a different front-end behind the same adapter without editing
 * this file; they are calibrated per front-end (the Gabor tuning header
 * sets its own). */
#define EGIS_CR_ACCEPT_NCC em_match_threshold
#define EGIS_CR_MIN_COVERAGE em_min_coverage
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

typedef struct
{
  int      nframes;
  EmFrame *frames;                                /* nframes EmFrames */
} CrEntry;

typedef struct
{
  int     active;
  int     count;
  int     steer_refusals;                         /* consecutive code-5 returns */
  uint8_t raw[EGIS_CR_MAX_FRAMES][EGIS_IMG_SIZE];
  EmFrame frames[EGIS_CR_MAX_FRAMES];
  EmFrame scratch;
} CrEnrol;                                         /* ~479 KB */

/* One matcher instance: everything the adapter remembers between calls.
 * Allocated by egis_engine_new(), owned by the driver's device instance. */
struct EgisEngine
{
  CrEnrol enrol;
  CrEntry gallery[EGIS_CR_MAX_GALLERY];
  int     gallery_n;
  EmFrame probe;                                   /* scratch for verify/identify */
};

/* ---- helpers -------------------------------------------------------------- */

static void
gallery_free (EgisEngine *e)
{
  for (int i = 0; i < EGIS_CR_MAX_GALLERY; i++)
    {
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
    return -1;                                     /* sentinel / no overlap */
  return (int) lround (ncc * ((double) EGIS_THRESHOLD / EGIS_CR_ACCEPT_NCC));
}

/* best-of-N over one gallery entry; -1 if nothing could be scored */
static int
score_entry (const EmFrame *p, const CrEntry *e, int stop_at_accept)
{
  double best = -1.0;

  for (int i = 0; i < e->nframes; i++)
    {
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
  EgisEngine *e = calloc (1, sizeof (*e));         /* ~1.2 MB, once per device */

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
  if (enrol->count >= EGIS_CR_MAX_FRAMES)
    {
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
  if (enrol->count >= EGIS_CR_STEER_FROM &&
      enrol->steer_refusals < EGIS_CR_STEER_MAX_RETRY)
    {
      for (int i = 0; i < enrol->count; i++)
        {
          EmMatchInfo in;
          double v = em_match_ex (&enrol->frames[i], &enrol->scratch, &in);
          if (v >= EGIS_CR_STEER_NCC &&
              abs (in.dx) <= EGIS_CR_STEER_SHIFT &&
              abs (in.dy) <= EGIS_CR_STEER_SHIFT)
            {
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
  uint8_t *blob = malloc ((size_t) size);        /* libc malloc: driver frees with free() */
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
  for (int i = 0; i < n; i++)
    {
      nframes[i] = blob_nframes (blobs[i], sizes[i]);
      if (nframes[i] < 1)
        return -1;
    }

  for (int i = 0; i < n; i++)
    {
      int nf = nframes[i];
      e->gallery[i].frames = calloc ((size_t) nf, sizeof (EmFrame));
      if (!e->gallery[i].frames)
        {
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
  for (int i = 0; i < e->gallery[idx].nframes; i++)
    {
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
  for (int i = 0; i < e->gallery_n; i++)
    {
      if (!e->gallery[i].frames)
        continue;
      int s = score_entry (&e->probe, &e->gallery[i], 0);
      if (s > best)                                /* strict: ties keep lower idx */
        {
          best = s;
          besti = i;
        }
    }
  if (out_idx)
    *out_idx = (best >= EGIS_THRESHOLD) ? besti : -1;
  return best;
}
