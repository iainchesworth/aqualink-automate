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

# Verify prerequisites
command -v xorriso >/dev/null 2>&1 || { echo "ERROR: xorriso is required but not installed"; exit 1; }
[ -f "$SOURCE_ISO" ] || { echo "ERROR: Source ISO not found: $SOURCE_ISO"; exit 1; }
[ -f "$SCRIPT_DIR/http/linux/user-data" ] || { echo "ERROR: user-data not found"; exit 1; }

echo "==> Preparing autoinstall files"
mkdir -p "$WORK_DIR/autoinstall"
cp "$SCRIPT_DIR/http/linux/user-data" "$WORK_DIR/autoinstall/user-data"
cp "$SCRIPT_DIR/http/linux/meta-data" "$WORK_DIR/autoinstall/meta-data"

echo "==> Extracting GRUB config from ISO"
xorriso -osirrox on -indev "$SOURCE_ISO" \
    -extract /boot/grub/grub.cfg "$WORK_DIR/grub.cfg" \
    2>/dev/null
[ -f "$WORK_DIR/grub.cfg" ] || { echo "ERROR: Failed to extract grub.cfg from ISO"; exit 1; }

echo "==> Modifying GRUB configuration"
cp "$WORK_DIR/grub.cfg" "$WORK_DIR/grub.cfg.orig"

# Set GRUB timeout to 1 second (auto-boot)
sed -i 's/^set timeout=.*/set timeout=1/' "$WORK_DIR/grub.cfg"

# Add autoinstall parameter to the kernel line.
# Match either /casper/vmlinuz or /casper/hwe-vmlinuz (varies by release).
sed -i 's|^\(\s*linux\s\+/casper/[a-z-]*vmlinuz\s.*\)---|\1--- autoinstall ds=nocloud\\;s=/cdrom/autoinstall/|' "$WORK_DIR/grub.cfg"

# Verify the modifications actually took effect
if ! grep -q 'autoinstall' "$WORK_DIR/grub.cfg"; then
    echo "ERROR: Failed to inject autoinstall parameter into GRUB config"
    echo "--- Original kernel lines ---"
    grep 'linux.*vmlinuz' "$WORK_DIR/grub.cfg.orig" | head -5
    echo "--- Modified kernel lines ---"
    grep 'linux.*vmlinuz' "$WORK_DIR/grub.cfg" | head -5
    exit 1
fi

if ! grep -q 'set timeout=1' "$WORK_DIR/grub.cfg"; then
    echo "ERROR: Failed to set GRUB timeout"
    exit 1
fi

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
