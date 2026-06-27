#!/bin/sh
# Aqualink Automate - RPM %preun  ($1 = 0 on final erase, >=1 on upgrade)
set -e

SERVICE=aqualink-automate.service

if [ "$1" -eq 0 ]; then
    # Final removal (not an upgrade): stop + disable.
    if [ -d /run/systemd/system ]; then
        systemctl stop "$SERVICE" >/dev/null 2>&1 || true
        systemctl disable "$SERVICE" >/dev/null 2>&1 || true
    fi
fi

exit 0
