#!/bin/bash
# systemd system-sleep hook: keep the EgisTec EH576 fingerprint sensor working
# across suspend/resume.
#
# WHY: this driver opens a TLS-PSK session to the sensor at device-open. Across a
# system suspend -- especially s2idle, where the USB device stays powered and is
# NOT re-enumerated -- that session goes stale. On resume the next fprintd
# IDENTIFY blocks on the dead session, which on some systems (observed on AMD
# Rembrandt laptops using s2idle) hangs the GNOME/KDE unlock screen hard.
#
# FIX: stop fprintd before suspend (cancels any in-flight operation cleanly) and
# force a USB re-enumeration of the sensor after resume, so the driver performs a
# fresh handshake on next use. Fingerprint stays enabled everywhere (login, sudo,
# unlock).
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
        # cancel any in-flight verify so it can't survive into resume half-dead
        systemctl stop fprintd.service 2>/dev/null
        ;;
    post)
        dev=$(find_dev)
        [ -n "$dev" ] || exit 0
        # deauthorize + reauthorize = USB re-enumeration -> the driver re-opens
        # the device and performs a fresh TLS-PSK handshake on next use
        echo 0 > "$dev/authorized" 2>/dev/null
        sleep 1
        echo 1 > "$dev/authorized" 2>/dev/null
        logger -t egis0576-fp-resume "EH576 re-enumerated after resume ($dev)" 2>/dev/null
        ;;
esac
exit 0
