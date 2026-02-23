#!/usr/bin/env bash
#
# Repack an Ubuntu Server ISO with autoinstall configuration.
# Creates a new ISO that boots and installs fully automatically.
#
# Usage: ./repack-iso.sh <source.iso> [output.iso]
#
# Requires: xorriso
#
set -euo pipefail

SOURCE_ISO="${1:?Usage: $0 <source.iso> [output.iso]}"
OUTPUT_ISO="${2:-${SOURCE_ISO%.iso}-autoinstall.iso}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
WORK_DIR="$(mktemp -d)"

cleanup() { rm -rf "$WORK_DIR"; }
trap cleanup EXIT

echo "==> Preparing autoinstall files"
mkdir -p "$WORK_DIR/autoinstall"
cp "$SCRIPT_DIR/http/linux/user-data" "$WORK_DIR/autoinstall/user-data"
cp "$SCRIPT_DIR/http/linux/meta-data" "$WORK_DIR/autoinstall/meta-data"

echo "==> Extracting GRUB config from ISO"
xorriso -osirrox on -indev "$SOURCE_ISO" \
    -extract /boot/grub/grub.cfg "$WORK_DIR/grub.cfg" \
    2>/dev/null

echo "==> Modifying GRUB configuration"
# Set GRUB timeout to 1 second (auto-boot)
sed -i 's/^set timeout=.*/set timeout=1/' "$WORK_DIR/grub.cfg"

# Add autoinstall parameter to the kernel line
sed -i 's|^\(\s*linux\s\+/casper/vmlinuz\s.*\)---|\1--- autoinstall ds=nocloud\\;s=/cdrom/autoinstall/|' "$WORK_DIR/grub.cfg"

echo "==> Modified GRUB config:"
grep -E '(timeout|linux.*vmlinuz)' "$WORK_DIR/grub.cfg" | head -5

echo "==> Building repacked ISO (preserving boot structures)"
# Clone the original ISO and inject modifications.
# -boot_image any replay preserves the original boot catalog exactly.
xorriso -indev "$SOURCE_ISO" \
    -outdev "$OUTPUT_ISO" \
    -map "$WORK_DIR/autoinstall" /autoinstall \
    -map "$WORK_DIR/grub.cfg" /boot/grub/grub.cfg \
    -boot_image any replay \
    -end

echo "==> Done: $OUTPUT_ISO"
ls -lh "$OUTPUT_ISO"
