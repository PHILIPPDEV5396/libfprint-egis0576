#!/usr/bin/env python3
"""Capture a raw-frame fingerprint dataset from an EgisTec EH576 for the
matcher accuracy evaluation (tools/accuracy/README.md).

What it records, mirroring drivers/egis0576.c exactly:
  * the driver's per-open exposure calibration (binary search over reg 0x0f
    until the no-finger frame mean is closest to 0x58) -- so a unit whose
    operating point differs from the reference unit's does not capture
    saturated frames; the baked / calibrated register values and the final
    no-finger mean go into manifest.json;
  * flat-field baseline = per-pixel mean of 8 no-finger frames
    (raw variance < 210, EGIS0576_BASELINE_MAX_VAR), truncated to uint8;
  * finger-on at raw frame variance >= 250, finger-off below 215 (hysteresis);
  * EVERY frame while the finger is down (up to --max-frames), because the
    driver's verify loop scores every one of them; the FIRST finger-on frame
    is what enrolment uses.
Frames are stored RAW (.npy, uint8, 3990 bytes each); flat-fielding and
preprocessing happen offline in score.c, identically for both matchers.

THE FRAMES ARE YOUR BIOMETRIC DATA. They are written to a directory only you
can read (mode 0700, under your $HOME by default) and never leave your
machine: evaluate.py reads them and writes a results.json of scores and rates
only. Do not post the frames anywhere.

Exit paths: Ctrl-C, SIGTERM, SIGHUP (closed terminal) and SIGQUIT (Ctrl-\\,
treated like Ctrl-C) all take the same path: the signal is honoured at the
end of the current USB SEQUENCE (a whole frame, the whole bring-up replay,
one calibration step -- never between the transfers of one; egis_eh576.py
module docstring), then the same cleanup runs -- release the sensor,
ForceReset it in a separate process, unmask and restart fprintd -- with
further signals ignored while that runs. On a healthy sensor that is a few ms
after the keypress; on a sensor that has stopped answering, a frame is up to
~2.6 s of read timeouts and the bring-up replay up to ~20 s; a bulk-OUT write
timeout (3 s) ends the sequence early, because the sensor is then already
unresponsive -- the same trade the driver makes for never aborting a transfer
in flight. One exception: a signal that arrives while the step [2] reset
child runs (sensor_reset()) is DROPPED, not deferred -- the parent is immune
for the child's whole lifetime; repeat it at the next prompt. The manifest is
rewritten after every press, so a partial dataset is always evaluable.

Requirements: python3, pyusb, numpy. Run it as `sudo python3 capture.py`:
root needs no sudo timestamp for the fprintd unmask/start at the end (a
timestamp expires after a few minutes, i.e. mid-capture), and the tool writes
into SUDO_USER's home and hands every file to that user as it is written. An
unprivileged run works too (udev rule, see README) and refreshes the sudo
timestamp after every accepted press.
"""
import argparse
import datetime
import json
import os
import pwd
import re
import signal
import stat
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import egis_eh576 as eh                                  # noqa: E402

FINGER_ON_VAR = 250.0        # EGIS0576_FINGER_ON_VAR
FINGER_OFF_VAR = 215.0       # EGIS0576_FINGER_OFF_VAR
BASELINE_MAX_VAR = 210.0     # EGIS0576_BASELINE_MAX_VAR
BASELINE_FRAMES = 8          # EGIS0576_BASELINE_FRAMES
DEFAULT_FINGERS = "R-thumb,R-index,L-index,R-middle,L-middle"
MANIFEST_VERSION = 2
RESET_CHILD_TIMEOUT = 300.0  # last resort only: the child bounds itself (~50 s worst case, ~2 s typical) and ignores signals; never SIGKILLed, see sensor_reset()
RESET_CMD = None             # override for tests; default: [python, egis_eh576.py, "reset"]


def say(msg=""):
    """print that survives a vanished terminal (SIGHUP path): a failed write
    to stdout must not turn the cleanup into a traceback."""
    try:
        print(msg, flush=True)
    except (OSError, ValueError):
        pass


# ------------------------------------------------------------- helpers ----

def target_home():
    """$HOME of the real user, even under sudo."""
    su = os.environ.get("SUDO_USER")
    if su and os.geteuid() == 0:
        try:
            return pwd.getpwnam(su).pw_dir, pwd.getpwnam(su)
        except KeyError:
            pass
    return os.path.expanduser("~"), None


def default_outdir():
    home, _ = target_home()
    stamp = datetime.datetime.now().strftime("%Y-%m-%d_%H%M")
    return os.path.join(home, "egis-accuracy", stamp)


def own(path, pw):
    """Hand a path to the real user right away when running under sudo, so a
    hard abort never leaves root-owned files the user cannot delete."""
    if pw is not None:
        try:
            os.chown(path, pw.pw_uid, pw.pw_gid)
        except OSError as e:
            say(f"  warning: chown {path}: {e}")


def make_private_dir(path, pw):
    """Create --out (and any missing ancestors) one level at a time, each
    0700 and handed to the real user right away. Returns the list of
    directories THIS call created -- the only ones chown_tree() may touch:
    a pre-existing parent (~/, ~/egis-accuracy from an earlier run) is never
    chowned or chmodded. A pre-existing --out is accepted only if it is empty
    (then it is chmodded/chowned only if that is actually needed); a non-empty
    one is refused, so the tool can never mix its frames into, or take
    ownership of, something that was already there."""
    path = os.path.abspath(path)
    missing = []
    p = path
    while not os.path.lexists(p):
        missing.append(p)
        parent = os.path.dirname(p)
        if parent == p:
            break
        p = parent
    if not os.path.isdir(p):
        raise SystemExit(f"{p} exists but is not a directory; choose another --out")
    if not missing:                                      # --out itself pre-exists
        if not os.path.isdir(path):
            raise SystemExit(f"{path} exists but is not a directory; choose another --out")
        if os.listdir(path):
            raise SystemExit(f"{path} already exists and is not empty; refusing to write a dataset into it "
                             "(choose another --out, or an empty directory)")
        st = os.stat(path)
        if stat.S_IMODE(st.st_mode) != 0o700:
            os.chmod(path, 0o700)
        if pw is not None and (st.st_uid, st.st_gid) != (pw.pw_uid, pw.pw_gid):
            own(path, pw)
        return []
    created = []
    for comp in reversed(missing):                       # first missing ancestor first
        try:
            os.mkdir(comp, 0o700)
        except FileExistsError:
            raise SystemExit(f"{comp} appeared while creating --out; choose another --out")
        os.chmod(comp, 0o700)                            # umask-proof
        own(comp, pw)
        created.append(comp)
    return created


def reject_bad_outdir(path):
    ap = os.path.realpath(path)                      # a symlink into /tmp is still /tmp
    for bad in ("/tmp", "/var/tmp", "/dev/shm"):
        if ap == bad or ap.startswith(bad + os.sep):
            raise SystemExit(f"refusing to write biometric frames under {bad}; use a directory under your $HOME")
    try:
        root = os.path.realpath(eh.find_repo_root())
        if ap == root or ap.startswith(root + os.sep):
            raise SystemExit("refusing to write biometric frames inside the repository checkout; "
                             "use a directory under your $HOME (the default)")
    except eh.SensorError:
        pass


def chown_tree(path, pw, created):
    """Final ownership pass under sudo: the directories make_private_dir()
    created and the files under --out (all written by this tool: --out was
    empty or created by it). Nothing above --out is touched."""
    if pw is None:
        return
    for d in created:
        own(d, pw)
    for dp, dns, fns in os.walk(path):
        for f in fns:
            own(os.path.join(dp, f), pw)


def safe_label(s):
    s = re.sub(r"[^A-Za-z0-9_.-]+", "_", s.strip())
    return s or "finger"


def sh(cmd):
    say(f"  $ {' '.join(cmd)}")
    try:
        return subprocess.run(cmd).returncode
    except OSError as e:                                 # sudo/systemctl missing
        say(f"  {cmd[0]}: {e}")
        return 127


def fprintd_mask():
    """Runtime mask: /run/systemd/system/fprintd.service -> /dev/null, gone at
    reboot, so a tool that died before unmasking cannot leave fprintd masked
    permanently. It stops D-Bus activation from bringing fprintd back during
    the capture (a lock screen, a login prompt, a `fprintd-list` would)."""
    if sh(["sudo", "systemctl", "mask", "--runtime", "fprintd"]) == 0:
        return True
    r = subprocess.run(["systemctl", "is-enabled", "fprintd"], capture_output=True, text=True)
    return r.stdout.strip() == "masked-runtime"          # left over from an earlier run: fine


def fprintd_unmask():
    return sh(["sudo", "systemctl", "unmask", "--runtime", "fprintd"]) == 0


def fprintd_stop():
    return sh(["sudo", "systemctl", "stop", "fprintd"]) == 0


def fprintd_start():
    sh(["sudo", "systemctl", "start", "fprintd"])
    r = subprocess.run(["systemctl", "is-active", "fprintd"], capture_output=True, text=True)
    say(f"  fprintd: {r.stdout.strip() or '?'} (inactive is normal too: it is D-Bus activated on demand)")


def sudo_refresh():
    """Keep the sudo timestamp alive during a long unprivileged capture so the
    cleanup's `sudo systemctl unmask/start` neither fails nor prompts. `-n`
    never prompts; a failure (no timestamp, sudo missing) is ignored -- the
    cleanup reports its own failure and the README has the manual commands."""
    if os.geteuid() == 0:
        return
    try:
        subprocess.run(["sudo", "-n", "-v"], stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                       stderr=subprocess.DEVNULL, timeout=10)
    except (OSError, subprocess.SubprocessError):
        pass


def sensor_reset(cmd=None):
    """ForceResetDevice in a SEPARATE process, then settle. The reset makes
    the device drop off the bus and re-enumerate (on the next transfer to it,
    which the child sends itself, then waits for the fresh device to answer:
    egis_eh576.force_reset); the issuing process's libusb
    device list and handles go stale (and a stale handle can still pass
    descriptor access). A throw-away process keeps that out of the capture
    process, which then opens the re-enumerated device with a fresh context.

    The child is NEVER killed: a SIGKILL while it sits in its ForceReset
    control transfer is an in-flight abort, which wedges the sensor until a
    board power cycle (docs/sensor-tuning.md section 4). subprocess.run() would
    do exactly that 0.25 s after a Ctrl-C in the parent, so this uses Popen +
    communicate() and, for the child's whole lifetime, ignores every
    eh.EXIT_SIGNALS member (SIGINT, SIGTERM, SIGHUP, SIGQUIT) in the parent.
    The child is immune on its own account as well: `egis_eh576.py reset` sets
    all of them to SIG_IGN itself and never installs the raising handler, so a
    terminal Ctrl-C (process-group SIGINT) reaching it directly does nothing.
    Any interruption that gets through to the parent anyway just resumes the
    wait. CONSEQUENCE: an exit signal that arrives while the child runs is
    DROPPED, not deferred -- a Ctrl-C during step [2] leaves no trace and the
    capture carries on; the user repeats it at the next prompt (deliberately
    no record-and-raise here: the parent stays immune while the child runs,
    and the cleanup's own step [6] reset is already on the way out). The
    child bounds itself: 40 x 0.25 s device
    search, one 2000 ms control transfer, then up to 24 short probes for the
    re-enumerated device (~50 s worst case, ~2 s typical). RESET_CHILD_TIMEOUT
    (300 s) is a last resort against a child that is truly hung: it gets ONE
    SIGTERM -- a no-op on the real child, which ignores it, so all that
    happens is that the parent keeps waiting for the child to exit on its own,
    which is intended -- and never SIGKILL. (Something that hangs a process
    inside a 2 s control transfer is a dead libusb/kernel path; a signal would
    not help there and an abort could only make it worse.)"""
    cmd = cmd or RESET_CMD or [sys.executable, os.path.join(HERE, "egis_eh576.py"), "reset"]
    sigs = eh.EXIT_SIGNALS
    saved = {sg: signal.getsignal(sg) for sg in sigs}
    for sg in sigs:
        signal.signal(sg, signal.SIG_IGN)
    try:
        p = subprocess.Popen(cmd, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE, text=True)
        deadline = time.monotonic() + RESET_CHILD_TIMEOUT
        nudged = False
        while True:
            try:
                out, err = p.communicate(timeout=max(0.5, deadline - time.monotonic()) if not nudged else None)
                break
            except subprocess.TimeoutExpired:            # communicate() does not kill on timeout
                if not nudged and time.monotonic() >= deadline:
                    say(f"  !! reset child still running after {RESET_CHILD_TIMEOUT:.0f} s: asking it to stop "
                        "(SIGTERM, honoured between transfers); waiting for it -- never killing it")
                    try:
                        p.send_signal(signal.SIGTERM)
                    except OSError:
                        pass
                    nudged = True
            except KeyboardInterrupt:                    # cannot normally happen (SIG_IGN): keep waiting
                say("  (interrupt ignored: waiting for the reset child to finish on its own)")
    finally:
        for sg, h in saved.items():
            signal.signal(sg, h)
    msg = (out + err).strip()
    say(f"  sensor reset: {msg or 'done'}")
    if p.returncode != 0:
        raise eh.SensorError(f"reset failed (exit {p.returncode})")
    time.sleep(1.0)                                      # the child returned on a fresh, answering device


# ------------------------------------------------------------- capture ----

def frame(d, np):
    f = eh.grab(d)
    return np.frombuffer(f, dtype=np.uint8).copy() if f else None


def wait_press(d, np, max_frames, timeout):
    """Wait for finger-on (var >= 250), then keep every frame until finger-off
    (var < 215) or max_frames. Same hysteresis as the driver."""
    t0 = time.monotonic()
    frames = []
    while time.monotonic() - t0 < timeout:
        f = frame(d, np)
        if f is None:
            continue
        if f.var() >= FINGER_ON_VAR:
            frames.append(f)
            while len(frames) < max_frames:
                g = frame(d, np)
                if g is None or g.var() < FINGER_OFF_VAR:
                    break
                frames.append(g)
            return frames
        time.sleep(0.08)
    return frames


def wait_lift(d, np, timeout=60):
    t0 = time.monotonic()
    while time.monotonic() - t0 < timeout:
        f = frame(d, np)
        if f is not None and f.var() < FINGER_OFF_VAR:
            return True
        time.sleep(0.12)
    return False


def take_baseline(d, np, timeout=45):
    """Per-pixel mean of 8 no-finger frames (var < 210), truncated to uint8 --
    drivers/egis0576.c baseline_feed()."""
    acc = np.zeros(eh.IMG, dtype=np.uint32)
    n = 0
    t0 = time.monotonic()
    rejected = 0
    while n < BASELINE_FRAMES and time.monotonic() - t0 < timeout:
        f = frame(d, np)
        if f is None:
            continue
        v = float(f.var())
        if v < BASELINE_MAX_VAR:
            acc += f
            n += 1
        else:
            rejected += 1
            if rejected in (5, 20, 60):
                say(f"    still seeing var {v:.0f} (need < {BASELINE_MAX_VAR:.0f}): is anything touching the sensor?")
        time.sleep(0.05)
    if n < BASELINE_FRAMES:
        return None
    return (acc // n).astype(np.uint8)


def save_frame(path, arr, np, pw):
    np.save(path, arr)
    os.chmod(path, 0o600)
    own(path, pw)


def write_manifest(out, manifest, pw, complete):
    manifest["complete"] = complete
    path = os.path.join(out, "manifest.json")
    tmp = path + ".tmp"
    with open(tmp, "w") as fh:
        json.dump(manifest, fh, indent=1)
    os.chmod(tmp, 0o600)
    own(tmp, pw)
    os.replace(tmp, path)


def capture_body(args, np, out, pw, recs, manifest, st):
    """Everything between 'stop fprintd' and 'last press'. Bookkeeping for
    the cleanup goes into `st` as it happens."""
    s = eh.records_summary(recs)
    if not args.no_fprintd:
        say("\n[1] masking (runtime) and stopping fprintd -- it owns the sensor otherwise")
        if not fprintd_mask():
            raise SystemExit("could not mask fprintd (sudo failed?); nothing was touched")
        st["masked"] = True
        if not fprintd_stop():
            raise SystemExit("could not stop fprintd (sudo failed?); the sensor was not touched")
        st["stopped"] = True
    st["touched"] = True                                 # from here on the sensor needs a reset at exit
    say("[2] resetting the sensor (separate process)")
    sensor_reset()
    say("[3] opening the sensor and replaying the driver's bring-up")
    d, tries = eh.acquire()
    st["dev"] = d
    say(f"    ready after {tries + 1} attempt(s)")
    ok = eh.run_init(d, recs, verbose=args.verbose)
    expected = s["commands"] - s["uploads"]              # the upload command's reply is never read (as in the driver)
    say(f"    {ok}/{expected} bring-up commands answered")
    if ok < expected:
        say("    !! some bring-up commands went unanswered; the capture goes on, but if the calibration below\n"
            "       does not converge, stop, run `egis_eh576.py reset` and start again")

    say("\n[4] EXPOSURE CALIBRATION + BASELINE: do NOT touch the sensor for a few seconds ...")
    cal = eh.calibrate(d)
    manifest["dc_c_baked"] = cal["baked"]
    manifest["dc_c_calibrated"] = cal["calibrated"]
    manifest["calibration_trace"] = cal["trace"]
    say(f"    reg 0x0f: baked 0x{cal['baked']:02x} -> calibrated 0x{cal['calibrated']:02x} "
        f"(target mean 0x{eh.EXPOSURE_TARGET:02x}; search {cal['trace']})")
    base = take_baseline(d, np)
    if base is None:
        raise SystemExit("could not collect 8 clean no-finger frames; clean the sensor and retry")
    save_frame(os.path.join(out, "baseline.npy"), base, np, pw)
    manifest["nofinger_mean"] = round(float(base.mean()), 2)
    say(f"    baseline from {BASELINE_FRAMES} no-finger frames, mean {float(base.mean()):.1f}, var {float(base.var()):.1f}")
    write_manifest(out, manifest, pw, False)

    fingers = manifest["fingers"]
    say(f"\n[5] CAPTURE: {len(fingers)} fingers x {args.presses} presses. Press naturally, as you\n"
        "    would to unlock; vary the placement a little between presses; lift fully in between.")
    for fi, name in enumerate(fingers):
        input(f"\n=== finger {fi + 1}/{len(fingers)}: {name} -- ready? [Enter] ")
        manifest["data"][name] = []
        p = 0
        misses = 0
        while p < args.presses:
            say(f"  [{name} {p + 1:2d}/{args.presses}] place finger ...")
            fs = wait_press(d, np, args.max_frames, args.wait)
            if not fs:
                misses += 1
                say("      nothing detected, try again" + (" (press a bit firmer / longer)" if misses > 1 else ""))
                if misses >= 5:
                    raise SystemExit("no finger detected five times in a row; aborting")
                continue
            misses = 0
            files = []
            for k, f in enumerate(fs):
                fn = f"{fi:02d}_{name}_p{p:02d}_k{k:02d}.npy"
                save_frame(os.path.join(out, fn), f, np, pw)
                files.append(fn)
            manifest["data"][name].append(files)
            write_manifest(out, manifest, pw, False)
            if not args.no_fprintd:
                sudo_refresh()                           # keep the cleanup's sudo alive (unprivileged run)
            say(f"      {len(fs):2d} frames, var {float(fs[0].var()):5.0f} (first) / "
                f"{max(float(x.var()) for x in fs):5.0f} (max)   -> lift")
            if not wait_lift(d, np):
                say("      finger still detected after 60 s; continuing anyway")
            time.sleep(0.35)
            p += 1
    st["completed"] = True


def cleanup_sensor(st):
    """Always runs, first, with every exit signal ignored: a second Ctrl-C, a
    SIGTERM or a vanished terminal must not skip releasing the sensor,
    resetting it and giving fprintd back.

    IDEMPOTENT: every step clears its `st` flag once it has been done (dev is
    cleared right after the release; touched / masked / stopped after the
    reset / unmask / start), so run_capture() can simply call it a second
    time if a signal slipped in between the exception leaving capture_body()
    and the SIG_IGN below (a microsecond window, but it exists on the
    SystemExit / SensorError path, where no signal has been seen yet). The
    retry re-does only the step that was interrupted, never one that
    completed."""
    retry = st.get("cleanup_started", False)         # first statement: a retry knows it is one
    st["cleanup_started"] = True
    for s in eh.EXIT_SIGNALS:
        signal.signal(s, signal.SIG_IGN)
    say("\n== cleaning up (Ctrl-C is ignored until this is done) ==" if not retry else
        "\n== cleanup interrupted at its start; continuing it (nothing is skipped) ==")
    try:
        d = st.get("dev")
        if d is not None:
            try:
                eh.release(d)
            except Exception as e:                       # noqa: BLE001
                say(f"  release failed: {e}")
            st["dev"] = None
        if st.get("touched"):
            say("[6] resetting the sensor (separate process)")
            try:
                sensor_reset()
            except Exception as e:                       # noqa: BLE001
                say(f"  reset failed: {e}")
            st["touched"] = False
    finally:
        if st.get("masked"):
            say("[7] unmasking fprintd")
            if not fprintd_unmask():
                say("  !! unmask failed: run `sudo systemctl unmask --runtime fprintd` yourself "
                    "(a reboot clears it too)")
            st["masked"] = False
        if st.get("stopped"):
            say("[8] starting fprintd again")
            fprintd_start()
            st["stopped"] = False


def run_capture(args, np):
    fingers = [safe_label(x) for x in args.fingers.split(",") if x.strip()]
    if len(set(fingers)) != len(fingers):
        raise SystemExit("finger labels must be unique")
    out = args.out
    reject_bad_outdir(out)
    home, pw = target_home()
    created = make_private_dir(out, pw)
    say(f"dataset directory: {out}  (mode 0700; frames are YOUR biometric data, keep them here)")

    recs = eh.load_records()
    s = eh.records_summary(recs)
    say(f"bring-up: {s['records']} records / {s['commands']} commands from {eh.init_header_path()}")

    manifest = {
        "kit": "egis0576-accuracy", "manifest_version": MANIFEST_VERSION,
        "created": datetime.datetime.now().isoformat(timespec="seconds"),
        "fingers": fingers, "presses": args.presses,
        "on": FINGER_ON_VAR, "off": FINGER_OFF_VAR,
        "baseline_frames": BASELINE_FRAMES, "baseline_max_var": BASELINE_MAX_VAR,
        "max_frames_per_press": args.max_frames,
        "exposure_target": eh.EXPOSURE_TARGET,
        "dc_c_baked": None, "dc_c_calibrated": None, "nofinger_mean": None,
        "data": {},
    }
    st = {"masked": False, "stopped": False, "touched": False, "dev": None, "completed": False}
    eh.install_exit_signals()
    try:
        try:
            capture_body(args, np, out, pw, recs, manifest, st)
        except KeyboardInterrupt:
            say("\ninterrupted -- keeping what was captured so far")
        except EOFError:
            say("\nstdin closed -- keeping what was captured so far")
        finally:
            try:
                cleanup_sensor(st)                        # innermost: sensor + fprintd first, always
            except KeyboardInterrupt:                    # a signal in the window before its SIG_IGN
                cleanup_sensor(st)                        # idempotent: finishes what was started
    finally:
        tot = sum(len(v) for v in manifest["data"].values())
        nfr = sum(len(f) for v in manifest["data"].values() for f in v)
        try:
            if tot:
                write_manifest(out, manifest, pw, st["completed"])
                for fn in os.listdir(out):
                    os.chmod(os.path.join(out, fn), 0o600)
        finally:
            chown_tree(out, pw, created)
        say(f"\n=== {'done' if st['completed'] else 'partial'}: {len(manifest['data'])} finger(s), {tot} presses, "
            f"{nfr} frames in {out} ===")
        if tot:
            say(f"next: python3 {os.path.join(HERE, 'evaluate.py')} {out}")


def user_site_tag(mod, acc):
    """' (user site)' if the module was imported from ~/.local (pip --user),
    which root's python does not search under sudo; also records its name."""
    import site
    try:
        f = os.path.realpath(getattr(mod, "__file__", "") or "")
        us = os.path.realpath(site.getusersitepackages())
    except Exception:                                    # noqa: BLE001
        return ""
    if f.startswith(us + os.sep):
        acc.append(mod.__name__)
        return " (user site: see below)"
    return ""


def dry_run(args):
    say("dry run: no USB I/O, no fprintd change, nothing written")
    reject_bad_outdir(args.out)
    root = eh.find_repo_root()
    recs = eh.load_records(eh.init_header_path(root))
    s = eh.records_summary(recs)
    say(f"  repo root      : {root}")
    say(f"  egis_init.h    : {s['records']} records, {s['commands']} EGIS commands, "
        f"{s['uploads']} upload command, {s['payload_bytes']} payload bytes")
    up = [i for i, r in enumerate(recs) if r[:4] == b"EGIS" and r[4] == 0x73]
    ok = (s["payload_bytes"] == eh.IMG and s["uploads"] == 1)
    say(f"  upload payload : {'ok' if ok else 'UNEXPECTED'} (record {up[0] if up else '?'} + {eh.IMG} bytes "
        f"in the {eh.UPLOAD_PAYLOAD_RECORDS} records after it, no reads in between)")
    say(f"  output dir     : {args.out} (would be created 0700)")
    say(f"  fingers        : {args.fingers}  x {args.presses} presses, <= {args.max_frames} frames each")
    say(f"  thresholds     : on {FINGER_ON_VAR}, off {FINGER_OFF_VAR}, baseline < {BASELINE_MAX_VAR} x {BASELINE_FRAMES}")
    say(f"  calibration    : reg 0x{eh.REG_DC_C:02x} binary search, {eh.CALIBRATE_ITERATIONS} steps, "
        f"target mean 0x{eh.EXPOSURE_TARGET:02x}")
    deps_ok = True
    user_site = []                                       # deps root's python will not see under sudo
    try:
        import numpy
        say(f"  numpy          : ok ({numpy.__version__}){user_site_tag(numpy, user_site)}")
    except ImportError:
        deps_ok = False
        say("  numpy          : MISSING (dnf install python3-numpy / apt install python3-numpy)")
    v = eh.pyusb_version()
    if v:
        import usb
        say(f"  pyusb          : ok ({v}){user_site_tag(usb, user_site)}")
    else:
        deps_ok = False
        say("  pyusb          : MISSING (dnf install python3-pyusb / apt install python3-usb)")
    if user_site and os.geteuid() != 0:
        say(f"  !! {' and '.join(user_site)} installed for your user only (pip --user): root's python will "
            "NOT find it, so\n     `sudo python3 capture.py` stops at once. Install the distro package "
            "(section 1) or run the capture\n     unprivileged with the udev rule.")
    say(f"  sensor present : {'yes' if eh.device_present() else 'no (1c7a:0576 not in sysfs)'}")
    say(f"  fprintd        : will be runtime-masked + stopped for the capture, unmasked + started afterwards"
        if not args.no_fprintd else "  fprintd        : left alone (--no-fprintd)")
    return 0 if (ok and deps_ok) else 1


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out", default=None, help="dataset directory (default: ~/egis-accuracy/<date_time>, mode 0700)")
    ap.add_argument("--fingers", default=DEFAULT_FINGERS, help=f"comma-separated labels (default: {DEFAULT_FINGERS})")
    ap.add_argument("--presses", type=int, default=12, help="presses per finger (default 12; keep it even)")
    ap.add_argument("--max-frames", type=int, default=12, help="max frames kept per press (default 12)")
    ap.add_argument("--wait", type=float, default=45.0, help="seconds to wait for a press before asking again")
    ap.add_argument("--no-fprintd", action="store_true", help="do not mask/stop/start fprintd (you did it yourself)")
    ap.add_argument("--dry-run", action="store_true", help="parse egis_init.h, check deps, touch nothing")
    ap.add_argument("-v", "--verbose", action="store_true", help="print every bring-up record and reply")
    args = ap.parse_args()
    if args.presses < 2 or args.presses % 2:
        ap.error("--presses must be an even number >= 2 (two-fold evaluation)")
    if args.max_frames < 1:
        ap.error("--max-frames must be >= 1")
    args.out = os.path.abspath(args.out) if args.out else default_outdir()
    if args.dry_run:
        try:
            return dry_run(args)
        except eh.SensorError as e:
            raise SystemExit(f"error: {e}")
    try:
        import numpy as np
    except ImportError:
        raise SystemExit("numpy is required: dnf install python3-numpy / apt install python3-numpy "
                         "(under sudo, root's python does not see a `pip install --user` copy: use the "
                         "distro package, or run unprivileged with the udev rule from the README)")
    if eh.pyusb_version() is None:
        raise SystemExit(eh.PYUSB_HINT)
    try:
        usb_errors = (eh._usb().core.USBError,)          # a transfer failure is a sensor error too
    except eh.SensorError:                               # no pyusb / no backend: reported by run_capture
        usb_errors = ()
    try:
        run_capture(args, np)
    except (eh.SensorError, *usb_errors) as e:
        raise SystemExit(f"sensor error: {e}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
