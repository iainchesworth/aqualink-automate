#!/usr/bin/env bash
set -euo pipefail

LLVM_VERSION=21

echo "==> Installing LLVM/Clang ${LLVM_VERSION} toolchain"

# Add LLVM apt repository
curl -fsSL https://apt.llvm.org/llvm-snapshot.gpg.key | gpg --dearmor -o /usr/share/keyrings/llvm-archive-keyring.gpg
CODENAME=$(lsb_release -cs)
echo "deb [signed-by=/usr/share/keyrings/llvm-archive-keyring.gpg] https://apt.llvm.org/${CODENAME}/ llvm-toolchain-${CODENAME}-${LLVM_VERSION} main" \
    > /etc/apt/sources.list.d/llvm.list

apt-get update
apt-get install -y --no-install-recommends \
    clang-${LLVM_VERSION} \
    clang-tidy-${LLVM_VERSION} \
    lld-${LLVM_VERSION} \
    libc++-${LLVM_VERSION}-dev \
    libc++abi-${LLVM_VERSION}-dev

# Set up alternatives
update-alternatives --install /usr/bin/clang clang /usr/bin/clang-${LLVM_VERSION} 100
update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-${LLVM_VERSION} 100
update-alternatives --install /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-${LLVM_VERSION} 100
update-alternatives --install /usr/bin/lld lld /usr/bin/lld-${LLVM_VERSION} 100

echo "==> Clang $(clang --version | head -1) installed"
