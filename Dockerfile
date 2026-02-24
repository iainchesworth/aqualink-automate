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
# Stage: runtime (minimal production image)
# ==============================================================================

FROM ubuntu:25.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends ca-certificates \
    && rm -rf /var/lib/apt/lists/*

RUN groupadd --gid 10000 aqualink \
    && useradd --uid 10000 --gid 10000 --create-home --shell /bin/false aqualink

WORKDIR /opt/aqualink-automate

# Copy installed application from the ci stage
COPY --from=ci /src/install/config-linux-gcc/ .

# Copy vcpkg shared libraries from the ci stage
COPY --from=ci /src/build/config-linux-gcc/vcpkg_installed/x64-linux-gcc/lib/*.so* ./lib/

ENV LD_LIBRARY_PATH=/opt/aqualink-automate/lib

USER aqualink

EXPOSE 80

ENTRYPOINT ["/opt/aqualink-automate/bin/aqualink-automate"]
CMD ["--address", "0.0.0.0", "--doc-root", "web", "--disable-https"]
