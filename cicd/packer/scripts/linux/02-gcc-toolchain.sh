#!/usr/bin/env bash
set -euo pipefail

echo "==> Installing GCC 14 toolchain"

add-apt-repository -y ppa:ubuntu-toolchain-r/test
apt-get update
apt-get install -y --no-install-recommends \
    gcc-14 \
    g++-14

# Set up alternatives so gcc/g++ point to version 14
update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-14 140 \
    --slave /usr/bin/g++ g++ /usr/bin/g++-14
update-alternatives --set gcc /usr/bin/gcc-14

echo "==> GCC $(gcc --version | head -1) installed"
