#!/usr/bin/env python3
# egis0576 driver test: enroll (12 presses), verify (match), identify (match),
# verify against a second, different object (no match), in one open session.
#
# Recorded with two textured, non-finger objects (see README): object A for the
# enrolment and the matching verify/identify, object B for the rejected verify.
# The driver's decisions (finger-on/off, the matcher's accept, the enrolment
# steering) are deterministic for a given recording and build configuration,
# so the replay consumes exactly the recorded frames.

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

def enroll_progress(dev, completed, print_, error):
    print("finger status: ", dev.get_finger_status())
    if error:
        print(f"enroll progress: {completed}/12 -- {error.message} (adjust object A and press again)")
    else:
        print(f"enroll progress: {completed}/12")

print("enrolling: press object A twelve times, shifting it a little each time")
template = FPrint.Print.new(d)
template.set_finger(FPrint.Finger.LEFT_INDEX)
assert d.get_finger_status() == FPrint.FingerStatusFlags.NONE
p1 = d.enroll_sync(template, None, enroll_progress, None)
assert d.get_finger_status() == FPrint.FingerStatusFlags.NONE
print("enroll done")
del template

assert p1.get_driver() == "egis0576"
assert p1.get_device_id() == d.get_device_id()

print("verifying: press object A")
assert d.get_finger_status() == FPrint.FingerStatusFlags.NONE
verify_res, verify_print = d.verify_sync(p1)
assert d.get_finger_status() == FPrint.FingerStatusFlags.NONE
print("verify done: ", verify_res)
assert verify_res == True

# the print survives a serialize/deserialize round trip and still matches
p1s = FPrint.Print.deserialize(p1.serialize())
assert p1s.equal(p1)

identified = False

def identify_done(dev, res):
    global identified
    identified = True
    identify_match, identify_print = dev.identify_finish(res)
    print('identification done: ', identify_match, identify_print)
    assert identify_match is not None
    assert identify_match.equal(p1s)

print("identifying: press object A")
assert d.get_finger_status() == FPrint.FingerStatusFlags.NONE
d.identify([p1s], callback=identify_done)
while not identified:
    ctx.iteration(True)
assert d.get_finger_status() == FPrint.FingerStatusFlags.NONE

print("verifying with the other object: press object B")
verify_res, verify_print = d.verify_sync(p1)
assert d.get_finger_status() == FPrint.FingerStatusFlags.NONE
print("verify done: ", verify_res)
assert verify_res == False

# an empty gallery: trivially no match, no sensor access
identify_match, identify_print = d.identify_sync([])
assert identify_match is None

d.close_sync()
print("closed")
