# Ubuntu TOD module build for the egis0576 driver.
#
# Ubuntu's libfprint (the TOD fork, package libfprint-2-2 / libfprint-2-tod1)
# loads a fingerprint driver as a standalone lib*.so from a fixed directory
# instead of linking it into libfprint-2.so.2. This project builds exactly
# that module: the driver and its USB protocol code, unmodified, plus the
# ten-line tod-entry.c shim in this directory.
#
# The driver sources are read from extra-driver/, a symlink to ../../driver
# checked into this repo. build-series.sh stages this whole directory with
# cp -rL before it runs dpkg-buildpackage, which dereferences that symlink
# into real files, so the assembled source package is self-contained: a
# dpkg-source .dsc built from the staged tree carries the driver sources
# with it and needs no sibling checkout to unpack and build. Running
# dpkg-buildpackage directly in this directory (not through
# build-series.sh) instead reads extra-driver/ through the symlink, which
# is only valid inside a full repository checkout.
#
# No fixed rule about building on the oldest supported series applies here:
# each series gets its own build from its own container (see ../README.md),
# so no module is ever asked to load on an older series than it was built
# on.

project('libfprint-tod-egis0576', 'c',
  meson_version: '>= 0.56.0',
)

# A relative path, not one resolved through project_source_root(): meson's
# include_directories() refuses an absolute path outright, so this must stay
# a plain relative string all the way down to the file lists below.
driver_root = 'extra-driver'

tod_dep = dependency('libfprint-2-tod-1')
fprint_dep = dependency('libfprint-2')
gusb_dep = dependency('gusb')

tod_driversdir = tod_dep.get_variable(pkgconfig: 'tod_driversdir')

# The vendor extractor/matcher (egis_funcs.c and friends) is translated,
# reverse-engineered C that intentionally mixes int/pointer types and relies
# on exact stack layout, so it is compiled with the same relaxed flag set the
# repo's own libfprint meson patch (../../patches/libfprint-*-egis0576.patch)
# applies to it. -fpermissive is load-bearing: without it,
# driver/egis0576/egis_funcs.c fails to compile on GCC 14 and later
# ('return' with no value, in function returning non-void) — GCC 15 on
# Ubuntu 26.04 hits this.
engine_c_args = [
  '-w',
  '-fno-stack-protector',
  '-fpermissive',
  '-fwrapv',
  '-Wno-int-conversion',
  '-Wno-incompatible-pointer-types',
  '-Wno-implicit-function-declaration',
]

egis0576_engine = static_library('egis0576-engine',
  sources: [
    driver_root / 'egis0576' / 'egis_funcs.c',
    driver_root / 'egis0576' / 'egis_rt.c',
    driver_root / 'egis0576' / 'egis_engine.c',
    driver_root / 'egis0576' / 'egis_preprocess.c',
    driver_root / 'egis0576' / 'egis_coherence_map.c',
  ],
  include_directories: include_directories(driver_root / 'egis0576'),
  # tod_dep costs nothing today, since none of these five sources includes
  # a libfprint header, but adding it now means a future source that does
  # add one fails to build normally instead of with a confusing missing
  # fpi-log.h.
  dependencies: [gusb_dep, tod_dep],
  c_args: engine_c_args,
  install: false,
)

# The driver proper and its USB protocol code build under the normal,
# strict flags — only the translated vendor engine above needs the relaxed
# set. The module file name must start with "lib" and end in ".so" or
# Ubuntu's loader skips it silently (tod-shared-loader.c); meson's default
# shared_library prefix/suffix on Linux already produce that, and no
# soversion is set here, so the output is exactly libfprint-tod-egis0576.so,
# not a versioned libfprint-tod-egis0576.so.0.0.0.
shared_library('fprint-tod-egis0576',
  sources: [
    driver_root / 'egis0576.c',
    driver_root / 'egis0576' / 'egis0576_proto.c',
    'tod-entry.c',
  ],
  include_directories: include_directories(driver_root, driver_root / 'egis0576'),
  link_with: egis0576_engine,
  # libfprint-2 (not just libfprint-2-tod-1) is a genuine link dependency:
  # fp_device_get_type, behind FP_TYPE_DEVICE in driver/egis0576.c, is
  # implemented in libfprint-2.so.2 itself, not in libfprint-2-tod.so.1.
  # At load time this is always satisfied anyway, because libfprint-2.so.2
  # is what dlopen()s this module in the first place; linking it directly
  # only matters here because dpkg's default -Wl,--no-undefined requires
  # every reference resolved at link time.
  dependencies: [tod_dep, fprint_dep, gusb_dep],
  install: true,
  install_dir: tod_driversdir,
)
