# Installation

Instructions for installing pre-built binaries or building Aqualink Automate from source.

## Pre-built Binaries

Download the appropriate package for your platform from the [Releases](https://github.com/iainchesworth/aqualink-automate/releases) page:

| Platform | Package types |
|----------|--------------|
| Windows | `.exe` (NSIS installer), `.zip` |
| Linux | `.deb`, `.rpm`, `.tgz` |
| macOS | `.dmg`, `.tgz` |

## Building from Source

### Prerequisites

| Tool | Minimum version | Notes |
|------|----------------|-------|
| Git | any | With submodule support |
| CMake | 3.31+ | [cmake.org](https://cmake.org) |
| Ninja | any | [ninja-build.org](https://ninja-build.org) |
| C++ compiler | C++23 support | GCC 14+, Clang 21+, or MSVC v143+ |

vcpkg is included as a git submodule and bootstrapped automatically by the build scripts.

### Clone

```bash
git clone --recurse-submodules https://github.com/iainchesworth/aqualink-automate.git
cd aqualink-automate
```

### Linux / macOS

The build script validates dependencies, bootstraps vcpkg, and runs configure/build/test:

```bash
./cicd/build.sh                              # GCC debug (Linux) or Clang debug (macOS)
./cicd/build.sh --compiler clang --type release
./cicd/build.sh --preset config-linux-gcc    # explicit preset
./cicd/build.sh --package                    # build + create packages
```

If dependencies are missing, the script reports what to install:

```bash
# Ubuntu/Debian
sudo apt install build-essential cmake ninja-build git

# macOS
brew install cmake ninja llvm
```

### Windows

Run from a PowerShell prompt:

```powershell
.\cicd\build.ps1                                      # MSVC debug
.\cicd\build.ps1 -Compiler clang -Type release
.\cicd\build.ps1 -Preset config-windows-msvc-debug    # explicit preset
.\cicd\build.ps1 -Package                              # build + create packages
```

The script checks for Git, CMake, Ninja, and Visual Studio Build Tools. If MSVC is selected, it automatically locates and activates the VS developer environment.

Missing tools can be installed via [Chocolatey](https://chocolatey.org):

```powershell
choco install git cmake ninja visualstudio2022buildtools
```

### Available CMake Presets

| Configure preset | Platform | Compiler | Type |
|-----------------|----------|----------|------|
| `config-linux-gcc` | Linux | GCC | Release |
| `config-linux-gcc-debug` | Linux | GCC | Debug |
| `config-linux-llvm` | Linux | Clang/LLVM | Release |
| `config-linux-llvm-debug` | Linux | Clang/LLVM | Debug |
| `config-macos-llvm` | macOS | Clang/LLVM | Release |
| `config-macos-llvm-debug` | macOS | Clang/LLVM | Debug |
| `config-windows-msvc` | Windows | MSVC | Release |
| `config-windows-msvc-debug` | Windows | MSVC | Debug |
| `config-windows-llvm` | Windows | Clang/LLVM | Release |
| `config-windows-llvm-debug` | Windows | Clang/LLVM | Debug |

Each configure preset has matching `build-*`, `test-*`, and `pack-*` presets (replace the `config-` prefix).

### Manual CMake Workflow

If you prefer not to use the build scripts:

```bash
# Bootstrap vcpkg (one-time)
./deps/vcpkg/bootstrap-vcpkg.sh -disableMetrics   # Linux/macOS
.\deps\vcpkg\bootstrap-vcpkg.bat -disableMetrics   # Windows

# Configure, build, test
cmake --preset config-linux-gcc
cmake --build --preset build-linux-gcc
ctest --preset test-linux-gcc
```

## Development Container

A Dev Container configuration is provided for VS Code. It builds the `dev` target from the project Dockerfile with GCC 15, Clang 21, clang-tidy, CMake, Ninja, and ccache pre-installed.

### Setup

1. Install [Docker](https://www.docker.com/products/docker-desktop/) and the [Dev Containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers) VS Code extension.

2. Clone and open:
   ```bash
   git clone --recurse-submodules https://github.com/iainchesworth/aqualink-automate.git
   cd aqualink-automate
   code .
   ```

3. When prompted, select **Reopen in Container**. The `postCreateCommand` initializes vcpkg and bootstraps it automatically.

### Persistent Caches

Named Docker volumes persist across container rebuilds:

| Volume | Mount point | Purpose |
|--------|------------|---------|
| `aqualink-ccache` | `/ccache` | Compiler cache (ccache) |
| `aqualink-vcpkg-cache` | `/vcpkg-cache` | vcpkg binary cache |
| `aqualink-vcpkg` | `/src/deps/vcpkg` | vcpkg installation (avoids cross-platform lock issues) |

### Building Inside the Container

```bash
cmake --preset config-linux-gcc
cmake --build --preset build-linux-gcc
ctest --preset test-linux-gcc
```

Both GCC and Clang/LLVM toolchains are available in the container. The VS Code extensions for C++ IntelliSense, CMake Tools, and clangd are installed automatically.

## Docker

### Runtime Container

Build and run the production image:

```bash
docker build --target runtime -t aqualink-automate .
docker run -p 80:80 aqualink-automate
```

Connect a serial device:

```bash
docker run -p 80:80 --device /dev/ttyUSB0 aqualink-automate --serial-port /dev/ttyUSB0
```

Or use Docker Compose:

```bash
docker compose up
```

### API Documentation (Swagger UI)

```bash
docker compose --profile docs up
```

Serves Swagger UI at `http://localhost:8080` from the repository's OpenAPI spec.
