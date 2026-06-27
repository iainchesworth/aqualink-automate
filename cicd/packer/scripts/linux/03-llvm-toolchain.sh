#!/usr/bin/env bash
set -euo pipefail

LLVM_VERSION=21

echo "==> Installing LLVM/Clang ${LLVM_VERSION} toolchain"

# Add the LLVM apt signing key (retry — apt.llvm.org is frequently flaky and a
# single transient error here used to abort the whole build).
curl -fsSL --retry 5 --retry-all-errors --retry-delay 3 --max-time 60 \
    https://apt.llvm.org/llvm-snapshot.gpg.key \
    | gpg --dearmor -o /usr/share/keyrings/llvm-archive-keyring.gpg

CODENAME=$(lsb_release -cs)

# Probe the ACTUAL per-version repo, with retries. The bare /<codename>/ URL
# returns 200 even for releases that have no LLVM repo (a false signal that the
# old check relied on), so probe the real dist Release file instead. Fall back to
# the nearest LTS (noble) only if the running codename genuinely lacks an
# LLVM ${LLVM_VERSION} repo — those packages install fine on a newer Ubuntu.
repo_has_llvm() {
    curl -fsS --retry 5 --retry-all-errors --retry-delay 3 --max-time 30 --head \
        "https://apt.llvm.org/$1/dists/llvm-toolchain-$1-${LLVM_VERSION}/Release" \
        >/dev/null 2>&1
}

if ! repo_has_llvm "$CODENAME"; then
    echo "WARNING: no LLVM ${LLVM_VERSION} repo for '${CODENAME}', falling back to 'noble'"
    CODENAME="noble"
    if ! repo_has_llvm "$CODENAME"; then
        echo "ERROR: no LLVM ${LLVM_VERSION} repo for '${CODENAME}' either; see https://apt.llvm.org/"
        exit 1
    fi
fi

echo "deb [signed-by=/usr/share/keyrings/llvm-archive-keyring.gpg] https://apt.llvm.org/${CODENAME}/ llvm-toolchain-${CODENAME}-${LLVM_VERSION} main" \
    > /etc/apt/sources.list.d/llvm.list

# apt-get retries are configured globally in 01-base-packages.sh
# (/etc/apt/apt.conf.d/80-retries), so these survive transient apt.llvm.org errors.
apt-get update
apt-get install -y --no-install-recommends \
    "clang-${LLVM_VERSION}" \
    "clang-tidy-${LLVM_VERSION}" \
    "lld-${LLVM_VERSION}" \
    "libc++-${LLVM_VERSION}-dev" \
    "libc++abi-${LLVM_VERSION}-dev"

# Set up alternatives
update-alternatives --install /usr/bin/clang clang "/usr/bin/clang-${LLVM_VERSION}" 100
update-alternatives --install /usr/bin/clang++ clang++ "/usr/bin/clang++-${LLVM_VERSION}" 100
update-alternatives --install /usr/bin/clang-tidy clang-tidy "/usr/bin/clang-tidy-${LLVM_VERSION}" 100
update-alternatives --install /usr/bin/lld lld "/usr/bin/lld-${LLVM_VERSION}" 100

echo "==> Clang $(clang --version | head -1) installed"
