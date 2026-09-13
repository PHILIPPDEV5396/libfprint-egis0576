#!/bin/bash
# systemd system-sleep hook: keep the EgisTec EH576 fingerprint sensor working
# across suspend/resume.
#
# WHY: two independent things go wrong across suspend/resume, neither of them
# session state in this driver (the EGIS/SIGE protocol is stateless):
#  1. fprintd keeps the device *claim* gnome-shell took before suspend and never
#     releases it; the post-resume Claim is refused ("Device was already
#     claimed") and libfprint is never reached -> lock screen offers only the
#     password (upstream gnome-shell #7791 / Ubuntu #2067135, not EH576-specific).
#  2. Across s2idle the USB device stays powered and is NOT re-enumerated, so the
#     sensor keeps its pre-suspend exposure state and the first capture after
#     resume can be badly exposed.
# (The historic hard hang of the unlock screen is fixed inside the driver by the
# bounded timeout on every bulk read -- not by this hook.)
#
# FIX: stop fprintd before suspend so no stale claim survives (fprintd is
# started again on demand on resume), and force a USB re-enumeration of the
# sensor after resume so the driver re-opens it and re-runs the bring-up
# (exposure reset) on next use. Fingerprint stays enabled everywhere (login,
# sudo, unlock).
#
# Install: copy to /usr/lib/systemd/system-sleep/ (or /etc/systemd/system-sleep/)
# and chmod 0755. Delete the file to revert. install.sh does this for you.
set +e

VID=1c7a
PID=0576

find_dev() {
    local v d
    for v in /sys/bus/usb/devices/*/idVendor; do
        [ "$(cat "$v" 2>/dev/null)" = "$VID" ] || continue
        d=$(dirname "$v")
        [ "$(cat "$d/idProduct" 2>/dev/null)" = "$PID" ] && { echo "$d"; return; }
    done
}

case "$1" in
    pre)
        # drop fprintd's stale device claim before we sleep (upstream
        # gnome-shell/fprintd bug); also ends any in-flight operation cleanly
        systemctl stop fprintd.service 2>/dev/null
        ;;
    post)
        dev=$(find_dev)
        [ -n "$dev" ] || exit 0
        # deauthorize + reauthorize = USB re-enumeration -> the driver re-opens
        # the device and re-runs the sensor bring-up on next use
        echo 0 > "$dev/authorized" 2>/dev/null
        sleep 1
        echo 1 > "$dev/authorized" 2>/dev/null
        logger -t egis0576-fp-resume "EH576 re-enumerated after resume ($dev)" 2>/dev/null
        ;;
esac
exit 0
