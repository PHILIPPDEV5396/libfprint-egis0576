#!/usr/bin/env python3
# egis0576 driver test.
#
# The recording behind this script is made with a TEXTURED NON-FINGER OBJECT,
# never a finger: a umockdev capture contains every frame the sensor
# delivered, and this repository is public. That choice decides what the
# test can assert. The driver matches by correlating the ridge texture and
# checking that the probe's local ridge period reproduces the template's, and
# no household object we could find reproduces a fingerprint's ridge-period
# statistics -- the one measured here enrols cleanly (coverage 0.63 against a
# 0.60 gate) and its two presses correlate at 0.947, yet every pair is
# rejected because the ridge-period estimator finds 2 usable blocks of 270:
# its grooves are finer than the 0.27-0.74 mm the estimator can measure.
#
# So this test pins the whole action path -- open with the exposure
# calibration, twelve enrolment stages including the settle loop and the
# placement steering, the gallery load, per-frame scoring, the finger-lift
# report, identify, close -- and it pins the driver REFUSING something that
# is not a finger. What it cannot pin is the successful-match report; that
# path is covered by the hardware runs recorded in the out-of-tree
# repository's docs/matcher-comparison.md.

import traceback
import sys
import gi

gi.require_version('FPrint', '2.0')
from gi.repository import FPrint, GLib

# Exit with error on any exception, included those happening in async callbacks
sys.excepthook = lambda *args: (traceback.print_exception(*args), sys.exit(1))

ctx = GLib.main_context_default()

c = FPrint.Context()
c.enumerate()
devices = c.get_devices()

d = devices[0]
del devices

assert d.get_driver() == "egis0576"
assert d.get_scan_type() == FPrint.ScanType.PRESS
assert d.get_nr_enroll_stages() == 12
assert not d.has_feature(FPrint.DeviceFeature.CAPTURE)
assert not d.has_feature(FPrint.DeviceFeature.STORAGE)
assert d.has_feature(FPrint.DeviceFeature.IDENTIFY)
assert d.has_feature(FPrint.DeviceFeature.VERIFY)

d.open_sync()
print("opened")

# An identify with nothing enrolled must not touch the sensor at all: this is
# what fprintd does before every enrolment to check for duplicates.
identify_match, identify_print = d.identify_sync([])
assert identify_match is None
print("empty-gallery identify done")


def enroll_progress(dev, completed, print_, error):
    assert dev.get_finger_status() & FPrint.FingerStatusFlags.NEEDED
    if error:
        print(f"enroll progress: {completed}/12 -- {error.message}")
    else:
        print(f"enroll progress: {completed}/12")


print("enrolling the object")
template = FPrint.Print.new(d)
template.set_finger(FPrint.Finger.LEFT_INDEX)
assert d.get_finger_status() == FPrint.FingerStatusFlags.NONE
p1 = d.enroll_sync(template, None, enroll_progress, None)
assert d.get_finger_status() == FPrint.FingerStatusFlags.NONE
print("enroll done")
del template

assert p1.get_driver() == "egis0576"
assert p1.get_device_id() == d.get_device_id()
assert p1.get_finger() == FPrint.Finger.LEFT_INDEX

# The template survives a round trip through fprintd's storage format.
p1s = FPrint.Print.deserialize(p1.serialize())
assert p1s.equal(p1)

print("verifying: the object must NOT match its own enrolment")
assert d.get_finger_status() == FPrint.FingerStatusFlags.NONE
verify_res, verify_print = d.verify_sync(p1)
assert d.get_finger_status() == FPrint.FingerStatusFlags.NONE
assert verify_res is False
print("verify done: no match, as it must be")

identified = False


def identify_done(dev, res):
    global identified
    identified = True
    identify_match, identify_print = dev.identify_finish(res)
    print('identification done: ', identify_match)
    assert identify_match is None


print("identifying against that one print, asynchronously")
d.identify([p1s], callback=identify_done)
while not identified:
    ctx.iteration(True)
assert d.get_finger_status() == FPrint.FingerStatusFlags.NONE

# A print from another driver is refused without touching the sensor.
alien = FPrint.Print.new(d)
alien.set_finger(FPrint.Finger.RIGHT_INDEX)
try:
    d.verify_sync(alien)
    assert False, "a print with no egis0576 template must not be accepted for verify"
except GLib.Error as error:
    assert error.matches(FPrint.DeviceError.quark(), FPrint.DeviceError.DATA_INVALID)
print("alien print refused")

d.close_sync()
print("closed")
