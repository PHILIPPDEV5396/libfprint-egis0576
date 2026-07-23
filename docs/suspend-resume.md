# Fingerprint after suspend/resume — the full story

**TL;DR.** After a system suspend, GNOME's lock screen sometimes won't offer the
fingerprint and drops straight to the password. This is **not** a bug in this
driver — it is a well-known, still-unfixed **gnome-shell/fprintd** bug where the
fingerprint device's *claim* is not released across suspend (`"Device was already
claimed"`). The driver is never even invoked at the failing moment. The reliable,
community-standard fix is to **restart `fprintd` around suspend**, which is exactly
what the [`integration/`](../integration/) systemd-sleep hook does. Keep it
installed. This document records the investigation so nobody (the author included)
re-treads the dead ends.

---

## Symptom

On an AMD Rembrandt laptop (Lenovo Yoga 7 14ARB7, Fedora, GNOME 50, `mem_sleep=
s2idle`):

- Historically (before the `read_record` deadline, see below) a suspend that
  happened *while the sensor was armed* could **hang the unlock screen hard**
  (gnome-shell unresponsive, forced reboot).
- After that hang was fixed, the remaining symptom is: **~40 % of resumes, the
  lock screen offers only the password** — no fingerprint. Locking again
  (`Super+L`) or switching VT makes the reader work again.

## The real root cause

With the hook disabled and `FP_DEBUG=all`, the fprintd journal right after resume
shows, verbatim:

```
Preparing devices for resume
Requesting authorization from :1.482 to call method 'Claim' for device '… EH576'
Authorization denied to :1.482 to call method 'Claim' for device '… EH576':
        Device was already claimed
```

No `open`, no `identify`, **nothing from the driver** — libfprint is never
reached. `fprintd` still holds a **stale claim** taken by gnome-shell's
pre-suspend lock-screen verify, which is not released across the suspend. The new
post-resume claim is therefore refused, and gnome-shell falls back to the password.

This is an upstream **gnome-shell + fprintd** interaction bug, entirely above
libfprint. It is widely reported and, as of mid-2026, **open / unfixed**:

- GNOME/gnome-shell #7791 — *Fingerprint unlock sometimes not available after
  suspend* (~40 % of resumes; workaround = VT switch / re-login)
  <https://gitlab.gnome.org/GNOME/gnome-shell/-/issues/7791>
- Ubuntu #2067135 — *Fingerprint authentication doesn't work after suspend*
  (ThinkPad T15/X1 Yoga/P1, Ubuntu 23.10/24.04)
  <https://bugs.launchpad.net/bugs/2067135>
- GNOME/gnome-control-center #3166, Ubuntu #2077837, Red Hat Bugzilla 503898,
  Framework and Arch forums — same class of bug on unrelated readers.

## The fix (what actually works): the `integration/` hook

The community-standard workaround is to **restart `fprintd` around suspend** so no
stale claim survives. The hook here does exactly that in its `pre` phase:

```sh
pre)  systemctl stop fprintd.service   # drop the stale claim before we sleep
```

On resume, `fprintd` is D-Bus-activated fresh, with no claim held, so the lock
screen's `Claim` succeeds and the fingerprint works on the first press. The `post`
phase additionally re-enumerates the sensor (`authorized` 0→1), which resets its
exposure state so the first post-resume capture is well-exposed. The udev rule
(`60-…-nosuspend.rules`) disables USB autosuspend so the reader's session isn't
dropped while idle.

None of this can live in libfprint: clearing an fprintd claim and restarting the
service are operations *above* the driver, and they must run *at suspend/resume
time*, which is inherently a systemd-sleep hook.

## What the driver *does* contribute

Two things, both real but both narrower than earlier versions of this doc claimed:

1. **A wall-clock deadline in `read_record`** (`egis0576_tls.c`). If the driver
   ever *does* talk to a dead TLS session (e.g. a suspend fires while a capture is
   genuinely in flight), every USB read is now bounded, so `getframe`/the
   handshake fail fast to a clean error instead of spinning forever. **This is
   what removed the original hard freeze.** Keep it.
2. **`suspend`/`resume` vfuncs** that flag the session stale and cancel an
   in-flight action. These only run when an action is *active at the instant of
   suspend* — which, in the real GNOME lock-screen flow, is usually **not** the
   case (fprintd has already gone idle and closed the device). So they are a minor
   defensive measure, not the mechanism that makes fingerprint-after-resume work.

## Why there is no pure in-driver fix (the dead ends)

Three in-driver approaches were built and tested on hardware; all failed for the
same underlying reason — **at the moment recovery is needed, the driver is not in
the call path** (fprintd hasn't opened/claimed the device yet, or refuses to).

1. **`suspend` cancels the action (egismoc-style).** No freeze, but GNOME does not
   re-arm after the cancel → password only. (And in the common idle-at-suspend
   case the vfunc never even fires.)
2. **`suspend` keeps the action alive + worker re-handshakes on resume.** The
   worker re-init either ran at the wrong time (the `suspend` vfunc fires ~2 s
   *before* the actual kernel suspend) or produced non-matching frames; recovery
   still came from a later fresh `open`, sometimes after a multi-second stall.
3. **Replicate the Windows recovery.** The Windows driver
   (`CRealTekDeviceCtrlForET576WithTLS::ForceResetDevice`) resets the sensor
   *without* USB re-enumeration via a class control request **`0x21/9`
   `wValue=0x00ff`** (the normal open trigger uses `wValue=0`), orchestrated by
   `check_and_recovery` in `egis_fp_common_5XX.c` (poll token `0xAA` → ForceReset
   → re-write regs `0x0A/0x0C/0x50` + vdm upload → `tz_calibrate_dvr`). This was
   implemented (`egis_tls_force_reset`) and tested — but it is **moot here**: in
   the real flow the post-resume `Claim` is refused with *"Device was already
   claimed"* before any `open` runs, so a driver-level reset is never reached.

The decisive measurement was a clean test with the *real* GNOME lock screen (not
`fprintd-verify`, whose single-shot lifecycle confounded earlier runs): the
driver's `suspend` vfunc did not fire at all, and the failure was the polkit-level
`Claim` denial above.

The `wValue=0x00ff` ForceReset sequence is documented above in case a future
maintainer needs an in-driver device reset for a *different* reason — but it is not
a fix for the suspend/resume claim problem.

## Bottom line

- **Keep the hook + udev rule** ([`integration/`](../integration/)). They are the
  correct, standard fix for a real upstream bug, not a workaround for a driver
  shortcoming.
- The `read_record` deadline in the driver prevents the hard freeze.
- If gnome-shell/fprintd ever fix the claim-across-suspend bug upstream, the hook
  simply becomes a harmless no-op and can be removed.
