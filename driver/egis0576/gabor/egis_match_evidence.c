/* egis_match_evidence.c -- identity evidence for the Gabor front-end
 *
 * Copyright (C) 2026 Philipp Oster
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * ---------------------------------------------------------------------------
 * WHY THIS EXISTS
 *
 * The Gabor front-end's masked NCC (egis_match_gabor.c) measures whether two
 * patches agree in ridge FLOW and ridge PERIOD, and that is not identity: on
 * the reference unit's 2026-09-18 session two different fingers of one
 * person scored up to 0.896 against a 0.78 threshold (egis_match_gabor.c,
 * LIMITS). What does tell skin apart is the fine structure the steered Gabor
 * filter discards -- ridge breaks, ridge-width and edge detail,
 * minutia-scale features. This file makes the accept decision on that alone.
 *
 * WHAT IT DOES
 *
 *   1. Pose hypotheses: a phase-invariant ENVELOPE correlation of the
 *      template's ridge image against the probe's ridge image and its
 *      quadrature, over +-EV_ROT_MAX deg (EV_ROT_COARSE steps) and
 *      +-EV_SRCH px (EV_CSTEP grid), down to EV_MIN_OVERLAP px of overlap.
 *      The EV_NHYP highest distinct peaks, plus the EV_NSIG most SIGNIFICANT
 *      ones not yet taken (envelope times the square root of the overlap it
 *      was computed over), are refined at full resolution; a hypothesis
 *      whose refined ridge-image NCC is under EV_GFLOOR is dropped. The
 *      ridge image only PROPOSES poses.
 *   2. At each hypothesis the frame is cut into blocks that tile it
 *      completely; each block with at least half its pixels in the overlap
 *      searches its own shift for the best masked NCC of the Gabor RESIDUAL
 *      (em_frame_residual), and blocks that reproduce the residual under one
 *      smoothly varying displacement form connected sets. When the best
 *      set's blocks agree on a mean shift of EV_RC_MIN px or more, the pose
 *      is moved by that shift once and the blocks search again around it
 *      (RE-CENTRING, ev_hyp_evidence); when the best set's shifts then say
 *      the probe is still turned against the template (a set of EV_RR_N
 *      blocks or more, EV_RR_MIN..EV_RR_MAX deg), the probe is turned by that
 *      much more and the blocks search once more (ROTATION RE-CENTRING).
 *      When no hypothesis clears the accept point, the EV_AR_H best ones are
 *      scored once more, moved by EV_AR_STEP px either way ALONG THE RIDGES of
 *      their overlap; there only a set of EV_AR_N independent blocks or more
 *      counts (ALONG-RIDGE HYPOTHESES).
 *   3. The evidence W of a set is the sum of its blocks' Fisher-z excess over
 *      EV_TAU, each block weighted by the area it adds to the union of the
 *      set's blocks, so that W counts INDEPENDENT AREA (ev_block_weight;
 *      v4.1 weighs set by set what v4 fixed by grid position); a set counts
 *      only when the area it actually scored -- its blocks' masked overlaps at
 *      their own shifts -- reaches EV_MIN_SCORED px, and below EV_AREA
 *      independent blocks its W is scaled down in proportion (THE AREA
 *      RULE: the accept point rises as the area shrinks). The result is the
 *      best over sets and hypotheses, in milli-W. The adapter accepts at
 *      W >= 2 (egis_engine_cleanroom.c) and corroborates an accept by the
 *      evidence of the un-flat-fielded frame, with the same statistic, at
 *      the pose the accept was found at (ev_evidence_at; which poses, when
 *      verify stops early, is the adapter's egis_verify_raw_ok).
 *
 * Why the blocks shift independently: on near-parallel ridges the ridge
 * image is nearly blind to a shift along the ridges, so its best alignment
 * lands a few px off, and two presses of one finger are related by a
 * smooth non-rigid displacement that no single rigid pose fits; one rigid
 * residual NCC over the whole overlap then decorrelates (measured on
 * 2026-09-18: the right thumb reproduced its own residual at median 0.04
 * across presses at the front-end's alignment, while blocks allowed their
 * own shift reproduced it at >= 0.6 in 18.5 of 21 blocks, at median offsets
 * of 4 px that vary smoothly from block to block). Why neighbour agreement:
 * each block's local maximum over its shifts finds noise peaks too;
 * requiring the offsets of adjacent blocks to agree, and summing only over a
 * connected set, keeps those from adding up.
 *
 * What the residual does and does not rule out: it is what the ridge image
 * (one local flow and one local period) does not explain, so two fingers
 * whose flow and period merely agree leave residuals that do not correlate
 * -- on the reference unit's sessions no frame of the same person's other
 * fingers reaches the accept point against any stored frame (at most
 * 0.99 x, 2026-09-13 templates against a 2026-09-18 probe). That single
 * frame is what the pose search found, not what the statistic can reach:
 * a dense scan of that pair over every rotation (2.5 deg) and shift (2 px)
 * within +-90 deg / +-48 px finds W 2.19 (1.09 x) at a pose none of the
 * hypotheses comes near. The margin for impostors is carried by the
 * driver's rule -- two consecutive frames and the raw-frame corroboration --
 * not by single frames staying under the accept point. It is NOT
 * proof against a structured texture: a synthetic pattern whose curvature or
 * period departs from the filter's single-period model leaves a structured
 * misfit in the residual, and that misfit can correlate. Measured on
 * 2026-09-27 against this file as of commit 5eb1590 (the verifier's port of
 * the out-of-tree attack harnesses: real twelve-press templates of the
 * 2026-09-13 and 2026-09-18 sessions, three frames per artefact press with
 * the sensor's fixed pattern and noise added, the driver's rule including
 * the raw check): fixed families of synthetic ridge textures -- gratings,
 * arcs and whorls, loops and deltas, chirps, period-modulated, wavy and fine
 * textures, 27,240 artefact-template presses -- stayed at or under 0.61 x
 * the accept point with no frame over it; but a score-ORACLE hill-climb of a
 * smooth 10-parameter synthetic ridge model (curvature plus one
 * period-modulation term, no minutiae), fed the returned score, reached W
 * 2.2-2.4 (1.10-1.19 x, three frames over the accept point, raw check
 * passed) against one real template (2026-09-18, right index) -- an accept
 * under the driver's rule; its other 39 climbs peaked at 0.57 x or less. So
 * an attacker who can query the score is not closed by this decision, as he
 * was not by the correlation it replaced.
 *
 * Why the search reaches small edge overlaps: on the reference unit's
 * 2026-09-18 session three genuine presses (left index 1, 7, 8) sat at
 * 35-41 px shift with 510-700 px of masked overlap, where the ridge image
 * matched at NCC 0.97-0.98 and 6-9 blocks reproduced the residual (W up to
 * 5.9) -- and the step-3 prototype's search (+-30 px, 800 px floor, blocks
 * missing the frame's last 6 columns and last row) never scored them. Until
 * v4 this said that W needs no overlap floor of its own, because a smaller
 * overlap holds fewer blocks and a chance W shrinks with it, and that the
 * floor only keeps the pose ranking meaningful, at about the least area in
 * which W can reach the accept point at a genuine press's block NCC (three
 * blocks at the 8 px step, 16 x 32 px). The 2026-09-27 session showed that
 * wrong: see "Why independent area" and after. The translation range is
 * about the largest shift that still leaves that much (a 70 x 15 px strip
 * at the masks' typical 0.7 x 0.7 coverage), and it stays.
 *
 * Why the significance proposals: an NCC over a small overlap reaches a
 * high value by chance far more easily than one over a large overlap, and
 * on near-parallel ridges the envelope's highest peaks are small edge
 * overlaps that the flow happens to fit. A genuine pose with a large
 * overlap, whose rigid correlation the skin's distortion has lowered, then
 * ranks far down. Measured on the two sessions of the reference unit,
 * enrolled on one and probed with the other (the right thumb came back
 * rotated by 50-55 deg): a dense scan over every rotation and shift found
 * W 6-14 (2026-09-18 templates, a 2026-09-13 probe of the right thumb) and
 * W 3-11 (the other way round) at poses of small shift and large overlap
 * that ranked 11th-46th among the distinct envelope peaks, or were no peak
 * at all, while E sqrt(n) ranked some of them first. With the EV_NSIG
 * proposals the weakest accepted genuine press of 2026-09-18 -> 2026-09-13
 * rises from 1.69 x to 2.89 x the accept point (1.11 x in the first
 * identity-gate build), no genuine press's level falls and no impostor
 * press's level under the driver's rule changes on any of the four sets
 * (below). They replace the orientation-field rotation proposals of that
 * first build (commit 5eb1590): added back on top of them, those change no
 * genuine press's level on any of the four sets, raise one impostor press
 * (09-18, to 0.25 x), and make each template frame 13 % slower (11.2
 * against 9.9 ms at -O2).
 *
 * Why the re-centring: on near-parallel ridges the refined pose can sit
 * 6-10 px from where the fine structure reproduces, displaced ALONG the
 * ridges, where the ridge image cannot tell; the blocks' own +-EV_RS px
 * search then ends at its border, and the set they form is short. Their
 * mean shift says where the pose should be. Measured on the hypotheses of
 * the first identity-gate build (2026-09-13 templates, 2026-09-18 probes):
 * three genuine presses were rejected for exactly this -- left index 9 (the
 * hypothesis at +33 px, the fine structure at +22..24 px: all 9 eligible
 * blocks in the set, W 1.6-1.8, every one at or next to the -6 px border of
 * its search; W 2.5-2.6 at the moved pose) and right thumb 5 and 9 -- and
 * all three are accepted with it (right thumb 5 also by the significance
 * proposals alone; left index 9 and right thumb 9 only by this). It is
 * selective:
 * over the four development sets (below) it raises the evidence of 2,821 of
 * 20,980 genuine frame pairs (1,897 by more than W 0.5) and of 194 of
 * 167,970 impostor frame pairs, 4 of them by more than W 0.5 -- one, of the
 * 2026-09-13 templates against a 2026-09-18 probe, from 0.66 to 1.98 (the
 * 0.99 x above).
 *
 * Why the rotation re-centring: the refinement turns the probe in EV_REF_ROT
 * steps within half a coarse step of the envelope's rotation and keeps the
 * turn with the best RIDGE-IMAGE NCC -- which, like the shift, the ridge
 * image resolves poorly on near-parallel ridges. At a pose a few degrees
 * off, the blocks still find the fine structure, each at its own shift, but
 * the shifts form a rotation field -- they drift by 2-3 px across the frame
 * -- and the set breaks where neighbours disagree by more than EV_EPS. The
 * field says how far to turn: the shifts are fitted as a common shift plus
 * a small turn about the set (least squares over the set's blocks), and the
 * evidence is looked for again with the probe turned by that much and the
 * set's centroid kept where it was. Measured on the nine development sets
 * of 2026-09-27 (the reference unit's three sessions, in session and across
 * them in all six directions): the reference unit's session of 2026-09-26,
 * right index 1 against its 2026-09-13 templates, refined at +20 deg with
 * W 2.2-2.9 on five frames, 12-17 blocks whose shifts run from (-3, +2) px
 * at the top left to (-1, -1) px at the bottom right; the raw frame, whose
 * block NCCs sit about 0.1 lower, broke that set into pieces and did not
 * corroborate it (raw W 0.41-0.69 against 0.75). An exhaustive scan over
 * every rotation (2.5 deg) and shift (2 px) puts the fine structure at
 * +27.5 deg (W 5.1 on the first frame); one rotation step takes the pose to
 * about +25 deg, the raw frame corroborates, and the press is accepted. The
 * step is selective: over the nine sets it raises the evidence of 22,780 of
 * 53,028 genuine frame pairs and of 14 of 258,930 impostor pairs, none of
 * those by more than W 0.5 (EV_RR_N has the numbers).
 *
 * Why a block needs half its pixels, at full weight: weighting each block's
 * atanh(r) by sqrt(n / 256) and letting blocks of >= 48 px count --
 * per-pixel significance -- was measured on 2026-09-18 and not adopted: at
 * the same accept point it rejected 5 of 60 genuine presses instead of 3
 * (the strongest impostor fell from W 0.89 to 0.51, the separation between
 * the weakest accepted genuine press and the strongest impostor stayed about
 * the same), and counting such partial blocks at full weight accepted 3 of
 * 480 impostor presses.
 *
 * v4 (2026-09-28). v3 (commit 743b796) was certified on the reference
 * unit's session of 2026-09-27 (S4) and FAILED: one false accept -- S4's
 * right thumb press 0 taken for the left index's fold-1 template, at
 * 1.083 x the accept point -- and more false rejects than the vendor
 * matcher on three sets. The false accept had been there since the edge
 * reach (commit f144f69); every version since accepted it bit for bit. It
 * was a 6-block set in a 527 px bottom-left corner overlap, and every
 * impostor near miss over 0.44 x was the same kind of thing: an edge or
 * corner strip of 483-1012 px, 2-7 blocks. v4 changes what W counts, not
 * the search, except for one addition (the along-ridge hypotheses). S4 is
 * development data now, like S1-S3.
 *
 * Why independent area: W summed Fisher-z over blocks as if they were
 * independent. The row of blocks anchored at the bottom border (y0 = 41)
 * shares 15 of its 16 rows with the regular row above it (y0 = 40), and the
 * column anchored at the right border (x0 = 54) 10 of its 16 columns with
 * its neighbour (8 is the grid's own); a set on the border counted those
 * pixels twice. In the false accept the border row carried W 0.55 of 2.16.
 * Each block now counts with the share of the tiling it adds beyond its
 * predecessor, in grid steps (ev_grid_weight): 1 for every block of the
 * regular grid -- whose 50 % overlap is the unit the accept point was set in
 * -- 0.75 for the right border column and 0.125 for the bottom border row;
 * a set's INDEPENDENT AREA is the sum of its blocks' weights. The false
 * accept's set: W 1.68 over 4.25 independent blocks.
 *
 * v4.1 (2026-09-28): those weights were FIXED by grid position -- a border
 * block counted 0.75 or 0.125 whether or not the regular block whose pixels
 * it repeats was in the set, so a border block that repeated nothing was
 * discounted too. Now each block counts the area it adds to the UNION of its
 * set's blocks (ev_block_weight): along each axis, row by row (column by
 * column), the columns (rows) it adds beyond the nearest block of the set
 * before it that covers that row, in grid steps, at most 1. Every block of
 * the regular grid still counts 1; a border block counts v4's 0.75 or 0.125
 * where the set holds its regular neighbour, 1 where the set holds nothing
 * before it, and in between by the share of its rows (columns) the set's
 * blocks before it cover; with every block in the set it is v4's weight
 * exactly. The pixels the false accept counted twice are still counted
 * once: where the neighbour is in the set, nothing changes. What it does on
 * the 16 development sets is in MEASURED: nothing to a decision. It raises
 * some genuine frames that are accepted anyway; in the sets that carry the
 * evidence of the presses v4 rejects, the border blocks come with their
 * regular neighbours (or there are none), and what holds those presses is
 * the floor on the area scored with the area rule, the raw check, or
 * evidence under the accept point even without them -- not the border
 * weights.
 *
 * Why the floor on the area scored: EV_MIN_OVERLAP gated only the
 * proposals. The re-centring, and the blocks' own +-EV_RS px search, reach
 * past a proposed pose: the false accept's evidence came from a pose of 527
 * px of overlap, and from a hypothesis 680 px nominally, whose blocks at the
 * edge of their search scored 357 template pixels between them. The floor
 * now also applies where the evidence is computed (EV_MIN_SCORED = the same
 * 500 px): a set counts only if the union of its blocks' masked overlaps,
 * at their own shifts, is that large.
 *
 * Why the area rule: a small set reaches W 2 by chance far more easily
 * than a large one -- a few blocks, each the best of 169 shifts, over 8
 * hypotheses and up to 12 template frames -- and on the 16 development sets
 * no impostor pair ever formed a set of more than 7 blocks (EV_AREA). So
 * the evidence a set must carry per independent block rises as the set
 * shrinks: from EV_AREA (8) independent blocks on the accept point is W 2,
 * below that W is scaled by a / 8, i.e. a set of 6 needs W 2.67 (a block
 * NCC of about 0.81 on each), of 4 W 4 (about 0.93). The alternative the
 * diagnosis offered, a MINIMUM area (a set under 8 counts nothing,
 * EV_AREA_HARD), was measured the same way and not adopted: it throws away
 * small-overlap evidence however strong -- the reason the search reaches
 * edge overlaps at all -- and on the 16 sets it rejects 53 of 960 genuine
 * presses against the rising accept point's 46, for the same impostor
 * result at press level (no impostor press above 0; the highest single
 * impostor frame 0 against 0.72 x) (MEASURED).
 *
 * What these three cost: the genuine presses whose evidence lay in exactly
 * that regime. Of the presses v3 accepted on the 16 development sets, v4
 * rejects five: S2's left index 7 and 8 (two of the three presses the edge
 * reach was built for; their best sets: 9 blocks of independent area 5.8,
 * 434 px scored, W 2.3 on one frame and under 1.8 on the next), S2 -> S4's
 * left middle 2 (4-5 blocks, 357-443 px, W 1.7-2.1 on every frame -- the
 * false accept's regime to the digit), S4 -> S2's right index 9 (5-7
 * blocks, 312-496 px) and S3 -> S2's right thumb 2 (7-block sets at
 * W 2.0-2.5, on every other frame). The vendor matcher rejects two of them
 * (S2 left index 8, S4 -> S2 right index 9) as well. All five carry their
 * evidence on sets of an impostor's size (4-7 blocks, under 8 independent
 * blocks), the regime the area rule and the floor exist to discount; by
 * what this statistic measures they are not distinguishable from the false
 * accept and the near misses.
 *
 * Why along-ridge hypotheses: two genuine presses v3 rejected and the
 * vendor matcher accepts were pure pose-finding failures (the diagnosis of
 * 2026-09-27, exhaustive scan): S4 -> S3's left middle 8, whose best
 * hypothesis (10 deg, (17, 6)) fits the ridge image better (NCC 0.79-0.82)
 * than the fine structure's own pose does (0.72), 10 px along the ridges
 * at 12.5 deg, (14, -4), where 21-27 blocks reproduce the residual at
 * W 3.7-5.0 on every frame; and S1 -> S4's right thumb 3, whose fourth
 * hypothesis lands 13 px and 5 deg off a 12-16-block set at W up to 5.5. On
 * near-parallel ridges the ridge image cannot tell where along them the
 * pose is, and the blocks' +-6 px cannot reach 10-14 px. A wider search
 * lifts impostors (the exhaustive scan took the false accept from 1.083 to
 * 1.125 x and another impostor press from 0.56 to 0.97 x) -- but only as
 * sets of 4-6 blocks. So the extra poses are few and admit only large sets:
 * the EV_AR_H (4) best hypotheses, moved by EV_AR_STEP (12) px either way
 * along the ridge direction of their overlap (the structure tensor of the
 * template's ridge image over it, ev_ridge_dir), after every hypothesis has
 * been scored and none cleared the accept point, counting only sets of
 * EV_AR_N (10) independent blocks or more. Both presses are accepted.
 *
 * Why cells (ev_block_search): the block search is where the extra poses
 * cost; summing each cell of the grid once per shift and every block from
 * its cells makes a pose about a third as expensive as summing every block
 * on its own, with the same result (see EvCells). That pays for the
 * along-ridge hypotheses and more (COST).
 *
 * MEASURED (tools/accuracy of the out-of-tree repository, kit_version 5:
 * the engine built as the driver builds it, verify's early exit on; the
 * driver's two-frame + raw rule; the gallery reloaded before every press;
 * the reference unit's sessions S1 = 2026-09-13, S2 = 2026-09-18,
 * S3 = 2026-09-26 and S4 = 2026-09-27, one person, impostors = the same
 * person's other fingers). In session: the kit's two folds of six-press
 * templates, 480 impostor presses. Across sessions: twelve-press templates
 * of one session, genuine and impostor probes of the other (240 impostor
 * presses). 960 genuine and 4,800 impostor presses in all. IN-SAMPLE: every
 * constant here and in the adapter was set on these 16 sets or a part of
 * them. S3 was the certification session of commit e27b8bc (v2) and S4 of
 * commit 743b796 (v3); both failed, and both are development data since.
 *
 *     set        rejects  accepts  strongest impostor   weakest   rejects
 *                                  press    one frame   genuine   v3   vendor
 *     S1         0 / 60   0 / 480  0.00 x   0.11 x      2.39 x     0     0
 *     S2         5 / 60   0 / 480  0.00 x   0.32 x      1.86 x     3     4
 *     S3         1 / 60   0 / 480  0.00 x   0.09 x      1.55 x     1     1
 *     S4         5 / 60   0 / 480  0.00 x   0.30 x      1.41 x     5     4
 *     S1 -> S2   6 / 60   0 / 240  0.00 x   0.25 x      1.18 x     6     7
 *     S2 -> S1   0 / 60   0 / 240  0.00 x   0.00 x      1.62 x     0     0
 *     S1 -> S3   5 / 60   0 / 240  0.00 x   0.18 x      1.25 x     5     3
 *     S3 -> S1   0 / 60   0 / 240  0.00 x   0.21 x      1.23 x     0     0
 *     S2 -> S3   0 / 60   0 / 240  0.00 x   0.46 x      1.06 x     0     2
 *     S3 -> S2   3 / 60   0 / 240  0.00 x   0.72 x      1.28 x     2     3
 *     S1 -> S4   6 / 60   0 / 240  0.00 x   0.00 x      1.24 x     7     7
 *     S4 -> S1   0 / 60   0 / 240  0.00 x   0.06 x      1.28 x     0     0
 *     S2 -> S4   4 / 60   0 / 240  0.00 x   0.18 x      1.13 x     3     3
 *     S4 -> S2   6 / 60   0 / 240  0.00 x   0.30 x      1.09 x     5     3
 *     S3 -> S4   3 / 60   0 / 240  0.00 x   0.21 x      1.39 x     3     4
 *     S4 -> S3   2 / 60   0 / 240  0.00 x   0.18 x      1.46 x     3     1
 *     pooled    46 / 960  0 / 4800                                43    42
 *
 *     ("press" = the highest threshold at which the driver's rule would
 *     accept an impostor press, as a multiple of the accept point; "one
 *     frame" = the highest single impostor frame score; a genuine level over
 *     the accept point is a lower bound, since verify stops at the first
 *     template frame that clears it.) No impostor press of the 4,800 reaches
 *     any level above 0: none has two consecutive corroborated frames with
 *     any evidence. Of the 56,960 on-gate impostor frames, 127 score above 0,
 *     5 reach 0.3 x, one 0.72 x (S3 -> S2, the right index template against
 *     right middle press 5, a 7-block set). v3 on the same sets: pooled
 *     43 / 960 rejects, 1 / 4800 accepts (S4, 1.083 x), 8 impostor presses at
 *     0.5 x or more, 19 impostor frames at 0.67 x or more, 7 at 1.0 x or
 *     more.
 *
 *     v4.1 (the set-dependent weights, ev_block_weight): every figure of the
 *     table is v4's -- the same decision on all 5,760 presses (the kit's
 *     cross.py and the per-frame runs agree on every press). Frame by frame:
 *     638 of the 11,428 genuine frame scores differ from v4's (634 higher; 4
 *     lower, where verify's first pose over the accept point moved), no raw
 *     verdict differs; 2 of the 57,140 impostor frames rise (S1 -> S3
 *     0.177 -> 0.183 x, S4 -> S2 0.125 -> 0.129 x), none falls. The impostor
 *     tail: every press at level 0; without the raw check (the higher of two
 *     consecutive on-gate flat-fielded frames' lower score) at most 0.196 x,
 *     none at 0.3 x; single frames 127 over 0, 5 at 0.3 x, 1 at 0.5 x and at
 *     0.67 x (0.721 x), none at 1.0 x -- v4's to the frame. v3's false accept
 *     (S4, left index fold 1 against right thumb 0) scores 0 on every frame,
 *     and each of the 42 impostor presses that reached 0.5 x under v3 or had
 *     a v3 or v4 frame at 0.5 x has v4's level and frame maximum.
 *
 *     The bar the owner set (2026-09-27) is not met on these sets: pooled
 *     false rejects 46 against the vendor matcher's 42. Disputed presses
 *     (rejected by one matcher only): 11 by v4 only, 7 by the vendor only,
 *     35 by both. Of the 11: S2 left index 7, S3 -> S2 right thumb 2 and
 *     S2 -> S4 left middle 2 are the area accounting's (above); S1 -> S3
 *     right index 10 and S4 -> S2 right thumb 0 are raw-check-bound (large
 *     sets, 10-18 blocks, at W 1.7-2.7 on the flat-fielded frames, and no
 *     set at all on the raw frames at those poses); S4's left middle 2 is
 *     a 4-5-block corner the ridge image never proposes (ridge NCC about
 *     0, against EV_GFLOOR 0.50) and would be the false accept's regime if
 *     it did; S1 -> S3 right middle 3, S4 -> S2 right thumb 8 and right
 *     index 5 and S4 -> S3 right index 10 stay under the accept point even
 *     under an exhaustive scan of every rotation and shift (0.44-0.87 x
 *     with v3's statistic, which this one never exceeds at a given pose);
 *     S3 -> S2 right thumb 0 shows no evidence at any pose the search finds
 *     (sets of 2-4 blocks, W <= 0.5).
 *
 *     v4.1 does not change that. Traced without the floor and the area rule
 *     (so that the search lands on the strongest evidence there is), the
 *     best set on every frame of those 11 presses and of the two more v4 and
 *     the vendor matcher both reject (S2 left index 8, S4 -> S2 right
 *     index 9) has the same independent area under v4.1 as under v4, to
 *     within 0.125 blocks: its border blocks come with their regular
 *     neighbours, or it has none. What holds them, frame by frame: the
 *     floor on the area scored with the area rule for three (S2 left index
 *     7 and 8, S2 -> S4 left middle 2: sets of 4-9 blocks, 2.3-5.8
 *     independent, at W up to 3.2 over 244-443 px); the area rule for S3 ->
 *     S2 right thumb 2 (over the accept point on frames 0, 3, 5 and 9, never
 *     two in a row: its 7-block sets in between, W 1.98-2.04, count
 *     1.73-1.79); the raw check for S4 -> S2 right thumb 0 (frames 0 and 1
 *     over, a 3-block raw set); S1 -> S3 right index 10 sits just under the
 *     accept point on sets of 10-14 blocks (0.66-1.08 x, over on frames 7
 *     and 9 only, so not raw-check-bound as said above); the other seven
 *     stay under it even without floor and area rule (W <= 1.76). Not the
 *     border weights.
 *
 *     Sensitivity, all 16 sets, each new or changed constant moved by
 *     +-10 % (integers to their neighbours), everything else as shipped:
 *
 *       constant           value  pooled rejects  impostor presses  highest
 *                                  (46 shipped)    accepted / level  frame
 *       EV_AREA            7.2        46             0 / 0.00 x      0.80 x
 *                          8.8        47             0 / 0.00 x      0.66 x
 *       EV_MIN_SCORED      450        46             0 / 0.00 x      0.72 x
 *                          550        48             0 / 0.00 x      0.72 x
 *       EV_AR_N            9          45             0 / 0.00 x      0.72 x
 *                          11         46             0 / 0.00 x      0.72 x
 *       EV_AR_STEP         10.8       46             0 / 0.00 x      0.72 x
 *                          13.2       46             0 / 0.00 x      0.72 x
 *       EV_AR_H            3          47             0 / 0.00 x      0.72 x
 *                          5          46             0 / 0.00 x      0.72 x
 *       EV_AREA_HARD       1          53             0 / 0.00 x      0.00 x
 *       EGIS_CR_RAW_ANY_POSE 0        48             0 / 0.00 x      0.72 x
 *
 *     (8.8: S4 -> S2 +1; 550: S4 +1, S4 -> S2 +1; 9: S3 -> S2 -1; EV_AR_H 3
 *     loses S1 -> S4's right thumb 3; the minimum area: S2, S4, S2 -> S3,
 *     S1 -> S4 and S4 -> S2 +1 each, S2 -> S4 +2; two poses for the raw
 *     check: S1 -> S2 and S3 -> S2 +1 each.) No setting accepts an impostor
 *     press or gives one a level above 0; the ones that move the pooled
 *     rejects by two or more are the floor at 550 and the minimum area.
 *
 *     That table is v4's. v4.1 adds and moves no constant; the two stated in
 *     independent blocks, whose accounting it changes, were measured again
 *     under it, the same way: EV_AREA 7.2 46 rejects, highest impostor frame
 *     0.80 x; 8.8 47 (S4 -> S2 +1), 0.66 x; EV_AR_N 9 45 (S3 -> S2 -1),
 *     0.72 x; 11 46, 0.72 x; each with no impostor press accepted or above
 *     level 0 -- every figure v4's.
 *
 * COST (egis_verify as the driver calls it, early exit on; gcc 16, one
 * core; four twelve-press templates -- S1 right index and left middle
 * probed with S2, S3 right thumb with S4, S4 left index with S2 -- against
 * presses 0, 3, 6, 9 of the template finger and presses 0 and 6 of every
 * other finger, best of three per frame; 3,757 template-frame calls): per
 * template frame 10.1 ms mean, 13.8 ms p95 at -O2 (11.4 / 14.8 at -O3),
 * against the shipped matcher's (v0.5.1, the front-end's NCC through the
 * same adapter call) 4.1 / 5.4 ms (4.5 / 5.8): 2.47 x the mean and 2.58 x
 * the p95 (-O3: 2.52 x and 2.58 x). v3 on the same sample: 10.0 / 14.2 ms
 * (2.45 x, 2.64 x). Genuine and impostor probes alike stay under 3 x
 * (-O2: impostor 10.0 / 13.7 ms against 4.0 / 5.0, genuine 10.8 / 17.0
 * against 4.4 / 5.9). A whole impostor verify takes 90 ms on average at
 * -O2. The block search by cells (ev_block_search) takes a pose's evidence
 * to about a third of what summing every block on its own cost; that
 * pays for the along-ridge hypotheses. On the first two templates alone
 * (shipped matcher 3.75 / 4.87 ms): without them 8.2 / 10.6 ms per
 * template frame; as shipped (the four best, +-12 px) 9.6 / 13.9 ms
 * (2.57 x, 2.85 x); on all eight hypotheses at +-6 and +-12 px (the first
 * design) 11.5 / 21.2 ms (3.1 x, 4.4 x); on the four best at +-6 and
 * +-12 px 10.6 / 16.0 ms (p95 3.3 x). The raw check asks its extra poses
 * (a verify without the early exit) on 5 of the 172 genuine frames over
 * the accept point in this sample: 6.1 ms per accepted frame on average,
 * at most 108 ms (v3: 6.2 and 103).
 *
 * v4.1, on the v4 verifier's bench (the same egis_verify call; 20 twelve-
 * press templates, every finger in four cross directions, S1 -> S2, S2 -> S3,
 * S3 -> S4, S4 -> S1; presses 0, 4, 8 of the template finger and 2, 7 of
 * every other, their on-gate frames 1-3; one pinned core, best of three
 * rounds, the builds' order rotated; 658 verifies): per template frame
 * 10.2 ms mean, 12.7 ms p95 at -O2 (11.4 / 13.8 at -O3) against the shipped
 * matcher's 4.4 / 5.5 (4.8 / 5.8): 2.32 x and 2.32 x (-O3: 2.37 x, 2.38 x);
 * impostor frames 2.37 x / 2.62 x. v4 in the same run: 2.32 x / 2.33 x. The
 * weights per set cost nothing measurable. The raw check where the raw
 * frame does not corroborate (a featureless raw frame behind each of the
 * 165 genuine frames over the accept point): 134 ms mean, 183 p95, 206 max
 * at -O2 (v4 142 / 189 / 210; the shipped matcher 37 / 52 / 53) -- the
 * adapter now skips the template frames verify already found under the
 * accept point, which is result-identical but saves only where verify did
 * not stop at the first template frame (61 of the 165).
 *
 * Plain C99, libm only, deterministic, no global or static state: every
 * call's scratch is malloc'd and freed before it returns.
 * ---------------------------------------------------------------------------
 */
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "egis_match_evidence.h"

#define EV_PI 3.14159265358979323846   /* M_PI is not C99 */

/* ---- sampling helpers (the front-end's bilinear arithmetic) --------------- */
static double
ev_bil_d (const double *a, double sx, double sy)
{
  int x0 = (int) floor (sx), y0 = (int) floor (sy);
  double fx = sx - x0, fy = sy - y0;

  if (sx < 0 || sx > EM_W - 1 || sy < 0 || sy > EM_H - 1)
    return 0.0;
  if (x0 >= EM_W - 1)
    {
      x0 = EM_W - 2;
      fx = 1.0;
    }
  if (y0 >= EM_H - 1)
    {
      y0 = EM_H - 2;
      fy = 1.0;
    }
  return a[y0 * EM_W + x0] * (1 - fx) * (1 - fy)
         + a[y0 * EM_W + x0 + 1] * fx * (1 - fy)
         + a[(y0 + 1) * EM_W + x0] * (1 - fx) * fy
         + a[(y0 + 1) * EM_W + x0 + 1] * fx * fy;
}

static double
ev_bil_u8 (const uint8_t *a, double sx, double sy)
{
  int x0 = (int) floor (sx), y0 = (int) floor (sy);
  double fx = sx - x0, fy = sy - y0;

  if (sx < 0 || sx > EM_W - 1 || sy < 0 || sy > EM_H - 1)
    return 0.0;
  if (x0 >= EM_W - 1)
    {
      x0 = EM_W - 2;
      fx = 1.0;
    }
  if (y0 >= EM_H - 1)
    {
      y0 = EM_H - 2;
      fy = 1.0;
    }
  return a[y0 * EM_W + x0] * (1 - fx) * (1 - fy)
         + a[y0 * EM_W + x0 + 1] * fx * (1 - fy)
         + a[(y0 + 1) * EM_W + x0] * (1 - fx) * fy
         + a[(y0 + 1) * EM_W + x0 + 1] * fx * fy;
}

/* caller guarantees 0 <= sx <= EM_W-1, 0 <= sy <= EM_H-1 */
static double
ev_bil_f (const float *a, double sx, double sy)
{
  int x0 = (int) floor (sx), y0 = (int) floor (sy);
  double fx = sx - x0, fy = sy - y0;

  if (x0 >= EM_W - 1)
    {
      x0 = EM_W - 2;
      fx = 1.0;
    }
  if (y0 >= EM_H - 1)
    {
      y0 = EM_H - 2;
      fy = 1.0;
    }
  return a[y0 * EM_W + x0] * (1 - fx) * (1 - fy) + a[y0 * EM_W + x0 + 1] * fx * (1 - fy)
         + a[(y0 + 1) * EM_W + x0] * (1 - fx) * fy + a[(y0 + 1) * EM_W + x0 + 1] * fx * fy;
}

/* ---- per-frame features ----------------------------------------------------- */
void
ev_frame_compute (const uint8_t *raw, EvFrame *F)
{
  em_frame_residual (raw, &F->f, F->res, EV_RES_SIGMA);
}

/* Quadrature of the ridge image: for a locally plane ridge wave g = A cos(k.x)
 * the Riesz transform is -grad(g)/|k|; the scale is fixed by energy balance
 * over the mask instead of by |k|. */
void
ev_probe_compute (const uint8_t *raw, EvProbe *P)
{
  const double *g = P->e.f.img;
  double eg = 0, eq = 0, sc;

  em_frame_residual (raw, &P->e.f, P->e.res, EV_RES_SIGMA);
  memset (P->q1, 0, sizeof P->q1);
  memset (P->q2, 0, sizeof P->q2);
  for (int y = 1; y < EM_H - 1; y++)
    for (int x = 1; x < EM_W - 1; x++)
      {
        int i = y * EM_W + x;
        if (!P->e.f.mask[i])
          continue;
        P->q1[i] = (float) (-(g[i + 1] - g[i - 1]) * 0.5);
        P->q2[i] = (float) (-(g[i + EM_W] - g[i - EM_W]) * 0.5);
        eg += g[i] * g[i];
        eq += (double) P->q1[i] * P->q1[i] + (double) P->q2[i] * P->q2[i];
      }
  sc = eq > 1e-12 ? sqrt (eg / eq) : 0.0;
  for (int i = 0; i < EM_N; i++)
    {
      P->q1[i] = (float) (P->q1[i] * sc);
      P->q2[i] = (float) (P->q2[i] * sc);
    }
}

/* ---- coarse hypothesis search ----------------------------------------------
 * Envelope correlation: the template's ridge image a against the probe's
 * (g, q1, q2) on an EV_CSTEP-px grid, E = sqrt(cov(a,g)^2 + cov(a,q1)^2 +
 * cov(a,q2)^2) / sqrt(var a var g). E does not change sign with the ridge
 * phase, so a grid coarser than the ridge period samples it safely; the phase
 * is recovered by the full-resolution refinement. */
#define EV_GX ((EM_W + EV_CSTEP - 1) / EV_CSTEP)   /* 24 */
#define EV_GY ((EM_H + EV_CSTEP - 1) / EV_CSTEP)   /* 19 */
#define EV_GN (EV_GX * EV_GY)
#define EV_S2 (2 * (EV_SRCH / EV_CSTEP) + 1)

typedef struct
{
  double s, rot;
  int    dx, dy;
  double q;          /* coarse significance, env * sqrt (overlap points) */
} EvHyp;
#define EV_NH (EV_NHYP + EV_NSIG)       /* most hypotheses per pair */

typedef struct
{
  float ta[EV_GN], tm[EV_GN], ta2[EV_GN];
  float pm[EV_GN], pg[EV_GN], pg2[EV_GN], p1[EV_GN], p2[EV_GN];
  float fa[EM_N], fm[EM_N], fa2[EM_N];          /* template, full resolution */
  float qa[EM_N], qm[EM_N], qa2[EM_N];          /* rotated probe, full res.  */
  float land[EV_S2 * EV_S2], landq[EV_S2 * EV_S2];
} EvWork;

static void
ev_grid_template (const EvFrame *A, EvWork *W)
{
  for (int j = 0; j < EV_GY; j++)
    for (int i = 0; i < EV_GX; i++)
      {
        int x = EV_CSTEP * i, y = EV_CSTEP * j, k = j * EV_GX + i;
        float v = 0, mk = 0;
        if (x < EM_W && y < EM_H && A->f.mask[y * EM_W + x])
          {
            v = (float) A->f.img[y * EM_W + x];
            mk = 1;
          }
        W->ta[k] = v;
        W->tm[k] = mk;
        W->ta2[k] = v * v;
      }
}

/* probe rotated by th about the centre, sampled at the grid points */
static void
ev_grid_probe (const EvProbe *B, double th, EvWork *W)
{
  double cy = (EM_H - 1) / 2.0, cx = (EM_W - 1) / 2.0, ct = cos (th), st = sin (th);

  for (int j = 0; j < EV_GY; j++)
    for (int i = 0; i < EV_GX; i++)
      {
        int k = j * EV_GX + i;
        double dx = EV_CSTEP * i - cx, dy = EV_CSTEP * j - cy;
        double sx = cx + dx * ct - dy * st, sy = cy + dx * st + dy * ct;
        W->pm[k] = W->pg[k] = W->pg2[k] = W->p1[k] = W->p2[k] = 0.0f;
        if (EV_CSTEP * i >= EM_W || EV_CSTEP * j >= EM_H ||
            sx < 0 || sx > EM_W - 1 || sy < 0 || sy > EM_H - 1)
          continue;
        if (ev_bil_u8 (B->e.f.mask, sx, sy) <= 0.6)
          continue;
        W->pm[k] = 1.0f;
        W->pg[k] = (float) ev_bil_d (B->e.f.img, sx, sy);
        W->pg2[k] = W->pg[k] * W->pg[k];
        W->p1[k] = (float) ev_bil_f (B->q1, sx, sy);
        W->p2[k] = (float) ev_bil_f (B->q2, sx, sy);
      }
}

/* template grid point (i, j) <-> probe grid point (i - dx/step, j - dy/step) */
static double
ev_grid_env (const EvWork *W, int dx, int dy, double *npts)
{
  int ox = -dx / EV_CSTEP, oy = -dy / EV_CSTEP;
  int j0 = oy < 0 ? -oy : 0, j1 = EV_GY - (oy > 0 ? oy : 0);
  int i0 = ox < 0 ? -ox : 0, i1 = EV_GX - (ox > 0 ? ox : 0);
  double n = 0, sa = 0, sg = 0, saa = 0, sgg = 0, sag = 0, s1 = 0, sa1 = 0, s2 = 0, sa2 = 0;
  double va, vg, c0, c1, c2;

  for (int j = j0; j < j1; j++)
    {
      /* row pointers at the first element used (column i0), so none points
       * before its array's start; i below counts from there */
      int t0 = j * EV_GX + i0, o = (j + oy) * EV_GX + ox + i0, ni = i1 - i0;
      const float *ta = W->ta + t0, *tm = W->tm + t0, *ta2 = W->ta2 + t0;
      const float *pm = W->pm + o, *pg = W->pg + o, *pg2 = W->pg2 + o, *p1 = W->p1 + o, *p2 = W->p2 + o;
      float L[10][4] = { { 0 } };
      float rn, ra, rg, raa, rgg, rag, r1, ra1, r2, ra2;
      int i = 0;
      /* four interleaved partial sums per quantity, summed in a fixed order:
       * the lanes are independent, so the compiler can keep them in vector
       * registers (a third of this search's time at -O2) */
      for (; i + 4 <= ni; i += 4)
        for (int l = 0; l < 4; l++)
          {
            float a = ta[i + l], m = tm[i + l], q = pm[i + l], g = pg[i + l];
            L[0][l] += m * q;
            L[1][l] += a * q;
            L[2][l] += ta2[i + l] * q;
            L[3][l] += m * g;
            L[4][l] += m * pg2[i + l];
            L[5][l] += a * g;
            L[6][l] += m * p1[i + l];
            L[7][l] += a * p1[i + l];
            L[8][l] += m * p2[i + l];
            L[9][l] += a * p2[i + l];
          }
      for (; i < ni; i++)
        {
          float a = ta[i], m = tm[i], q = pm[i], g = pg[i];
          L[0][0] += m * q;
          L[1][0] += a * q;
          L[2][0] += ta2[i] * q;
          L[3][0] += m * g;
          L[4][0] += m * pg2[i];
          L[5][0] += a * g;
          L[6][0] += m * p1[i];
          L[7][0] += a * p1[i];
          L[8][0] += m * p2[i];
          L[9][0] += a * p2[i];
        }
      rn = (L[0][0] + L[0][1]) + (L[0][2] + L[0][3]);
      ra = (L[1][0] + L[1][1]) + (L[1][2] + L[1][3]);
      raa = (L[2][0] + L[2][1]) + (L[2][2] + L[2][3]);
      rg = (L[3][0] + L[3][1]) + (L[3][2] + L[3][3]);
      rgg = (L[4][0] + L[4][1]) + (L[4][2] + L[4][3]);
      rag = (L[5][0] + L[5][1]) + (L[5][2] + L[5][3]);
      r1 = (L[6][0] + L[6][1]) + (L[6][2] + L[6][3]);
      ra1 = (L[7][0] + L[7][1]) + (L[7][2] + L[7][3]);
      r2 = (L[8][0] + L[8][1]) + (L[8][2] + L[8][3]);
      ra2 = (L[9][0] + L[9][1]) + (L[9][2] + L[9][3]);
      n += rn;
      sa += ra;
      saa += raa;
      sg += rg;
      sgg += rgg;
      sag += rag;
      s1 += r1;
      sa1 += ra1;
      s2 += r2;
      sa2 += ra2;
    }
  *npts = n;
  if (n < 1 || n * EV_CSTEP * EV_CSTEP < EV_MIN_OVERLAP)
    return -1.0;
  va = saa - sa * sa / n;
  vg = sgg - sg * sg / n;
  if (va <= 1e-9 || vg <= 1e-9)
    return -1.0;
  c0 = sag - sa * sg / n;
  c1 = sa1 - sa * s1 / n;
  c2 = sa2 - sa * s2 / n;
  return sqrt (c0 * c0 + c1 * c1 + c2 * c2) / sqrt (va * vg);
}

/* full-resolution masked NCC of the ridge images, float, contiguous rows:
 * template (x,y) <-> rotated probe (x - dx, y - dy) */
static void
ev_full_template (const EvFrame *A, EvWork *W)
{
  for (int i = 0; i < EM_N; i++)
    {
      float m = A->f.mask[i] ? 1.0f : 0.0f;
      W->fa[i] = (float) A->f.img[i] * m;
      W->fm[i] = m;
      W->fa2[i] = W->fa[i] * W->fa[i];
    }
}

static void
ev_full_probe (const EvProbe *B, double th, EvWork *W)
{
  double cy = (EM_H - 1) / 2.0, cx = (EM_W - 1) / 2.0, ct = cos (th), st = sin (th);

  for (int y = 0; y < EM_H; y++)
    for (int x = 0; x < EM_W; x++)
      {
        int i = y * EM_W + x;
        double dx = x - cx, dy = y - cy;
        double sx = cx + dx * ct - dy * st, sy = cy + dx * st + dy * ct;
        W->qa[i] = W->qm[i] = W->qa2[i] = 0.0f;
        if (sx < 0 || sx > EM_W - 1 || sy < 0 || sy > EM_H - 1)
          continue;
        if (ev_bil_u8 (B->e.f.mask, sx, sy) > 0.6)
          {
            float v = (float) ev_bil_d (B->e.f.img, sx, sy);
            W->qa[i] = v;
            W->qm[i] = 1.0f;
            W->qa2[i] = v * v;
          }
      }
}

static double
ev_full_ncc (const EvWork *W, int dx, int dy)
{
  int ya = dy > 0 ? dy : 0, yb = EM_H + (dy < 0 ? dy : 0);
  int xa = dx > 0 ? dx : 0, xb = EM_W + (dx < 0 ? dx : 0);
  double n = 0, sa = 0, sb = 0, saa = 0, sbb = 0, sab = 0, cov, va, vb;

  for (int y = ya; y < yb; y++)
    {
      /* row pointers at the first pixel used (column xa), none before its
       * array's start */
      int t0 = y * EM_W + xa, o = (y - dy) * EM_W - dx + xa, nx = xb - xa;
      const float *ta = W->fa + t0, *tm = W->fm + t0, *ta2 = W->fa2 + t0;
      const float *pa = W->qa + o, *pm = W->qm + o, *pa2 = W->qa2 + o;
      float rn = 0, rsa = 0, rsb = 0, rsaa = 0, rsbb = 0, rsab = 0;
      for (int x = 0; x < nx; x++)
        {
          rn += tm[x] * pm[x];
          rsa += ta[x] * pm[x];
          rsb += tm[x] * pa[x];
          rsaa += ta2[x] * pm[x];
          rsbb += tm[x] * pa2[x];
          rsab += ta[x] * pa[x];
        }
      n += rn;
      sa += rsa;
      sb += rsb;
      saa += rsaa;
      sbb += rsbb;
      sab += rsab;
    }
  if (n < 1 || n < EV_MIN_OVERLAP)
    return -1.0;
  cov = sab - sa * sb / n;
  va = saa - sa * sa / n;
  vb = sbb - sb * sb / n;
  if (va <= 0 || vb <= 0)
    return -1.0;
  return cov / (sqrt (va * vb) + 1e-6);
}

/* Insert c into list[] (at most ncap, *n used), kept sorted descending by
 * the key selected: envelope (s) or significance (q). */
static void
ev_cand_insert (EvHyp *list, int *n, int ncap, EvHyp c, int by_q)
{
  double v = by_q ? c.q : c.s;
  int k;

  if (*n >= ncap && v <= (by_q ? list[*n - 1].q : list[*n - 1].s))
    return;
  k = *n < ncap ? (*n)++ : ncap - 1;
  while (k > 0 && (by_q ? list[k - 1].q : list[k - 1].s) < v)
    {
      list[k] = list[k - 1];
      k--;
    }
  list[k] = c;
}

/* The envelope landscape at one rotation: its local maxima (3x3 on the
 * coarse grid) above EV_HYP_MIN go into cand[], kept sorted by envelope,
 * descending, at most ncap, and into candq[] sorted by significance (below).
 * Returns 1 if any shift reached the overlap floor. */
static int
ev_land_peaks (const EvProbe *B, double rot, EvWork *W, EvHyp *cand, int *nc,
               EvHyp *candq, int *nq, int ncap)
{
  int any = 0;

  ev_grid_probe (B, rot * EV_PI / 180.0, W);
  for (int y = 0; y < EV_S2; y++)
    for (int x = 0; x < EV_S2; x++)
      {
        double npts = 0;
        double v = ev_grid_env (W, (x - EV_S2 / 2) * EV_CSTEP, (y - EV_S2 / 2) * EV_CSTEP, &npts);
        W->land[y * EV_S2 + x] = (float) v;
        /* significance of the envelope: E^2 is about chi^2 with 3 degrees
         * of freedom over npts under no relation, so E sqrt(npts) ranks
         * peaks by how unlikely they are, not by how high a small overlap
         * can push a correlation */
        W->landq[y * EV_S2 + x] = (float) (v > 0 ? v * sqrt (npts) : 0.0);
        if (v > -1.0)
          any = 1;
      }
  for (int y = 0; y < EV_S2; y++)
    for (int x = 0; x < EV_S2; x++)
      {
        float v = W->land[y * EV_S2 + x];
        int lm = v > EV_HYP_MIN;
        for (int j = -1; j <= 1 && lm; j++)
          for (int i = -1; i <= 1 && lm; i++)
            {
              int yy = y + j, xx = x + i;
              if ((i || j) && yy >= 0 && yy < EV_S2 && xx >= 0 && xx < EV_S2 &&
                  W->land[yy * EV_S2 + xx] > v)
                lm = 0;
            }
        if (!lm)
          continue;
        {
          EvHyp c = { v, rot, (x - EV_S2 / 2) * EV_CSTEP, (y - EV_S2 / 2) * EV_CSTEP,
                      W->landq[y * EV_S2 + x] };
          ev_cand_insert (cand, nc, ncap, c, 0);
          ev_cand_insert (candq, nq, ncap, c, 1);
        }
      }
  return any;
}

static int
ev_distinct (const EvHyp *c, const EvHyp *H, int nh)
{
  for (int k = 0; k < nh; k++)
    if (fabs (c->rot - H[k].rot) <= EV_ROT_COARSE / 2.0 &&
        hypot (c->dx - H[k].dx, c->dy - H[k].dy) <= EV_CSTEP + 0.5)
      return 0;
  return 1;
}

/* The best distinct hypotheses -- the EV_NHYP highest envelope peaks over the
 * coarse rotation grid, plus the EV_NSIG most significant ones not yet taken
 * -- refined at full resolution and sorted by refined NCC, descending. H must
 * hold EV_NH. Returns how many, or -1 if no alignment reaches the overlap
 * floor at all. */
static int
ev_hyps (const EvFrame *A, const EvProbe *B, EvHyp *H, EvWork *W)
{
  enum { NCAND = 64 };
  EvHyp cand[NCAND], candq[NCAND];
  int nc = 0, nq = 0, any = 0, nr = EV_ROT_MAX / EV_ROT_COARSE, nh = 0;

  ev_grid_template (A, W);
  for (int r = -nr; r <= nr; r++)
    any |= ev_land_peaks (B, r * EV_ROT_COARSE, W, cand, &nc, candq, &nq, NCAND);
  if (!any)
    return -1;
  /* distinct: another candidate within half a coarse rotation step AND one
   * grid step is the same peak */
  for (int c = 0; c < nc && nh < EV_NHYP; c++)
    if (ev_distinct (&cand[c], H, nh))
      H[nh++] = cand[c];
#if EV_NSIG > 0
  /* the most significant envelope peaks not yet taken, whatever their
   * envelope rank */
  {
    int added = 0;
    for (int c = 0; c < nq && added < EV_NSIG; c++)
      if (ev_distinct (&candq[c], H, nh))
        {
          H[nh++] = candq[c];
          added++;
        }
  }
#endif
  /* refine at full resolution: +-EV_REF px at the coarse rotation, then the
   * rotations EV_REF_ROT deg apart within half a coarse step either side, at
   * +-1 px around that shift. The probe is resampled once per distinct
   * rotation for every hypothesis that needs it -- hypotheses share coarse
   * rotations, and neighbouring ones share fine rotations: on impostor pairs
   * about 15 of the 40 resamplings per pair were repeats, a fifth of the
   * pair's time -- and each hypothesis takes its candidates in the order it
   * always did (strict '>', the coarse rotation's shifts, then the fine
   * rotations -1, +1, -2, +2 steps), so the result is the same to the bit. */
  ev_full_template (A, W);
  if (nh > 0)
    {
      int nfs = 0, nsl;
      EvHyp *fine;                               /* per hypothesis and fine rotation: its best */
      int *cx0 = malloc ((size_t) nh * sizeof *cx0), *cy0 = malloc ((size_t) nh * sizeof *cy0);
      while ((nfs + 1) * EV_REF_ROT <= EV_ROT_COARSE / 2.0 + 1e-6)
        nfs++;
      nsl = 2 * nfs;
      fine = malloc ((size_t) (nh * (nsl > 0 ? nsl : 1)) * sizeof *fine);
      if (!cx0 || !cy0 || !fine)
        {
          free (cx0);
          free (cy0);
          free (fine);
          return -1;
        }
      /* the coarse rotations, each resampled once */
      for (int k = 0; k < nh; k++)
        {
          int seen = 0;
          for (int j = 0; j < k && !seen; j++)
            seen = H[j].rot == H[k].rot;
          if (seen)
            continue;
          ev_full_probe (B, H[k].rot * EV_PI / 180.0, W);
          for (int kk = k; kk < nh; kk++)
            {
              EvHyp b = H[kk];
              if (H[kk].rot != H[k].rot)
                continue;
              b.s = -2;
              for (int dy = H[kk].dy - EV_REF; dy <= H[kk].dy + EV_REF; dy++)
                for (int dx = H[kk].dx - EV_REF; dx <= H[kk].dx + EV_REF; dx++)
                  {
                    double v = ev_full_ncc (W, dx, dy);
                    if (v > b.s)
                      {
                        b.s = v;
                        b.dx = dx;
                        b.dy = dy;
                      }
                  }
              cx0[kk] = b.dx;
              cy0[kk] = b.dy;
              H[kk].s = b.s;
              H[kk].dx = b.dx;
              H[kk].dy = b.dy;
            }
        }
      /* the fine rotations, each resampled once; slot j = 2 (st - 1) + (sg > 0)
       * holds hypothesis k's best over its 3 x 3 shifts at that rotation */
      for (int k = 0; k < nh; k++)
        for (int j = 0; j < nsl; j++)
          {
            double rot = (H[k].rot + (j & 1 ? 1 : -1) * (j / 2 + 1) * EV_REF_ROT);
            int seen = 0;
            for (int k2 = 0; k2 <= k && !seen; k2++)
              for (int j2 = 0; j2 < (k2 < k ? nsl : j) && !seen; j2++)
                seen = (H[k2].rot + (j2 & 1 ? 1 : -1) * (j2 / 2 + 1) * EV_REF_ROT) == rot;
            if (seen)
              continue;
            ev_full_probe (B, rot * EV_PI / 180.0, W);
            for (int kk = k; kk < nh; kk++)
              for (int jj = (kk == k ? j : 0); jj < nsl; jj++)
                {
                  EvHyp c;
                  if ((H[kk].rot + (jj & 1 ? 1 : -1) * (jj / 2 + 1) * EV_REF_ROT) != rot)
                    continue;
                  c = H[kk];
                  c.s = -HUGE_VAL;
                  c.rot = rot;
                  for (int dy = cy0[kk] - 1; dy <= cy0[kk] + 1; dy++)
                    for (int dx = cx0[kk] - 1; dx <= cx0[kk] + 1; dx++)
                      {
                        double v = ev_full_ncc (W, dx, dy);
                        if (v > c.s)
                          {
                            c.s = v;
                            c.dx = dx;
                            c.dy = dy;
                          }
                      }
                  fine[kk * nsl + jj] = c;
                }
          }
      /* each hypothesis: its coarse-rotation best, then the fine rotations
       * in the original order */
      for (int k = 0; k < nh; k++)
        {
          EvHyp b = H[k];
          for (int j = 0; j < nsl; j++)
            if (fine[k * nsl + j].s > b.s)
              b = fine[k * nsl + j];
          H[k] = b;
        }
      free (cx0);
      free (cy0);
      free (fine);
    }
  /* sort by refined NCC, drop exact duplicates */
  for (int i = 1; i < nh; i++)
    {
      EvHyp x = H[i];
      int j = i;
      while (j > 0 && H[j - 1].s < x.s)
        {
          H[j] = H[j - 1];
          j--;
        }
      H[j] = x;
    }
  {
    int m = 0;
    for (int i = 0; i < nh; i++)
      {
        int dup = 0;
        for (int k = 0; k < m && !dup; k++)
          if (H[k].rot == H[i].rot && H[k].dx == H[i].dx && H[k].dy == H[i].dy)
            dup = 1;
        if (!dup)
          H[m++] = H[i];
      }
    nh = m;
  }
  return nh;
}

/* ---- block residual evidence ------------------------------------------------ */
typedef struct
{
  float ta[EM_N], tm[EM_N], ta2[EM_N];          /* template residual          */
  float pa[EM_N], pm[EM_N], pa2[EM_N];          /* probe residual, rotated    */
} EvEvWork;

static void
ev_template_res (const EvFrame *A, EvEvWork *E)
{
  for (int i = 0; i < EM_N; i++)
    {
      E->ta[i] = A->res[i];
      E->tm[i] = A->f.mask[i] ? 1.0f : 0.0f;
      E->ta2[i] = A->res[i] * A->res[i];
    }
}

static void
ev_rotate_res (const EvProbe *B, double th, EvEvWork *E)
{
  double cy = (EM_H - 1) / 2.0, cx = (EM_W - 1) / 2.0, ct = cos (th), st = sin (th);

  for (int y = 0; y < EM_H; y++)
    for (int x = 0; x < EM_W; x++)
      {
        int i = y * EM_W + x;
        double dx = x - cx, dy = y - cy;
        double sx = cx + dx * ct - dy * st, sy = cy + dx * st + dy * ct;
        E->pa[i] = E->pm[i] = E->pa2[i] = 0.0f;
        if (sx < 0 || sx > EM_W - 1 || sy < 0 || sy > EM_H - 1)
          continue;
        if (ev_bil_u8 (B->e.f.mask, sx, sy) > 0.6)
          {
            double v = ev_bil_f (B->e.res, sx, sy);
            E->pa[i] = (float) v;
            E->pm[i] = 1.0f;
            E->pa2[i] = (float) (v * v);
          }
      }
}

/* The evidence W at one pose (the probe residual already rotated into E):
 * every block that has at least half its pixels in the overlap searches its
 * own shift; 8-connected sets of blocks at or above EV_TAU whose shifts agree
 * with a neighbour's within EV_EPS px are formed, and a set scores the sum of
 * atanh(best) - atanh(EV_TAU) over its blocks, each block weighted by the
 * area it adds to the union of the set's blocks (INDEPENDENT AREA,
 * ev_block_weight). What the set scores counts only over EV_MIN_SCORED px of
 * area actually scored and under the area rule (ev_area_rule). */
typedef struct
{
  double mx, my;     /* mean block shift of the set, px */
  int    n;          /* its blocks */
  double cx, cy;     /* centroid of its blocks' centres, template px */
  double rot;        /* the rotation, deg, that best explains its blocks'
                      * shifts (least squares, ev_hyp_evidence); 0 when the
                      * blocks do not span an area */
  double ws;         /* its weighted Fisher-z excess, before the area rule */
} EvSet;

/* Top-left corner of block column i (row i) along a side of the given
 * length: every EV_STEP px, the last one anchored at the border. */
static int
ev_block_origin (int i, int extent)
{
  return i * EV_STEP < extent - EV_BS ? i * EV_STEP : extent - EV_BS;
}

/* The share of the tiling block column i (row i) owns: the columns (rows) it
 * adds beyond its predecessor, in grid steps -- 1 for every block of the
 * regular grid, (70 - 16 - 48) / 8 = 0.75 for the column anchored at the
 * right border and (57 - 16 - 40) / 8 = 0.125 for the row anchored at the
 * bottom one, whose pixels the regular neighbour already covers but for 6
 * columns and 1 row. v4's fixed weights; since v4.1 what a border block
 * counts depends on its set (ev_block_weight), and this is its share where
 * the set holds its predecessor. */
static double
ev_grid_weight (int i, int extent)
{
  if (i == 0)
    return 1.0;
  return (double) (ev_block_origin (i, extent) - ev_block_origin (i - 1, extent)) / EV_STEP;
}

/* v4.1: the share block (i, j) adds to the UNION of the blocks marked
 * in[] == v (its set), along one axis, in grid steps. Along x (vert 0), row
 * by row: the columns it adds beyond the nearest marked block to its left
 * that covers that row, over EV_STEP, at most 1; only a predecessor whose
 * origin lies less than a step before its own shares more than the regular
 * grid's half (ev_grid_weight < 1), so a row no such marked block covers
 * keeps its full share. Along y (vert 1) the same, column by column, with
 * the marked blocks above it. The mean over its rows (columns). */
static double
ev_axis_share (const int *in, int v, int i, int j, int vert)
{
  int c = vert ? j : i, ext = vert ? EM_H : EM_W;
  int nq = vert ? EV_NBX : EV_NBY, oext = vert ? EM_W : EM_H;
  int o0 = ev_block_origin (vert ? i : j, oext);
  double s = 0;

  if (ev_grid_weight (c, ext) >= 1.0)
    return 1.0;                                    /* the regular grid */
  for (int t = o0; t < o0 + EV_BS; t++)
    {
      double f = 1.0;
      for (int m = 1; m <= c; m++)
        {
          int d = ev_block_origin (c, ext) - ev_block_origin (c - m, ext), hit = 0;
          if (d >= EV_STEP)
            break;
          for (int q = 0; q < nq && !hit; q++)
            {
              int q0 = ev_block_origin (q, oext);
              hit = t >= q0 && t < q0 + EV_BS &&
                    in[vert ? (c - m) * EV_NBX + q : q * EV_NBX + c - m] == v;
            }
          if (hit)
            {
              f = (double) d / EV_STEP;
              break;
            }
        }
      s += f;
    }
  return s / EV_BS;
}

/* The INDEPENDENT AREA block k adds to its set (the blocks marked in[] == v):
 * the product of its two axis shares, i.e. the mean over its pixels of what
 * each adds along x and along y beyond the set's blocks before it. Every
 * block of the regular grid 1; the column anchored at the right border 0.75
 * where the set holds its left neighbour and 1 where the set holds nothing
 * to its left, the row anchored at the bottom border 0.125 and 1 likewise,
 * and in between by the share of its rows (columns) such a block covers.
 * With every block marked it is ev_grid_weight's, v4's fixed weight. */
static double
ev_block_weight (const int *in, int v, int k)
{
  int i = k % EV_NBX, j = k / EV_NBX;

  return ev_axis_share (in, v, i, j, 0) * ev_axis_share (in, v, i, j, 1);
}

/* What a set's evidence counts for, given its weighted Fisher-z excess w
 * and its independent area a (sum of its blocks' weights): the accept
 * point, W 2, holds from EV_AREA independent blocks on and rises as the area
 * shrinks below that (w a / EV_AREA); with EV_AREA_HARD a smaller set counts
 * nothing. */
static double
ev_area_rule (double w, double a)
{
  if (a >= EV_AREA)
    return w;
#if EV_AREA_HARD
  return 0.0;
#else
  return w * a / EV_AREA;
#endif
}

/* Template pixels the set labelled k0 actually scored: the union over its
 * blocks of the pixels where the template's mask and the probe's, at the
 * block's own shift, overlap. */
static int
ev_set_pixels (const EvEvWork *E, const int *lab, int k0, const int *ax, const int *ay, int dx, int dy)
{
  uint8_t cov[EM_N];
  int n = 0;

  memset (cov, 0, sizeof cov);
  for (int k = 0; k < EV_NB; k++)
    {
      int x0, y0, sx, sy;
      if (lab[k] != k0)
        continue;
      x0 = ev_block_origin (k % EV_NBX, EM_W);
      y0 = ev_block_origin (k / EV_NBX, EM_H);
      sx = dx + ax[k];
      sy = dy + ay[k];
      for (int y = y0; y < y0 + EV_BS; y++)
        for (int x = x0; x < x0 + EV_BS; x++)
          {
            int px = x - sx, py = y - sy, i = y * EM_W + x;
            if (px < 0 || px >= EM_W || py < 0 || py >= EM_H || cov[i])
              continue;
            if (E->tm[i] > 0.5f && E->pm[py * EM_W + px] > 0.5f)
              {
                cov[i] = 1;
                n++;
              }
          }
    }
  return n;
}

/* The block search by CELLS: the frame cut at every block edge, so that
 * every block is a union of whole cells (a regular block four of them).
 * All blocks search the same shifts around the pose, so a cell's sums at a
 * shift serve every block that covers it; the search costs about a third of
 * summing every block on its own (as up to v3), with the same result: over
 * the 2,817 frames of three development sets (S2, S4 -> S2, S3 -> S2, genuine
 * and the impostor tail) every frame score and raw verdict is identical. */
#define EV_MAXCUT (2 * (EV_NBX > EV_NBY ? EV_NBX : EV_NBY))
#define EV_MAXCELL ((2 * EV_NBX - 1) * (2 * EV_NBY - 1))
typedef struct
{
  int nx, ny;                                    /* cells per axis */
  int cx[EV_MAXCUT], cy[EV_MAXCUT];              /* cuts, 0 .. EM_W / EM_H */
  int bx0[EV_NBX], bx1[EV_NBX];                  /* block column i: cells bx0..bx1-1 */
  int by0[EV_NBY], by1[EV_NBY];
} EvCells;

static int
ev_cuts (int nb, int extent, int *cut, int *b0, int *b1)
{
  int n = 0;

  for (int i = 0; i < nb; i++)
    for (int e = 0; e < 2; e++)
      {
        int v = ev_block_origin (i, extent) + (e ? EV_BS : 0), k = n;
        int dup = 0;
        for (int q = 0; q < n && !dup; q++)
          dup = cut[q] == v;
        if (dup)
          continue;
        while (k > 0 && cut[k - 1] > v)
          {
            cut[k] = cut[k - 1];
            k--;
          }
        cut[k] = v;
        n++;
      }
  for (int i = 0; i < nb; i++)
    for (int q = 0; q < n; q++)
      {
        if (cut[q] == ev_block_origin (i, extent))
          b0[i] = q;
        if (cut[q] == ev_block_origin (i, extent) + EV_BS)
          b1[i] = q;
      }
  return n - 1;
}

static void
ev_cells (EvCells *C)
{
  C->nx = ev_cuts (EV_NBX, EM_W, C->cx, C->bx0, C->bx1);
  C->ny = ev_cuts (EV_NBY, EM_H, C->cy, C->by0, C->by1);
}

/* The six masked sums of cell [x0,x1) x [y0,y1) against the probe shifted
 * by (sx, sy) -- template (x,y) <-> probe (x - sx, y - sy), pixels whose
 * probe point falls outside the frame left out: the overlap count, the two
 * masked sums, the two masked sums of squares and the cross sum; row by row
 * in float, the rows added in double. */
static void
ev_cell_sums (const EvEvWork *E, int x0, int x1, int y0, int y1, int sx, int sy, double *o)
{
  int xa = x0 > sx ? x0 : sx, xb = x1 < EM_W + sx ? x1 : EM_W + sx;
  int ya = y0 > sy ? y0 : sy, yb = y1 < EM_H + sy ? y1 : EM_H + sy;

  for (int m = 0; m < 6; m++)
    o[m] = 0;
  /* an empty overlap forms no row pointer: xa may lie past the row's end,
   * and a pointer beyond its array's end is undefined (C99 6.5.6) */
  if (xa >= xb || ya >= yb)
    return;
  for (int y = ya; y < yb; y++)
    {
      /* row pointers at the first pixel used (column xa), none before its
       * array's start */
      int t0 = y * EM_W + xa, off = (y - sy) * EM_W - sx + xa, nx = xb - xa;
      const float *ta = E->ta + t0, *tm = E->tm + t0, *ta2 = E->ta2 + t0;
      const float *pa = E->pa + off, *pm = E->pm + off, *pa2 = E->pa2 + off;
      float rn = 0, rsa = 0, rsb = 0, rsaa = 0, rsbb = 0, rsab = 0;
      for (int x = 0; x < nx; x++)
        {
          rn += tm[x] * pm[x];
          rsa += ta[x] * pm[x];
          rsb += tm[x] * pa[x];
          rsaa += ta2[x] * pm[x];
          rsbb += tm[x] * pa2[x];
          rsab += ta[x] * pa[x];
        }
      o[0] += rn;
      o[1] += rsa;
      o[2] += rsb;
      o[3] += rsaa;
      o[4] += rsbb;
      o[5] += rsab;
    }
}

static double
ev_cell_count (const EvEvWork *E, int x0, int x1, int y0, int y1, int sx, int sy)
{
  double o[6];

  ev_cell_sums (E, x0, x1, y0, y1, sx, sy, o);
  return o[0];
}

/* The independent area of the blocks marked in[] == v taken together as one
 * set (ev_block_weight). */
static double
ev_marked_area (const int *in, int v)
{
  double a = 0;

  for (int k = 0; k < EV_NB; k++)
    if (in[k] == v)
      a += ev_block_weight (in, v, k);
  return a;
}

/* Every block with at least half its pixels in the overlap at the pose
 * searches its own shift (+-EV_RS px) for its best residual NCC: best[],
 * ax[], ay[], ok[] per block. Returns how many blocks searched. With amin >
 * 0: when the eligible blocks' independent area, taken together, is under
 * amin, nothing is searched (0 is returned), and *a_hit receives that of the
 * ones at or above EV_TAU (0 otherwise). v4's gate with v4.1's accounting.
 * Taken together, a border block counts less where its regular neighbour is
 * among them, so unlike v4's this is not an upper bound for every part of
 * them: a set that holds border blocks but not their regular neighbour
 * (which the other blocks then hold as another set) can count a little
 * more than all of them together. Measured on the 16 development sets
 * against a gate that counts every block 1 (a strict bound, and more
 * along-ridge poses searched): the same decision on every press and the
 * same score on every impostor frame; 24 of the 11,428 genuine frame
 * scores differ (where the extra poses let an earlier template frame clear
 * the accept point first), one of them under it either way. */
static int
ev_block_search (const EvEvWork *E, int dx, int dy, double amin, double *best,
                 int *ax, int *ay, int *ok, double *a_hit)
{
  EvCells C;
  int need = (EV_BS * EV_BS) / 2, nel = 0, nnc = 0, ncell;
  int elig[EV_NB], need_cell[EV_MAXCELL], cell_list[EV_MAXCELL], mark[EV_NB];
  double cn[EV_MAXCELL];

  *a_hit = 0;
  ev_cells (&C);
  ncell = C.nx * C.ny;
  /* eligibility at the pose itself, from every cell's overlap count */
  for (int c = 0; c < ncell; c++)
    {
      cn[c] = ev_cell_count (E, C.cx[c % C.nx], C.cx[c % C.nx + 1], C.cy[c / C.nx], C.cy[c / C.nx + 1], dx, dy);
      need_cell[c] = 0;
    }
  for (int j = 0; j < EV_NBY; j++)
    for (int i = 0; i < EV_NBX; i++)
      {
        int k = j * EV_NBX + i;
        double on = 0;
        ok[k] = 0;
        best[k] = -2;
        ax[k] = ay[k] = 0;
        mark[k] = 0;
        for (int cy = C.by0[j]; cy < C.by1[j]; cy++)
          for (int cx = C.bx0[i]; cx < C.bx1[i]; cx++)
            on += cn[cy * C.nx + cx];
        if ((int) (on + 0.5) < need)
          continue;
        elig[nel++] = k;
        mark[k] = 1;
        for (int cy = C.by0[j]; cy < C.by1[j]; cy++)
          for (int cx = C.bx0[i]; cx < C.bx1[i]; cx++)
            need_cell[cy * C.nx + cx] = 1;
      }
  if (amin > 0 && ev_marked_area (mark, 1) < amin)
    return 0;
  for (int c = 0; c < ncell; c++)
    if (need_cell[c])
      cell_list[nnc++] = c;
  /* every shift of the window once for every cell the eligible blocks
   * cover; a block's sums are its cells' */
  for (int yy = -EV_RS; yy <= EV_RS; yy++)
    for (int xx = -EV_RS; xx <= EV_RS; xx++)
      {
        double cs[EV_MAXCELL][6];
        for (int q = 0; q < nnc; q++)
          {
            int c = cell_list[q];
            ev_cell_sums (E, C.cx[c % C.nx], C.cx[c % C.nx + 1], C.cy[c / C.nx], C.cy[c / C.nx + 1],
                          dx + xx, dy + yy, cs[c]);
          }
        for (int e = 0; e < nel; e++)
          {
            int k = elig[e], i = k % EV_NBX, j = k / EV_NBX;
            double t[6] = { 0, 0, 0, 0, 0, 0 }, cov, va, vb, v;
            for (int cy = C.by0[j]; cy < C.by1[j]; cy++)
              for (int cx = C.bx0[i]; cx < C.bx1[i]; cx++)
                for (int m = 0; m < 6; m++)
                  t[m] += cs[cy * C.nx + cx][m];
            /* the shifted block must still have half its pixels in the
             * overlap; under 20 px or flat it has no NCC */
            if ((int) (t[0] + 0.5) < need || t[0] < 20)
              continue;
            cov = t[5] - t[1] * t[2] / t[0];
            va = t[3] - t[1] * t[1] / t[0];
            vb = t[4] - t[2] * t[2] / t[0];
            if (va <= 1e-12 || vb <= 1e-12)
              continue;
            v = cov / sqrt (va * vb);
            if (v > best[k])
              {
                best[k] = v;
                ax[k] = xx;
                ay[k] = yy;
              }
          }
      }
  for (int e = 0; e < nel; e++)
    {
      ok[elig[e]] = 1;
      mark[elig[e]] = best[elig[e]] >= EV_TAU ? 2 : 1;
    }
  if (amin > 0)
    *a_hit = ev_marked_area (mark, 2);
  return nel;
}

/* Returns the best counted evidence over the sets (ev_area_rule; a set of
 * less than amin independent blocks, or under EV_MIN_SCORED px of scored
 * area, counts nothing). *set (may be NULL) receives the set that steers the
 * search -- the one with the highest weighted excess, whatever the area
 * rule says of it; empty when amin > 0 and no set can reach amin. */
static double
ev_evidence (const EvEvWork *E, int dx, int dy, double amin, EvSet *set)
{
  double best[EV_NB];
  int ax[EV_NB], ay[EV_NB], ok[EV_NB], lab[EV_NB], stack[EV_NB], mem[EV_NB];
  double wbest = 0, sbest = 0, a_hit = 0;
  EvSet bs = { 0.0, 0.0, 0, 0.0, 0.0, 0.0, 0.0 };

  /* nothing searched, or (amin > 0) the blocks at or above EV_TAU, taken
   * together, hold less than amin independent blocks: nothing counts,
   * nothing steers */
  if (ev_block_search (E, dx, dy, amin, best, ax, ay, ok, &a_hit) == 0 || a_hit < amin)
    {
      if (set)
        *set = bs;
      return 0.0;
    }
  for (int k = 0; k < EV_NB; k++)
    lab[k] = -1;
  for (int k0 = 0; k0 < EV_NB; k0++)
    {
      int sp = 0, cnt = 0, sx = 0, sy = 0;
      double w = 0, a = 0, su = 0, sv = 0, suu = 0, sxv = 0, syu = 0, wc;
      if (!ok[k0] || best[k0] < EV_TAU || lab[k0] >= 0)
        continue;
      lab[k0] = k0;
      stack[sp++] = k0;
      while (sp)
        {
          int k = stack[--sp], y = k / EV_NBX, x = k % EV_NBX;
          /* the block's centre, template px */
          double u = ev_block_origin (x, EM_W) + (EV_BS - 1) / 2.0;
          double v = ev_block_origin (y, EM_H) + (EV_BS - 1) / 2.0;
          mem[cnt++] = k;
          sx += ax[k];
          sy += ay[k];
          su += u;
          sv += v;
          suu += u * u + v * v;
          sxv += ax[k] * v;
          syu += ay[k] * u;
          for (int dj = -1; dj <= 1; dj++)
            for (int di = -1; di <= 1; di++)
              {
                int yy = y + dj, xx = x + di, kk;
                if ((!di && !dj) || yy < 0 || yy >= EV_NBY || xx < 0 || xx >= EV_NBX)
                  continue;
                kk = yy * EV_NBX + xx;
                if (!ok[kk] || best[kk] < EV_TAU || lab[kk] >= 0)
                  continue;
                if (hypot (ax[kk] - ax[k], ay[kk] - ay[k]) <= EV_EPS)
                  {
                    lab[kk] = k0;
                    stack[sp++] = kk;
                  }
              }
        }
      /* the set is complete: each block counts with the area it adds to
       * the union of the set's blocks (in the order they were reached) */
      for (int q = 0; q < cnt; q++)
        {
          int k = mem[q];
          double wk = ev_block_weight (lab, k0, k);
          w += wk * (atanh (best[k] < 0.999 ? best[k] : 0.999) - atanh (EV_TAU));
          a += wk;
        }
      if (w > sbest)
        {
          /* a small rotation r of the probe about the pose moves the block
           * at (u, v) by about r (v, -u) on top of a common shift; its least
           * squares estimate from the set's shifts */
          double spread = suu - (su * su + sv * sv) / cnt;
          sbest = w;
          bs.ws = w;
          bs.n = cnt;
          bs.mx = (double) sx / cnt;
          bs.my = (double) sy / cnt;
          bs.cx = su / cnt;
          bs.cy = sv / cnt;
          bs.rot = spread > 1e-9 ? ((sxv - sx * sv / cnt) - (syu - sy * su / cnt)) / spread * 180.0 / EV_PI : 0.0;
        }
      wc = a >= amin ? ev_area_rule (w, a) : 0.0;
      if (wc > wbest && ev_set_pixels (E, lab, k0, ax, ay, dx, dy) >= EV_MIN_SCORED)
        wbest = wc;
    }
  if (set)
    *set = bs;
  return wbest;
}

/* The evidence at a hypothesis pose, re-centred once in translation and
 * once in rotation (the probe residual is already rotated by rot into E).
 * Translation: when the best set's blocks agree on a mean shift of
 * EV_RC_MIN px or more (and it has at least EV_RC_N blocks), the evidence is
 * looked for again with the pose moved by that shift. Rotation: when the
 * best set of the pose kept so far has at least EV_RR_N blocks and its
 * shifts say the probe is turned by EV_RR_MIN..EV_RR_MAX deg against it,
 * the probe residual is rotated by that much more, the pose moved so that
 * the set's centroid keeps its place in the probe, and the evidence looked
 * for again. Each time the higher W is kept. *orot, *odx, *ody receive the
 * pose it was found at. The steps follow the set that steers the search
 * (ev_evidence), not only one that counts. Evidence counts only from amin
 * independent blocks on (0: the area rule alone). *turned is set when E was
 * left rotated by more than rot. */
static double
ev_hyp_evidence (EvEvWork *E, const EvProbe *p, double rot, int dx, int dy, double amin,
                 double *orot, int *odx, int *ody, int *turned)
{
  EvSet st;
  double w = ev_evidence (E, dx, dy, amin, &st);
  int sdx = dx, sdy = dy;                        /* the pose the search is at */

  *orot = rot;
  *odx = dx;
  *ody = dy;
  *turned = 0;
#if EV_RC_N > 0
  if (st.n >= EV_RC_N && hypot (st.mx, st.my) >= EV_RC_MIN)
    {
      int rx = dx + (int) lround (st.mx), ry = dy + (int) lround (st.my);
      EvSet s2;
      double w2 = ev_evidence (E, rx, ry, amin, &s2);
      if (w2 > w)
        {
          w = w2;
          *odx = rx;
          *ody = ry;
        }
      if (s2.ws > st.ws)
        {
          st = s2;
          sdx = rx;
          sdy = ry;
        }
    }
#endif
#if EV_RR_N > 0
  if (st.n >= EV_RR_N && fabs (st.rot) >= EV_RR_MIN && fabs (st.rot) <= EV_RR_MAX)
    {
      /* template point q (centred) <-> rotated probe q - d - a; with the
       * probe turned by r more, the pose d' that keeps the centroid's probe
       * point is d' = q - R(-r) (q - d - a) */
      double r = st.rot * EV_PI / 180.0, cr = cos (r), sr = sin (r);
      double qx = st.cx - (EM_W - 1) / 2.0, qy = st.cy - (EM_H - 1) / 2.0;
      double px = qx - sdx - st.mx, py = qy - sdy - st.my;
      int nx = (int) lround (qx - (cr * px + sr * py));
      int ny = (int) lround (qy - (-sr * px + cr * py));
      double w3;
      ev_rotate_res (p, (rot + st.rot) * EV_PI / 180.0, E);
      *turned = 1;
      w3 = ev_evidence (E, nx, ny, amin, NULL);
      if (w3 > w)
        {
          w = w3;
          *orot = rot + st.rot;
          *odx = nx;
          *ody = ny;
        }
    }
#endif
  return w;
}

#if EV_AR_N > 0
/* The ridge direction over the overlap at a pose (the probe residual's mask
 * already rotated into E): the structure tensor of the template's ridge image
 * over the template pixels the pose overlaps; the ridges run across its
 * dominant gradient. (*ux, *uy) is that direction, a unit vector in template
 * px; 0 when the overlap holds no gradient. */
static int
ev_ridge_dir (const EvFrame *t, const EvEvWork *E, int dx, int dy, double *ux, double *uy)
{
  const double *g = t->f.img;
  double gxx = 0, gyy = 0, gxy = 0, th;

  for (int y = 1; y < EM_H - 1; y++)
    for (int x = 1; x < EM_W - 1; x++)
      {
        int i = y * EM_W + x, px = x - dx, py = y - dy;
        double gx, gy;
        if (px < 0 || px >= EM_W || py < 0 || py >= EM_H)
          continue;
        if (!t->f.mask[i] || E->pm[py * EM_W + px] < 0.5f)
          continue;
        gx = (g[i + 1] - g[i - 1]) * 0.5;
        gy = (g[i + EM_W] - g[i - EM_W]) * 0.5;
        gxx += gx * gx;
        gyy += gy * gy;
        gxy += gx * gy;
      }
  if (gxx + gyy <= 1e-12)
    return 0;
  th = 0.5 * atan2 (2.0 * gxy, gxx - gyy);      /* the dominant gradient */
  *ux = -sin (th);
  *uy = cos (th);
  return 1;
}
#endif

/* Keep the running maximum of a frame pair's evidence and where it was found
 * (the first pose at which the rounded maximum was reached). */
static void
ev_note (double w, double orot, int odx, int ody, double *wmax, int *best, EvPose *bp)
{
  int ev;

  if (w > *wmax)
    *wmax = w;
  ev = (int) lround (*wmax * 1000.0);
  if (ev > *best || !bp->valid)
    {
      if (ev > *best)
        *best = ev;
      bp->rot_deg = orot;
      bp->dx = odx;
      bp->dy = ody;
      bp->valid = 1;
    }
}

/* ---- public ------------------------------------------------------------------ */
int
ev_match (const EvFrame *t, const EvProbe *p, int stop_at, EvPose *pose)
{
  EvWork *W = malloc (sizeof *W);
  EvEvWork *E = malloc (sizeof *E);
  EvHyp H[EV_NH];
  EvPose bp = { 0.0, 0, 0, 0 };
  int nh, best = -1;
  double wmax = 0;

  if (!W || !E)
    {
      free (W);
      free (E);
      if (pose)
        *pose = bp;
      return -1;
    }
  nh = ev_hyps (t, p, H, W);
  if (nh >= 0)
    {
      int nsc = 0;                                 /* hypotheses scored */
#if EV_AR_N <= 0
      (void) nsc;
#endif
      best = 0;
      ev_template_res (t, E);
      for (int k = 0; k < nh; k++)
        {
          int odx, ody, turned;
          double orot, w;
          if (H[k].s < EV_GFLOOR)
            break;                               /* sorted: the rest are lower */
          nsc++;
          ev_rotate_res (p, H[k].rot * EV_PI / 180.0, E);
          w = ev_hyp_evidence (E, p, H[k].rot, H[k].dx, H[k].dy, 0.0, &orot, &odx, &ody, &turned);
          ev_note (w, orot, odx, ody, &wmax, &best, &bp);
          if (stop_at > 0 && best >= stop_at)
            break;
        }
#if EV_AR_N > 0
      /* ALONG-RIDGE hypotheses, once every hypothesis has been scored and none
       * cleared stop_at: the EV_AR_H best ones, moved by EV_AR_STEP px either
       * way along the ridges of their overlap, where the ridge image cannot
       * tell the pose; what they find counts only from EV_AR_N independent
       * blocks on */
      for (int k = 0; k < nsc && k < EV_AR_H && !(stop_at > 0 && best >= stop_at); k++)
        {
          double ux, uy;
          ev_rotate_res (p, H[k].rot * EV_PI / 180.0, E);
          if (!ev_ridge_dir (t, E, H[k].dx, H[k].dy, &ux, &uy))
            continue;
          for (int j = 0; j < 2 && !(stop_at > 0 && best >= stop_at); j++)
            {
              double d = j ? -EV_AR_STEP : EV_AR_STEP;
              int odx, ody, turned;
              double orot, w;
              w = ev_hyp_evidence (E, p, H[k].rot, H[k].dx + (int) lround (d * ux),
                                   H[k].dy + (int) lround (d * uy), EV_AR_N, &orot, &odx, &ody, &turned);
              ev_note (w, orot, odx, ody, &wmax, &best, &bp);
              if (turned)
                ev_rotate_res (p, H[k].rot * EV_PI / 180.0, E);
            }
        }
#endif
    }
  free (W);
  free (E);
  if (pose)
    *pose = bp;
  return best;
}

int
ev_evidence_at (const EvFrame *t, const EvProbe *p, const EvPose *pose)
{
  EvEvWork *E;
  double w;

  if (!pose || !pose->valid)
    return -1;
  E = malloc (sizeof *E);
  if (!E)
    return -1;
  ev_template_res (t, E);
  ev_rotate_res (p, pose->rot_deg * EV_PI / 180.0, E);
  w = ev_evidence (E, pose->dx, pose->dy, 0.0, NULL);
  free (E);
  return (int) lround (w * 1000.0);
}
