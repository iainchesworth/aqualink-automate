# Building Aqualink Automate

## Checking out Aqualink Automate

    git clone "https://github.com/iainchesworth/aqualink-automate"

## Build Prerequisites

Unless otherwise noted, build tools must be recent.  If in doubt, use the
most recent stable version of each tool.

  * [CMake](https://cmake.org/download/) 3.12 or later is required.

  * Building with [Ninja](https://ninja-build.org/) instead of Make is
    recommended, because it makes builds faster. On Windows, CMake's Visual
    Studio generator may also work, but it not tested regularly and requires
    recent versions of CMake.

  * Compiler support for C++20 is required. On Windows, MSVC from
    Visual Studio 2022 or later is necessary but using the latest 
    version is recommended. Recent versions of GCC (13.1+) and Clang 
    (17+) should work on non-Windows platforms and maybe on Windows 
    too.

## Building

There are presets available for each platform and take the format as follows: 

    ci-<make_tool>-<compiler>-<platform>-x86_64-<build_type>

so to build on Linux using GCC, Ninja, you would use the following invocation:

    cmake --fresh --preset ci-ninja-gcc-linux-x86_64-release
    cmake --build --preset ci-ninja-gcc-linux-x86_64-release

### Building for Windows

Available presets:

    ci-ninja-msvc-windows-x86_64-release
    dev-ninja-gcc-windows-x86_64-release
    dev-ninja-llvm-windows-x86_64-release
    dev-ninja-msvc-windows-x86_64-debug

### Building for Linux

Available presets:

    ci-ninja-gcc-linux-x86_64-coverage
    ci-ninja-gcc-linux-x86_64-release
    dev-ninja-llvm-linux-x86_64-debug

### Building for MacOS

TBD


# Running Tests

There are two sets of tests: the unit tests and the performance tests.  Both are
built using the presets above however only the unit tests are run automatically.  
Performance tests need to be invoked seperately from the command line.
