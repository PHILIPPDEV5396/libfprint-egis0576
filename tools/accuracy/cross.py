#!/usr/bin/env python3
"""Cross-session FAR/FRR of an EH576 matcher: enrol on one capture session,
verify with another -- and, for any set, how far each press is from the
driver's accept decision.

evaluate.py measures one session against itself. What a user lives with is a
finger enrolled on one day (one boot, one flat-field baseline) and verified on
later ones, and that is a different measurement: the template carries the
enrolment session's flat field and skin, the probe the current one. This tool
takes two datasets written by capture.py:

    python3 cross.py <enrol-dataset> <probe-dataset> [--flavour gabor] [--out results-cross.json]

Protocol (every rule below is evaluate.py's except the split into sessions):
  * templates: per finger, ALL presses of that finger from the enrolment
    session, enrolled exactly as evaluate.py enrols -- the first finger-on
    frame of each press, with the next frames of the same press (up to
    SETTLE_FRAMES) tried when the engine refuses one for quality ("--"
    groups, score.c's enrolment loop);
  * every frame is flat-fielded with ITS OWN session's baseline: enrolment
    frames with the enrolment dataset's baseline.npy, probe frames with the
    probe dataset's (score_xs.c);
  * genuine trials: every press of the same finger from the probe session;
    impostor trials: every press of every OTHER finger from the probe
    session. Presses of the enrolment session are never probes. With five
    fingers of twelve presses that is 60 genuine and 5 x 48 = 240 impostor
    trials per set;
  * the gallery is reloaded before every probe press (a "--" before each
    press, kit_version 4), as the driver loads it at the start of every
    verify;
  * the accept decision is the driver's rule, exactly evaluate.py's
    driver_accepts(): only frames with raw variance >= the probe session's
    finger-on gate count, two consecutive such frames must score >= the
    threshold, and the second must be corroborated on the un-flat-fielded
    bytes (score_xs's third column, egis_verify_raw_ok). The kit's lenient
    press-maximum rule is reported next to it as frr / far, as in evaluate.py.

Per press it also reports the driver's-rule LEVEL: the highest threshold at
which that press's two-frame chain accepts, in score units and as a multiple
of the threshold -- so "the strongest impostor press reached 0.30x" and "the
weakest accepted genuine press cleared it 1.02x" can be quoted. A level of 0
means no positive threshold accepts the press. The rule is not monotonic in
the threshold (an uncorroborated confirming frame discards the candidate
before it, and which frames are candidates depends on the threshold), so the
level is found by trying every score the press produced, not read off a pair;
and a press can in principle reach a level above 1 and still be rejected at
the threshold -- it is then listed as rejected, with its level.

    python3 cross.py --in-session <dataset>

runs evaluate.py's own two-fold in-session protocol with the kit scorer
(score-<flavour>) and reports the same driver's-rule counts -- identical to
evaluate.py's frr_confirmed / far_confirmed -- plus the per-press levels,
which evaluate.py does not print.

    python3 cross.py --self-test <dataset>

checks that score_xs-<flavour>, given the same baseline twice, prints
byte-for-byte what score-<flavour> prints (one template per flavour).

The results file has evaluate.py's privacy rules: scores, counts, rates,
finger labels, press indices (0-based, within the probe session) and
counts, capture dates (day only). No frame data, no file names, no paths.
Frame lists reach the scorer in memory (memfd) or, failing that, in a private
temporary directory inside the probe dataset -- never under /tmp.
"""
import argparse
import datetime
import json
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import evaluate as ev  # noqa: E402  (the kit's own loaders, scorer runner and constants)

CROSS_VERSION = 1
FLAVOURS = ev.FLAVOURS


class Runner(ev.Scorer):
    """evaluate.Scorer with the baseline arguments generalised: score-<fl>
    takes one baseline, score_xs-<fl> two (enrolment session's, probe
    session's). Output parsing is evaluate.Scorer.run's, unchanged."""

    def __init__(self, binary, bases, ddir):
        super().__init__(binary, None, ddir)
        self.bases = list(bases)

    def _run_memfd(self, enroll, probes):
        fds = []
        try:
            for name, lines in (("enroll.lst", enroll), ("probe.lst", probes)):
                fd = os.memfd_create(name)
                os.write(fd, ("\n".join(lines) + "\n").encode())
                os.lseek(fd, 0, os.SEEK_SET)
                fds.append(fd)
            args = [self.binary] + self.bases + [f"/dev/fd/{fd}" for fd in fds]
            return subprocess.run(args, capture_output=True, text=True, pass_fds=fds)
        finally:
            for fd in fds:
                os.close(fd)

    def _run_files(self, enroll, probes):
        el = os.path.join(self.tmp.name, f"enroll-{os.getpid()}-{id(enroll)}.lst")
        pl = os.path.join(self.tmp.name, f"probe-{os.getpid()}-{id(probes)}.lst")
        try:
            with open(el, "w") as fh:
                fh.write("\n".join(enroll) + "\n")
            with open(pl, "w") as fh:
                fh.write("\n".join(probes) + "\n")
            return subprocess.run([self.binary] + self.bases + [el, pl], capture_output=True, text=True)
        finally:
            for p in (el, pl):
                if os.path.exists(p):
                    os.unlink(p)

    def raw(self, enroll, probes):
        """The scorer's CompletedProcess, for --self-test only (never printed)."""
        return self._run_memfd(enroll, probes) if self.memfd else self._run_files(enroll, probes)


def driver_accepts(seq, thr, on_var):
    """evaluate.py's driver_accepts(), verbatim: seq = [(score, raw_ok, raw variance)]."""
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


def driver_level(seq, on_var):
    """The highest threshold at which driver_accepts(seq) holds (score units),
    0 if no positive threshold accepts the press. Acceptance only changes at a
    score the press produced, but it is not monotonic, so every such score is
    tried from the top down."""
    for t in sorted({s for s, _, v in seq if v >= on_var and s > 0}, reverse=True):
        if driver_accepts(seq, t, on_var):
            return t
    return 0


def press_seq(scores, rawok, var, press):
    return [(scores.get(x, -1), rawok.get(x, 1), var[x]) for x in press]


def enrol_list(presses):
    """evaluate.py's enrolment list: the first SETTLE_FRAMES frames of each
    press, one "--"-terminated group per press."""
    out = []
    for p in presses:
        out += p[:ev.SETTLE_FRAMES] + ["--"]
    return out


def probe_list(presses):
    """A "--" before every press: the scorer reloads the gallery there."""
    return [x for p in presses for x in ["--"] + p]


def summarise(flavour, thr, on_var, trials, diags, fails):
    """trials: list of dicts {kind: genuine|impostor, template, fold, finger,
    press, seq}. Returns the per-flavour results block."""
    gen = [t for t in trials if t["kind"] == "genuine"]
    imp = [t for t in trials if t["kind"] == "impostor"]
    if not gen:
        return {"flavour": flavour, "failures": fails, "n_genuine": 0, "n_impostor": 0}
    frames_total = frames_nofeat = gen_nofeat = imp_nofeat = 0
    for t in trials:
        s = [x[0] for x in t["seq"]]
        t["max"] = max(s)
        t["accepted"] = driver_accepts(t["seq"], thr, on_var)
        t["level"] = driver_level(t["seq"], on_var)
        frames_total += len(s)
        frames_nofeat += sum(1 for v in s if v < 0)
        if all(v < 0 for v in s):
            if t["kind"] == "genuine":
                gen_nofeat += 1
            else:
                imp_nofeat += 1
    gmax = [t["max"] for t in gen]
    imax = [t["max"] for t in imp]
    g_acc = [t for t in gen if t["accepted"]]
    i_acc = [t for t in imp if t["accepted"]]

    def x(level):
        return round(level / thr, 4)

    def gentry(t):
        e = {"finger": t["finger"], "press": t["press"], "level": t["level"], "level_x": x(t["level"]),
             "accepted": t["accepted"]}
        if t.get("fold") is not None:
            e["fold"] = t["fold"]
        return e

    def ientry(t):
        e = {"template": t["template"], "finger": t["finger"], "press": t["press"],
             "level": t["level"], "level_x": x(t["level"]), "accepted": t["accepted"]}
        if t.get("fold") is not None:
            e["fold"] = t["fold"]
        return e

    weakest = min(g_acc, key=lambda t: t["level"]) if g_acc else None
    strongest = max(imp, key=lambda t: t["level"]) if imp else None
    return {
        "flavour": flavour,
        "threshold": thr,
        "n_genuine": len(gen), "n_impostor": len(imp),
        # the kit's lenient rule (any frame of the press over the threshold)
        "frr": sum(1 for s in gmax if s < thr) / len(gmax),
        "far": (sum(1 for s in imax if s >= thr) / len(imax)) if imax else None,
        # the driver's rule, on both populations
        "frr_confirmed": 1.0 - len(g_acc) / len(gen),
        "far_confirmed": (len(i_acc) / len(imp)) if imp else None,
        "genuine_presses_confirmed": len(g_acc),
        "impostor_presses_confirmed": len(i_acc),
        "genuine_rejected": [gentry(t) for t in gen if not t["accepted"]],
        "impostor_accepted": [ientry(t) for t in i_acc],
        "genuine_min": min(gmax), "genuine_median": ev.median(gmax), "genuine_max": max(gmax),
        "impostor_min": min(imax) if imax else None,
        "impostor_median": ev.median(imax) if imax else None,
        "impostor_max": max(imax) if imax else None,
        # driver's-rule levels: highest threshold at which the press's chain
        # accepts (score units; level_x = multiple of the threshold)
        "weakest_accepted_genuine": gentry(weakest) if weakest else None,
        "strongest_impostor": ientry(strongest) if strongest else None,
        "genuine_levels": [gentry(t) for t in gen],
        "impostor_levels": [ientry(t) for t in imp],
        "genuine_press_scores": gmax, "impostor_press_scores": imax,
        "enrolment": diags,
        "frames_scored": frames_total,
        "frames_no_features": frames_nofeat,
        "genuine_presses_no_features": gen_nofeat,
        "impostor_presses_no_features": imp_nofeat,
        "failures": fails,
    }


def run_jobs(jobs, n):
    """jobs: list of (key, callable). Runs them on up to n threads (each one
    one scorer process), returns {key: result} in job order."""
    if n <= 1:
        return {k: f() for k, f in jobs}
    with ThreadPoolExecutor(max_workers=n) as pool:
        futs = [(k, pool.submit(f)) for k, f in jobs]
        return {k: fu.result() for k, fu in futs}


def cross_evaluate(flavour, binary, A, B, thr, jobs):
    """A, B: (manifest, fingers, presses, baseline, skipped, var, ddir)."""
    manA, fA, PA, baseA, _, _, _ = A
    manB, fB, PB, baseB, _, varB, ddirB = B
    on_var = float(manB.get("on", 250.0))
    templates = [f for f in fA if f in fB]
    sc = Runner(binary, [baseA, baseB], ddirB)
    work = []
    for f in templates:
        others = [(g, i, p) for g in fB if g != f for i, p in enumerate(PB[g])]
        genuine = [(f, i, p) for i, p in enumerate(PB[f])]
        probes = probe_list([p for _, _, p in genuine] + [p for _, _, p in others])
        work.append((f, genuine, others, enrol_list(PA[f]), probes))
    try:
        res = run_jobs([(w[0], (lambda w=w: sc.run(w[3], w[4]))) for w in work], jobs)
    finally:
        sc.close()
    trials, diags, fails = [], [], []
    for f, genuine, others, _, _ in work:
        scores, rawok, diag, err = res[f]
        if scores is None:
            fails.append({"finger": f, **err})
            continue
        diags.append({"finger": f, "enrolled": diag["enrolled"], "codes": diag["codes"]})
        for g, i, p in genuine:
            trials.append({"kind": "genuine", "template": f, "fold": None, "finger": g, "press": i,
                           "seq": press_seq(scores, rawok, varB, p)})
        for g, i, p in others:
            trials.append({"kind": "impostor", "template": f, "fold": None, "finger": g, "press": i,
                           "seq": press_seq(scores, rawok, varB, p)})
    return summarise(flavour, thr, on_var, trials, diags, fails)


def in_session_evaluate(flavour, binary, D, thr, jobs):
    """evaluate.py's two-fold protocol, run with the kit scorer, keeping every
    frame's (score, raw_ok, variance) for the levels."""
    man, fingers, presses, base, _, var, ddir = D
    on_var = float(man.get("on", 250.0))
    sc = Runner(binary, [base], ddir)
    work = []
    for f in fingers:
        P = presses[f]
        half = len(P) // 2
        idx = list(range(len(P)))
        for fold, (E, T) in enumerate([(idx[:half], idx[half:]), (idx[half:], idx[:half])]):
            genuine = [(f, i, P[i]) for i in T]
            others = [(g, i, p) for g in fingers if g != f for i, p in enumerate(presses[g])]
            probes = probe_list([p for _, _, p in genuine] + [p for _, _, p in others])
            work.append(((f, fold), genuine, others, enrol_list([P[i] for i in E]), probes))
    try:
        res = run_jobs([(w[0], (lambda w=w: sc.run(w[3], w[4]))) for w in work], jobs)
    finally:
        sc.close()
    trials, diags, fails = [], [], []
    for (f, fold), genuine, others, _, _ in work:
        scores, rawok, diag, err = res[(f, fold)]
        if scores is None:
            fails.append({"finger": f, "fold": fold, **err})
            continue
        diags.append({"finger": f, "fold": fold, "enrolled": diag["enrolled"], "codes": diag["codes"]})
        for g, i, p in genuine:
            trials.append({"kind": "genuine", "template": f, "fold": fold, "finger": g, "press": i,
                           "seq": press_seq(scores, rawok, var, p)})
        for g, i, p in others:
            trials.append({"kind": "impostor", "template": f, "fold": fold, "finger": g, "press": i,
                           "seq": press_seq(scores, rawok, var, p)})
    return summarise(flavour, thr, on_var, trials, diags, fails)


def self_test(flavour, bin_dir, D):
    """score_xs-<fl> with the same baseline twice must print exactly what
    score-<fl> prints (stdout and stderr), on the first finger's first fold."""
    man, fingers, presses, base, _, _, ddir = D
    f = fingers[0]
    P = presses[f]
    half = len(P) // 2
    enroll = enrol_list(P[:half])
    probes = probe_list(P[half:] + [p for g in fingers if g != f for p in presses[g]])
    one = Runner(os.path.join(bin_dir, f"score-{flavour}"), [base], ddir)
    two = Runner(os.path.join(bin_dir, f"score_xs-{flavour}"), [base, base], ddir)
    try:
        a, b = one.raw(enroll, probes), two.raw(enroll, probes)
    finally:
        one.close()
        two.close()
    same = a.returncode == b.returncode and a.stdout == b.stdout and a.stderr == b.stderr
    return {"flavour": flavour, "identical": same, "probe_lines": len(probes),
            "scored_lines": len(a.stdout.splitlines())}


def dataset_info(man, fingers, presses, skipped):
    created = man.get("created")
    return {
        "captured": created[:10] if isinstance(created, str) else None,     # date only
        "manifest_version": man.get("manifest_version"),
        "fingers": fingers,
        "presses_per_finger": [len(presses[f]) for f in fingers],
        "n_presses": sum(len(presses[f]) for f in fingers),
        "n_frames": sum(len(p) for f in fingers for p in presses[f]),
        "skipped": skipped,
        "finger_on_var": man.get("on"), "finger_off_var": man.get("off"),
    }


def load(path, only):
    ddir = os.path.abspath(path)
    man, fingers, presses, base, skipped = ev.load_dataset(ddir, only)
    for s in skipped:
        print(f"warning: skipping finger {s['finger']!r}: {s['reason']}", file=sys.stderr)
    if not fingers:
        raise SystemExit("no finger has two or more presses with all frames present; nothing to evaluate")
    return man, fingers, presses, base, skipped, ddir


def lx(t):
    return "n/a" if t is None else f"{t['level_x']:.3f}x"


def print_result(label, r):
    print(f"\n### {label}")
    if not r.get("n_genuine"):
        print(f"no results ({len(r.get('failures', []))} template failures): {r.get('failures')}")
        return
    print(f"| | genuine (n = {r['n_genuine']}) | impostor (n = {r['n_impostor']}) |")
    print("|---|---:|---:|")
    print(f"| score min / median / max | {ev.stats3(r, 'genuine')} | {ev.stats3(r, 'impostor')} |")
    print(f"| at threshold {r['threshold']}, any frame | FRR {ev.pct(r['frr'])} | FAR {ev.pct(r['far'])} |")
    print(f"| the driver's rule (two frames + raw) | FRR {ev.pct(r['frr_confirmed'])} "
          f"| FAR {ev.pct(r['far_confirmed'])} |")
    w, s = r["weakest_accepted_genuine"], r["strongest_impostor"]
    print(f"driver's rule: genuine presses accepted {r['genuine_presses_confirmed']}/{r['n_genuine']}, "
          f"impostor presses accepted {r['impostor_presses_confirmed']}/{r['n_impostor']}")
    print(f"levels (x threshold): weakest accepted genuine {lx(w)}"
          + (f" ({w['finger']} p{w['press']})" if w else "")
          + f", strongest impostor {lx(s)}"
          + (f" ({s['finger']} p{s['press']} vs template {s['template']})" if s else ""))
    rej = ", ".join(f"{t['finger']} p{t['press']}" + (f" (fold {t['fold']})" if "fold" in t else "")
                    + f" [{t['level_x']:.3f}x]" for t in r["genuine_rejected"])
    print(f"genuine presses rejected: {rej or 'none'}")
    if r["impostor_accepted"]:
        print("impostor presses ACCEPTED: " + ", ".join(
            f"{t['finger']} p{t['press']} vs template {t['template']} [{t['level_x']:.3f}x]"
            for t in r["impostor_accepted"]))
    if r["failures"]:
        print(f"template failures: {r['failures']}")
    print(f"template sizes (frames): {[d['enrolled'] for d in r['enrolment']]}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("datasets", nargs="+", metavar="dataset",
                    help="<enrol-dataset> <probe-dataset>; with --in-session or --self-test one dataset")
    mode = ap.add_mutually_exclusive_group()
    mode.add_argument("--in-session", action="store_true",
                      help="evaluate.py's two-fold in-session protocol (kit scorer), with per-press levels")
    mode.add_argument("--self-test", action="store_true",
                      help="check score_xs-<flavour> == score-<flavour> when both baselines are the same")
    ap.add_argument("--bin-dir", default=HERE, help="where the score-* / score_xs-* binaries live (default: here)")
    ap.add_argument("--flavour", action="append", choices=FLAVOURS, help="only this flavour (repeatable)")
    ap.add_argument("--fingers", default=None, help="comma-separated subset of the finger labels")
    ap.add_argument("--threshold", type=int, default=ev.THRESHOLD, help=f"accept threshold (default {ev.THRESHOLD})")
    ap.add_argument("--jobs", type=int, default=1,
                    help="scorer processes run at once (one per template; default 1)")
    ap.add_argument("--out", default=None,
                    help="results file (default: <probe-dataset>/results-cross.json, "
                         "<dataset>/results-in-session-levels.json with --in-session)")
    ap.add_argument("--note", default=None,
                    help="free-text note, goes verbatim into the shareable results file (no personal data)")
    args = ap.parse_args()

    single = args.in_session or args.self_test
    if len(args.datasets) != (1 if single else 2):
        ap.error("--in-session / --self-test take one dataset" if single
                 else "give two datasets: <enrol-dataset> <probe-dataset>")
    only = [x.strip() for x in args.fingers.split(",") if x.strip()] if args.fingers else None
    flavours = args.flavour or FLAVOURS
    bin_dir = os.path.abspath(args.bin_dir)
    kinds = ["score", "score_xs"] if args.self_test else (["score"] if args.in_session else ["score_xs"])
    for fl in flavours:
        for k in kinds:
            b = os.path.join(bin_dir, f"{k}-{fl}")
            if not os.access(b, os.X_OK):
                raise SystemExit(f"{b} not found -- run `make` in {HERE} first")

    sets = []
    for path in args.datasets:
        man, fingers, presses, base, skipped, ddir = load(path, only)
        var = ev.frame_variances([x for f in fingers for p in presses[f] for x in p])
        sets.append((man, fingers, presses, base, skipped, var, ddir))

    if args.self_test:
        ok = True
        for fl in flavours:
            r = self_test(fl, bin_dir, sets[0])
            ok &= r["identical"]
            print(f"[{fl}] score_xs with one baseline twice vs score: "
                  f"{'IDENTICAL' if r['identical'] else 'DIFFERENT'} "
                  f"({r['scored_lines']} output lines, {r['probe_lines']} probe lines)")
        return 0 if ok else 1

    results = {}
    if args.in_session:
        D = sets[0]
        print(f"in-session (evaluate.py's two-fold protocol): {len(D[1])} fingers; threshold {args.threshold}")
        for fl in flavours:
            results[fl] = in_session_evaluate(fl, os.path.join(bin_dir, f"score-{fl}"), D, args.threshold,
                                              args.jobs)
            print_result(f"{fl} matcher, in session", results[fl])
        datasets = {"dataset": dataset_info(D[0], D[1], D[2], D[4])}
        protocol = {
            "sets": "in session",
            "folds": "two-fold per finger (first half enrol / second half test, then reversed), as evaluate.py",
            "enrol_frame": "first finger-on, SETTLE_FRAMES settle groups",
            "impostor_set": "every press of every other finger of the same session",
        }
        dest_default = os.path.join(D[6], "results-in-session-levels.json")
    else:
        A, B = sets
        templates = [f for f in A[1] if f in B[1]]
        missing = [f for f in A[1] if f not in B[1]]
        if missing:
            print(f"warning: no probe presses for {missing} in the probe session; no template built for them",
                  file=sys.stderr)
        if not templates:
            raise SystemExit("the two datasets have no finger label in common")
        print(f"cross-session: {len(templates)} templates (all presses of the enrolment session), probes = "
              f"{sum(len(B[2][f]) for f in B[1])} presses of the probe session; threshold {args.threshold}")
        for fl in flavours:
            results[fl] = cross_evaluate(fl, os.path.join(bin_dir, f"score_xs-{fl}"), A, B, args.threshold,
                                         args.jobs)
            print_result(f"{fl} matcher, across sessions", results[fl])
        datasets = {"enrol": dataset_info(A[0], A[1], A[2], A[4]),
                    "probe": dataset_info(B[0], B[1], B[2], B[4])}
        protocol = {
            "sets": "across sessions: enrol on one dataset, probe with another",
            "templates": "per finger, all presses of the enrolment session",
            "enrol_frame": "first finger-on, SETTLE_FRAMES settle groups",
            "flat_field": "every frame with its own session's baseline",
            "genuine_set": "every press of the same finger from the probe session",
            "impostor_set": "every press of every other finger from the probe session "
                            "(enrolment-session presses are never probes)",
        }
        dest_default = os.path.join(B[6], "results-cross.json")
    protocol.update({
        "gallery": "reloaded before every probe press (kit_version 4)",
        "accept_rule": "driver's rule (frr_confirmed / far_confirmed): two consecutive frames with raw "
                       "variance >= finger_on_var scoring >= threshold, the second raw-corroborated; "
                       "frr / far = any frame of the press >= threshold",
        "level": "per press, the highest threshold at which the driver's rule accepts it (level_x = "
                 "level / threshold; 0 = no positive threshold accepts it)",
        "threshold": args.threshold,
        "settle_frames": ev.SETTLE_FRAMES,
    })
    out = {
        "kit": "egis0576-accuracy", "kit_version": ev.KIT_VERSION,
        "tool": "cross.py", "cross_version": CROSS_VERSION,
        "evaluated": datetime.date.today().isoformat(),
        "note": args.note,
        "datasets": datasets,
        "protocol": protocol,
        "results": results,
    }
    if out["note"] is None:
        del out["note"]
    dest = os.path.abspath(args.out) if args.out else dest_default
    with open(dest, "w") as fh:
        json.dump(out, fh, indent=1)
    print(f"\n-> {dest}  (scores, counts, levels and rates only -- no frames, no paths)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
