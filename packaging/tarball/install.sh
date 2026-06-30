#!/bin/sh
# Install Aqualink Automate (systemd service + /etc config) from an extracted
# tarball. The .deb/.rpm do this for you - this script is for the .tar.gz.
#
# Usage (from the extracted tarball root, as root):
#       sudo ./install.sh            # installs to /opt/aqualink-automate
#       sudo AQ_PREFIX=/srv/aqua ./install.sh
set -e

HERE="$(cd "$(dirname "$0")" && pwd)"
PKG="$HERE/share/aqualink-automate/packaging"
DEST="${AQ_PREFIX:-/opt/aqualink-automate}"
USER=aqualink
GROUP=aqualink
STATE_DIR=/var/lib/aqualink-automate

[ "$(id -u)" -eq 0 ] || { echo "Please run as root (sudo)." >&2; exit 1; }
[ -x "$HERE/bin/aqualink-automate" ] || { echo "Run this from the extracted tarball root (bin/aqualink-automate not found)." >&2; exit 1; }
[ -d "$PKG" ] || { echo "Packaging files not found at $PKG." >&2; exit 1; }

echo "Installing application tree to $DEST ..."
mkdir -p "$DEST"
cp -a "$HERE"/. "$DEST"/
rm -f "$DEST/install.sh" "$DEST/uninstall.sh"

echo "Creating service account '$USER' ..."
if command -v systemd-sysusers >/dev/null 2>&1 && [ -f "$PKG/sysusers.d/aqualink-automate.conf" ]; then
    install -m 0644 "$PKG/sysusers.d/aqualink-automate.conf" /usr/lib/sysusers.d/aqualink-automate.conf
    systemd-sysusers /usr/lib/sysusers.d/aqualink-automate.conf >/dev/null 2>&1 || true
fi
getent group "$GROUP" >/dev/null 2>&1 || groupadd --system "$GROUP" >/dev/null 2>&1 || true
getent passwd "$USER" >/dev/null 2>&1 || useradd --system --gid "$GROUP" --no-create-home \
    --home-dir "$STATE_DIR" --shell /usr/sbin/nologin --comment "Aqualink Automate" "$USER" >/dev/null 2>&1 || true
getent group dialout >/dev/null 2>&1 && usermod --append --groups dialout "$USER" >/dev/null 2>&1 || true
mkdir -p "$STATE_DIR" && chown "$USER":"$GROUP" "$STATE_DIR" 2>/dev/null || true

echo "Installing /etc/aqualink-automate/aqualink-automate.conf ..."
mkdir -p /etc/aqualink-automate
if [ -f /etc/aqualink-automate/aqualink-automate.conf ]; then
    echo "  (keeping your existing config)"
else
    install -m 0640 "$PKG/config/aqualink-automate.conf" /etc/aqualink-automate/aqualink-automate.conf
    chgrp "$GROUP" /etc/aqualink-automate/aqualink-automate.conf 2>/dev/null || true
fi

echo "Installing systemd unit (ExecStart -> $DEST/bin) ..."
sed "s|/usr/bin/aqualink-automate|$DEST/bin/aqualink-automate|" \
    "$PKG/systemd/aqualink-automate.service" > /etc/systemd/system/aqualink-automate.service

if [ -d /run/systemd/system ]; then
    systemctl daemon-reload
    systemctl enable aqualink-automate.service >/dev/null 2>&1 || true
fi

cat <<EOF

Aqualink Automate installed to $DEST.
  1. Edit /etc/aqualink-automate/aqualink-automate.conf  (set serial-port = ...)
  2. sudo systemctl start aqualink-automate
  Web UI: http://<this-host>:9000   Logs: journalctl -u aqualink-automate -f
  Uninstall: sudo $DEST/share/aqualink-automate/packaging/tarball/uninstall.sh
EOF
