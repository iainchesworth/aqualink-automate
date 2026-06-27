#!/usr/bin/env bash
#
# Repack an Ubuntu Server ISO with autoinstall configuration.
# Creates a new ISO that boots and installs fully automatically.
#
# The source ISO is downloaded on demand (and cached under ISOs/) rather than
# kept in the repo. Run clean-isos.ps1 to reclaim the disk when not building.
#
# Usage:
#   ./repack-iso.sh                            # download the default Ubuntu source + repack
#   ./repack-iso.sh <source.iso|URL> [output.iso]
#
# Requires: xorriso, curl, sha256sum
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ISO_DIR="$SCRIPT_DIR/ISOs"

# Default upstream source — Ubuntu 26.04 LTS (Resolute Raccoon), a current LTS
# served straight from releases.ubuntu.com. Keep the checksum in step with the
# URL — a mismatch aborts the repack. When a point release lands (26.04.1, …) the
# upstream GA filename is replaced; update both the URL and SHA256 to match (grab
# them from https://releases.ubuntu.com/26.04/SHA256SUMS).
DEFAULT_ISO_URL="https://releases.ubuntu.com/26.04/ubuntu-26.04-live-server-amd64.iso"
DEFAULT_ISO_SHA256="dec49008a71f6098d0bcfc822021f4d042d5f2db279e4d75bdd981304f1ca5d9"
DEFAULT_OUTPUT_ISO="$ISO_DIR/ubuntu-26.04-autoinstall.iso"

SOURCE_ARG="${1:-$DEFAULT_ISO_URL}"

verify_sha256() {
    # $1=file $2=expected-hash; succeeds (no-op) when no hash is supplied.
    local file="$1" expected="$2"
    [ -n "$expected" ] || return 0
    echo "${expected}  ${file}" | sha256sum -c --status -
}

fetch_iso() {
    # Download $1 into ISOs/, reusing a cached copy whose checksum ($2) matches.
    # Sets RESOLVED_SOURCE to the local path.
    local url="$1" expected="$2" name dest
    name="$(basename "${url%%\?*}")"
    dest="$ISO_DIR/$name"
    mkdir -p "$ISO_DIR"

    if [ -f "$dest" ] && verify_sha256 "$dest" "$expected"; then
        echo "==> Using cached source ISO: $dest"
        RESOLVED_SOURCE="$dest"
        return 0
    fi
    if [ -f "$dest" ] && [ -n "$expected" ]; then
        echo "==> Cached ISO failed checksum; re-downloading"
    fi

    command -v curl >/dev/null 2>&1 || { echo "ERROR: curl is required to download ISOs"; exit 1; }
    echo "==> Downloading source ISO"
    echo "    from: $url"
    echo "    to:   $dest"
    # Resume an interrupted download; fall back to a clean fetch if resume fails
    # (e.g. a complete-but-stale file makes the server answer 416).
    curl -fL -C - -o "$dest" "$url" || { echo "==> Resume failed; restarting download"; rm -f "$dest"; curl -fL -o "$dest" "$url"; }

    if [ -n "$expected" ]; then
        if verify_sha256 "$dest" "$expected"; then
            echo "==> Download verified (SHA256 OK)"
        else
            echo "ERROR: SHA256 mismatch for downloaded ISO: $dest"
            echo "       expected: $expected"
            echo "       actual:   $(sha256sum "$dest" | awk '{print $1}')"
            exit 1
        fi
    else
        echo "WARNING: no known SHA256 for this URL; skipping verification"
    fi
    RESOLVED_SOURCE="$dest"
}

case "$SOURCE_ARG" in
    http://*|https://*)
        EXPECTED_SHA256=""
        [ "$SOURCE_ARG" = "$DEFAULT_ISO_URL" ] && EXPECTED_SHA256="$DEFAULT_ISO_SHA256"
        fetch_iso "$SOURCE_ARG" "$EXPECTED_SHA256"
        SOURCE_ISO="$RESOLVED_SOURCE"
        ;;
    *)
        SOURCE_ISO="$SOURCE_ARG"
        ;;
esac

# Output ISO: an explicit $2 wins; the default source maps to the conventional
# name Packer expects; a custom local source derives the name beside it.
if [ -n "${2:-}" ]; then
    OUTPUT_ISO="$2"
elif [ "$SOURCE_ARG" = "$DEFAULT_ISO_URL" ]; then
    OUTPUT_ISO="$DEFAULT_OUTPUT_ISO"
else
    OUTPUT_ISO="${SOURCE_ISO%.iso}-autoinstall.iso"
fi

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
