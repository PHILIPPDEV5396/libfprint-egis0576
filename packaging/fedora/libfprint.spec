# COPR spec: a rebuild of libfprint with the out-of-tree EgisTec EH576 (egis0576)
# driver compiled in. It keeps the package name `libfprint` so it is a DROP-IN
# replacement that the stock fprintd loads unchanged (same soname libfprint-2.so.2).
#
# Built from PRISTINE upstream libfprint v1.94.10 + the egis0576 driver.
#
# NOTE (tradeoff): this build does NOT carry Fedora's downstream patches — most
# notably Fedora's own `egis_etu905` driver (USB 1c7a:05ae / 1c7a:9201). That only
# matters if you ALSO own one of those (rare) readers; for the EH576 this is a
# complete, working libfprint. If you need a true SUPERSET on top of Fedora's own
# package (keeping egis_etu905 + Fedora's fixes), see ./README.md and the tested
# rebased patch libfprint-1.94.10-egis0576-fedora.patch in this directory.

%global egis_tag v0.1.0

Name:           libfprint
Version:        1.94.10
# Release sorts ABOVE Fedora's current 5.fcNN so `dnf upgrade` prefers this build.
# The robust backstop is a COPR repo *priority* (see README) which wins regardless
# of version. When Fedora ships a NEWER libfprint VERSION, rebase onto it (Fedora
# then legitimately wins and you bump this spec's Version).
Release:        99%{?dist}.egis1
Summary:        Toolkit for fingerprint scanner (rebuilt with the EgisTec EH576 / 1c7a:0576 driver)

License:        LGPL-2.1-or-later AND NIST-PD
URL:            https://github.com/PHILIPPDEV5396/libfprint-egis0576
Source0:        https://gitlab.freedesktop.org/libfprint/libfprint/-/archive/v%{version}/libfprint-v%{version}.tar.gz
Source10:       https://github.com/PHILIPPDEV5396/libfprint-egis0576/archive/refs/tags/%{egis_tag}.tar.gz#/libfprint-egis0576-%{egis_tag}.tar.gz

# --- BuildRequires: copied verbatim from Fedora's own libfprint.spec (known-good
#     for 1.94.10). openssl-devel already covers the egis0576 engine's libcrypto. ---
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
%meson -Ddrivers=all
%meson_build

%install
%meson_install

%ldconfig_scriptlets

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
* Sun Jul 19 2026 PHILIPPDEV5396 - 1.94.10-99.egis1
- Rebuild of libfprint 1.94.10 with the out-of-tree EgisTec EH576 (1c7a:0576)
  driver egis0576 compiled in.
