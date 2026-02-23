#!/usr/bin/env bash
set -euo pipefail

echo "==> Installing development tools"

apt-get install -y --no-install-recommends \
    python3 \
    python3-pip \
    python3-venv \
    autoconf \
    automake \
    libtool

# Install gcovr via pip
python3 -m pip install --break-system-packages gcovr

echo "==> gcovr $(gcovr --version | head -1) installed"
echo "==> Development tools installed"
