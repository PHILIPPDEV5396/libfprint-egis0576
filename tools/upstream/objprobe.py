#!/usr/bin/env python3
"""Is this object usable as the "finger" of the umockdev driver test?

The recording for tests/egis0576/ contains every frame the sensor delivered
and is committed publicly, so it is made with textured non-finger objects
(tools/upstream/README.md). Finding one that works is trial and error: this
is the fast test -- two presses, about twenty seconds, no enrolment.

It brings the sensor up exactly as the driver does (reset, init replay,
exposure calibration, 8-frame flat-field baseline), captures one press, then
a second one, and prints the matcher's coverage per frame plus the best NCC
between the two presses -- the number the accept threshold is compared
against. An object is usable when both presses reach the enrolment coverage
gate and reproduce each other above the threshold.

Frames are written flat-fielded to --out (0600, default a private directory
under $XDG_RUNTIME_DIR or /var/tmp) and nothing else; they are the object's
texture, not anyone's fingerprint, but they stay local anyway.

fprintd is masked and stopped for the run and restored afterwards; the
helpers and the direct-USB path come from tools/accuracy/ next door.

    tools/upstream/objprobe.py [--out DIR]
"""
import argparse
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
KIT = os.path.join(REPO, "tools", "accuracy")
DRV = os.path.join(REPO, "driver", "egis0576")

sys.path.insert(0, KIT)
import egis_eh576 as eh          # noqa: E402
import capture as cap            # noqa: E402
import numpy as np               # noqa: E402

MAX_FRAMES = 12


def build_checker(out):
    """Compile objcheck.c against the shipped matcher, once per --out."""
    exe = os.path.join(out, "objcheck")
    src = os.path.join(HERE, "objcheck.c")
    if os.path.exists(exe) and os.path.getmtime(exe) > os.path.getmtime(src):
        return exe
    cmd = [os.environ.get("CC", "cc"), "-O2", "-o", exe, src,
           os.path.join(DRV, "gabor", "egis_match_gabor.c"),
           "-I" + os.path.join(DRV, "gabor"), "-I" + os.path.join(DRV, "tsteppy"), "-lm"]
    subprocess.run(cmd, check=True)
    return exe


def flat(raw, base):
    m = int(base.mean())
    return np.clip(raw.astype(int) - base.astype(int) + m, 0, 255).astype(np.uint8)


def one_press(d, base, out, label):
    print(f"\n>>> press the object now ({label}) -- firmly, flat, hold about a second")
    frames = cap.wait_press(d, np, MAX_FRAMES, timeout=60)
    if not frames:
        print("    nothing seen (the variance never reached 250): the sensor does not feel this object")
        return None
    print(f"    {len(frames)} frames, variance "
          f"{min(f.var() for f in frames):.0f}..{max(f.var() for f in frames):.0f}")
    print("    lift it again ...")
    cap.wait_lift(d, np)
    path = os.path.join(out, f"objprobe-{label}.raw")
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
    for f in frames:
        os.write(fd, flat(f, base).tobytes())
    os.close(fd)
    return path


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out", default=os.path.join(os.environ.get("XDG_RUNTIME_DIR", "/var/tmp"),
                                                  "egis0576-objprobe"),
                    help="where the flat-fielded frames and the checker go (mode 0700)")
    a = ap.parse_args()
    os.makedirs(a.out, mode=0o700, exist_ok=True)
    os.chmod(a.out, 0o700)
    checker = build_checker(a.out)
    recs = eh.load_records(eh.init_header_path(REPO))

    print("[1] stopping fprintd (it owns the sensor otherwise)")
    cap.fprintd_mask()
    cap.fprintd_stop()
    dev = None
    try:
        print("[2] resetting the sensor")
        cap.sensor_reset()
        print("[3] bring-up")
        dev, _ = eh.acquire()
        eh.run_init(dev, recs)
        cal = eh.calibrate(dev)
        print(f"    exposure register 0x{cal['baked']:02x} -> 0x{cal['calibrated']:02x}")
        print("[4] baseline: do NOT touch the sensor for a few seconds ...")
        base = cap.take_baseline(dev, np)
        if base is None:
            print("    could not collect 8 clean no-finger frames; clean the sensor and retry")
            return 1
        p1 = one_press(dev, base, a.out, "1")
        p2 = one_press(dev, base, a.out, "2") if p1 else None
    finally:
        if dev is not None:
            try:
                eh.release(dev)
            except Exception:
                pass
        cap.fprintd_unmask()
        cap.fprintd_start()
    if not (p1 and p2):
        return 1
    print()
    return subprocess.run([checker, p1, p2]).returncode


if __name__ == "__main__":
    sys.exit(main())
