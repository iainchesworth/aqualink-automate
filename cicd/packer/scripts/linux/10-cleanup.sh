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

# Reset the machine-id so every VM cloned from this template generates its OWN
# unique id on first boot. machine-id(5): "for operating system images which
# are replicated... /etc/machine-id should be empty". A cloned, populated
# machine-id makes systemd-networkd send an identical DHCP client-id, so every
# clone is handed the SAME DHCP lease -> duplicate-IP conflict on the network.
# (Hostname is made unique per clone by the auto-register step in
# 08-github-runner.sh, which sets it from guestinfo.runner_name.)
truncate -s 0 /etc/machine-id
rm -f /var/lib/dbus/machine-id
ln -s /etc/machine-id /var/lib/dbus/machine-id

# Reclaim freed blocks via TRIM/UNMAP so the thin-provisioned virtual disk
# actually deflates on the datastore.
#
# Do NOT zero-fill free space (`dd if=/dev/zero of=/zero`): on a thin vmdk that
# forces VMware to allocate every "free" block, inflating the disk toward its
# full provisioned size (~150 GB) with no automatic deflate. The template then
# clones huge and fills the datastore. `fstrim` issues SCSI UNMAP instead, which
# VMFS6 reclaims, keeping the template — and every VM cloned from it — genuinely
# thin (in-guest usage is only ~20-25 GB).
fstrim -av || true
sync

echo "==> Cleanup complete"
