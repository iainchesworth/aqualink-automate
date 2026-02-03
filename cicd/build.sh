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

# Build preset name if not explicitly provided
if [[ -z "${PRESET}" ]]; then
    case "${BUILD_TYPE}" in
        debug)   PRESET="config-${PLATFORM_NAME}-${COMPILER_NAME}-debug" ;;
        release) PRESET="config-${PLATFORM_NAME}-${COMPILER_NAME}" ;;
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

# Bootstrap vcpkg if needed
VCPKG_DIR="${PROJECT_DIR}/deps/vcpkg"
if [[ ! -f "${VCPKG_DIR}/vcpkg" ]]; then
    echo "--- Bootstrapping vcpkg ---"
    if [[ ! -f "${VCPKG_DIR}/bootstrap-vcpkg.sh" ]]; then
        echo "Error: vcpkg not found at ${VCPKG_DIR}. Did you clone with --recurse-submodules?"
        exit 1
    fi
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
