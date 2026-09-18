/* egis_engine.h — clean API over the native Egis extractor/matcher for the
 * libfprint driver. Hides the flat-memory runtime and the internal state
 * machine. All images are raw 70x57 = 3990-byte grayscale sensor frames. */
#ifndef EGIS_ENGINE_H
#define EGIS_ENGINE_H
#include <stdint.h>

#define EGIS_IMG_W 70
#define EGIS_IMG_H 57
#define EGIS_IMG_SIZE 3990

/* Accept threshold on the match score. The Windows default (660) assumes a
 * 20-view chip-calibrated enrollment; on our raw multi-view templates the
 * genuine/impostor operating point sits higher (measured: genuine up to 19328,
 * impostor <= 4657). For the VENDOR matcher ~5000 has admitted nothing measured
 * since: across the three accuracy runs in docs/matcher-comparison.md all 1440
 * impostor comparisons scored exactly 0. That does NOT carry over to the
 * clean-room flavour, which compiles this same threshold and admitted 5 of 480
 * impostor comparisons on one of those runs (5238..5481) -- see
 * egis_engine_cleanroom.c. Genuine acceptance is not uniformly high either:
 * the three vendor runs rejected 0, 2 and 37 of 60 genuine presses, and on the
 * worst run 33 of the 37 scored exactly 0, so lowering the threshold would not
 * have recovered them. */
#define EGIS_THRESHOLD 5000

int egis_engine_init(void);            /* map + configure (mode 5). 0 = ok */

/* --- enrollment: build a template from several placements --- */
int egis_enroll_begin(void);
/* add one captured frame; returns 1=need more, 2=done, 4=redundant, <0 error.
 * *progress (0..100) is set if non-NULL. */
int egis_enroll_add(const uint8_t *raw, int *progress);
/* finalize -> serialized template blob copied into caller's buffer.
 * returns blob size (>0) or <0 on error; *out is malloc'd (caller frees). */
int egis_enroll_finish(uint8_t **out);

/* --- verify / identify against enrolled templates --- */
/* load N stored template blobs into the match gallery (call before matching) */
int egis_gallery_load(const uint8_t *const *blobs, const int *sizes, int n);
/* score a probe frame against gallery entry `idx`; returns score (<0 error) */
int egis_verify(const uint8_t *raw, int idx);

/* Corroborate an accept on the UN-flat-fielded sensor frame. The driver calls
 * this after egis_verify()/egis_identify() crossed the threshold, with the
 * raw bytes of the confirming frame (no flat-field, no preprocessing), and
 * only reports success if it returns 1. Closes the poisoned-baseline hole:
 * a flat-field baseline captured with the enrolled finger resting lightly
 * on the sensor paints that finger's ridges (inverted) into every later
 * frame, and a correlation matcher cannot tell an inverted, half-period
 * shifted copy from the real thing -- so a featureless smudge scored as the
 * enrolled finger. The raw frame cannot be painted: either the ridges are
 * physically there or they are not. Flavours without the concern (vendor)
 * return 1 unconditionally. */
int egis_verify_raw_ok(const uint8_t *raw_unfielded, int idx);

/* How many gallery entries egis_gallery_load() accepts. fprintd passes every
 * print of the user (up to 10 finger names), so a flavour that can hold them
 * all must say so, or fingers past the cap silently never authenticate. */
int egis_gallery_capacity(void);

/* --- byte-exact Windows per-frame preprocessing (egis_preprocess.c) ---
 * min-subtract -> invert -> auto-brightness -> Otsu stretch(0x8c) -> row-flip.
 * Normalises per-session brightness/contrast so a template enrolled in one
 * session matches a probe from another. Apply to BOTH enroll and verify frames. */
void egis_preprocess(const uint8_t *raw, uint8_t *out);
/* best match of a probe across the whole gallery; sets *out_idx to the matched
 * gallery index (or -1). returns the best score (<0 error). */
int egis_identify(const uint8_t *raw, int *out_idx);

#endif
