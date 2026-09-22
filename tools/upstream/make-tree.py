#!/usr/bin/env python3
"""Lay the egis0576 driver into a libfprint checkout the way it is submitted.

    tools/upstream/make-tree.py <libfprint-checkout> [--tests DIR]

The out-of-tree repository builds three matcher flavours behind one contract;
libfprint gets exactly one, the Gabor front-end, as a flat driver directory
with no build option, no static library and no include directories:

    libfprint/drivers/egis0576.c            driver/egis0576.c
    libfprint/drivers/egis0576.h            driver/egis0576.h
    libfprint/drivers/egis0576/
        egis0576_proto.c, .h                driver/egis0576/egis0576_proto.{c,h}
        egis_init.h                         driver/egis0576/egis_init.h
        egis_engine.h                       driver/egis0576/egis_engine.h
        egis_engine.c                       driver/egis0576/egis_engine_cleanroom.c
                                            (the comparison-flavour #ifdefs stripped)
        egis_match.h                        driver/egis0576/tsteppy/egis_match.h (byte for byte)
        egis_match_gabor.c                  driver/egis0576/gabor/egis_match_gabor.c
        egis_match_check.h                  driver/egis0576/gabor/egis_match_check.h
        egis_cr_tuning.h                    driver/egis0576/gabor/egis_cr_tuning_gabor.h

plus the meson entries (driver sources, drivers_info), the hwdb bookkeeping
(1c7a:0576 off the known-unsupported allowlist, its autosuspend.hwdb entry
under the driver) and, with --tests, tests/egis0576/ and its drivers_tests
entry. The checkout is modified in place; review it with git diff/status.
Everything else -- packaging, integration hooks, the accuracy kit, the other
flavours -- stays out of tree.
"""
import argparse
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, '..', '..'))


def read(p):
    with open(p, encoding='utf-8') as f:
        return f.read()


def write(p, s):
    os.makedirs(os.path.dirname(p), exist_ok=True)
    with open(p, 'w', encoding='utf-8') as f:
        f.write(s)


def must_replace(s, old, new, what):
    if old not in s:
        sys.exit(f'make-tree: cannot find {what}')
    return s.replace(old, new, 1)


def adapter(src):
    """egis_engine_cleanroom.c -> egis_engine.c: one front-end, flat includes."""
    s = read(src)
    # the comparison-flavour selection block becomes the two includes
    s = must_replace(
        s,
        '#ifndef EGIS_CR_FRONTEND_TSTEPPY\n'
        '#include "gabor/egis_cr_tuning_gabor.h"\n'
        '#include "gabor/egis_match_check.h"\n'
        '#define EGIS_CR_HAVE_MATCH_EX 1\n'
        '#endif\n',
        '#include "egis_cr_tuning.h"\n'
        '#include "egis_match_check.h"\n',
        'the front-end selection block in egis_engine_cleanroom.c')
    # its comment paragraph
    s = re.sub(r'/\* The out-of-tree repository also builds this adapter on the original\n'
               r'(?: \*.*\n)*? \* point\. Only the Gabor build is submitted\. \*/\n',
               '', s, count=1)
    # the steering block is unconditional
    if s.count('#ifdef EGIS_CR_HAVE_MATCH_EX') != 1:
        sys.exit('make-tree: expected exactly one EGIS_CR_HAVE_MATCH_EX block')
    i = s.index('#ifdef EGIS_CR_HAVE_MATCH_EX\n')
    j = s.index('#endif\n', i)
    s = s[:i] + s[i + len('#ifdef EGIS_CR_HAVE_MATCH_EX\n'):j] + s[j + len('#endif\n'):]
    s = s.replace('egis_engine_cleanroom.c', 'egis_engine.c')
    return s


def meson_libfprint(s):
    entry = ("    'egis0576' : files(\n"
             "        'drivers/egis0576.c',\n"
             "        'drivers/egis0576/egis0576_proto.c',\n"
             "        'drivers/egis0576/egis_engine.c',\n"
             "        'drivers/egis0576/egis_match_gabor.c',\n"
             "    ),\n")
    if "'egis0576' : files(" in s:
        return s
    return must_replace(s, "    'egis0570' : files('drivers/egis0570.c'),\n",
                        "    'egis0570' : files('drivers/egis0570.c'),\n" + entry,
                        "the egis0570 entry of libfprint/meson.build")


def meson_root(s):
    if "'egis0576': {}," in s:
        return s
    return must_replace(s, "    'egis0570': {},\n", "    'egis0570': {},\n    'egis0576': {},\n",
                        "the egis0570 entry of drivers_info")


def hwdb_list(s):
    return s.replace('  { .vid = 0x1c7a, .pid = 0x0576 },\n', '')


def hwdb_data(s):
    s = s.replace('usb:v1C7Ap0575*\nusb:v1C7Ap0576*\n', 'usb:v1C7Ap0575*\n')
    block = ('# Supported by libfprint driver egis0576\n'
             'usb:v1C7Ap0576*\n'
             ' ID_AUTOSUSPEND=1\n'
             ' ID_PERSIST=0\n\n')
    if block in s:
        return s
    anchor = ('# Supported by libfprint driver egis0570\n'
              'usb:v1C7Ap0570*\nusb:v1C7Ap0571*\n ID_AUTOSUSPEND=1\n ID_PERSIST=0\n\n')
    return must_replace(s, anchor, anchor + block, 'the egis0570 block of data/autosuspend.hwdb')


def tests_meson(s):
    if "'egis0576'" in s:
        return s
    # the recording replays ~two thousand frames through the matcher; give it room
    return must_replace(s, "    'elanspi': { 'timeout': 30 },\n",
                        "    'egis0576': { 'timeout': 60 },\n    'elanspi': { 'timeout': 30 },\n",
                        "the elanspi entry of tests/meson.build drivers_tests")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('checkout', help='a libfprint git checkout (modified in place)')
    ap.add_argument('--tests', default=None, help='directory with custom.py, device and custom.pcapng to install as tests/egis0576/')
    a = ap.parse_args()
    lf = os.path.abspath(a.checkout)
    if not os.path.isfile(os.path.join(lf, 'libfprint', 'meson.build')):
        sys.exit(f'{lf} is not a libfprint checkout')
    d = os.path.join(REPO, 'driver')
    out = os.path.join(lf, 'libfprint', 'drivers')

    copies = {
        'egis0576.c': os.path.join(d, 'egis0576.c'),
        'egis0576.h': os.path.join(d, 'egis0576.h'),
        'egis0576/egis0576_proto.c': os.path.join(d, 'egis0576', 'egis0576_proto.c'),
        'egis0576/egis0576_proto.h': os.path.join(d, 'egis0576', 'egis0576_proto.h'),
        'egis0576/egis_init.h': os.path.join(d, 'egis0576', 'egis_init.h'),
        'egis0576/egis_engine.h': os.path.join(d, 'egis0576', 'egis_engine.h'),
        'egis0576/egis_match.h': os.path.join(d, 'egis0576', 'tsteppy', 'egis_match.h'),
        'egis0576/egis_match_gabor.c': os.path.join(d, 'egis0576', 'gabor', 'egis_match_gabor.c'),
        'egis0576/egis_match_check.h': os.path.join(d, 'egis0576', 'gabor', 'egis_match_check.h'),
        'egis0576/egis_cr_tuning.h': os.path.join(d, 'egis0576', 'gabor', 'egis_cr_tuning_gabor.h'),
    }
    for rel, src in copies.items():
        s = read(src)
        s = s.replace('egis_cr_tuning_gabor.h', 'egis_cr_tuning.h')
        write(os.path.join(out, rel), s)
    write(os.path.join(out, 'egis0576', 'egis_engine.c'),
          adapter(os.path.join(d, 'egis0576', 'egis_engine_cleanroom.c')))

    p = os.path.join(lf, 'libfprint', 'meson.build'); write(p, meson_libfprint(read(p)))
    p = os.path.join(lf, 'meson.build'); write(p, meson_root(read(p)))
    p = os.path.join(lf, 'libfprint', 'fprint-list-udev-hwdb.c'); write(p, hwdb_list(read(p)))
    p = os.path.join(lf, 'data', 'autosuspend.hwdb'); write(p, hwdb_data(read(p)))

    if a.tests:
        tdir = os.path.join(lf, 'tests', 'egis0576')
        os.makedirs(tdir, exist_ok=True)
        for f in ('custom.py', 'device', 'custom.pcapng'):
            src = os.path.join(a.tests, f)
            if os.path.exists(src):
                shutil.copyfile(src, os.path.join(tdir, f))
        p = os.path.join(lf, 'tests', 'meson.build'); write(p, tests_meson(read(p)))

    # nothing submitted may still name the flavours or the other front-end's files
    bad = re.compile(r'EGIS_CR_FRONTEND_TSTEPPY|EGIS_CR_HAVE_MATCH_EX|gabor/|tsteppy/egis_match\.c|egis0576_matcher')
    for rel in list(copies) + ['egis0576/egis_engine.c']:
        for n, line in enumerate(read(os.path.join(out, rel)).splitlines(), 1):
            if bad.search(line):
                print(f'note: {rel}:{n}: {line.strip()}', file=sys.stderr)
    print(f'egis0576 laid into {lf}: {len(copies) + 1} driver files, meson, hwdb'
          + (', tests' if a.tests else ''))


if __name__ == '__main__':
    main()
