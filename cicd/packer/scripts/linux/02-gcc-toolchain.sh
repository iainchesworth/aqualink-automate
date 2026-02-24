#!/usr/bin/env bash
set -euo pipefail

echo "==> Installing GCC 15 toolchain"

add-apt-repository -y ppa:ubuntu-toolchain-r/test
apt-get update
apt-get install -y --no-install-recommends \
    gcc-15 \
    g++-15

# Set up alternatives so gcc/g++/gcov point to version 15
update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-15 150 \
    --slave /usr/bin/g++ g++ /usr/bin/g++-15 \
    --slave /usr/bin/gcov gcov /usr/bin/gcov-15
update-alternatives --set gcc /usr/bin/gcc-15

echo "==> GCC $(gcc --version | head -1) installed"
