# ==============================================================================
# Stage: base (shared tooling for dev, ci, and runtime builds)
# ==============================================================================

FROM ubuntu:24.04 AS base

ARG GCC_VERSION=15
ARG LLVM_VERSION=21

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        git \
        gpg \
        linux-libc-dev \
        ninja-build \
        pkg-config \
        software-properties-common \
        tar \
        unzip \
        wget \
        zip \
    # GCC from ubuntu-toolchain-r PPA
    && add-apt-repository -y ppa:ubuntu-toolchain-r/test \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        gcc-${GCC_VERSION} \
        g++-${GCC_VERSION} \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-${GCC_VERSION} ${GCC_VERSION} \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-${GCC_VERSION} ${GCC_VERSION} \
    # clang-tidy from LLVM apt repo
    && wget -qO - https://apt.llvm.org/llvm-snapshot.gpg.key | gpg --dearmor -o /usr/share/keyrings/llvm-archive-keyring.gpg \
    && echo "deb [signed-by=/usr/share/keyrings/llvm-archive-keyring.gpg] http://apt.llvm.org/noble/ llvm-toolchain-noble-${LLVM_VERSION} main" \
        > /etc/apt/sources.list.d/llvm.list \
    && apt-get update \
    && apt-get install -y --no-install-recommends clang-tidy-${LLVM_VERSION} \
    && update-alternatives --install /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-${LLVM_VERSION} ${LLVM_VERSION} \
    # CMake from Kitware PPA
    && wget -qO - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null \
        | gpg --dearmor -o /usr/share/keyrings/kitware-archive-keyring.gpg \
    && echo "deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ noble main" \
        > /etc/apt/sources.list.d/kitware.list \
    && apt-get update \
    && apt-get install -y --no-install-recommends cmake \
    # ccache
    && apt-get install -y --no-install-recommends ccache \
    # Cleanup
    && rm -rf /var/lib/apt/lists/*

ENV CCACHE_DIR=/ccache
ENV CMAKE_C_COMPILER_LAUNCHER=ccache
ENV CMAKE_CXX_COMPILER_LAUNCHER=ccache

# ==============================================================================
# Stage: dev (development container — source bind-mounted at runtime)
# ==============================================================================

FROM base AS dev

RUN groupadd --gid 1000 dev \
    && useradd --uid 1000 --gid 1000 --create-home --shell /bin/bash dev \
    && mkdir -p /ccache /vcpkg-cache /src \
    && chown dev:dev /ccache /vcpkg-cache /src

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
        -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
        -DENABLE_CLANG_TIDY=ON

# Build (clang-tidy runs during build)
RUN --mount=type=cache,target=/ccache \
    cmake --build --preset build-linux-gcc

# Test
RUN ctest --preset test-linux-gcc

# Install (creates install tree for runtime stage)
RUN cmake --install build/config-linux-gcc

# ==============================================================================
# Stage: runtime (minimal production image)
# ==============================================================================

FROM ubuntu:24.04 AS runtime

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
