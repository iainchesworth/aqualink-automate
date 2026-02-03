# ==============================================================================
# Stage 1: Build
# ==============================================================================

FROM ubuntu:22.04 AS build

ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies (CMake installed from Kitware PPA below)
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
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
    && add-apt-repository -y ppa:ubuntu-toolchain-r/test \
    && apt-get update \
    && apt-get install -y --no-install-recommends gcc-13 g++-13 \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 13 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 13 \
    && rm -rf /var/lib/apt/lists/*

# Install a recent CMake from Kitware
RUN wget -qO - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null \
        | gpg --dearmor - > /usr/share/keyrings/kitware-archive-keyring.gpg \
    && echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main' \
        > /etc/apt/sources.list.d/kitware.list \
    && apt-get update \
    && apt-get install -y --no-install-recommends cmake \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

# Copy source tree
COPY . .

# Bootstrap vcpkg
RUN ./deps/vcpkg/bootstrap-vcpkg.sh -disableMetrics

# Configure
RUN cmake --preset=config-linux-gcc

# Build
RUN cmake --build --preset=build-linux-gcc

# Install
RUN cmake --install build/config-linux-gcc

# ==============================================================================
# Stage 2: Runtime
# ==============================================================================

FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/aqualink-automate

# Copy the installed application from the build stage
COPY --from=build /src/install/config-linux-gcc/ .

# Copy vcpkg shared libraries from the build stage
COPY --from=build /src/build/config-linux-gcc/vcpkg_installed/x64-linux-gcc/lib/*.so* /opt/aqualink-automate/lib/

ENV LD_LIBRARY_PATH=/opt/aqualink-automate/lib

EXPOSE 80 443

ENTRYPOINT ["/opt/aqualink-automate/bin/aqualink-automate"]
