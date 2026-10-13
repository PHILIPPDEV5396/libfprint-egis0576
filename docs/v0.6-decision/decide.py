#!/usr/bin/env python3
"""Decision rule: is the Gabor matcher (v4.1) good enough next to the vendor one?

    python3 decide.py results-A.json [results-B.json ...]

Reads results.json files written by tools/accuracy/evaluate.py (kit_version 5,
branch identity-gate, with the per-press lists genuine_press_confirmed /
impostor_press_confirmed and the build provenance object) and applies, to the
'gabor' flavour against the 'vendor' flavour:

  (a) per dataset: gabor impostor_presses_confirmed == 0 (the driver's rule:
      two consecutive finger-on frames + raw corroboration). Any > 0 -> FAIL.
  (c) per dataset: gabor impostor_max / threshold <= 0.67 (the strongest
      impostor press, the kit's any-frame score, no corroboration), with the
      threshold stored in the gabor result. Any > 0.67 -> FAIL.
  (b) pooled over all paired datasets, genuine presses paired by index within
      each dataset (index i is the same press for both flavours):
        b = presses vendor accepts and gabor rejects,
        c = presses gabor accepts and vendor rejects;
      one-sided exact McNemar p = P(X >= b | X ~ Bin(b + c, 1/2)), p = 1 when
      b + c == 0. p < 0.05 -> FAIL ("measurably worse than vendor"). With
      b + c < 5 no split can reach p < 0.05: (b) is then reported as
      underpowered (a PASS on (b) means no evidence of a deficit, not
      equivalence), together with the smallest b that would fail.

A dataset also FAILS when gabor has a fold failure and vendor has none
("v4.1 could not enrol/score finger X fold k"): a matcher that cannot enrol a
finger is worse, never neutral. Such a dataset cannot be paired and is left
out of (b); (a) and (c) are still reported on what it holds.

A file is INVALID -- reported loudly, excluded, and the verdict can then not
be PASS -- when it is unreadable, not strict JSON (NaN / Infinity are
rejected), or:
  * the configuration is not the pre-registered one: kit_version != 5,
    threshold != 5000 (vendor, gabor or protocol), protocol enrol_frame not
    'first finger-on' (the driver's enrolment), a flavour or a per-press
    list missing, a count that is a boolean or disagrees with its list, a
    press score outside a C int, or a press confirmed with a score below
    the threshold;
  * the build provenance does not show v4.1 as the Makefile builds it:
    provenance missing or no kit commit ("please run from a git checkout"),
    driver tree != 2bafaf3's, kit tree not one of the reviewed kits
    (KIT_TREES), driver/ or tools/accuracy/ modified, a vendor
    or gabor scorer not up to date, its build info missing, not describing
    the binary, or not the default build (extra defines, other flags);
  * vendor has a fold failure (nothing to pair with), or the two flavours do
    not cover the same presses;
  * it is a second copy of a dataset already given.

Verdict, in this order:
  FAIL (<reasons>)  any dataset fails (a), (c) or the fold rule -- no further
                    data can undo that, so it stands even if a file is INVALID;
  INCOMPLETE (n invalid file(s); ask for a rerun)
                    any file INVALID -- never PASS while an input is excluded
                    (and a pooled (b) failure is not final while one is);
  NO DATA           no file given, or decide.py itself failed;
  FAIL / PASS       on (b) otherwise.
The last line of output is exactly one of
    VERDICT: PASS
    VERDICT: FAIL (<reasons>)
    VERDICT: INCOMPLETE (<n> invalid file(s); ask for a rerun)
    VERDICT: NO DATA
Exit status 0 PASS, 1 FAIL, 2 INCOMPLETE / NO DATA. Stdlib only; reads
nothing but the results.json files named on the command line (scores,
counts, booleans, hashes) and prints only their base names.
"""
import hashlib
import json
import math
import os
import sys
from fractions import Fraction

LEVEL_MAX = Fraction(67, 100)   # (c): gabor impostor_max / threshold must not exceed this (exact)
ALPHA = 0.05                    # (b): one-sided exact McNemar significance level
NEED = ("vendor", "gabor")
KIT_VERSION = 5
THRESHOLD = 5000                # EGIS_THRESHOLD, the driver's accept point
ENROL_FRAME = "first finger-on"  # evaluate.py's protocol.enrol_frame without --best-frame
# git rev-parse 2bafaf3:driver -- the frozen v4.1 driver sources. evaluate.py
# records git rev-parse HEAD:driver; any checkout whose driver/ tree is this one
# (and clean) compiles exactly v4.1, whatever commit it is on.
DRIVER_TREE_V41 = "824b6718ded9e937481cd0d862daec232da59d4e"
# git rev-parse <commit>:tools/accuracy of the reviewed kits. The kit is part
# of the instrument (score.c reads and flat-fields the frames, evaluate.py
# applies the driver's rule), and `clean` alone does not pin it: a local commit
# of a modified kit is clean. A later kit commit is INVALID here until its
# tree is reviewed and added. daa9233's Makefile can call a stale object up to
# date after a driver header changed and changed back (fixed in 2fae97b); it is
# listed because its only results (S5, S4, 2026-10-14) came from from-scratch
# builds whose seven binaries are byte-identical to 254b56e's.
KIT_TREES = {
    "a971481a034f9210e1b708c3d400267aba539504": "daa9233",
    "e9d006c33f229b965933235faacb7cac578ef062": "254b56e",
}
# build-info as the kit's Makefile (identity-gate, daa9233 .. 254b56e) writes
# it for a plain `make`: no extra defines, the driver directory of the
# checkout, the shipped flags. The compiler (cc) is recorded but not pinned.
_SCORE_CFLAGS = "-std=gnu99 -O3 -fno-strict-aliasing -D_GNU_SOURCE -Idriver/egis0576 -Wall"
EXPECTED_BUILD = {
    "vendor": {
        "flavour": "vendor", "extra_defines": "", "driver_dir": "driver/egis0576",
        "cflags": "-std=gnu99 -O3 -fno-strict-aliasing -D_GNU_SOURCE -Idriver/egis0576 -w "
                  "-fno-stack-protector -fpermissive -fwrapv -Wno-int-conversion "
                  "-Wno-incompatible-pointer-types -Wno-implicit-function-declaration",
        "score_cflags": _SCORE_CFLAGS,
    },
    "gabor": {
        "flavour": "gabor", "extra_defines": "", "driver_dir": "driver/egis0576",
        "cflags": "-std=gnu99 -O3 -fno-strict-aliasing -D_GNU_SOURCE -Idriver/egis0576 -Wall "
                  "-std=c99 -Idriver/egis0576/tsteppy -Idriver/egis0576/gabor",
        "score_cflags": _SCORE_CFLAGS,
    },
}
MAX_BYTES = 16 << 20            # a results.json is ~40 kB
INT_MIN, INT_MAX = -2 ** 31, 2 ** 31 - 1   # press scores are C ints


def is_int(x):
    return isinstance(x, int) and not isinstance(x, bool)


def short(x, n=80):
    """repr of a value taken from an input file, bounded (control characters escaped)."""
    s = repr(x)
    return s if len(s) <= n else s[:n - 3] + "..."


def mcnemar_one_sided(b, c):
    """P(X >= b) for X ~ Bin(b + c, 1/2), exact (rational arithmetic)."""
    n = b + c
    if n == 0:
        return 1.0
    tail = sum(math.comb(n, k) for k in range(b, n + 1))
    return float(Fraction(tail, 2 ** n))


def min_failing_b(n):
    """Smallest b with p(b, n - b) < ALPHA, or None when no split of n fails."""
    for b in range(n + 1):
        if mcnemar_one_sided(b, n - b) < ALPHA:
            return b
    return None


def pct(k, n):
    return f"{100.0 * k / n:.1f} %" if n else "n/a"


def load_strict_json(raw):
    """json.loads that refuses NaN, Infinity and numbers that overflow to them."""
    def bad_constant(s):
        raise ValueError(f"non-finite number {s}")

    def finite_float(s):
        v = float(s)
        if not math.isfinite(v):
            raise ValueError(f"non-finite number {s[:20]}")
        return v
    return json.loads(raw, parse_constant=bad_constant, parse_float=finite_float)


def bool_list(r, key):
    lst = r.get(key)
    return lst if isinstance(lst, list) and all(isinstance(x, bool) for x in lst) else None


def int_list(r, key):
    lst = r.get(key)
    return lst if isinstance(lst, list) and all(is_int(x) for x in lst) else None


def check_flavour(fl, r):
    """Every field decide.py reads, typed and cross-checked. Returns problems."""
    P = []
    if r.get("flavour") != fl:
        P.append(f"{fl}: 'flavour' field is {short(r.get('flavour'))}")
    thr = r.get("threshold")
    if not (is_int(thr) and thr == THRESHOLD):
        P.append(f"{fl}: threshold is {short(thr)}, must be {THRESHOLD}")
    for key, nkey, skey, sckey in (
            ("genuine_press_confirmed", "n_genuine", "genuine_presses_confirmed", "genuine_press_scores"),
            ("impostor_press_confirmed", "n_impostor", "impostor_presses_confirmed", "impostor_press_scores")):
        n = r.get(nkey)
        if not is_int(n) or n < 1:
            P.append(f"{fl}: '{nkey}' is {short(n)} (an integer >= 1 is needed"
                     + ("; one finger only has no impostor set, so (a) and (c) cannot be checked"
                        if nkey == "n_impostor" else "") + ")")
            n = None
        lst = r.get(key)
        if not isinstance(lst, list):
            P.append(f"{fl}: '{key}' missing (evaluate.py older than the per-press lists, 97d5ca5?)")
        elif bool_list(r, key) is None:
            P.append(f"{fl}: '{key}' holds non-boolean entries")
        else:
            if n is not None and len(lst) != n:
                P.append(f"{fl}: len({key}) = {len(lst)} != {nkey} = {n}")
            s = r.get(skey)
            if not is_int(s):
                P.append(f"{fl}: '{skey}' is {short(s)}, not an integer")
            elif sum(lst) != s:
                P.append(f"{fl}: sum({key}) = {sum(lst)} != {skey} = {s}")
        sc = int_list(r, sckey)
        if sc is None:
            P.append(f"{fl}: '{sckey}' missing or not a list of integers")
        elif n is not None and len(sc) != n:
            P.append(f"{fl}: len({sckey}) = {len(sc)} != {nkey} = {n}")
        elif sckey == "impostor_press_scores" and sc:
            imax = r.get("impostor_max")
            if not is_int(imax):
                P.append(f"{fl}: impostor_max is {short(imax)}, not an integer")
            elif imax != max(sc):
                P.append(f"{fl}: impostor_max {imax} != max(impostor_press_scores) {max(sc)}")
        if sc is not None and any(not (INT_MIN <= x <= INT_MAX) for x in sc):
            P.append(f"{fl}: '{sckey}' holds a value outside a C int (score.c prints C ints)")
        conf = bool_list(r, key)
        if sc is not None and conf is not None and len(conf) == len(sc) \
                and any(c and s < THRESHOLD for c, s in zip(conf, sc)):
            # the driver's rule needs two frames >= the threshold, so a confirmed
            # press has a press score (its best frame) >= the threshold
            P.append(f"{fl}: a press confirmed in '{key}' has a score below {THRESHOLD} "
                     f"in '{sckey}' (inconsistent)")
    if not is_int(r.get("frames_scored")):
        P.append(f"{fl}: frames_scored is {short(r.get('frames_scored'))}, not an integer")
    return P


def describe_failures(fails):
    out = []
    for f in fails:
        if isinstance(f, dict):
            out.append(f"finger {short(f.get('finger'), 30)} fold {short(f.get('fold'), 10)}"
                       + (f" ({short(f.get('reason'), 60)})" if f.get("reason") is not None else ""))
        else:
            out.append(short(f, 40))
    return out


def check_provenance(p):
    """The build provenance must show v4.1 as the kit's Makefile builds it."""
    missing = "provenance missing: please run from a git checkout"
    if not isinstance(p, dict):
        return [missing + " (no 'provenance' object: evaluate.py older than daa9233?)"]
    kc = p.get("kit_commit")
    if not (isinstance(kc, str) and len(kc) in (40, 64) and all(ch in "0123456789abcdef" for ch in kc)):
        return [missing + f" (kit_commit is {short(kc)})"]
    P = []
    if p.get("driver_tree") != DRIVER_TREE_V41:
        P.append(f"provenance: driver tree {short(p.get('driver_tree'))} is not v4.1's "
                 f"(2bafaf3:driver = {DRIVER_TREE_V41}): this file measured other matcher code")
    kt = p.get("kit_tree")
    if not (isinstance(kt, str) and kt in KIT_TREES):
        P.append(f"provenance: kit tree {short(kt)} is not a reviewed kit "
                 f"({', '.join(KIT_TREES.values())} on identity-gate): other or locally committed "
                 "kit code (score.c, evaluate.py, Makefile) measured this file")
    if p.get("clean") is not True:
        P.append(f"provenance: clean is {short(p.get('clean'))} -- driver/ or tools/accuracy/ differs "
                 "from the commit (modified or untracked files)")
    sc = p.get("scorers")
    if not isinstance(sc, dict):
        P.append("provenance: no 'scorers' object")
        return P
    for fl in NEED:
        s = sc.get(fl)
        if not isinstance(s, dict):
            P.append(f"provenance: scorer {fl} not recorded")
            continue
        for key, what in (("in_kit_dir", "is not the kit's own binary (--bin-dir)"),
                          ("up_to_date", "is not up to date (make -q: the binary is not what make builds now)"),
                          ("build_info_matches_binary", "is not the binary its build-info-%s.txt describes" % fl)):
            if s.get(key) is not True:
                P.append(f"provenance: score-{fl} {what} ({key} = {short(s.get(key))})")
        bi = s.get("build_info")
        if not isinstance(bi, dict):
            P.append(f"provenance: score-{fl} has no build info")
            continue
        for key, want in EXPECTED_BUILD[fl].items():
            if bi.get(key) != want:
                P.append(f"provenance: score-{fl} build {key} is {short(bi.get(key), 160)}, "
                         f"the default build has {want!r}")
    return P


def check_file(raw):
    """(invalid reasons, data, gabor fold failures). The last is non-empty only
    for a file that is otherwise valid and in which only gabor failed folds."""
    try:
        d = load_strict_json(raw)
    except (ValueError, UnicodeDecodeError, RecursionError) as e:
        msg = str(e) if isinstance(e, ValueError) and str(e).startswith("non-finite") else type(e).__name__
        return [f"not valid JSON ({msg})"], None, []
    if not isinstance(d, dict):
        return ["top level is not an object"], None, []
    P = []
    kv = d.get("kit_version")
    if not (is_int(kv) and kv == KIT_VERSION):
        P.append(f"kit_version is {short(kv)}, must be {KIT_VERSION}")
    proto = d.get("protocol")
    if not isinstance(proto, dict):
        P.append("no 'protocol' object")
    else:
        if not (is_int(proto.get("threshold")) and proto.get("threshold") == THRESHOLD):
            P.append(f"protocol threshold is {short(proto.get('threshold'))}, must be {THRESHOLD}")
        if proto.get("enrol_frame") != ENROL_FRAME:
            P.append(f"protocol enrol_frame is {short(proto.get('enrol_frame'))}, must be {ENROL_FRAME!r} "
                     "(the driver's enrolment; --best-frame runs do not count)")
    P += check_provenance(d.get("provenance"))
    res = d.get("results")
    if not isinstance(res, dict):
        return P + ["no 'results' object"], d, []
    flv = {}
    for fl in NEED:
        if isinstance(res.get(fl), dict):
            flv[fl] = res[fl]
        else:
            P.append(f"flavour '{fl}' missing (evaluate.py --flavour gabor --flavour vendor)")
    if len(flv) < len(NEED):
        return P, d, []
    v, g = flv["vendor"], flv["gabor"]
    vf, gf = v.get("failures"), g.get("failures")
    if not isinstance(vf, list) or not isinstance(gf, list):
        P.append("'failures' missing or not a list")
        return P, d, []
    P += check_flavour("vendor", v)
    if vf:
        P.append(f"vendor: {len(vf)} fold failure(s) [{'; '.join(describe_failures(vf))}] -- the vendor "
                 "matcher could not score that fold, so gabor's presses there have nothing to be paired with"
                 + (f"; gabor also failed {len(gf)} fold(s) [{'; '.join(describe_failures(gf))}]" if gf else ""))
        P += check_flavour("gabor", g) if g.get("n_genuine") else []
        return P, d, []
    if gf:
        # only gabor failed: the dataset fails (main). Its remaining lists are
        # still checked for consistency when there are any.
        if g.get("n_genuine") or g.get("n_impostor"):
            P += check_flavour("gabor", g)
        elif g.get("flavour") != "gabor":
            P.append(f"gabor: 'flavour' field is {short(g.get('flavour'))}")
        return P, d, describe_failures(gf)
    P += check_flavour("gabor", g)
    if not P:
        # the same presses for both: index i is the same press
        for key in ("n_genuine", "n_impostor", "frames_scored"):
            if v.get(key) != g.get(key):
                P.append(f"{key} differs: vendor {v.get(key)}, gabor {g.get(key)} (not the same presses)")
    return P, d, []


def main(argv):
    try:
        sys.stdout.reconfigure(errors="backslashreplace")
        sys.stderr.reconfigure(errors="backslashreplace")
    except (AttributeError, ValueError):
        pass
    if any(a in ("-h", "--help") for a in argv):
        print(__doc__)
        print("VERDICT: NO DATA")
        return 2
    opts = [a for a in argv if a.startswith("-")]
    if opts:
        print(f"decide.py: unknown option(s) {', '.join(short(o, 30) for o in opts)} (see --help)")
        print("VERDICT: NO DATA")
        return 2
    print("decide.py: gabor (matcher v4.1) vs vendor, the driver's rule, press by press")
    print(f"criteria: (a) gabor impostor presses confirmed == 0 per dataset; "
          f"(c) gabor impostor_max / threshold <= {float(LEVEL_MAX):.2f} per dataset; "
          f"(b) pooled one-sided exact McNemar p >= {ALPHA} (b = vendor accepts & gabor rejects, "
          "c = gabor accepts & vendor rejects); a gabor-only fold failure fails its dataset")
    print(f"required: kit_version {KIT_VERSION}, threshold {THRESHOLD}, enrol_frame {ENROL_FRAME!r}, "
          f"driver tree {DRIVER_TREE_V41[:12]} (2bafaf3, v4.1), a reviewed kit "
          f"({', '.join(KIT_TREES.values())}), clean tree, default up-to-date builds")
    if not argv:
        print("\nno results file given")
        print("VERDICT: NO DATA")
        return 2

    paired, failed_ds, invalid = [], [], []     # paired: (label, v, g); failed_ds: (label, v, g, folds)
    seen_bytes, seen_dataset = {}, {}
    for i, name in enumerate(argv, 1):
        base = os.path.basename(os.path.normpath(name))          # never the directory part
        label = f"#{i} " + "".join(ch if ch.isprintable() else "?" for ch in base)[:80]
        print(f"\n== {label}")
        d, gfails = None, []
        try:
            size = os.path.getsize(name)
            if size > MAX_BYTES:
                raise ValueError(f"{size} bytes")
            with open(name, "rb") as fh:
                raw = fh.read(MAX_BYTES + 1)
        except OSError as e:
            probs = [f"cannot read ({e.strerror or type(e).__name__})"]
        except ValueError as e:
            probs = [f"too large ({e}; a results.json is ~40 kB)"]
        else:
            probs, d, gfails = check_file(raw)
            h = hashlib.sha256(raw).hexdigest()
            if h in seen_bytes:
                probs = probs + [f"byte-identical to {seen_bytes[h]}"]
            seen_bytes.setdefault(h, label)
            if not probs:
                # The vendor scorer is deterministic: the same dataset gives the
                # same vendor score lists, so two such files are one dataset twice.
                v = d["results"]["vendor"]
                key = json.dumps([v["genuine_press_scores"], v["impostor_press_scores"]])
                if key in seen_dataset:
                    probs = [f"same dataset as {seen_dataset[key]} (identical vendor score lists)"]
                else:
                    seen_dataset[key] = label
        if isinstance(d, dict):
            ds = d.get("dataset") if isinstance(d.get("dataset"), dict) else {}
            pv = d.get("provenance") if isinstance(d.get("provenance"), dict) else {}
            print(f"   kit_version {short(d.get('kit_version', 'not recorded'))}, evaluated "
                  f"{short(d.get('evaluated'), 20)}, captured {short(ds.get('captured'), 20)}, "
                  f"fingers {short(ds.get('fingers'), 120)}, presses {short(ds.get('n_presses'), 12)}")
            print(f"   kit commit {short(pv.get('kit_commit'), 44)}, driver tree {short(pv.get('driver_tree'), 44)}, "
                  f"clean {short(pv.get('clean'), 8)}")
        if probs:
            print("   !!! INVALID -- EXCLUDED FROM THE DECISION !!!")
            for p in probs:
                print(f"   !!!   {p}")
            if gfails:
                print(f"   !!!   (gabor also failed {len(gfails)} fold(s): {'; '.join(gfails)})")
            print("   verdict for this file: INVALID", flush=True)
            print(f"decide.py: {label}: INVALID, excluded: {'; '.join(probs)}", file=sys.stderr, flush=True)
            invalid.append(label)
            continue
        v, g = d["results"]["vendor"], d["results"]["gabor"]
        if gfails:
            print(f"   gabor fold failure(s), vendor none: {'; '.join(gfails)} -> this dataset FAILS")
            failed_ds.append((label, v, g, gfails))
        else:
            paired.append((label, v, g))

    fails = []
    if paired or failed_ds:
        print("\nper dataset (genuine: rejected under the driver's rule; b/c: discordant pairs)")
    for label, v, g, gfails in failed_ds:
        print(f"-- {label}")
        for f in gfails:
            print(f"   v4.1 could not enrol/score {f} -> FAIL")
        fails.append(f"{label}: v4.1 could not enrol/score {'; '.join(gfails)}")
        if g.get("n_impostor"):
            ia, ni, imax, thr = (g["impostor_presses_confirmed"], g["n_impostor"], g["impostor_max"],
                                 g["threshold"])
            level = Fraction(imax) / Fraction(thr)
            print(f"   (a) on the folds gabor did score: {ia}/{ni} impostor presses confirmed -> "
                  f"{'ok' if ia == 0 else 'FAIL'}")
            print(f"   (c) on the folds gabor did score: impostor_max {imax} / {thr} = {float(level):.3f} -> "
                  f"{'ok' if level <= LEVEL_MAX else 'FAIL'}")
            if ia:
                fails.append(f"a: {label}: {ia} impostor press(es) accepted by gabor")
            if level > LEVEL_MAX:
                fails.append(f"c: {label}: impostor level {imax}/{thr} = {float(level):.3f} > {float(LEVEL_MAX):.2f}")
        print("   not paired: left out of (b)")
    B = C = NG = RV = RG = 0
    for label, v, g in paired:
        gv, gg = v["genuine_press_confirmed"], g["genuine_press_confirmed"]
        n = len(gg)
        b = sum(1 for x, y in zip(gv, gg) if x and not y)
        c = sum(1 for x, y in zip(gv, gg) if y and not x)
        rv, rg = n - sum(gv), n - sum(gg)
        B, C, NG, RV, RG = B + b, C + c, NG + n, RV + rv, RG + rg
        ni, ia, thr, imax = len(g["impostor_press_confirmed"]), g["impostor_presses_confirmed"], \
            g["threshold"], g["impostor_max"]
        level = Fraction(imax) / Fraction(thr)    # exact: no rounding at the limit
        a_ok, c_ok = ia == 0, level <= LEVEL_MAX
        print(f"-- {label}")
        print(f"   genuine n={n}: rejected vendor {rv} ({pct(rv, n)}), gabor {rg} ({pct(rg, n)}); "
              f"b={b} c={c}  [informational]")
        print(f"   (a) gabor impostor presses confirmed: {ia}/{ni} -> {'ok' if a_ok else 'FAIL'}"
              f"   (vendor: {v['impostor_presses_confirmed']}/{len(v['impostor_press_confirmed'])})")
        print(f"   (c) gabor impostor_max {imax} / threshold {thr} = {float(level):.3f}"
              f" (limit {float(LEVEL_MAX):.2f}) -> {'ok' if c_ok else 'FAIL'}")
        if not a_ok:
            fails.append(f"a: {label}: {ia} impostor press(es) accepted by gabor")
        if not c_ok:
            fails.append(f"c: {label}: impostor level {imax}/{thr} = {float(level):.3f} > {float(LEVEL_MAX):.2f}")

    b_fail = False
    if paired:
        p = mcnemar_one_sided(B, C)
        b_fail = p < ALPHA
        n = B + C
        print(f"\npooled over {len(paired)} paired dataset(s)"
              + (f" ({len(failed_ds)} failed dataset(s) not paired)" if failed_ds else "")
              + (f" ({len(invalid)} INVALID file(s) excluded: {', '.join(invalid)})" if invalid else ""))
        print(f"   genuine n={NG}: rejected vendor {RV} ({pct(RV, NG)}), gabor {RG} ({pct(RG, NG)})  [informational]")
        print(f"   (b) b={B} (vendor accepts, gabor rejects), c={C} (gabor accepts, vendor rejects); "
              f"one-sided exact McNemar p = P(X >= {B} | Bin({n}, 0.5)) = {p:.4g} "
              f"-> {'FAIL' if b_fail else 'ok'}" + (" (not final: invalid files excluded)" if invalid else ""))
        mb = min_failing_b(n)
        if mb is None:
            print(f"   (b) with b+c={n} no split can FAIL (even b={n}, c=0 gives p = {mcnemar_one_sided(n, 0):.4g})")
        else:
            print(f"   (b) with b+c={n}, FAIL needs b >= {mb} (p = {mcnemar_one_sided(mb, n - mb):.4g}); "
                  f"observed b={B}")
        if n < 5:
            print(f"   (b) underpowered: b+c={n} < 5, the test cannot reject at alpha 0.05 — PASS on (b) "
                  "here means no evidence of a deficit, not equivalence")
        if b_fail:
            fails.append(f"b: measurably worse than vendor, b={B} c={C} p={p:.4g}")

    definitive = [f for f in fails if not f.startswith("b:")]
    if definitive:
        note = f"; {len(invalid)} invalid file(s) also excluded" if invalid else ""
        print(f"VERDICT: FAIL ({'; '.join(fails)}{note})")
        return 1
    if invalid:
        print(f"\n{len(invalid)} of {len(argv)} file(s) INVALID ({', '.join(invalid)}): "
              "no PASS while an input is excluded")
        print(f"VERDICT: INCOMPLETE ({len(invalid)} invalid file(s); ask for a rerun)")
        return 2
    if not paired:
        print("VERDICT: NO DATA")
        return 2
    if fails:
        print(f"VERDICT: FAIL ({'; '.join(fails)})")
        return 1
    print("VERDICT: PASS")
    return 0


def run(argv):
    try:
        return main(argv)
    except BaseException as e:      # noqa: BLE001 -- never a traceback, never exit 1 on an error
        try:
            print(f"\ndecide.py: internal error ({type(e).__name__}); no decision was made", flush=True)
            print("VERDICT: NO DATA", flush=True)
        except BaseException:
            pass
        return 2


if __name__ == "__main__":
    sys.exit(run(sys.argv[1:]))
