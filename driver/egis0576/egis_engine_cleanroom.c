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
 * file implements that contract as an image matcher: each enrolment press is
 * kept as a 70x57 frame, a probe frame is compared with every stored frame,
 * and the best score decides. The frame representation (EmFrame) and the two
 * functions em_frame_compute() / em_match() are the interface Thaddeus
 * Stepanovich designed for his LGPL correlation matcher (egis_match.h, his
 * file, unchanged); the implementation compiled here is the Gabor front-end
 * in egis_match_gabor.c, with its operating-point constants in
 * egis_cr_tuning_gabor.h and the alignment report of egis_match_check.h
 * that the enrolment steering below uses. Since 2026-09-26 the front-end's
 * correlation no longer decides verify and identify: the decision is the
 * IDENTITY EVIDENCE of egis_match_evidence.c -- whether the fine structure
 * the Gabor filter discards reproduces over a contiguous area -- and the
 * correlation only proposes the poses it is looked for at (see "the
 * decision" below).
 *
 * Why not minutiae: on a 70x57 sensor (roughly 5.5 x 4.5 mm, inferred from
 * the measured 6.4 px ridge period) NBIS mindtct finds a
 * median of one minutia per frame, and an own extractor about seven with
 * only ~50 % of them repeating between adjacent frames of the same press;
 * there is nothing to match. Correlation on the ridge texture is what the
 * sensor's size leaves.
 *
 * MEASURED, THE IDENTITY EVIDENCE (this build, as the driver builds it;
 * tools/accuracy kit_version 5 of the out-of-tree repository, driver's
 * rule, one person, impostors = the same person's other fingers; the table
 * and what the rejects are in egis_match_evidence.c, MEASURED):
 *
 *   reference unit, sessions 2026-09-13, 2026-09-18, 2026-09-26 and
 *     2026-09-27, in session and across them in all twelve directions --
 *     16 sets, every constant of the evidence stage was set on them or a
 *     part of them, so IN-SAMPLE (v4, 2026-09-28): 46 / 960 genuine presses
 *     rejected, the vendor matcher on the same frames 42 / 960; 0 / 4800
 *     impostor presses accepted, and no impostor press reaches any level
 *     above 0 (the highest single impostor frame 0.72 x the accept point).
 *     This misses the owner's bar (pooled false rejects no more than the
 *     vendor matcher's) by four presses; it is not certified and not yet
 *     measured on data it was not tuned on. Its predecessors were certified
 *     and FAILED: e27b8bc (v2) on 2026-09-26 (more false rejects than the
 *     vendor matcher on two sets), 743b796 (v3) on 2026-09-27 (one false
 *     accept at 1.083 x, more false rejects on three sets).
 *
 * MEASURED, THE FRONT-END'S NCC AS THE DECISION (until 2026-09-26; one
 * person per unit, every frame of a press scored, impostors = the same
 * person's other fingers; scores are NCC; rejects / accepts under the
 * driver's two-frame rule unless noted):
 *
 *   reference unit, session 2026-09-13, 60 genuine / 480 impostor presses
 *     (the kit's two folds of six-press templates; until 2026-09-26 this
 *     header said twelve-press enrolment):
 *     genuine 0.80 / 0.97 / 0.99 (min / median / max), impostor 0.06 / 0.41 /
 *     0.69 -> at the shipped 0.78: 0 rejects, 0 accepts; held-out threshold
 *     across folds 0.75 with the same result. IN-SAMPLE: every constant of
 *     the front-end was chosen on this session.
 *   the same unit, person and fingers, session 2026-09-18: 13 / 60 rejects,
 *     18 / 480 accepts, worst impostor 0.896 (right thumb against right
 *     index); the vendor matcher on the same frames 2 / 60 and 0 / 480.
 *     Twelve-press templates from one session, probes from the other:
 *     9 / 240 and 15 / 240 accepts, worst impostor 0.895 / 0.886. So the
 *     0 / 0 above is one favourable session, not a property of the unit --
 *     until 2026-09-26 this comment presented it as the unit's, and read
 *     this session as two fingers placed on skin the enrolment never saw
 *     (withdrawn at the enrolment steering below: they came back rotated).
 *   second unit (T. Stepanovich, flat-fielded frames, his protocol, held
 *     out): 6.9 % FRR, 1.1 % FAR (this line said 0 % FAR until 2026-09-26)
 *   sam-dant's unit (2026-09-25): 2 / 60 rejects, 2 / 480 accepts, worst
 *     impostor 0.845; irvingpop's: 58 / 480 accepts under the kit's
 *     press-maximum rule (the driver's rule not measured there)
 *   live through fprintd: genuine presses 0.86-0.95, another finger 0.55
 *
 * Why the NCC stopped deciding: the front-end scores whether two patches
 * agree in ridge flow and ridge period, and that is not identity. Two
 * different fingers that put near-parallel ridges at a similar angle and
 * period on the sensor score like one finger, and whether they do depends on
 * how the fingers land in a session. Threshold, overlap floor and search
 * width were measured against it; each only trades false rejects for false
 * accepts. See egis_match_gabor.c, LIMITS, and the out-of-tree repository's
 * docs/matcher-comparison.md, "2026-09-26: what prevents a universal 0/0".
 *
 * The same-person impostor set is the ceiling this could be measured
 * against; nobody else's fingers were available. Physical artefacts were not
 * tried.
 *
 * SCORE MAPPING
 *
 *   The contract is an int score, accept iff score >= EGIS_THRESHOLD (5000),
 *   < 0 means "nothing to score". The evidence W (egis_match_evidence.c,
 *   in milli-units) is scaled so that its accept point
 *   (EGIS_CR_ACCEPT_EVIDENCE, W 2.0) lands exactly on it: score = lround
 *   (W * 2500). It is unbounded above: on the 2026-09-18 session the
 *   genuine presses' median is 52,604, the highest impostor press 3,238.
 *   (The comparison build on tsteppy's front-end keeps the NCC: score =
 *   lround (ncc * 5000 / em_match_threshold).)
 *
 *   "Nothing to score" and "compared and rejected" are kept apart, because
 *   the driver answers them differently -- the first asks the user to press
 *   again (and fprintd restarts without spending an attempt), the second is
 *   a failed attempt. -1 therefore means only: no pose of any stored frame
 *   reaches the overlap floor, a probe under the coverage gate, an empty
 *   gallery slot, a bad index. A pair with no evidence scores 0 -- a
 *   non-match like any other, so an artefact cannot be presented over and
 *   over for free.
 *
 * TEMPLATE BYTES
 *
 *   egis_enroll_finish() hands the driver the stored frames back to back,
 *   nframes x 3990 bytes and nothing else (47,880 bytes for twelve presses);
 *   the driver wraps them in a versioned GVariant. A blob is valid iff it is
 *   a whole number of frames, 1..EGIS_CR_MAX_FRAMES of them. The frames'
 *   features (ridge image, mask, residual, orientation lattice) are
 *   recomputed from the bytes once per action in egis_gallery_load()
 *   (~1.8 ms each), never per probe. The template format did not change when
 *   the evidence took over the decision: v0.5 templates keep working.
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
 *   Gallery frames live on the heap (51,880 bytes each as EvFrames, about
 *   10 MB for 16 prints of 12 frames); the frame computations use < 8 kB of
 *   stack and a malloc'd scratch block, ev_match() about 213 kB of heap
 *   scratch per call, freed before it returns, and about 16 kB of stack
 *   (the block search's per-shift cell sums, since v4).
 *
 *   What is kept between calls besides the gallery is what
 *   egis_verify_raw_ok needs to corroborate the frame just scored: each
 *   entry's last evidence pose and template frame (CrEntry.last_pose,
 *   last_frame), whether verify's early exit cut that search short
 *   (last_cut), and the flat-fielded probe's features (EgisEngine.probe,
 *   which the raw frame no longer overwrites: it has its own scratch,
 *   rawprobe), from which egis_verify_raw_ok looks for the best pose when
 *   the early-exit pose does not corroborate. Every verify, identify and
 *   gallery load invalidates the poses first, so nothing of an earlier frame
 *   can be corroborated, and a score never depends on what was scored before
 *   it.
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
 * point. Only the Gabor build is meant for submission (on hold since
 * 2026-09-26, see docs/upstream-mr.md). */
#ifndef EGIS_CR_FRONTEND_TSTEPPY
#include "gabor/egis_cr_tuning_gabor.h"
#include "gabor/egis_match_check.h"
#include "gabor/egis_match_evidence.h"
#define EGIS_CR_HAVE_MATCH_EX 1
#define EGIS_CR_HAVE_EVIDENCE 1
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
/* Enrolment steering. Placement decides part of the genuine floor: on the
 * reference unit a second session five days later matched the first
 * session's templates at a median of 0.94 / 0.92 / 0.83 on three fingers and
 * 0.55 / 0.15 on two, and enrolling from a session whose presses were spread
 * out lifted the cross-session minimum from 0.52-0.59 to 0.82-0.92 on the
 * fingers that overlap at all. tsteppy saw the same on his unit. Until
 * 2026-09-26 this comment said the two low fingers "landed on skin the
 * twelve enrolment presses never covered". That was partly wrong: they came
 * back ROTATED, the thumb by about 50-60 deg and the right middle finger by
 * about 30 deg, outside the front-end's +-10 deg search. Widening only the
 * rotation search (+-90 / +-30 deg) accepts 21 of 24 cross-session presses
 * of each instead of 0 (the thumb's 21 under the press maximum; 19 of 24
 * under the driver's rule) -- and is no fix, because the wider search
 * admits more impostors too (18 -> 32 of 480 at +-30 deg on the second
 * session, driver's rule). Steering addresses placement only. The driver
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
#ifndef EGIS_CR_HAVE_EVIDENCE
/* Raw-frame corroboration (egis_verify_raw_ok) in tsteppy's build: the
 * un-flat-fielded confirming frame must reach this NCC against the accepted
 * gallery entry, else the accept is dropped; 0 disables the check. His
 * front-end has not been measured on raw-vs-flat-fielded pairs, so it ships
 * disabled. The Gabor build corroborates on the identity evidence instead
 * (EGIS_CR_RAW_EVIDENCE, gabor/egis_cr_tuning_gabor.h). */
#ifndef EGIS_CR_RAW_CORROBORATE_NCC
#define EGIS_CR_RAW_CORROBORATE_NCC 0.0
#endif
#endif
/* egis_verify() stops scoring template frames at the first one over the
 * accept point (the driver only compares the result with the threshold).
 * The accuracy kit builds the Gabor flavour exactly as the driver does, with
 * no define (since its kit_version 5: the raw corroboration below starts at
 * the pose verify recorded, so a build with this at 0, which the kit used
 * until then, makes other decisions than the driver). */
#ifndef EGIS_CR_VERIFY_EARLY_EXIT
#define EGIS_CR_VERIFY_EARLY_EXIT 1
#endif

/* ---- the decision --------------------------------------------------------
 *
 * verify and identify decide on IDENTITY EVIDENCE (egis_match_evidence.c),
 * not on the front-end's NCC: the fine structure the Gabor filter discards
 * has to reproduce over a contiguous area of the overlap. The evidence W
 * (milli-units) maps to the contract's score as score = W * EGIS_THRESHOLD /
 * EGIS_CR_ACCEPT_EVIDENCE, so the accept point lands exactly on
 * EGIS_THRESHOLD. The ridge image only proposes the poses the evidence is
 * looked for at; the front-end's NCC still drives enrolment (quality gate,
 * steering) unchanged. */
#ifdef EGIS_CR_HAVE_EVIDENCE
typedef EvFrame CrFrame;
typedef EvProbe CrProbe;
#define CR_PROBE_FRAME(p) (&(p)->e.f)
#else
/* tsteppy's front-end has no residual: its NCC decides, as before */
typedef EmFrame CrFrame;
typedef EmFrame CrProbe;
#define CR_PROBE_FRAME(p) (p)
#endif


/* ---- state ---------------------------------------------------------------- */

typedef struct
{
  int      nframes;
  CrFrame *frames;                                /* nframes CrFrames */
#ifdef EGIS_CR_HAVE_EVIDENCE
  /* The pose and template frame of this entry's best evidence in the LAST
   * egis_verify / egis_identify call, which egis_verify_raw_ok checks the
   * raw frame at. Valid only for the frame just scored: the driver calls
   * egis_verify_raw_ok right after the verify or identify whose accept it
   * corroborates, on the same thread (driver/egis0576.c, match_thread).
   * Every verify and identify invalidates all entries' poses before it
   * scores anything -- also when it then returns -1 for a probe under the
   * coverage gate -- and so does a gallery reload (poses_invalidate), so a
   * stale pose cannot be corroborated. Nothing else reads it, so a frame's
   * score never depends on what was scored before it. */
  EvPose last_pose;
  int    last_frame;
  /* The search stopped at the accept point (verify's early exit): the pose
   * above is the FIRST one to clear it, not necessarily the best one, and
   * egis_verify_raw_ok may look further (see there). */
  int last_cut;
#endif
} CrEntry;

typedef struct
{
  int     active;
  int     count;
  int     steer_refusals;                         /* consecutive code-5 returns */
  uint8_t raw[EGIS_CR_MAX_FRAMES][EGIS_IMG_SIZE];
  EmFrame frames[EGIS_CR_MAX_FRAMES];
  EmFrame scratch;
} CrEnrol;                                         /* ~515 kB */

/* One matcher instance: everything the adapter remembers between calls.
 * Allocated by egis_engine_new(), owned by the driver's device instance. */
struct EgisEngine
{
  CrEnrol enrol;
  CrEntry gallery[EGIS_CR_MAX_GALLERY];
  int     gallery_n;
  CrProbe probe;                                   /* scratch for verify/identify */
#ifdef EGIS_CR_HAVE_EVIDENCE
  /* egis_verify_raw_ok's scratch for the raw frame, so that probe still
   * holds the flat-fielded frame the last verify scored */
  CrProbe rawprobe;
#endif
};

/* ---- helpers -------------------------------------------------------------- */

static void
poses_invalidate (EgisEngine *e)
{
  /* nothing egis_verify_raw_ok could corroborate survives a new verify or
   * identify, a failed one, or a gallery reload */
#ifdef EGIS_CR_HAVE_EVIDENCE
  for (int i = 0; i < EGIS_CR_MAX_GALLERY; i++)
    {
      e->gallery[i].last_pose.valid = 0;
      e->gallery[i].last_frame = -1;
      e->gallery[i].last_cut = 0;
    }
#else
  (void) e;
#endif
}

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
  poses_invalidate (e);
}

#ifdef EGIS_CR_HAVE_EVIDENCE
static int
ev_to_score (int ev)
{
  if (ev < 0)
    return -1;                                     /* no alignment at all */
  return (int) lround (ev * ((double) EGIS_THRESHOLD / EGIS_CR_ACCEPT_EVIDENCE));
}

/* Best evidence of probe p over one entry's template frames, milli-W, -1 if
 * nothing could be scored; *pose and *frame receive where (the first
 * maximum). With stop_at_accept the search ends at the first template frame,
 * and inside it at the first pose, that clears the accept point. Reads the
 * entry only. */
static int
entry_best (const CrProbe *p, const CrEntry *e, int stop_at_accept, EvPose *pose, int *frame)
{
  int best = -1;

  pose->valid = 0;
  *frame = -1;
  for (int i = 0; i < e->nframes; i++)
    {
      EvPose ps;
      /* template first, probe second: only the probe is resampled */
      int s = ev_match (&e->frames[i], p, stop_at_accept ? EGIS_CR_ACCEPT_EVIDENCE : 0, &ps);
      if (s > best)
        {
          best = s;
          *pose = ps;
          *frame = i;
        }
      /* verify only needs "over the threshold or not", so it stops at the
       * first template frame (and inside it, the first pose) that clears it.
       * identify keeps the full maximum, because it ranks prints. */
      if (stop_at_accept && best >= EGIS_CR_ACCEPT_EVIDENCE)
        break;
    }
  return best;
}

/* best-of-N over one gallery entry, recording where for egis_verify_raw_ok;
 * -1 if nothing could be scored */
static int
score_entry (const CrProbe *p, CrEntry *e, int stop_at_accept)
{
  int best = entry_best (p, e, stop_at_accept, &e->last_pose, &e->last_frame);

  e->last_cut = stop_at_accept && best >= EGIS_CR_ACCEPT_EVIDENCE;
  return ev_to_score (best);
}
#else
static int
ncc_to_score (double ncc)
{
  if (ncc < 0.0)
    return -1;                                     /* sentinel / no overlap */
  return (int) lround (ncc * ((double) EGIS_THRESHOLD / EGIS_CR_ACCEPT_NCC));
}

/* best-of-N over one gallery entry; -1 if nothing could be scored */
static int
score_entry (const CrProbe *p, CrEntry *e, int stop_at_accept)
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
#endif

static void
probe_compute (const uint8_t *raw, CrProbe *p)
{
#ifdef EGIS_CR_HAVE_EVIDENCE
  ev_probe_compute (raw, p);
#else
  em_frame_compute (raw, p);
#endif
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
  EgisEngine *e = calloc (1, sizeof (*e));         /* ~680 kB, once per device */

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
egis_enroll_finish (EgisEngine *e, uint8_t *out, int cap)
{
  CrEnrol *enrol = e ? &e->enrol : NULL;
  int size;

  if (!enrol || !enrol->active || enrol->count < 1)
    return -1;

  /* The template is the stored frames, nothing else: count x EGIS_IMG_SIZE
   * bytes. Versioning and validation of what fprintd hands back belong to
   * the driver (it wraps this in a typed GVariant); here a blob is valid
   * iff it is a whole number of frames, 1..EGIS_CR_MAX_FRAMES of them. */
  size = enrol->count * EGIS_IMG_SIZE;
  if (!out)
    return size;
  if (cap < size)
    return -1;
  for (int i = 0; i < enrol->count; i++)
    memcpy (out + i * EGIS_IMG_SIZE, enrol->raw[i], EGIS_IMG_SIZE);
  enrol->active = 0;
  enrol->count = 0;
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
      e->gallery[i].frames = calloc ((size_t) nf, sizeof (CrFrame));
      if (!e->gallery[i].frames)
        {
          gallery_free (e);
          return -1;
        }
      e->gallery[i].nframes = nf;
      for (int k = 0; k < nf; k++)
#ifdef EGIS_CR_HAVE_EVIDENCE
        ev_frame_compute (blobs[i] + k * EGIS_IMG_SIZE, &e->gallery[i].frames[k]);
#else
        em_frame_compute (blobs[i] + k * EGIS_IMG_SIZE, &e->gallery[i].frames[k]);
#endif
    }
  e->gallery_n = n;
  return 0;
}

int
egis_verify (EgisEngine *e, const uint8_t *raw, int idx)
{
  if (!e)
    return -1;
  poses_invalidate (e);
  if (!raw || idx < 0 || idx >= e->gallery_n || !e->gallery[idx].frames)
    return -1;
  probe_compute (raw, &e->probe);
  if (CR_PROBE_FRAME (&e->probe)->coverage < EGIS_CR_MIN_COVERAGE)
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
#ifndef EGIS_CR_HAVE_EVIDENCE
  double best = -1.0;

  if (EGIS_CR_RAW_CORROBORATE_NCC <= 0.0)
    return 1;
#endif
  if (!e || !raw_unfielded || idx < 0 || idx >= e->gallery_n || !e->gallery[idx].frames)
    return 0;
  /* The raw frame still carries the sensor's fixed pattern (that is the
   * point: nothing has been subtracted from it, so nothing can have been
   * painted into it). */
#ifdef EGIS_CR_HAVE_EVIDENCE
  probe_compute (raw_unfielded, &e->rawprobe);
#else
  probe_compute (raw_unfielded, &e->probe);
#endif
#ifdef EGIS_CR_HAVE_EVIDENCE
  /* The accept being corroborated is, first, the pose and template frame
   * the last egis_verify / egis_identify recorded for this entry; the raw
   * frame has to reproduce the fine structure there too
   * (EGIS_CR_RAW_EVIDENCE, with its measurement, in
   * egis_cr_tuning_gabor.h). No pose -- nothing was compared, or this is
   * not right after a verify of idx -- means nothing to corroborate. */
  {
    const CrEntry *en = &e->gallery[idx];
    EvPose pose;
    int fr;
    if (!en->last_pose.valid || en->last_frame < 0 || en->last_frame >= en->nframes)
      return 0;
    if (ev_evidence_at (&en->frames[en->last_frame], &e->rawprobe, &en->last_pose) >=
        EGIS_CR_RAW_EVIDENCE)
      return 1;
    /* verify's early exit recorded the first pose that cleared the accept
     * point, which is often the weakest one that does. Before the accept is
     * discarded, the raw frame gets the other poses the flat-fielded frame
     * the verify scored (still in e->probe) clears the accept point at: every
     * template frame's, at the pose its evidence was found at (v3 asked only
     * the best one over the template, EGIS_CR_RAW_ANY_POSE 0). Every pose
     * asked is chosen on the flat-fielded frame alone, never on the raw one.
     * Measured on the reference unit's nine development sets of 2026-09-27
     * (v3): of the finger-on genuine frames over the accept point, 92.7 %
     * corroborated at the early-exit pose alone and 98.7 % with the best
     * one, and four genuine presses the early-exit pose alone rejected were
     * accepted. v4, on the 16 development sets: 98.0 % of the genuine frames
     * over the accept point (97.5 % with v3's two poses), and two genuine
     * presses more than with the two poses; the attacks the check exists for
     * -- a featureless raw frame, another finger's raw frame, a blurred-out
     * contact -- corroborated 0 times behind each of those 10,603 frames
     * (EGIS_CR_RAW_ANY_POSE, egis_cr_tuning_gabor.h). The extra poses cost a
     * verify without the early exit on the frames that need them; no
     * impostor frame of the 16 sets clears the accept point, so none of
     * them ever needed them. */
    if (!en->last_cut)
      return 0;
#if EGIS_CR_RAW_ANY_POSE
    for (int i = 0; i < en->nframes; i++)
      if (ev_match (&en->frames[i], &e->probe, 0, &pose) >= EGIS_CR_ACCEPT_EVIDENCE && pose.valid &&
          ev_evidence_at (&en->frames[i], &e->rawprobe, &pose) >= EGIS_CR_RAW_EVIDENCE)
        return 1;
    (void) fr;
    return 0;
#else
    if (entry_best (&e->probe, en, 0, &pose, &fr) < 0 || fr < 0 || !pose.valid)
      return 0;
    return ev_evidence_at (&en->frames[fr], &e->rawprobe, &pose) >= EGIS_CR_RAW_EVIDENCE;
#endif
  }
#else
  /* Genuine raw probes against flat-fielded templates measured NCC >= 0.79
   * on the reference unit (Gabor front-end); a featureless contact whose
   * flat-fielded twin scored 0.93 through a poisoned baseline scores like a
   * blank here. */
  for (int i = 0; i < e->gallery[idx].nframes; i++)
    {
      double v = em_match (&e->gallery[idx].frames[i], &e->probe);
      if (v > best)
        best = v;
      if (best >= EGIS_CR_RAW_CORROBORATE_NCC)
        return 1;
    }
  return 0;
#endif
}

int
egis_identify (EgisEngine *e, const uint8_t *raw, int *out_idx)
{
  if (out_idx)
    *out_idx = -1;
  if (!e)
    return -1;
  poses_invalidate (e);
  if (!raw || e->gallery_n <= 0)
    return -1;
  probe_compute (raw, &e->probe);
  if (CR_PROBE_FRAME (&e->probe)->coverage < EGIS_CR_MIN_COVERAGE)
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
