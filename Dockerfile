# ==============================================================================
# Stage: base (shared tooling for dev, ci, and runtime builds)
# ==============================================================================

FROM ubuntu:25.04 AS base

ARG GCC_VERSION=15
ARG LLVM_VERSION=21
ARG CMAKE_VERSION=3.31.6

ENV DEBIAN_FRONTEND=noninteractive

# Install build toolchain:
#   - GCC 15 (default in Ubuntu 25.04)
#   - LLVM/Clang 21 from apt.llvm.org (compiler, linker, libc++, clang-tidy)
#   - CMake 3.31+ via official binary (project requires 3.31)
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        autoconf \
        automake \
        ca-certificates \
        ccache \
        curl \
        git \
        gpg \
        libtool \
        linux-libc-dev \
        gcovr \
        ninja-build \
        pkg-config \
        tar \
        unzip \
        wget \
        zip \
    # GCC (available in default Ubuntu 25.04 repos)
        build-essential \
        gcc-${GCC_VERSION} \
        g++-${GCC_VERSION} \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-${GCC_VERSION} ${GCC_VERSION} \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-${GCC_VERSION} ${GCC_VERSION} \
    && update-alternatives --install /usr/bin/gcov gcov /usr/bin/gcov-${GCC_VERSION} ${GCC_VERSION} \
    # LLVM/Clang from apt.llvm.org
    && wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key \
        | tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc > /dev/null \
    && echo "deb http://apt.llvm.org/plucky/ llvm-toolchain-plucky-${LLVM_VERSION} main" \
        > /etc/apt/sources.list.d/llvm.list \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
        clang-${LLVM_VERSION} \
        clang-tidy-${LLVM_VERSION} \
        lld-${LLVM_VERSION} \
        libc++-${LLVM_VERSION}-dev \
        libc++abi-${LLVM_VERSION}-dev \
    && update-alternatives --install /usr/bin/clang clang /usr/bin/clang-${LLVM_VERSION} ${LLVM_VERSION} \
    && update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-${LLVM_VERSION} ${LLVM_VERSION} \
    && update-alternatives --install /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-${LLVM_VERSION} ${LLVM_VERSION} \
    && update-alternatives --install /usr/bin/lld lld /usr/bin/lld-${LLVM_VERSION} ${LLVM_VERSION} \
    && update-alternatives --install /usr/bin/ld.lld ld.lld /usr/bin/ld.lld-${LLVM_VERSION} ${LLVM_VERSION} \
    # CMake via official binary release
    && wget -qO- "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz" \
        | tar xz -C /usr/local --strip-components=1 \
    # Cleanup
    && rm -rf /var/lib/apt/lists/*

ENV CCACHE_DIR=/ccache
ENV CMAKE_C_COMPILER_LAUNCHER=ccache
ENV CMAKE_CXX_COMPILER_LAUNCHER=ccache

# ==============================================================================
# Stage: dev (development container — source bind-mounted at runtime)
# ==============================================================================

FROM base AS dev

RUN apt-get update \
    && apt-get install -y --no-install-recommends sudo \
    && rm -rf /var/lib/apt/lists/* \
    && groupmod --new-name dev ubuntu \
    && usermod --login dev --home /home/dev --move-home ubuntu \
    && echo "dev ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/dev \
    && mkdir -p /ccache /vcpkg-cache /src \
    && chown dev:dev /ccache /vcpkg-cache /src \
    && git config --system --add safe.directory /src

RUN ccache --set-config=max_size=2G \
    && ccache --set-config=compression=true

ENV VCPKG_DEFAULT_BINARY_CACHE=/vcpkg-cache

WORKDIR /src

USER dev

CMD ["bash"]

# ==============================================================================
# Stage: ci (build + test + clang-tidy verification)
# ==============================================================================

FROM base AS ci

# Version to stamp into the binary. The build context intentionally omits .git
# (see .dockerignore), so `git describe` cannot derive the version inside the
# container; the release workflow passes the resolved tag via --build-arg
# VERSION=v<M>.<M>.<P>[-prerelease]. Empty (local builds) => 0.0.0-dev fallback.
ARG VERSION=""

WORKDIR /src
COPY . .

# Bootstrap vcpkg
RUN ./deps/vcpkg/bootstrap-vcpkg.sh -disableMetrics

# Configure (with ccache and vcpkg binary caching)
RUN --mount=type=cache,target=/ccache \
    --mount=type=cache,target=/vcpkg-cache \
    --mount=type=cache,target=/vcpkg-downloads \
    VCPKG_DEFAULT_BINARY_CACHE=/vcpkg-cache \
    VCPKG_DOWNLOADS=/vcpkg-downloads \
    cmake --preset config-linux-gcc \
        -DDERIVED_VERSION_OVERRIDE="${VERSION}" \
        -DCMAKE_C_COMPILER_LAUNCHER=ccache \
        -DCMAKE_CXX_COMPILER_LAUNCHER=ccache

# Build
RUN --mount=type=cache,target=/ccache \
    cmake --build --preset build-linux-gcc

# Test
RUN ctest --preset test-linux-gcc

# Install (creates install tree for runtime stage)
RUN cmake --install build/config-linux-gcc

# ==============================================================================
# Stage: matter-builder (build the matter.js sidecar)
# ==============================================================================
#
# Built on a glibc Node image so the produced node_modules are ABI-compatible with
# the glibc Node installed in the runtime stage. @matter/main and ws are pure JS, so
# this is belt-and-braces, but it keeps the door open for any future native dep.

FROM node:22-bookworm-slim AS matter-builder

WORKDIR /opt/matter-bridge

# Install deps against the lockfile first (cached unless the lockfile changes).
COPY matter-bridge/package.json matter-bridge/package-lock.json ./
RUN npm ci

# Build the TypeScript, then drop dev dependencies so only the runtime tree ships.
COPY matter-bridge/ ./
RUN npm run build \
    && npm prune --omit=dev

# ==============================================================================
# Stage: runtime (minimal production image)
# ==============================================================================

FROM ubuntu:25.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

# ca-certificates for TLS; tini as a tiny init that reaps zombies and forwards
# signals to docker-entrypoint.sh (which supervises the app + Matter sidecar);
# Node.js 22 (NodeSource) to run the Matter bridge sidecar.
RUN apt-get update \
    && apt-get install -y --no-install-recommends ca-certificates curl gnupg tini \
    && mkdir -p /etc/apt/keyrings \
    && curl -fsSL https://deb.nodesource.com/gpgkey/nodesource-repo.gpg.key | gpg --dearmor -o /etc/apt/keyrings/nodesource.gpg \
    && echo "deb [signed-by=/etc/apt/keyrings/nodesource.gpg] https://deb.nodesource.com/node_22.x nodistro main" > /etc/apt/sources.list.d/nodesource.list \
    && apt-get update \
    && apt-get install -y --no-install-recommends nodejs \
    && apt-get purge -y --auto-remove curl gnupg \
    && rm -rf /var/lib/apt/lists/*

RUN groupadd --gid 10000 aqualink \
    && useradd --uid 10000 --gid 10000 --create-home --shell /bin/false aqualink

WORKDIR /opt/aqualink-automate

# Copy the installed application from the ci stage. The install tree already
# carries the vcpkg runtime libraries in lib/aqualink-automate/, and the binary
# is linked with an $ORIGIN-relative RPATH (see cmake/CPackConfig.cmake), so no
# separate shared-library copy or LD_LIBRARY_PATH is required.
COPY --from=ci /src/install/config-linux-gcc/ .

# The Matter bridge sidecar (built dist/ + production node_modules) and the
# supervising entrypoint that launches it alongside the app.
COPY --from=matter-builder /opt/matter-bridge /opt/matter-bridge
COPY docker-entrypoint.sh /usr/local/bin/docker-entrypoint.sh
RUN chmod +x /usr/local/bin/docker-entrypoint.sh

# Fabric/commissioning persistence for the Matter bridge (mount a volume here so
# pairing survives container restarts). Owned by the app user.
RUN mkdir -p /data/matter && chown -R aqualink:aqualink /data/matter

USER aqualink

# 80 = HTTP API. Matter commissioning additionally needs UDP 5540 + mDNS 5353,
# which require host networking (see docker-compose.yml) -- they cannot be mapped
# through Docker's bridge driver, so EXPOSE alone is insufficient for Matter.
EXPOSE 80

ENV MATTER_ENABLED=true \
    MATTER_STORAGE_PATH=/data/matter \
    AQUALINK_API_URL=http://127.0.0.1:80

# tini (PID 1) reaps zombies + forwards signals to the entrypoint, which supervises
# the app and (unless MATTER_ENABLED=false) the Matter sidecar. CMD is forwarded to
# the app, so the historical args/behaviour are preserved.
#
# doc-root and the SSL material default to paths resolved relative to the
# executable (share/aqualink-automate/...), so they do not need to be passed.
ENTRYPOINT ["/usr/bin/tini", "--", "/usr/local/bin/docker-entrypoint.sh"]
CMD ["--address", "0.0.0.0", "--disable-https"]
