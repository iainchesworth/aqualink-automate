#!/usr/bin/env bash
set -euo pipefail

echo "==> Cleaning up for template compression"

# Clean apt cache
apt-get autoremove -y
apt-get clean
rm -rf /var/lib/apt/lists/*

# Truncate log files
find /var/log -type f -name "*.log" -exec truncate -s 0 {} \;

# Clear tmp
rm -rf /tmp/* /var/tmp/*

# Clear bash history
unset HISTFILE
rm -f /root/.bash_history /home/runner/.bash_history

# Zero free space for better template compression
dd if=/dev/zero of=/zero bs=1M 2>/dev/null || true
rm -f /zero
sync

echo "==> Cleanup complete"
