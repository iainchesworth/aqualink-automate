#!/usr/bin/env bash
set -euo pipefail

echo "==> Installing base packages"

apt-get update
apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    curl \
    git \
    gnupg \
    jq \
    libssl-dev \
    linux-libc-dev \
    lsb-release \
    pkg-config \
    software-properties-common \
    tar \
    unzip \
    wget \
    zip

# Mount the root filesystem with `discard` (continuous TRIM) so the thin-
# provisioned vmdk reclaims freed blocks in real time. Churny CI workloads
# (vcpkg dependency builds, CodeQL databases, per-job _work checkouts) write
# then delete tens of GB per run; without continuous discard those freed blocks
# stay allocated in the thin disk until the weekly fstrim.timer fires, so the
# vmdk balloons toward full size and can fill the shared datastore. Takes effect
# on the clone's first boot. Idempotent.
if grep -qE '[[:space:]]/[[:space:]]+ext4[[:space:]]' /etc/fstab \
   && ! grep -qE '[[:space:]]/[[:space:]]+ext4[[:space:]]+[^[:space:]]*discard' /etc/fstab; then
    sed -i -E 's#([[:space:]]/[[:space:]]+ext4[[:space:]]+)defaults#\1defaults,discard#' /etc/fstab
    echo "==> Enabled 'discard' (continuous TRIM) on the root filesystem"
fi

# Run fstrim hourly instead of the weekly default. This is a backstop alongside
# the `discard` mount above: continuous discard reclaims on delete, while the
# hourly timer mops up anything that bypasses it, so the thin vmdk never drifts
# far from actual usage between trims.
mkdir -p /etc/systemd/system/fstrim.timer.d
cat > /etc/systemd/system/fstrim.timer.d/override.conf <<'TIMER'
[Timer]
OnCalendar=
OnCalendar=hourly
AccuracySec=5min
TIMER
systemctl enable fstrim.timer >/dev/null 2>&1 || true
echo "==> Set fstrim.timer to run hourly"

echo "==> Base packages installed"
