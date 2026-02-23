# Building Aqualink Automate

## Checking out Aqualink Automate

    git clone --recurse-submodules "https://github.com/iainchesworth/aqualink-automate"

## Build Prerequisites

Unless otherwise noted, build tools must be recent.  If in doubt, use the
most recent stable version of each tool.

  * [CMake](https://cmake.org/download/) 3.31 or later is required.

  * Building with [Ninja](https://ninja-build.org/) instead of Make is
    recommended, because it makes builds faster.

  * Compiler support for C++23 is required. On Windows, MSVC from
    Visual Studio 2022 or later is necessary but using the latest
    version is recommended. Recent versions of GCC (13.1+) and Clang
    (17+) should work on non-Windows platforms and maybe on Windows
    too.

## Building

Presets follow the naming convention `config-<platform>-<compiler>[-debug]`.

### Quick start

Using the convenience scripts:

    # Linux/macOS (defaults to GCC debug on Linux, Clang debug on macOS)
    ./cicd/build.sh

    # Windows (defaults to MSVC debug)
    .\cicd\build.ps1

### Building with presets directly

    cmake --preset config-linux-gcc-debug
    cmake --build --preset build-linux-gcc-debug

### Available presets

#### Windows

    config-windows-msvc           # MSVC Release
    config-windows-msvc-debug     # MSVC Debug + ASan
    config-windows-llvm           # Clang Release
    config-windows-llvm-debug     # Clang Debug + ASan

#### Linux

    config-linux-gcc              # GCC Release
    config-linux-gcc-debug        # GCC Debug + ASan
    config-linux-gcc-coverage     # GCC Debug + Coverage
    config-linux-llvm             # Clang Release
    config-linux-llvm-debug       # Clang Debug + ASan

#### macOS

    config-macos-llvm             # Clang Release
    config-macos-llvm-debug       # Clang Debug + ASan

## Running Tests

There are two sets of tests: the unit tests and the performance tests.  Both are
built using the presets above however only the unit tests are run automatically.
Performance tests need to be invoked separately from the command line.

    ctest --preset test-linux-gcc-debug
