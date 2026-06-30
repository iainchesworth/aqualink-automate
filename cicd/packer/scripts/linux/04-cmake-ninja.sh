#!/usr/bin/env bash
set -euo pipefail

CMAKE_VERSION="3.31.12"

echo "==> Installing CMake ${CMAKE_VERSION} and Ninja"

# Install CMake from Kitware binary release
CMAKE_URL="https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz"
curl -fsSL --retry 5 --retry-all-errors --retry-delay 3 --max-time 180 "${CMAKE_URL}" | tar xz -C /opt
ln -sf "/opt/cmake-${CMAKE_VERSION}-linux-x86_64/bin/cmake" /usr/local/bin/cmake
ln -sf "/opt/cmake-${CMAKE_VERSION}-linux-x86_64/bin/ctest" /usr/local/bin/ctest
ln -sf "/opt/cmake-${CMAKE_VERSION}-linux-x86_64/bin/cpack" /usr/local/bin/cpack

# Install Ninja
apt-get install -y --no-install-recommends ninja-build

echo "==> CMake $(cmake --version | head -1) installed"
echo "==> Ninja $(ninja --version) installed"
