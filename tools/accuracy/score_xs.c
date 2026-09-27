/* score_xs.c -- cross-session scorer for the egis0576 accuracy kit.
 *
 *   score_xs <enrol-baseline> <probe-baseline> <enroll.lst> <probe.lst>
 *
 * score.c with one difference: the enrolment frames and the probe frames come
 * from two different capture sessions, and each session has its own
 * flat-field baseline (every boot, the driver captures a new one). A frame is
 * flat-fielded with the baseline of the session it was captured in: enrolment
 * frames with <enrol-baseline>, probe frames with <probe-baseline>. That is
 * what the driver does when a finger enrolled on one boot is verified on a
 * later one -- the template carries the enrolment session's flat field inside
 * it, the probe carries the current one.
 *
 * Everything else is score.c, and deliberately so: the frame reader and the
 * flat field ARE score.c's (it is #included below, its main() renamed out of
 * the way, so there is exactly one copy of either), and the enrolment loop
 * (the settle groups, "--"-separated) and the probe loop (the gallery reloaded
 * at every "--", verify then the raw corroboration on the un-flat-fielded
 * bytes) are score.c's line for line. With both baselines the same file the
 * output is byte-for-byte score.c's; cross.py relies on that and checks it
 * (--self-test).
 *
 * Built once per matcher flavour against the same objects as score-<flavour>
 * (Makefile: score_xs-vendor, score_xs-gabor, score_xs-cleanroom). Output
 * format, stdout and stderr, exactly as score.c documents it:
 *   <probe-line-index> <score> <raw-ok>
 * No frame content ever reaches stdout or stderr. */
#define main egis_score_single_session_main
#include "score.c"
#undef main

int main(int argc, char **argv)
{
  uint8_t base_e[N], base_p[N], raw[N], cor[N];
  char line[4096];
  FILE *el, *pl;
  int added = 0, r, sz, idx;
  uint8_t *blob = NULL;
  const uint8_t *blobs[1];
  int sizes[1];

  if (argc != 5) {
    fprintf(stderr, "usage: %s <enrol-baseline.npy|bin> <probe-baseline.npy|bin> <enroll.lst> <probe.lst>\n",
            argv[0]);
    return 2;
  }
  if (rd(argv[1], base_e)) { fprintf(stderr, "bad baseline file\n"); return 2; }
  if (rd(argv[2], base_p)) { fprintf(stderr, "bad baseline file\n"); return 2; }
  EgisEngine *eng = egis_engine_new();
  if (!eng) { fprintf(stderr, "engine init failed\n"); return 3; }
  if (egis_enroll_begin(eng) != 0) { fprintf(stderr, "enroll_begin failed\n"); return 3; }

  el = fopen(argv[3], "r");
  if (!el) { fprintf(stderr, "cannot open enroll list\n"); return 2; }
  /* score.c's enrolment loop, flat-fielding with the ENROLMENT session's
   * baseline: one line per frame, a press's frames grouped by a "--" line
   * after the group, tried in order until the engine returns anything but -2. */
  idx = 0;
  {
    int settled = 0;
    int last = -2;
    while (fgets(line, sizeof line, el)) {
      int prog = 0;
      chomp(line);
      if (!*line) continue;
      if (strcmp(line, "--") == 0) {
        if (!settled)
          fprintf(stderr, "enroll %d -> %d (progress 0)\n", idx, last), idx++;
        settled = 0; last = -2;
        continue;
      }
      if (settled) continue;
      if (rd(line, raw)) { fprintf(stderr, "enroll %d -> unreadable\n", idx); idx++; settled = 1; continue; }
      prep(raw, base_e, cor);
      r = egis_enroll_add(eng, cor, &prog);
      last = r;
      if (r == -2) continue;
      fprintf(stderr, "enroll %d -> %d (progress %d)\n", idx, r, prog);
      idx++;
      settled = 1;
      if (r == 1 || r == 2) added++;
      if (r == 2) break;
    }
  }
  fclose(el);

  sz = egis_enroll_finish(eng, NULL, 0);
  if (sz > 0) { blob = malloc((size_t)sz); sz = blob ? egis_enroll_finish(eng, blob, sz) : -1; }
  fprintf(stderr, "enrolled %d frames, template %d bytes\n", added, sz);
  if (sz <= 0) { fprintf(stderr, "enroll_finish failed %d\n", sz); return 4; }
  blobs[0] = blob;
  sizes[0] = sz;
  if (egis_gallery_load(eng, blobs, sizes, 1) != 0) { fprintf(stderr, "gallery_load failed\n"); return 4; }

  pl = fopen(argv[4], "r");
  if (!pl) { fprintf(stderr, "cannot open probe list\n"); return 2; }
  /* score.c's probe loop, flat-fielding with the PROBE session's baseline;
   * the raw corroboration gets the un-flat-fielded bytes, as in score.c. */
  idx = 0;
  while (fgets(line, sizeof line, pl)) {
    chomp(line);
    if (!*line) continue;
    if (strcmp(line, "--") == 0) {
      /* press boundary: back to the template as enrolled (kit_version 4) */
      if (egis_gallery_load(eng, blobs, sizes, 1) != 0) { fprintf(stderr, "gallery_load failed\n"); return 4; }
      idx++;
      continue;
    }
    if (rd(line, raw)) {
      printf("%d unreadable\n", idx);
    } else {
      int s, ok;
      prep(raw, base_p, cor);
      s = egis_verify(eng, cor, 0);
      ok = s >= 0 ? egis_verify_raw_ok(eng, raw, 0) : 1;
      printf("%d %d %d\n", idx, s, ok);
    }
    idx++;
  }
  fclose(pl);
  free(blob);
  return 0;
}
