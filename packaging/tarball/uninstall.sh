#!/bin/sh
# Uninstall an Aqualink Automate tarball install (the reverse of install.sh).
# Usage:  sudo ./uninstall.sh            # keeps /etc config + state
#         sudo AQ_PURGE=1 ./uninstall.sh # also removes config + state
set -e

DEST="${AQ_PREFIX:-/opt/aqualink-automate}"

[ "$(id -u)" -eq 0 ] || { echo "Please run as root (sudo)." >&2; exit 1; }

if [ -d /run/systemd/system ]; then
    systemctl stop aqualink-automate.service >/dev/null 2>&1 || true
    systemctl disable aqualink-automate.service >/dev/null 2>&1 || true
fi
rm -f /etc/systemd/system/aqualink-automate.service
[ -d /run/systemd/system ] && systemctl daemon-reload >/dev/null 2>&1 || true

rm -rf "$DEST"

if [ "${AQ_PURGE:-0}" = "1" ]; then
    rm -rf /etc/aqualink-automate /var/lib/aqualink-automate
    echo "Removed application, config, and state. The 'aqualink' service account was left in place."
else
    echo "Removed application + service unit. Kept /etc/aqualink-automate and /var/lib/aqualink-automate."
    echo "Run with AQ_PURGE=1 to remove those too."
fi
