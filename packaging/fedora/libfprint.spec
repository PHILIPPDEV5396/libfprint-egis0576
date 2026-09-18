# COPR spec: a rebuild of libfprint with the out-of-tree EgisTec EH576 (egis0576)
# driver compiled in. It keeps the package name `libfprint` so it is a DROP-IN
# replacement that the stock fprintd loads unchanged (same soname libfprint-2.so.2).
#
# Built from PRISTINE upstream libfprint v1.94.100 + the egis0576 driver.
#
# NOTE: as of libfprint 1.94.100 Fedora's package carries NO downstream patches
# (its spec has no Patch: lines) and upstream ships the egis_etu905 driver itself,
# so this build is a true superset of Fedora's — nothing of theirs is dropped.
# (Up to 1.94.10 that was not the case, and this directory carried a separately
# rebased patch for Fedora's own tree; it is gone with the rebase.)

%global egis_tag v0.4.5

# Matcher flavour. Default: the vendor matcher, byte-identical to before.
#   rpmbuild --with gabor     -> -Degis0576_matcher=gabor (experimental, see
#                                driver/egis0576/gabor/), Release gets ".gabor"
#                                so the package is distinguishable and sorts
#                                above the default build for the same version.
# Templates are not portable between flavours; re-enrol after switching.
%bcond_with gabor
%if %{with gabor}
%global flavour_rel .gabor
%global flavour_meson -Degis0576_matcher=gabor
%else
%global flavour_rel %{nil}
%global flavour_meson %{nil}
%endif

Name:           libfprint
Version:        1.94.100
# Release sorts ABOVE Fedora's own 1.fcNN for the SAME Version, so `dnf upgrade`
# prefers this build. The robust backstop is the COPR repo *priority* (see
# ../README.md, e.g. priority=90), which wins regardless of version — keep it
# set, because it is what saves enabled users when Fedora ships a version this
# spec has not been rebased onto yet.
Release:        99%{?dist}.egis10%{flavour_rel}
Summary:        Toolkit for fingerprint scanner (rebuilt with the EgisTec EH576 / 1c7a:0576 driver)

License:        LGPL-2.1-or-later AND NIST-PD
URL:            https://github.com/PHILIPPDEV5396/libfprint-egis0576
Source0:        https://gitlab.freedesktop.org/libfprint/libfprint/-/archive/v%{version}/libfprint-v%{version}.tar.gz
Source10:       https://github.com/PHILIPPDEV5396/libfprint-egis0576/archive/refs/tags/%{egis_tag}.tar.gz#/libfprint-egis0576-%{egis_tag}.tar.gz

# --- BuildRequires: copied verbatim from Fedora's own libfprint.spec (known-good
#     for 1.94.100). openssl-devel is upstream's own (uru4000); the egis0576 driver
#     needs no crypto. ---
BuildRequires:  meson
BuildRequires:  gcc
BuildRequires:  gcc-c++
BuildRequires:  git
BuildRequires:  openssl-devel
BuildRequires:  pkgconfig(glib-2.0) >= 2.50
BuildRequires:  pkgconfig(gio-2.0) >= 2.44.0
BuildRequires:  pkgconfig(gusb) >= 0.3.0
BuildRequires:  pkgconfig(nss)
BuildRequires:  pkgconfig(pixman-1)
BuildRequires:  gtk-doc
BuildRequires:  libgudev-devel
BuildRequires:  systemd
BuildRequires:  gobject-introspection-devel
BuildRequires:  python3-cairo python3-gobject cairo-devel
BuildRequires:  umockdev >= 0.13.2

%description
libfprint offers support for consumer fingerprint reader devices.
This build additionally includes an out-of-tree driver for the EgisTec EH576
(USB 1c7a:0576), compiled into libfprint-2.so. It is a drop-in replacement for
the stock Fedora libfprint.

%package        devel
Summary:        Development files for %{name}
Requires:       %{name}%{?_isa} = %{version}-%{release}
%description    devel
The %{name}-devel package contains libraries and header files for
developing applications that use %{name}.

%package        tests
Summary:        Tests for the %{name} package
Requires:       %{name}%{?_isa} = %{version}-%{release}
%description    tests
The %{name}-tests package contains tests to verify %{name}.

%prep
%autosetup -n libfprint-v%{version} -N
# Unpack the egis0576 driver repo (auto-detect its top dir), drop the driver into
# the libfprint tree, then apply the meson integration patch.
tar -xf %{SOURCE10}
egisdir=$(tar -tf %{SOURCE10} | head -1 | cut -d/ -f1)
cp -a "$egisdir"/driver/egis0576.c "$egisdir"/driver/egis0576.h libfprint/drivers/
cp -a "$egisdir"/driver/egis0576 libfprint/drivers/
patch -p1 < "$egisdir"/patches/libfprint-%{version}-egis0576.patch

%build
# -Ddrivers=all builds every default+virtual driver, including egis0576 (which the
# patch added to default_drivers, so its engine static-lib is built and linked).
%meson -Ddrivers=all %{flavour_meson}
%meson_build

%install
%meson_install

# Suspend/resume integration from the egis0576 repo tarball: a systemd-sleep hook
# and a udev rule. The hook's `pre` phase stops fprintd so no stale device claim
# survives suspend (a known gnome-shell/fprintd bug — see the repo's
# docs/suspend-resume.md); its `post` phase re-enumerates the reader. The udev rule
# disables USB autosuspend for 1c7a:0576. The build dir still holds the unpacked
# tarball from %prep, so re-detect its top dir the same way.
egisdir=$(tar -tf %{SOURCE10} | head -1 | cut -d/ -f1)
install -Dm 0755 "$egisdir/integration/50-egis0576-fp-resume.sh" \
    %{buildroot}%{_prefix}/lib/systemd/system-sleep/50-egis0576-fp-resume.sh
install -Dm 0644 "$egisdir/integration/60-egis0576-fp-nosuspend.rules" \
    %{buildroot}%{_udevrulesdir}/60-egis0576-fp-nosuspend.rules
# Gate that holds fprintd's start while the hook's post phase re-enumerates the
# sensor; fail-open after 15 s, so a stale marker cannot disable fingerprint
# authentication. Diagnosed by sam-dant (see integration/README.md).
install -Dm 0755 "$egisdir/integration/egis0576-fp-wait" \
    %{buildroot}%{_libexecdir}/egis0576-fp-wait
install -Dm 0644 "$egisdir/integration/egis0576-fprintd-wait.conf" \
    %{buildroot}%{_prefix}/lib/systemd/system/fprintd.service.d/10-egis0576-resume-wait.conf

%ldconfig_scriptlets

# The fprintd.service.d drop-in this package installs is a systemd unit
# fragment, and rpm has no trigger that reloads systemd for it. Without these,
# the resume gate stays inert until the next reboot on install, and on erase a
# still-cached unit keeps an ExecStartPre pointing at a deleted binary, which
# fails activation with 203/EXEC -- fingerprint authentication silently dead
# until someone reloads. install.sh has always done this; the package did not.
# Only a reload: fprintd.service belongs to the fprintd package, so this one
# must not touch its enablement with %%systemd_post.
%post
/usr/bin/systemctl daemon-reload >/dev/null 2>&1 || :

%postun
/usr/bin/systemctl daemon-reload >/dev/null 2>&1 || :

# %%check is intentionally omitted (matches current Fedora: disabled under recent
# pygobject/umockdev).

%files
%license COPYING
%doc NEWS THANKS AUTHORS README.md
%{_libdir}/*.so.*
%{_libdir}/girepository-1.0/*.typelib
%{_udevhwdbdir}/60-autosuspend-libfprint-2.hwdb
%{_udevrulesdir}/70-libfprint-2.rules
%{_datadir}/metainfo/org.freedesktop.libfprint.metainfo.xml
# egis0576 suspend/resume integration (see docs/suspend-resume.md)
%{_prefix}/lib/systemd/system-sleep/50-egis0576-fp-resume.sh
%{_libexecdir}/egis0576-fp-wait
%dir %{_prefix}/lib/systemd/system/fprintd.service.d
%{_prefix}/lib/systemd/system/fprintd.service.d/10-egis0576-resume-wait.conf
%{_udevrulesdir}/60-egis0576-fp-nosuspend.rules

%files devel
%doc HACKING.md
%{_includedir}/*
%{_libdir}/*.so
%{_libdir}/pkgconfig/%{name}-2.pc
%{_datadir}/gir-1.0/*.gir
%{_datadir}/gtk-doc/html/libfprint-2/

%files tests
%{_libexecdir}/installed-tests/libfprint-2/
%{_datadir}/installed-tests/libfprint-2/

%changelog
* Mon Sep 14 2026 PHILIPPDEV5396 - 1.94.100-99.egis10
- Update egis0576 driver to v0.4.5.
  * integration: hold fprintd's start while the resume hook re-enumerates the
    sensor. The hook needs about a second for that, fprintd is D-Bus activated,
    and a claim landing in that window is refused with "Device was already
    claimed". A marker in /run plus an ExecStartPre gate closes it; the gate is
    fail-open after 15 s so a stale marker can never disable fingerprint
    authentication. Diagnosed, built and tested by sam-dant.
  * Ships two new files: %{_libexecdir}/egis0576-fp-wait and the
    fprintd.service.d drop-in that runs it.
  * Documentation: the accuracy claims now cover three units instead of one
    (false rejects 0/60, 2/60 and 37/60 at the shipped threshold), the
    enrolment code legend in the accuracy kit was wrong (-8 is a success, not a
    rejection), and the kit no longer prints a vendor "EER" that is not one.
* Sun Sep 13 2026 PHILIPPDEV5396 - 1.94.100-99.egis9
- Rebase onto upstream libfprint 1.94.100 (Fedora 44 and rawhide ship it).
  * The meson integration patch moved with upstream's build refactor: drivers
    are registered in the new drivers_info dict, driver_sources entries use
    files(), and the egis0576 matcher static library now attaches to
    libfprint_drivers (the fprint-list-* helpers link it too, so attaching it
    to the shared library alone no longer resolves).
  * Fedora's package carries no downstream patches at this version and upstream
    ships egis_etu905 itself, so this build is a true superset of Fedora's; the
    separately rebased Fedora-tree patch in this directory is retired.
  * Both matcher flavours (-Degis0576_matcher=vendor|cleanroom) build clean.
- Update egis0576 driver to v0.4.4: re-apply the calibrated exposure after
  every sensor bring-up.
  * The vendor init replay block-writes registers 0x09..0x13 and so resets the
    exposure register 0x0f to the baked value. Since fprintd opens the device
    on every claim, only the first open of a process ran with the calibrated
    exposure; every later open and every post-resume re-init fell back to the
    baked value, and the flat-field baseline is exposure-tied. The calibrated
    value is now cached process-wide and re-applied at the end of each
    bring-up. No effect on units whose calibration equals the baked value (the
    reference unit); no re-enrollment needed.
* Sun Sep 13 2026 PHILIPPDEV5396 - 1.94.10-99.egis8
- Update egis0576 driver to v0.4.3: fix a hang in the capture thread.
  * Symptom: a verify that never answers, then every later attempt refused
    with "Device was already claimed" until fprintd is restarted (the lock
    screen falls back to the password). Root cause, found with gdb on the
    stuck daemon: the driver's worker thread used gusb's synchronous transfer
    helpers, whose completion is dispatched on the DEFAULT GMainContext -- by
    the fprintd main thread, not the worker. If that completion lands before
    the worker's g_main_loop_run is running, the quit is lost and the worker
    waits forever, holding the device claim.
  * The transport now drives gusb's async API on a private GMainContext the
    worker iterates itself, so no other thread is involved in completing a
    transfer. Timeouts, cancellation (polled between transfers, never handed
    to gusb) and the wire protocol are unchanged. Written up in
    docs/worker-thread.md.
- No re-enrollment needed; no change to capture or matching.
* Sun Sep 13 2026 PHILIPPDEV5396 - 1.94.10-99.egis7
- Update egis0576 driver to v0.4.2.
  * Cancel latency bounded to one transfer sequence (a getframe, ~2.6 s worst
    case on a sensor that stopped answering) instead of a whole re-init
    (~11 s): the worker now honours a cancel at every sequence boundary, the
    recovery path included. Aborting the in-flight USB transfer was measured
    to wedge the sensor until a board power cycle, so the driver deliberately
    never does that; documented in docs/sensor-tuning.md.
  * Experimental build option -Degis0576_matcher=cleanroom selects Thaddeus
    Stepanovich's LGPL clean-room correlation matcher instead of the vendor
    one, for comparing both on the same captures. Default unchanged. Prints
    enrolled under one matcher are rejected by the other.
  * egis0576_proto.c is built under libfprint's normal driver flags (stack
    protector on) instead of the vendor engine's relaxed ones.
  * install.sh passes EGIS0576_MESON_ARGS through to meson.
- No re-enrollment needed; no change to capture or matching in the default
  build.
* Wed Sep 09 2026 PHILIPPDEV5396 - 1.94.10-99.egis6
- Update egis0576 driver to v0.4.1: documentation and measured limits.
  * New docs/sensor-tuning.md records three tested non-improvements, so they
    are not re-attempted: uploading a measured background to the sensor has
    no effect on this fetch path; raising the gain buys no signal-to-noise
    and starts clipping pixels from reg 0x12 = 0x06 upward; and the exposure
    register's usable window is only about eight counts wide, which is why
    per-frame auto-exposure is deliberately not wired into the capture loop.
  * States plainly that no EER/FAR/FRR has ever been measured, and qualifies
    the README's security row accordingly.
  * Removes the last pre-plaintext leftovers from the driver, including two
    user-visible fp_dbg messages that still spoke of a TLS session.
  * Documentation pass across all six markdown files: dead cross-references,
    a broken `dnf install rpmbuild` (the package is rpm-build), terminology
    drift, and a garbled sentence introduced by an earlier edit.
- No functional change to capture, matching or enrolled templates.
* Wed Sep 09 2026 PHILIPPDEV5396 - 1.94.10-99.egis5
- Update egis0576 driver to v0.4.0: replace the TLS-PSK transport with the
  sensor's plaintext EGIS/SIGE protocol -- the same one the vendor's own
  Windows driver for this device uses.
  * Fixes dual-boot: earlier releases left the sensor in a session mode it
    keeps across autosuspend, USB port reset and reboot, which made the
    Windows vendor driver fail to start it (Device Manager Code 10) until
    the machine was fully powered down. The driver no longer enters that
    mode, and resets a sensor it finds stuck in it (one-time, on first open).
  * Better images, not merely equivalent: finger/no-finger frame variance
    1069/140 = 7.6x, against 890/160 = 5.6x through the TLS path on the
    same unit.
  * Removes ~460 lines of hand-rolled TLS 1.2, the hardcoded pre-shared key,
    and the driver's libcrypto dependency. libcrypto remains in libfprint
    only for upstream's own uru4000 driver.
  * Adds the vendor's readiness poll (check_and_recovery) before bring-up.
  * No re-enrollment needed: existing prints keep matching.
* Fri Aug 21 2026 PHILIPPDEV5396 - 1.94.10-99.egis4
- Update egis0576 driver to v0.3.1: TLS record bounds hardening.
  Rejects device-supplied record lengths beyond TLS_RECORD_MAX, passes
  real buffer capacities through the receive path, bounds the handshake
  transcript, and validates the ClientHello minimum length. Fixes
  device-controlled stack overflows in the TLS path. Contributed by
  @adventureFAN; hardware-verified on Intel (Yoga 15ITL5) and AMD
  (Yoga 14ARB7) platforms.

* Thu Jul 23 2026 PHILIPPDEV5396 - 1.94.10-99.egis3
- Actually install the suspend/resume integration (systemd-sleep hook + udev
  rule) that egis2 only documented. This is the real fix for "no fingerprint
  after resume": fprintd holds a stale device claim across suspend (a known,
  unfixed gnome-shell/fprintd bug, not a driver bug), so the hook restarts
  fprintd around suspend to clear it. See docs/suspend-resume.md in the repo.
- Driver code unchanged from egis2 (v0.3.0 tag differs only in docs/packaging).

* Tue Jul 21 2026 PHILIPPDEV5396 - 1.94.10-99.egis2
- Update egis0576 driver to v0.2.0: in-driver suspend/resume TLS-session
  recovery. Fingerprint unlock now survives s2idle suspend without hanging
  the lock screen (the driver re-establishes its TLS session on resume;
  read_record has a wall-clock deadline that falls back to password on a
  dead session). Ships the integration/ sleep-hook + udev rule as fallback.

* Sun Jul 19 2026 PHILIPPDEV5396 - 1.94.10-99.egis1
- Rebuild of libfprint 1.94.10 with the out-of-tree EgisTec EH576 (1c7a:0576)
  driver egis0576 compiled in.
