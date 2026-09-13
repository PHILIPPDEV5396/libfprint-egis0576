#!/usr/bin/env python3
"""Minimal plaintext EGIS/SIGE transport for the EgisTec EH576 (USB 1c7a:0576).

This is the capture side of the accuracy kit (tools/accuracy). It replays the
driver's own bring-up records from driver/egis0576/egis_init.h -- the same
bytes drivers/egis0576/egis0576_proto.c sends -- runs the driver's per-open
exposure calibration, and pulls raw 70x57 frames with "EGIS 64 0f 96". pyusb
only; numpy is not needed here. pyusb is imported lazily, so parsing
egis_init.h (and capture.py --dry-run) works without it.

SENSOR-SAFETY RULE (docs/sensor-tuning.md section 4): this module only ever
uses pyusb's synchronous bulk/control calls with timeouts. It never cancels a
transfer in flight. Aborting an in-flight transfer wedges the EH576 until the
board is power-cycled; a timeout never does (it fires only when no data is
flowing). Python delivers KeyboardInterrupt only between C calls, so a Ctrl-C
also lands at a transfer boundary, never inside one. SIGTERM, SIGHUP and
SIGQUIT (Ctrl-\\) are turned into KeyboardInterrupt (install_exit_signals,
first signal only) so a kill, a closed terminal or a Ctrl-\\ takes the same
boundary-exit path instead of dying inside a bulk read (SIGQUIT's default
action would core-dump the process mid-transfer). Keep it that way.

INTERRUPTS ARE HONOURED BETWEEN SEQUENCES, NOT BETWEEN TRANSFERS. A transfer
boundary is not a safe place to stop either: the bring-up replay and a
frame's preamble + GetFrame + reads each arm the sensor as one unit, and
abandoning either half-way leaves it in a state nobody has tested it in
(egis0576_proto.c makes the same choice: its cancellable is consulted only
at sequence boundaries). So the exit-signal handler defers: while a sequence
is open (`with sequence():`, a module-level depth counter) the first signal
is only RECORDED, and the KeyboardInterrupt is raised when the outermost
sequence exits. Sequences are: grab() as a whole; run_init() as a whole
(readiness poll, replay incl. the 8-record upload group, trailing reads); one
write + grab per calibration step; each readiness-poll iteration in
acquire(); force_reset()'s control transfer; and every single cmd() round
trip. Between sequences (the sleeps in capture.py's wait_press / wait_lift,
between presses) a signal raises immediately. Consequence: Ctrl-C is honoured
at the end of the current frame / sequence. On a healthy sensor that is a few
ms; on a sensor that has stopped answering, a frame is up to ~2.6 s of read
timeouts (six 300 ms preamble replies + one 800 ms frame read; a calibration
step adds one 300 ms register-write reply), the bring-up replay up to ~20 s
(24 reads x 800 ms) -- and a bulk-OUT write timeout (3 s) ends the sequence
early by exception, because the sensor is then already unresponsive and
nothing else in the sequence could reach it. The replay runs once, right
after the readiness poll proved the sensor alive. That is the same trade the
driver makes for never touching a URB in flight.

Prints inside a sequence (run_init -v) go through say(): a print() that
raises because the terminal is gone (SIGHUP -> EIO on the pty, or a broken
pipe) must not abandon the sequence between two of its transfers.

Standalone use (stop fprintd first: sudo systemctl stop fprintd):
    python3 egis_eh576.py reset     ForceResetDevice (0x21/9 wValue=0x00ff)
    python3 egis_eh576.py probe     PLAINTEXT / TLS / no answer, from the FIRST reply
    python3 egis_eh576.py records   parse egis_init.h and print a summary

`reset` is meant to be run in ITS OWN PROCESS (capture.py does so via
subprocess): the reset takes the device off the bus and it re-enumerates --
not on the request itself but on the NEXT transfer to the device (measured
2026-09-13: 5 s idle after the request, device number unchanged; the first
bulk access failed with ENODEV and the kernel re-enumerated it ~0.4 s later),
so `reset` sends that transfer itself and returns only once the fresh device
answers the readiness poll. The
libusb device list and any claimed handle in the issuing process are stale
afterwards, and a stale handle can look alive (descriptor access still
succeeds on it) while every transfer fails. A throw-away process for the reset
and a fresh libusb context for the capture avoids that class of bug entirely.
`reset` IGNORES SIGINT, SIGTERM, SIGHUP and SIGQUIT outright (it never
installs the raising handler): it is self-bounding (40 x 0.25 s device
search, one 2 s control transfer, then up to 24 short probes for the
re-enumerated device: ~50 s worst case, ~2 s typical) and an interrupt could have no useful
effect -- only a harmful one, if it stopped the reset short. A terminal
Ctrl-C during capture.py's cleanup reset therefore reaches the child and does
nothing.
"""
import errno
import os
import re
import signal
import sys
import time

VID, PID = 0x1c7a, 0x0576
INTF, EP_OUT, EP_IN = 0, 0x01, 0x82
IMG_W, IMG_H, IMG = 70, 57, 3990
INIT_HEADER = os.path.join("driver", "egis0576", "egis_init.h")

# "EGIS" ReadRegister(0): the readiness poll (Windows' check_and_recovery).
READY_PROBE = b"EGIS" + bytes([0x60, 0x00, 0x00])
# the driver's per-frame preamble, then the frame request
FRAME_PREAMBLE = ["632c020057", "602d00", "626703", "600f00", "632c020013", "600000"]
GRAB_CMD = b"EGIS" + bytes([0x64, 0x0f, 0x96])

# exposure calibration, egis0576_proto.c egis_dev_calibrate()
REG_DC_C = 0x0f              # sensor_dc_c, the register the calibration searches
EXPOSURE_TARGET = 0x58       # EGIS_EXPOSURE_TARGET: wanted no-finger frame mean
CALIBRATE_ITERATIONS = 6     # binary search steps over [0, 0x3f]
UPLOAD_PAYLOAD_RECORDS = 8   # payload records that follow the 0x73 upload command

PYUSB_HINT = ("pyusb is not installed: Fedora `sudo dnf install python3-pyusb`, "
              "Debian/Ubuntu `sudo apt install python3-usb`, Arch `sudo pacman -S python-pyusb` "
              "(under sudo, root's python does not see a `pip install --user` copy: use the "
              "distro package, or run unprivileged with the udev rule from the README)")


class SensorError(RuntimeError):
    pass


def say(msg=""):
    """print that survives a vanished terminal: after a SIGHUP the pty
    returns EIO, a closed pipe raises too. A print inside a sequence must never
    turn into an exception between two transfers of that sequence."""
    try:
        print(msg, flush=True)
    except (OSError, ValueError):
        pass


# ---------------------------------------------------------------- pyusb ----

_usb_mod = None


def _usb():
    """Import pyusb on first use. Everything that touches USB goes through
    this, so the record parser and --dry-run work without pyusb."""
    global _usb_mod
    if _usb_mod is None:
        try:
            import usb.core    # noqa: F401
            import usb.util    # noqa: F401
        except ImportError as e:
            raise SensorError(PYUSB_HINT) from e
        _usb_mod = usb
    return _usb_mod


def pyusb_version():
    """Version string if pyusb imports, else None. No device I/O."""
    try:
        import usb
    except ImportError:
        return None
    return getattr(usb, "__version__", "present")


# Ctrl-C, kill, closed terminal, Ctrl-\ -- all take the same boundary-exit path.
EXIT_SIGNALS = (signal.SIGINT, signal.SIGTERM, signal.SIGHUP, signal.SIGQUIT)

_exit_signal_seen = False    # the first exit signal has arrived (raised or recorded)
_seq_depth = 0               # > 0 while a USB sequence is open (see the module docstring)
_pending = None              # signum recorded while a sequence was open, raised at its exit


def raise_interrupt(signum, frame):
    """The exit-signal handler: the FIRST signal becomes a KeyboardInterrupt,
    later ones are no-ops. Inside a sequence (depth > 0) the signal is only
    recorded and the interrupt is raised by sequence() when the outermost
    sequence exits -- never between the transfers of one sequence."""
    global _exit_signal_seen, _pending
    if _exit_signal_seen:
        return
    _exit_signal_seen = True
    if _seq_depth > 0:
        _pending = signum
        return
    raise KeyboardInterrupt(f"signal {signum}")


class sequence:
    """`with sequence():` marks a run of USB transfers that must complete as
    one unit. Nests; a signal that arrived meanwhile is raised as
    KeyboardInterrupt exactly once, when the OUTERMOST sequence exits (also
    when it exits with another exception: the interrupt wins, the original
    error stays attached as __context__)."""

    def __enter__(self):
        global _seq_depth
        _seq_depth += 1
        return self

    def __exit__(self, et, ev, tb):
        global _seq_depth, _pending
        _seq_depth -= 1
        if _seq_depth > 0 or _pending is None:
            return False
        signum, _pending = _pending, None
        if et is not None and issubclass(et, KeyboardInterrupt):
            return False                             # already on its way out
        raise KeyboardInterrupt(f"signal {signum}")


def install_exit_signals():
    """SIGINT, SIGTERM, SIGHUP and SIGQUIT all raise KeyboardInterrupt -- but
    only the FIRST one, and only at a sequence boundary (raise_interrupt /
    sequence).
    Python runs the handler between C calls, so the exception surfaces at a
    transfer boundary and the normal cleanup path runs. Without this a kill or
    a closed terminal terminates the process wherever it is -- possibly with a
    bulk read in flight. After the first signal the handler is a no-op: a
    second Ctrl-C (or a SIGTERM after a Ctrl-C) can never interrupt the
    cleanup, which sets SIG_IGN explicitly as well, belt and braces."""
    for s in EXIT_SIGNALS:
        signal.signal(s, raise_interrupt)


def ignore_exit_signals():
    """SIG_IGN for every EXIT_SIGNALS member (SIGINT, SIGTERM, SIGHUP,
    SIGQUIT): for the `reset` subcommand, which must never be cut short and
    has nothing to clean up."""
    for s in EXIT_SIGNALS:
        signal.signal(s, signal.SIG_IGN)


# ----------------------------------------------------------------- repo ----

def find_repo_root(start=None):
    """Walk up from this file (or `start`) until driver/egis0576/egis_init.h
    is found. The kit may be run from anywhere; it must not depend on cwd."""
    p = os.path.abspath(start or __file__)
    if os.path.isfile(p):
        p = os.path.dirname(p)
    while True:
        if os.path.isfile(os.path.join(p, INIT_HEADER)):
            return p
        parent = os.path.dirname(p)
        if parent == p:
            raise SensorError(f"cannot find {INIT_HEADER} above {os.path.abspath(start or __file__)}; "
                              "run this from a checkout of libfprint-egis0576 (tools/accuracy/)")
        p = parent


def init_header_path(root=None):
    return os.path.join(root or find_repo_root(), INIT_HEADER)


_REC = re.compile(r'\{\s*\(const unsigned char\[\]\)\{([^}]*)\}\s*,\s*(\d+)\s*\}')


def load_records(path=None):
    """Parse egis_init.h into a list of byte strings, in order. Each record is
    either an "EGIS" command or a raw payload chunk of the 0x73 upload."""
    txt = open(path or init_header_path()).read()
    recs = []
    for m in _REC.finditer(txt):
        data = bytes(int(x, 16) for x in re.findall(r'0x([0-9a-fA-F]{2})', m.group(1)))
        if len(data) != int(m.group(2)):
            raise SensorError(f"egis_init.h: record length mismatch {len(data)} vs {m.group(2)}")
        recs.append(data)
    m = re.search(r'#define\s+EGIS_INIT_RECORD_COUNT\s+(\d+)', txt)
    if m and int(m.group(1)) != len(recs):
        raise SensorError(f"egis_init.h: parsed {len(recs)} records, header says {m.group(1)}")
    if not recs:
        raise SensorError("egis_init.h: no records parsed")
    return recs


def records_summary(recs):
    cmds = [r for r in recs if r[:4] == b"EGIS"]
    payload = sum(len(r) for r in recs if r[:4] != b"EGIS")
    uploads = sum(1 for r in cmds if r[4] == 0x73)
    return {"records": len(recs), "commands": len(cmds), "payload_bytes": payload, "uploads": uploads}


# ------------------------------------------------------------------ USB -----

def device_present():
    """sysfs-only presence check: no device I/O at all (used by --dry-run)."""
    base = "/sys/bus/usb/devices"
    try:
        for d in os.listdir(base):
            try:
                vid = open(os.path.join(base, d, "idVendor")).read().strip()
                pid = open(os.path.join(base, d, "idProduct")).read().strip()
            except OSError:
                continue
            if vid == f"{VID:04x}" and pid == f"{PID:04x}":
                return True
    except OSError:
        pass
    return False


def _permission_hint(e):
    return (f"{e}\n"
            "  -> no permission to open the sensor. Either run the capture with sudo, or add a\n"
            "     udev rule once, as /etc/udev/rules.d/70-egis-eh576.rules (the number matters:\n"
            "     uaccess is applied by 73-seat-late.rules, so the file must sort before it):\n"
            '     SUBSYSTEM=="usb", ATTRS{idVendor}=="1c7a", ATTRS{idProduct}=="0576", TAG+="uaccess"\n'
            "     (then `sudo udevadm control --reload && sudo udevadm trigger`; on Debian without\n"
            '     uaccess use GROUP="plugdev", MODE="0660" and be in that group)')


def _errno(e):
    return getattr(e, "errno", None)


def _find_device():
    """usb.core.find with the two library-level failures translated."""
    usb = _usb()
    try:
        return usb.core.find(idVendor=VID, idProduct=PID)
    except usb.core.NoBackendError as e:
        raise SensorError(f"pyusb found no libusb backend ({e}); install libusb-1.0 (libusb1)") from e


def _claim(d):
    """Detach the kernel driver if any and claim the interface. errno 13
    anywhere here is the permission problem; EBUSY means another process
    (fprintd) holds the interface."""
    usb = _usb()
    try:
        if d.is_kernel_driver_active(INTF):
            d.detach_kernel_driver(INTF)
    except usb.core.USBError as e:
        if _errno(e) == errno.EACCES:
            raise SensorError(_permission_hint(e))
    except Exception:                                 # noqa: BLE001  (backend without the call)
        pass
    try:
        usb.util.claim_interface(d, INTF)
    except usb.core.USBError as e:
        if _errno(e) == errno.EACCES:
            raise SensorError(_permission_hint(e))
        if _errno(e) == errno.EBUSY:
            raise SensorError(f"{e}\n  -> the interface is busy: is fprintd still running? "
                              "(sudo systemctl stop fprintd)")
        raise


def _drain(d):
    """Discard stale IN bytes left over from a previous session."""
    usb = _usb()
    for _ in range(4):
        try:
            d.read(EP_IN, 4096, 60)
        except usb.core.USBError:
            break


def acquire(max_attempts=10):
    """Find, claim and prove the sensor is alive, retrying through the
    re-enumeration window a ForceReset leaves behind. Liveness is proved by a
    real plaintext round trip -- the readiness poll -- not by descriptor
    access, which succeeds on stale handles. Returns (device, attempts).

    The budget is deliberately small: a sensor that answers with TLS records
    or not at all will not become ready by waiting, and each silent attempt
    costs about a second.

    The claimed handle is disposed on EVERY exit but the successful return
    (`keep`): a USBError, a SensorError from _claim(), and a KeyboardInterrupt
    that lands between two poll iterations (depth 0, in the sleep) all release
    the interface, so the cleanup's reset child never finds it still claimed
    by this process (the caller has no handle yet at that point)."""
    usb = _usb()
    last = "(nothing tried)"
    tls_seen = False
    for attempt in range(max_attempts):
        d = _find_device()
        if d is None:
            last = "device not on the bus"
            time.sleep(0.3)
            continue
        keep = False
        try:
            _claim(d)
            _drain(d)
            for _ in range(10):                      # the readiness poll
                with sequence():                     # one poll = one write + one read
                    d.write(EP_OUT, READY_PROBE, 1000)
                    r = bytes(d.read(EP_IN, 64, 800))
                last = r[:8].hex() or "(empty)"
                if len(r) >= 6 and r[0:4] == b"SIGE" and (r[5] & 0xfe) == 0xaa:
                    keep = True
                    return d, attempt
                if r[:1] == b"\x16":
                    tls_seen = True
                    break                            # TLS: retrying is pointless
                time.sleep(0.05)
        except usb.core.USBError as e:
            if _errno(e) == errno.EACCES:
                raise SensorError(_permission_hint(e))
            last = f"({e})"
        finally:
            if not keep:                             # also on KeyboardInterrupt / SensorError
                usb.util.dispose_resources(d)
        if tls_seen:
            break
        time.sleep(0.3)
    raise SensorError(
        f"sensor never became ready after {attempt + 1} attempt(s); last answer: {last}\n"
        + ("  -> the sensor answered with a TLS record (0x16): it is in TLS mode.\n" if tls_seen else "")
        + "  Likely causes, in order:\n"
        "   - fprintd (or another process) still holds the device: sudo systemctl stop fprintd\n"
        "   - the sensor is in TLS mode, from an older driver build or after a Windows boot:\n"
        "     run `python3 egis_eh576.py reset` first (with fprintd stopped), then retry\n"
        "   - no permission to open the device: udev rule or sudo (see README)")


def release(d):
    """Release the interface and the handle. Nothing is in flight at this
    point (every call above returned or timed out), so this is safe."""
    usb = _usb()
    try:
        usb.util.release_interface(d, INTF)
    except usb.core.USBError:
        pass
    usb.util.dispose_resources(d)


def run_init(d, recs, verbose=False):
    """Replay the bring-up records as raw plaintext bulk writes. Returns how
    many commands were answered with SIGE.

    Record 15 ("EGIS 73 0f 96") is an UPLOAD: its 3990-byte payload follows
    in the next UPLOAD_PAYLOAD_RECORDS (8) records and the sensor answers only
    once it is complete. Reading in between stalls the upload and wedges the
    sensor, so -- exactly as egis0576_proto.c does it -- nothing is read after
    the 0x73 command for the next eight records, BY COUNT, whatever they
    contain.

    The WHOLE replay is one sequence: an exit signal that arrives during it
    is raised after the last record, never in between (a sensor left with a
    prefix of the vendor sequence is an untested state). The verbose output
    goes through say(): a terminal that vanished mid-replay (SIGHUP, EIO on
    the pty) must not end the replay early either."""
    usb = _usb()
    answered = 0
    in_upload = 0
    with sequence():
        for i, rec in enumerate(recs):
            is_cmd = len(rec) >= 5 and rec[0:4] == b"EGIS"
            d.write(EP_OUT, rec, 3000)
            if is_cmd and rec[4] == 0x73:
                in_upload = UPLOAD_PAYLOAD_RECORDS
                if verbose:
                    say(f"  rec{i:02d} UPLOAD cmd (no read for the next {in_upload} records)")
            elif in_upload > 0:
                in_upload -= 1
                if verbose:
                    say(f"  rec{i:02d} upload group {len(rec)}B (no read)")
            elif is_cmd:
                try:
                    r = bytes(d.read(EP_IN, 4096, 800))
                    if r[0:4] == b"SIGE":
                        answered += 1
                    if verbose:
                        say(f"  rec{i:02d} {rec[:7].hex()} -> {r[:8].hex()}")
                except usb.core.USBError:
                    if verbose:
                        say(f"  rec{i:02d} {rec[:7].hex()} -> (no reply)")
            elif verbose:
                say(f"  rec{i:02d} raw {len(rec)}B outside an upload group (unexpected)")
    return answered


def cmd(d, tail_hex, timeout=300):
    """One command round trip (write, then read the reply); the smallest
    sequence, so an interrupt never leaves a reply unread."""
    usb = _usb()
    with sequence():
        d.write(EP_OUT, b"EGIS" + bytes.fromhex(tail_hex), 3000)
        try:
            return bytes(d.read(EP_IN, 4096, timeout))
        except usb.core.USBError:
            return b""


def read_reg(d, reg):
    """ReadRegister: "EGIS 60 reg 00" -> "SIGE reg value status"; -1 on no
    answer (egis0576_proto.c egis_readreg)."""
    r = cmd(d, f"60{reg & 0xff:02x}00", 800)
    return r[5] if len(r) >= 6 and r[:1] == b"S" else -1


def write_reg(d, reg, val):
    """WriteRegister: "EGIS 61 reg val" (egis0576_proto.c egis_writereg)."""
    cmd(d, f"61{reg & 0xff:02x}{val & 0xff:02x}", 300)


def grab(d):
    """Per-frame preamble, then EGIS 64 0f 96 -> accumulate IMG bytes.
    Returns the raw 3990-byte frame, or None if it did not complete.

    Preamble + GetFrame + all frame reads are ONE sequence: the preamble
    arms the sensor for a frame, and an exit signal is raised only once the
    frame has arrived (or the reads have timed out), never in between."""
    usb = _usb()
    with sequence():
        for t in FRAME_PREAMBLE:
            cmd(d, t, 300)                           # egis0576_proto.c: 300 ms per preamble reply
        d.write(EP_OUT, GRAB_CMD, 3000)
        buf = b""
        while len(buf) < IMG:
            try:
                buf += bytes(d.read(EP_IN, 4096, 800))  # egis0576_proto.c: 800 ms per frame chunk
            except usb.core.USBError:
                break
    return buf[:IMG] if len(buf) >= IMG else None


def frame_mean(b):
    """Integer mean of the raw bytes, truncated like the driver's (sum / EGIS_IMG)."""
    return sum(b) // len(b)


def frame_variance(b):
    """Population variance of the raw bytes -- the driver's frame_variance()."""
    n = len(b)
    mean = sum(b) / n
    return sum((x - mean) * (x - mean) for x in b) / n


def calibrate(d):
    """The driver's per-open exposure calibration, egis0576_proto.c
    egis_dev_calibrate(), step for step: a binary search over reg 0x0f in
    [0, 0x3f] -- write mid, grab one no-finger frame, compare its mean with
    EXPOSURE_TARGET, keep the mid whose mean came closest -- then write the
    best. No finger may be on the sensor. Without this, a unit whose
    operating point differs from the reference unit's 0x20 captures saturated
    frames (the usable window is only ~0x1c..0x24, docs/sensor-tuning.md).

    Returns {"baked": reg value after the bring-up, "calibrated": value
    written, "trace": [[reg, mean], ...]}. A frame that fails to arrive
    aborts with SensorError after the baked value has been written back."""
    baked = read_reg(d, REG_DC_C)
    lo, hi, best, best_diff = 0, 0x3f, 0x1f, 0x100
    trace = []
    for _ in range(CALIBRATE_ITERATIONS):
        mid = (lo + hi) >> 1
        with sequence():                             # register write + its frame: one step
            write_reg(d, REG_DC_C, mid)
            f = grab(d)
        if f is None:
            if baked >= 0:
                write_reg(d, REG_DC_C, baked)
            raise SensorError(f"no frame during exposure calibration (reg 0x0f = 0x{mid:02x})")
        level = frame_mean(f)
        trace.append([mid, level])
        diff = abs(level - EXPOSURE_TARGET)
        if diff < best_diff:
            best_diff, best = diff, mid
        if level > EXPOSURE_TARGET:
            hi = mid
        else:
            lo = mid
    write_reg(d, REG_DC_C, best)
    return {"baked": baked, "calibrated": best, "trace": trace}


def force_reset(tries=24):
    """ForceResetDevice: class request 9 on the interface, wValue 0x00ff.
    The request completes normally and is accepted at once, but the device
    stays on the bus at its old address until the NEXT transfer to it: that
    transfer fails (no-device / pipe / io) and the kernel re-enumerates the
    sensor ~0.4 s later. Measured 2026-09-13: 5 s idle after the request,
    device number unchanged; the first bulk access dropped it. So this does
    not stop at the request: _wait_reenumerated() sends the readiness probe to
    trigger the drop right away and returns only when the RE-ENUMERATED device
    answers the plaintext readiness poll -- neither the capture's acquire()
    nor fprintd (after the cleanup) must inherit an armed reset. Busy and
    timeout on the request itself are failures. Run this in its own process
    -- see the module docstring."""
    usb = _usb()
    d = None
    for _ in range(40):
        try:
            d = _find_device()
            if d is not None:
                d.get_active_configuration()
                break
        except usb.core.USBError as e:
            if _errno(e) == errno.EACCES:
                raise SensorError(_permission_hint(e))
            d = None
        time.sleep(0.25)
    if d is None:
        raise SensorError("sensor not found for reset (1c7a:0576 not on the bus)")
    old = (d.bus, d.address)
    try:
        with sequence():                             # the one transfer that must not be cut short
            d.ctrl_transfer(0x21, 9, 0x00ff, INTF, None, 2000)
    except usb.core.USBError as e:
        en = _errno(e)
        if en == errno.EACCES:
            raise SensorError(_permission_hint(e))
        if en == errno.EBUSY:
            raise SensorError(f"reset refused, device busy ({e}): is fprintd still running? "
                              "(sudo systemctl stop fprintd)")
        if en == errno.ETIMEDOUT:
            raise SensorError(f"sensor did not respond to the reset request ({e})")
        if en not in (errno.ENODEV, errno.EPIPE, errno.EIO):
            raise SensorError(f"reset failed: {e}")
        # no-device / pipe / io: the device dropped during the request itself -- fine too
    finally:
        usb.util.dispose_resources(d)
    return _wait_reenumerated(old, tries)


def _wait_reenumerated(old, tries=24):
    """Trigger the armed reset with one readiness probe and wait for the fresh
    device: success is a device answering `SIGE .. aa` on a bus address other
    than `old` (or on the same address once an access failure -- the drop --
    has been seen). Each try is <= ~1.3 s of transfer timeouts plus 0.25 s:
    ~35 s worst case for the default 24 tries, ~1.5 s typical. Every handle
    opened here is disposed again; the caller opens the fresh device itself."""
    usb = _usb()
    kicked = False
    last = "device not on the bus"
    for _ in range(tries):
        time.sleep(0.25)
        d = _find_device()
        if d is None:
            continue
        fresh = (d.bus, d.address) != old
        try:
            _claim(d)
            with sequence():                         # one write + one read, never cut short
                d.write(EP_OUT, READY_PROBE, 1000)
                r = bytes(d.read(EP_IN, 64, 300))
        except usb.core.USBError as e:
            en = _errno(e)
            if en == errno.EACCES:
                raise SensorError(_permission_hint(e))
            if en == errno.EBUSY:
                raise SensorError(f"reset: device busy ({e}): is fprintd still running? "
                                  "(sudo systemctl stop fprintd)")
            kicked = True                            # the access failed: the drop is under way
            last = f"({e})"
            continue
        finally:
            usb.util.dispose_resources(d)
        last = r[:8].hex() or "(empty)"
        if len(r) >= 6 and r[:4] == b"SIGE" and (r[5] & 0xfe) == 0xaa and (fresh or kicked):
            return True
    raise SensorError(f"sensor accepted the reset request but did not re-enumerate "
                      f"({tries} probes); last answer: {last}")


def probe_mode():
    """Classify the sensor from its FIRST answer, no acquire loop: claim,
    drain, one ReadRegister(0) with a short timeout. Returns (mode, reply)
    with mode in "plaintext" / "tls" / "none"."""
    usb = _usb()
    d = _find_device()
    if d is None:
        raise SensorError("sensor not found (1c7a:0576 not on the bus)")
    try:
        _claim(d)
        _drain(d)
        with sequence():
            d.write(EP_OUT, READY_PROBE, 1000)
            try:
                r = bytes(d.read(EP_IN, 64, 800))
            except usb.core.USBError as e:
                if _errno(e) == errno.EACCES:
                    raise SensorError(_permission_hint(e))
                r = b""
    except usb.core.USBError as e:
        if _errno(e) == errno.EACCES:
            raise SensorError(_permission_hint(e))
        raise SensorError(f"probe failed: {e}")
    finally:
        release(d)
    if r[:4] == b"SIGE":
        return "plaintext", r
    if r[:1] == b"\x16":
        return "tls", r
    return "none", r


if __name__ == "__main__":
    what = sys.argv[1] if len(sys.argv) > 1 else "records"
    if what == "reset":
        ignore_exit_signals()      # immune to Ctrl-C / kill / SIGHUP / Ctrl-\: self-bounding, nothing to clean up
    else:
        install_exit_signals()
    try:
        if what == "reset":
            force_reset()
            print("ForceResetDevice sent; the sensor re-enumerated and answers plaintext")
        elif what == "probe":
            mode, r = probe_mode()
            label = {"plaintext": "PLAINTEXT", "tls": "TLS (0x16 record: run `reset` first)",
                     "none": "no answer (sensor unresponsive: run `reset`, then power-cycle if it persists)"}[mode]
            print(f"first answer {r[:16].hex() or '(none)'} -> {label}")
            sys.exit(0 if mode == "plaintext" else 1)
        elif what == "records":
            root = find_repo_root()
            s = records_summary(load_records(init_header_path(root)))
            print(f"repo root: {root}")
            print(f"egis_init.h: {s['records']} records, {s['commands']} EGIS commands, "
                  f"{s['uploads']} upload, {s['payload_bytes']} payload bytes")
        else:
            sys.exit(__doc__)
    except KeyboardInterrupt:
        sys.exit("interrupted")
    except SensorError as e:
        sys.exit(f"error: {e}")
