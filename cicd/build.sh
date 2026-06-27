#!/bin/bash
set -euo pipefail

########################################################################################
#
# Cross-platform (Linux/macOS) build helper for aqualink-automate.
#
# Usage:
#   ./cicd/build.sh                                  # default: debug, platform primary compiler
#   ./cicd/build.sh --compiler clang --type release
#   ./cicd/build.sh --preset config-linux-gcc-debug
#   ./cicd/build.sh --package
#
########################################################################################

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Defaults
COMPILER=""
BUILD_TYPE="debug"
PRESET=""
PACKAGE=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --preset)
            PRESET="$2"
            shift 2
            ;;
        --compiler)
            COMPILER="$2"
            shift 2
            ;;
        --type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        --package)
            PACKAGE=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [--preset <name>] [--compiler gcc|clang] [--type debug|release] [--package]"
            exit 0
            ;;
        *)
            echo "Unknown argument: $1"
            echo "Usage: $0 [--preset <name>] [--compiler gcc|clang] [--type debug|release] [--package]"
            exit 1
            ;;
    esac
done

# Detect platform
PLATFORM="$(uname -s)"
case "${PLATFORM}" in
    Linux*)  PLATFORM_NAME="linux" ;;
    Darwin*) PLATFORM_NAME="macos" ;;
    *)
        echo "Error: Unsupported platform '${PLATFORM}'. Use cicd/build.ps1 on Windows."
        exit 1
        ;;
esac

# Set default compiler per platform
if [[ -z "${COMPILER}" ]]; then
    case "${PLATFORM_NAME}" in
        linux) COMPILER="gcc" ;;
        macos) COMPILER="llvm" ;;
    esac
fi

# Normalize compiler name
case "${COMPILER}" in
    gcc)   COMPILER_NAME="gcc" ;;
    clang) COMPILER_NAME="llvm" ;;
    llvm)  COMPILER_NAME="llvm" ;;
    *)
        echo "Error: Unknown compiler '${COMPILER}'. Use gcc or clang."
        exit 1
        ;;
esac

# Validate platform/compiler combination
if [[ "${PLATFORM_NAME}" == "macos" && "${COMPILER_NAME}" != "llvm" ]]; then
    echo "Error: macOS only supports Clang (llvm). Got '${COMPILER}'."
    exit 1
fi

# Detect the target architecture. Linux GCC is the only family with a non-x64
# triplet (arm64), so a native build on an aarch64 host (e.g. a Raspberry Pi)
# must select the -arm64 presets rather than the x64 default. Overridden by an
# explicit --preset.
ARCH_SUFFIX=""
if [[ "${PLATFORM_NAME}" == "linux" && "${COMPILER_NAME}" == "gcc" ]]; then
    case "$(uname -m)" in
        aarch64|arm64) ARCH_SUFFIX="-arm64" ;;
    esac
fi

# Build preset name if not explicitly provided
if [[ -z "${PRESET}" ]]; then
    case "${BUILD_TYPE}" in
        debug)   PRESET="config-${PLATFORM_NAME}-${COMPILER_NAME}${ARCH_SUFFIX}-debug" ;;
        release) PRESET="config-${PLATFORM_NAME}-${COMPILER_NAME}${ARCH_SUFFIX}" ;;
        *)
            echo "Error: Unknown build type '${BUILD_TYPE}'. Use debug or release."
            exit 1
            ;;
    esac
fi

# Derive build and test preset names from configure preset
BUILD_PRESET="${PRESET/config-/build-}"
TEST_PRESET="${PRESET/config-/test-}"

echo "=== aqualink-automate build ==="
echo "  Platform:  ${PLATFORM_NAME}"
echo "  Compiler:  ${COMPILER_NAME}"
echo "  Type:      ${BUILD_TYPE}"
echo "  Preset:    ${PRESET}"
echo ""

# ---- Dependency validation ----
echo "--- Checking dependencies ---"
MISSING=()

command -v git >/dev/null 2>&1    || MISSING+=("git")
command -v cmake >/dev/null 2>&1  || MISSING+=("cmake (https://cmake.org)")
command -v ninja >/dev/null 2>&1  || MISSING+=("ninja (https://ninja-build.org)")

if [[ "${COMPILER_NAME}" == "gcc" ]]; then
    command -v gcc >/dev/null 2>&1 || MISSING+=("gcc")
    command -v g++ >/dev/null 2>&1 || MISSING+=("g++")
elif [[ "${COMPILER_NAME}" == "llvm" ]]; then
    command -v clang >/dev/null 2>&1   || MISSING+=("clang")
    command -v clang++ >/dev/null 2>&1 || MISSING+=("clang++")
fi

if [[ "${PACKAGE}" == "true" && "${PLATFORM_NAME}" == "linux" ]]; then
    command -v dpkg-deb >/dev/null 2>&1 || command -v rpmbuild >/dev/null 2>&1 || true
fi

if [[ ${#MISSING[@]} -gt 0 ]]; then
    echo "Error: Missing required tools:"
    for tool in "${MISSING[@]}"; do
        echo "  - ${tool}"
    done
    echo ""
    if [[ "${PLATFORM_NAME}" == "linux" ]]; then
        echo "Install with: sudo apt install build-essential cmake ninja-build git"
    elif [[ "${PLATFORM_NAME}" == "macos" ]]; then
        echo "Install with: brew install cmake ninja llvm"
    fi
    exit 1
fi

# Report versions
echo "  git:    $(git --version | head -1)"
echo "  cmake:  $(cmake --version | head -1)"
echo "  ninja:  $(ninja --version)"
if [[ "${COMPILER_NAME}" == "gcc" ]]; then
    echo "  gcc:    $(gcc --version | head -1)"
elif [[ "${COMPILER_NAME}" == "llvm" ]]; then
    echo "  clang:  $(clang --version | head -1)"
fi
echo ""

# ---- Submodules ----
if [[ ! -f "${PROJECT_DIR}/deps/vcpkg/bootstrap-vcpkg.sh" ]]; then
    echo "--- Initializing submodules ---"
    git -C "${PROJECT_DIR}" submodule update --init --recursive
fi

# ---- Bootstrap vcpkg ----
VCPKG_DIR="${PROJECT_DIR}/deps/vcpkg"
if [[ ! -f "${VCPKG_DIR}/vcpkg" ]]; then
    echo "--- Bootstrapping vcpkg ---"
    "${VCPKG_DIR}/bootstrap-vcpkg.sh" -disableMetrics
fi

# Configure
echo "--- Configure ---"
cmake --preset="${PRESET}" "${PROJECT_DIR}"

# Build
echo "--- Build ---"
cmake --build --preset="${BUILD_PRESET}"

# Test
echo "--- Test ---"
ctest --preset="${TEST_PRESET}"

# Package (optional)
if [[ "${PACKAGE}" == "true" ]]; then
    echo "--- Package ---"
    cmake --build "build/${PRESET}" --target pack-aqualink-automate
fi

echo ""
echo "=== Build complete ==="
