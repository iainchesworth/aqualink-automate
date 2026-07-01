#!/bin/sh
# Aqualink Automate - RPM %post  ($1 = 1 on first install, >=2 on upgrade)
set -e

SERVICE=aqualink-automate.service
USER=aqualink
GROUP=aqualink
STATE_DIR=/var/lib/aqualink-automate
SYSUSERS=/usr/lib/sysusers.d/aqualink-automate.conf

if command -v systemd-sysusers >/dev/null 2>&1 && [ -f "$SYSUSERS" ]; then
    systemd-sysusers "$SYSUSERS" >/dev/null 2>&1 || true
fi
if ! getent group "$GROUP" >/dev/null 2>&1; then
    groupadd --system "$GROUP" >/dev/null 2>&1 || true
fi
if ! getent passwd "$USER" >/dev/null 2>&1; then
    useradd --system --gid "$GROUP" --no-create-home --home-dir "$STATE_DIR" \
            --shell /sbin/nologin --comment "Aqualink Automate" "$USER" >/dev/null 2>&1 || true
fi
if getent group dialout >/dev/null 2>&1; then
    usermod --append --groups dialout "$USER" >/dev/null 2>&1 || true
fi
mkdir -p "$STATE_DIR"
chown "$USER":"$GROUP" "$STATE_DIR" 2>/dev/null || true

if command -v udevadm >/dev/null 2>&1; then
    udevadm control --reload-rules >/dev/null 2>&1 || true
fi

if [ -d /run/systemd/system ]; then
    systemctl daemon-reload >/dev/null 2>&1 || true
    systemctl enable "$SERVICE" >/dev/null 2>&1 || true
    if [ "$1" -ge 2 ]; then
        # Upgrade: restart only if already running.
        if systemctl is-active --quiet "$SERVICE"; then
            systemctl restart "$SERVICE" >/dev/null 2>&1 || true
        fi
    else
        echo "Aqualink Automate is installed and will start on boot."
        echo "  1. Set your RS-485 device in /etc/aqualink-automate/aqualink-automate.conf"
        echo "  2. sudo systemctl start aqualink-automate"
        echo "  Web UI: http://<this-host>:9000   Logs: journalctl -u aqualink-automate -f"
    fi
fi

exit 0
