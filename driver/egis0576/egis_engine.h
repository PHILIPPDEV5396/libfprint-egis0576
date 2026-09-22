/* egis_engine.h -- the host matcher as the egis0576 driver sees it.
 *
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
 * One implementation is compiled in per build flavour. The driver knows
 * nothing about the matcher beyond this file: it hands in 70x57 8-bit frames
 * (per-boot flat-fielded), gets back integer scores, and stores whatever
 * bytes egis_enroll_finish() returns as the print's template. All images are
 * EGIS_IMG_SIZE bytes, row-major, one byte per pixel. */
#ifndef EGIS_ENGINE_H
#define EGIS_ENGINE_H
#include <stdint.h>

#define EGIS_IMG_W 70
#define EGIS_IMG_H 57
#define EGIS_IMG_SIZE 3990

/* Presses the driver collects for one enrolment; the engine may store fewer
 * (quality gates) and must accept exactly this many adds. */
#define EGIS_ENROLL_STAGES 12

/* Accept threshold on the match score: a verify or identify result >= this
 * is a match. Each engine scales its native score so that its own operating
 * point lands exactly here (clean-room adapter: NCC x 5000 / accept-NCC). */
#define EGIS_THRESHOLD 5000

/* One matcher instance. The clean-room adapter allocates real per-instance
 * state; a flavour whose matcher is inherently process-global may return a
 * singleton, in which case free is a no-op. NULL on failure. */
typedef struct EgisEngine EgisEngine;
EgisEngine *egis_engine_new (void);
void        egis_engine_free (EgisEngine *e);

/* --- enrolment: build a template from several presses --- */
int egis_enroll_begin (EgisEngine *e);
/* Add the frame of one press. Returns 1 = stored, more wanted; 2 = stored,
 * done; 4 = not stored, duplicate of a frame already in the session; 5 = not
 * stored, same placement as a stored frame (ask the user to shift the
 * finger); -2 = not stored, quality under the gate (ask for another press);
 * other < 0 = error. *progress (0..100) is set if non-NULL. */
int egis_enroll_add (EgisEngine    *e,
                     const uint8_t *frame,
                     int           *progress);
/* Serialise the template: *out is malloc'd (caller frees), returns its size
 * (> 0) or < 0 on error. The bytes are opaque to the driver. */
int egis_enroll_finish (EgisEngine *e,
                        uint8_t   **out);

/* --- verify / identify against stored templates --- */
/* How many templates one egis_gallery_load() accepts. */
int egis_gallery_capacity (EgisEngine *e);
/* Load n template blobs (as egis_enroll_finish produced them) into the
 * gallery; 0 = ok, < 0 = a blob is invalid (nothing is loaded). */
int egis_gallery_load (EgisEngine           *e,
                       const uint8_t *const *blobs,
                       const int            *sizes,
                       int                   n);
/* Score a frame against gallery entry idx; < 0 = nothing to score. */
int egis_verify (EgisEngine    *e,
                 const uint8_t *frame,
                 int            idx);
/* Best score of a frame across the gallery; *out_idx = the matched entry
 * (score >= EGIS_THRESHOLD) or -1. Returns the best score, < 0 = error. */
int egis_identify (EgisEngine    *e,
                   const uint8_t *frame,
                   int           *out_idx);
/* Corroborate an accept on the UN-flat-fielded sensor frame (raw bytes, no
 * flat-field): the driver reports success only if this returns 1. It closes
 * the case where a flat-field baseline captured with the enrolled finger
 * resting on the sensor paints that finger's ridges, inverted, into every
 * later frame, and a correlation matcher then accepts a featureless smudge --
 * nothing can be painted into the raw frame. Engines without the concern
 * return 1. */
int egis_verify_raw_ok (EgisEngine    *e,
                        const uint8_t *raw,
                        int            idx);

#endif
