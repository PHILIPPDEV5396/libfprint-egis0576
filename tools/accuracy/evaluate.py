#!/usr/bin/env python3
"""Two-fold FAR/FRR evaluation of a captured EH576 dataset, per matcher.

Protocol (docs/matcher-comparison.md): for each finger, enrol on the first
half of its presses and test on the second half, then the reverse. Enrolment
uses the first finger-on frame of each press (the driver's policy) -- or, with
--best-frame, the highest-variance frame (the sensitivity variant). Two accept
rules are reported for BOTH populations: the kit's optimistic one (any frame of
the press scores >= threshold), and the driver's own (two consecutive finger-on
frames over the threshold, the second corroborated on the un-flat-fielded
bytes) as frr_confirmed / far_confirmed. Quote the pair, never one of each.
Genuine trials: the same finger's held-out presses. Impostor trials:
every press of every other finger against each template -- deliberately the
hard set (same person, adjacent fingers).

A partial dataset (capture interrupted) is fine: fingers with fewer than two
presses, or with frames missing, are skipped with a warning and listed under
"skipped" in results.json; --fingers a,b,c evaluates a subset.

Output: the table docs/matcher-comparison.md uses, and a results.json that
holds ONLY scores, counts, thresholds, rates, finger labels, press/frame
COUNTS, the exposure calibration values (register bytes, means, the six-step
[register, mean] search trace), the capture and evaluation dates (day only)
and the hardware info you opt in to.
No frame data, no file names, nothing an image could be rebuilt from. That
file is what you paste into an issue -- never the frames. The frame lists the
scorer reads are handed over in memory (memfd) or, failing that, in a private
temporary directory INSIDE the dataset directory -- never under /tmp.

    python3 evaluate.py ~/egis-accuracy/2026-09-13_1530 --model "Lenovo Yoga 7 14ARB7" --distro "Fedora 44"
"""
import argparse
import datetime
import json
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
THRESHOLD = 5000                          # EGIS_THRESHOLD in egis_engine.h
FLAVOURS = ["vendor", "cleanroom", "gabor"]
SETTLE_FRAMES = 8   # driver/egis0576.c EGIS0576_ENROLL_SETTLE_FRAMES: frames of one press tried before a quality refusal counts
KIT_VERSION = 3   # 3: far_confirmed + raw corroboration (the driver's own rule on BOTH populations)

# Scorer exit codes (score.c) and the only failure texts results.json may carry.
EXIT_REASONS = {
    2: "bad input (baseline or list file unreadable)",
    3: "engine init failed",
    4: "enrolment failed (every enrolment frame rejected)",
}
KNOWN_STDERR = ["enroll_finish failed", "gallery_load failed", "engine init failed",
                "enroll_begin failed", "bad baseline file", "cannot open enroll list",
                "cannot open probe list"]


def median(xs):
    s = sorted(xs)
    n = len(s)
    return float(s[n // 2]) if n % 2 else (s[n // 2 - 1] + s[n // 2]) / 2.0


def load_dataset(ddir, only=None):
    """Returns (manifest, evaluable fingers, presses, baseline path, skipped).
    A finger is evaluable when it has >= 2 presses in the manifest's data and
    every listed frame exists; anything else goes to `skipped` with a reason."""
    mpath = os.path.join(ddir, "manifest.json")
    if not os.path.isfile(mpath):
        raise SystemExit(f"{mpath} not found: is this a dataset directory written by capture.py?")
    man = json.load(open(mpath))
    listed = list(man.get("fingers", []))
    data = man.get("data", {})
    if only is not None:
        unknown = [f for f in only if f not in listed]
        if unknown:
            raise SystemExit(f"--fingers: not in this dataset: {unknown}; available: {listed}")
        listed = [f for f in listed if f in only]
    fingers, presses, skipped = [], {}, []
    for f in listed:
        P = data.get(f, [])
        if len(P) < 2:
            skipped.append({"finger": f, "presses": len(P),
                            "reason": "no presses captured" if not P else "fewer than 2 presses"})
            continue
        paths = [[os.path.join(ddir, x) for x in press] for press in P]
        missing = sum(1 for press in paths for p in press if not os.path.isfile(p))
        if missing:
            skipped.append({"finger": f, "presses": len(P), "reason": f"{missing} listed frame(s) missing"})
            continue
        fingers.append(f)
        presses[f] = paths
    for cand in ("baseline.npy", "baseline.bin"):
        base = os.path.join(ddir, cand)
        if os.path.isfile(base):
            break
    else:
        raise SystemExit("no baseline.npy in the dataset directory")
    return man, fingers, presses, base, skipped


def frame_variances(paths):
    import numpy as np
    return {p: float(np.load(p).astype(np.float64).var()) for p in paths}


class Scorer:
    """Runs one score-<flavour> binary. The enrol/probe lists (frame PATHS)
    are passed as anonymous in-memory files where the platform has them
    (Linux memfd, read by the child through /dev/fd/N) so nothing is written
    anywhere; otherwise as files in a 0700 temporary directory inside the
    dataset directory (never /tmp)."""

    def __init__(self, binary, base, ddir):
        self.binary, self.base, self.ddir = binary, base, ddir
        self.memfd = hasattr(os, "memfd_create") and os.path.isdir("/dev/fd")
        self.tmp = None
        if not self.memfd:
            self.tmp = tempfile.TemporaryDirectory(prefix=".egis-eval-", dir=ddir)

    def close(self):
        if self.tmp is not None:
            self.tmp.cleanup()
            self.tmp = None

    def _run_memfd(self, enroll, probes):
        fds = []
        try:
            for name, lines in (("enroll.lst", enroll), ("probe.lst", probes)):
                fd = os.memfd_create(name)
                os.write(fd, ("\n".join(lines) + "\n").encode())
                os.lseek(fd, 0, os.SEEK_SET)
                fds.append(fd)
            args = [self.binary, self.base] + [f"/dev/fd/{fd}" for fd in fds]
            return subprocess.run(args, capture_output=True, text=True, pass_fds=fds)
        finally:
            for fd in fds:
                os.close(fd)

    def _run_files(self, enroll, probes):
        el = os.path.join(self.tmp.name, "enroll.lst")
        pl = os.path.join(self.tmp.name, "probe.lst")
        try:
            with open(el, "w") as fh:
                fh.write("\n".join(enroll) + "\n")
            with open(pl, "w") as fh:
                fh.write("\n".join(probes) + "\n")
            return subprocess.run([self.binary, self.base, el, pl], capture_output=True, text=True)
        finally:
            for p in (el, pl):
                if os.path.exists(p):
                    os.unlink(p)

    def run(self, enroll, probes):
        r = self._run_memfd(enroll, probes) if self.memfd else self._run_files(enroll, probes)
        if r.returncode != 0:
            return None, None, None, failure_info(r)
        scores, rawok = {}, {}
        for line in r.stdout.splitlines():
            parts = line.split()
            if len(parts) >= 2 and parts[1].lstrip("-").isdigit():
                p = probes[int(parts[0])]
                scores[p] = int(parts[1])
                # third column since kit_version 3; an older scorer omits it
                # and every frame then counts as corroborated, which is what
                # the kit assumed before.
                rawok[p] = int(parts[2]) if len(parts) > 2 and parts[2].lstrip("-").isdigit() else 1
        codes = [int(c) for c in re.findall(r"^enroll \d+ -> (-?\d+)", r.stderr, re.M)]
        m = re.search(r"^enrolled (\d+) frames", r.stderr, re.M)
        diag = {"enrolled": int(m.group(1)) if m else -1, "codes": codes}
        return scores, rawok, diag, None


def failure_info(r):
    """Return code plus a whitelisted reason -- never raw scorer stderr."""
    rc = r.returncode
    if rc < 0:
        reason = "scorer killed by signal"
    else:
        reason = EXIT_REASONS.get(rc, "scorer failed")
    detail = next((k for k in KNOWN_STDERR if k in r.stderr), None)
    return {"returncode": rc, "reason": reason, "detail": detail}


def evaluate(flavour, binary, fingers, presses, base, thr, best_frame, var, ddir, on_var):
    gen, imp, diags, fails = [], [], [], []
    gen_frames = []                       # per genuine press, every frame's score
    # per press, every frame as (score, raw-corroboration verdict, raw variance)
    # -- everything the driver's own accept rule looks at, for both populations
    gen_seq, imp_seq = [], []
    # Per-press extraction bookkeeping. The engine returns -1 for a frame whose
    # feature extraction failed (too few minutiae) and >= 0 for one it compared,
    # so a press score of 0 means "compared, no match" while a press where every
    # frame came back -1 means "nothing extractable". max() alone cannot tell
    # those apart, and that distinction is what a run with many zero-scoring
    # genuine presses needs in order to be diagnosable at all.
    frames_total = frames_nofeat = 0
    gen_nofeat = imp_nofeat = 0
    sc = Scorer(binary, base, ddir)
    try:
        for f in fingers:
            P = presses[f]
            half = len(P) // 2
            for fold, (E, T) in enumerate([(P[:half], P[half:]), (P[half:], P[:half])]):
                if best_frame:
                    enroll = [max(p, key=lambda x: var[x]) for p in E]
                else:
                    # the driver's policy: the first finger-on frame, but a frame
                    # the engine refuses for quality is followed by the next
                    # frames of the SAME press (up to SETTLE_FRAMES, ~300 ms,
                    # driver/egis0576.c EGIS0576_ENROLL_SETTLE_FRAMES) before the
                    # press counts as refused. score.c gets each press as a
                    # group, "--" separated, and tries the group in order.
                    enroll = []
                    for p in E:
                        enroll += p[:SETTLE_FRAMES] + ["--"]
                others = [(g, p) for g in fingers if g != f for p in presses[g]]
                probes = [x for p in T for x in p] + [x for _, p in others for x in p]
                scores, rawok, diag, err = sc.run(enroll, probes)
                if scores is None:
                    fails.append({"finger": f, "fold": fold, **err})
                    continue
                diags.append({"finger": f, "fold": fold, "enrolled": diag["enrolled"], "codes": diag["codes"]})
                for p in T:
                    s = [scores.get(x, -1) for x in p]
                    frames_total += len(s)
                    frames_nofeat += sum(1 for v in s if v < 0)
                    if all(v < 0 for v in s):
                        gen_nofeat += 1
                    gen.append(max(s))
                    gen_frames.append(s)
                    gen_seq.append([(scores.get(x, -1), rawok.get(x, 1), var[x]) for x in p])
                for _, p in others:
                    s = [scores.get(x, -1) for x in p]
                    frames_total += len(s)
                    frames_nofeat += sum(1 for v in s if v < 0)
                    if all(v < 0 for v in s):
                        imp_nofeat += 1
                    imp.append(max(s))
                    # The impostor frames the press maximum throws away are
                    # exactly what the driver's rule needs; without them a
                    # far under that rule cannot be computed at all.
                    imp_seq.append([(scores.get(x, -1), rawok.get(x, 1), var[x]) for x in p])
    finally:
        sc.close()
    if not gen:
        return {"flavour": flavour, "failures": fails, "n_genuine": 0, "n_impostor": 0}
    frr = sum(1 for s in gen if s < thr) / len(gen)

    def driver_accepts(seq):
        """The driver's own accept rule, applied frame by frame.

        Three things distinguish it from "the press maximum cleared the
        threshold", and all three matter (driver/egis0576.c match_result and
        capture_poll):
          * it scores only frames the capture loop hands to the matcher, i.e.
            raw variance >= FINGER_ON_VAR. capture.py keeps the rest of a
            press down to FINGER_OFF_VAR, so the kit's frame list contains
            frames the driver never sees;
          * a frame over the threshold is only a CANDIDATE; the next scored
            frame must clear it too, and a frame below the threshold breaks
            the chain ("a stale frame stands alone, a genuine press does not");
          * the confirming frame must corroborate on the un-flat-fielded
            bytes, otherwise the accept is discarded and the chain restarts.
        Measuring FRR under this rule and FAR under the press maximum -- which
        is what this kit did until kit_version 3 -- compares a strict rule
        against a lenient one and understates the driver's margin twice over.
        """
        prev = False
        for s, ok, v in seq:
            if v < on_var:
                continue
            if s >= thr:
                if prev:
                    if ok:
                        return True
                    prev = False      # not corroborated: candidate discarded
                else:
                    prev = True
            else:
                prev = False
        return False

    drv_acc = sum(1 for seq in gen_seq if driver_accepts(seq))
    frr_confirmed = 1.0 - drv_acc / len(gen_seq) if gen_seq else None
    far = (sum(1 for s in imp if s >= thr) / len(imp)) if imp else None
    imp_acc = sum(1 for seq in imp_seq if driver_accepts(seq))
    far_confirmed = imp_acc / len(imp_seq) if imp_seq else None
    # Equal error rate, but ONLY where the two curves actually cross. When no
    # impostor scores at all (the vendor matcher does this: every impostor
    # comparison in all three published runs scored exactly 0), FAR is 0 at
    # every threshold >= 1, the minimum of |FRR - FAR| is just the smallest
    # non-zero genuine score, and (FRR + FAR) / 2 there is FRR/2 -- a number
    # that looks like an error rate and is not one. Report null instead.
    best = None
    if imp:
        for t in sorted(set(gen) | set(imp)):
            fr = sum(1 for s in gen if s < t) / len(gen)
            fa = sum(1 for s in imp if s >= t) / len(imp)
            if best is None or abs(fr - fa) < best[0]:
                best = (abs(fr - fa), t, fr, fa)
        if best is not None and best[3] <= 0.0:
            best = None          # no impostor reaches the best point: no equal-error point exists
    return {
        "flavour": flavour,
        "threshold": thr,
        "n_genuine": len(gen), "n_impostor": len(imp),
        "frr": frr, "far": far,
        # The pair the driver actually produces: both populations under the
        # two-frame + raw-corroboration rule. frr/far above are the kit's
        # optimistic press-maximum rule, kept for continuity with older runs.
        "frr_confirmed": frr_confirmed,
        "far_confirmed": far_confirmed,
        "genuine_presses_confirmed": drv_acc,
        "impostor_presses_confirmed": imp_acc,
        "genuine_min": min(gen), "genuine_median": median(gen), "genuine_max": max(gen),
        "impostor_min": min(imp) if imp else None,
        "impostor_median": median(imp) if imp else None,
        "impostor_max": max(imp) if imp else None,
        "eer": (best[2] + best[3]) / 2.0 if best else None,
        "eer_threshold": best[1] if best else None,
        # null when the FAR and FRR curves never cross, i.e. no threshold admits
        # an impostor: there is no equal-error point to report.
        "eer_defined": bool(best),
        "genuine_press_scores": gen, "impostor_press_scores": imp,
        "enrolment": diags,
        # Extraction health, counts only: how many probe frames the engine could
        # not extract features from, and how many presses had no extractable
        # frame at all. A press that scored 0 with extraction_failed_presses not
        # counting it was compared and did not match.
        "frames_scored": frames_total,
        "frames_no_features": frames_nofeat,
        "genuine_presses_no_features": gen_nofeat,
        "impostor_presses_no_features": imp_nofeat,
        "failures": fails,
    }


def pct(x):
    return "n/a" if x is None else f"{x * 100:.1f} %"


def stats3(r, k):
    if r[f"{k}_min"] is None:
        return "n/a"
    return f"{r[f'{k}_min']} / {r[f'{k}_median']:.0f} / {r[f'{k}_max']}"


def print_table(label, r):
    print(f"\n### {label}")
    if not r.get("n_genuine"):
        print(f"no results ({len(r.get('failures', []))} fold failures): {r.get('failures')}")
        return
    print(f"| | genuine (n = {r['n_genuine']}) | impostor (n = {r['n_impostor']}) |")
    print("|---|---:|---:|")
    print(f"| score min / median / max | {stats3(r, 'genuine')} | {stats3(r, 'impostor')} |")
    print(f"| at threshold {r['threshold']}, any frame | FRR {pct(r['frr'])} | FAR {pct(r['far'])} |")
    print(f"| the driver's rule (two frames + raw) | FRR {pct(r['frr_confirmed'])} | FAR {pct(r['far_confirmed'])} |")
    if r["eer"] is not None:
        print(f"\nEER {pct(r['eer'])} at score {r['eer_threshold']}")
    elif r["n_impostor"] == 0:
        print("\nEER n/a (no impostor trials: only one finger evaluated)")
    else:
        print("\nEER n/a: the two populations do not overlap (no threshold has both"
              " a false accept and a false reject; lowest genuine "
              f"{min(r['genuine_press_scores'])} > highest impostor {max(r['impostor_press_scores'])})")
    if r["failures"]:
        print(f"fold failures: {r['failures']}")
    tpl = [d["enrolled"] for d in r["enrolment"]]
    print(f"template sizes per fold (frames): {tpl}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("dataset", help="directory written by capture.py (manifest.json + frames)")
    ap.add_argument("--bin-dir", default=HERE, help="where score-vendor / score-cleanroom / score-gabor live (default: here)")
    ap.add_argument("--flavour", action="append", choices=FLAVOURS, help="evaluate only this flavour (repeatable)")
    ap.add_argument("--fingers", default=None, help="comma-separated subset of the dataset's finger labels")
    ap.add_argument("--threshold", type=int, default=THRESHOLD, help=f"accept threshold (default {THRESHOLD})")
    ap.add_argument("--best-frame", action="store_true",
                    help="sensitivity variant: enrol the highest-variance frame of each press instead of the first")
    ap.add_argument("--out", default=None, help="results file (default: <dataset>/results.json)")
    ap.add_argument("--pair-list", action="store_true",
                    help="write <dataset>/pairdiag.lst (one line per press: label + first frame) and stop, "
                         "for the pairdiag diagnostic")
    ap.add_argument("--model", default=None, help='laptop model to record, e.g. "Lenovo Yoga 7 14ARB7" (opt-in)')
    ap.add_argument("--distro", default=None, help='distribution to record, e.g. "Fedora 44" (opt-in)')
    ap.add_argument("--note", default=None,
                    help="free-text note, goes verbatim into the shareable results.json (no personal data)")
    args = ap.parse_args()

    ddir = os.path.abspath(args.dataset)
    only = [x.strip() for x in args.fingers.split(",") if x.strip()] if args.fingers else None
    man, fingers, presses, base, skipped = load_dataset(ddir, only)
    for s in skipped:
        print(f"warning: skipping finger {s['finger']!r}: {s['reason']}", file=sys.stderr)
    if not fingers:
        raise SystemExit("no finger has two or more presses with all frames present; nothing to evaluate")
    if args.pair_list:
        lp = os.path.join(ddir, "pairdiag.lst")
        with open(lp, "w") as fh:
            for f in fingers:
                for pr in presses[f]:
                    fh.write(f"{f} {pr[0]}\n")
        os.chmod(lp, 0o600)
        print(f"wrote {lp} ({sum(len(presses[f]) for f in fingers)} presses)\n\n"
              f"    {os.path.join(HERE, 'pairdiag')} {base} {lp}\n\n"
              "prints a JSON block of counts and rates only -- that one is safe to paste.")
        return 0

    if len(fingers) == 1:
        print("warning: only one finger evaluable -- genuine trials only, no impostor set, no FAR/EER",
              file=sys.stderr)
    flavours = args.flavour or FLAVOURS
    binaries = {}
    for fl in flavours:
        b = os.path.join(os.path.abspath(args.bin_dir), f"score-{fl}")
        if not os.access(b, os.X_OK):
            raise SystemExit(f"{b} not found -- run `make` in {HERE} first")
        binaries[fl] = b

    # Needed for every run since kit_version 3, not just --best-frame: the
    # driver scores a frame only when its raw variance says a finger is on the
    # sensor, and the kit cannot apply the driver's accept rule without that.
    var = frame_variances([x for f in fingers for p in presses[f] for x in p])

    n_frames = sum(len(p) for f in fingers for p in presses[f])
    n_presses = sum(len(presses[f]) for f in fingers)
    print(f"dataset: {len(fingers)} fingers, {n_presses} presses, {n_frames} frames"
          + (f" ({len(skipped)} finger(s) skipped)" if skipped else "") + "; "
          f"enrolment frame = {'best-variance' if args.best_frame else 'first finger-on'}; "
          f"threshold {args.threshold}")

    results = {}
    for fl in flavours:
        results[fl] = evaluate(fl, binaries[fl], fingers, presses, base, args.threshold,
                               args.best_frame, var, ddir, float(man.get("on", 250.0)))
        print_table(f"{fl} matcher" + (" (best-frame enrolment)" if args.best_frame else ""), results[fl])
        r = results[fl]
        if r.get("n_genuine"):
            eer = f"EER {pct(r['eer'])} @ {r['eer_threshold']}" if r["eer"] is not None else "EER n/a"
            conf = r.get("frr_confirmed")
            if conf is not None:
                print(f"[{fl}] press accepted by the kit's rule (any frame over the threshold): "
                      f"{r['n_genuine'] - round(r['frr'] * r['n_genuine'])}/{r['n_genuine']}; "
                      f"by the driver's rule (two consecutive frames + raw corroboration): "
                      f"{r['genuine_presses_confirmed']}/{r['n_genuine']}, "
                      f"impostor presses accepted under that rule: "
                      f"{r['impostor_presses_confirmed']}/{r['n_impostor']}")
            print(f"[{fl}] genuine n={r['n_genuine']} impostor n={r['n_impostor']} | @{r['threshold']}: "
                  f"FRR {pct(r['frr'])}  FAR {pct(r['far'])} | driver's rule: "
                  f"FRR {pct(r['frr_confirmed'])}  FAR {pct(r['far_confirmed'])} | "
                  f"genuine {stats3(r, 'genuine')}  impostor {stats3(r, 'impostor')} | {eer}")

    created = man.get("created")
    out = {
        "kit": "egis0576-accuracy", "kit_version": KIT_VERSION,
        "evaluated": datetime.date.today().isoformat(),
        "hardware": {k: v for k, v in (("model", args.model), ("distro", args.distro)) if v},
        "note": args.note,
        "dataset": {
            "captured": created[:10] if isinstance(created, str) else None,     # date only
            "manifest_version": man.get("manifest_version"),
            "fingers": fingers,
            "presses_per_finger": [len(presses[f]) for f in fingers],
            "frames_per_press": {f: [len(p) for p in presses[f]] for f in fingers},
            "n_presses": n_presses, "n_frames": n_frames,
            "skipped": skipped,
            "finger_on_var": man.get("on"), "finger_off_var": man.get("off"),
            "baseline_frames": man.get("baseline_frames", 8),
            "max_frames_per_press": man.get("max_frames_per_press"),
            "exposure": {
                "dc_c_baked": man.get("dc_c_baked"),
                "dc_c_calibrated": man.get("dc_c_calibrated"),
                "target_mean": man.get("exposure_target"),
                "nofinger_mean": man.get("nofinger_mean"),
                "calibration_trace": man.get("calibration_trace"),   # six [register, mean] pairs
            },
            "complete": man.get("complete", True),
        },
        "protocol": {
            "folds": "two-fold per finger (first half enrol / second half test, then reversed)",
            "enrol_frame": "best-variance" if args.best_frame else "first finger-on",
            "accept_rule": "press accepted if any of its frames scores >= threshold "
                           "(frr); the driver needs two consecutive such frames (frr_confirmed)",
            "impostor_set": "every press of every other finger of the same person",
            "threshold": args.threshold,
        },
        "results": results,
    }
    if not out["hardware"]:
        del out["hardware"]
    if out["note"] is None:
        del out["note"]
    dest = os.path.abspath(args.out) if args.out else os.path.join(ddir, "results.json")
    with open(dest, "w") as fh:
        json.dump(out, fh, indent=1)
    print(f"\n-> {dest}  (scores, counts and rates only -- this is the file to share; the frames are not)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
