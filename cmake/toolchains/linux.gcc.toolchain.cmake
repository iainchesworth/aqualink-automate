#------------------------------------------------------------------------------
# Linux GCC Toolchain Configuration
#------------------------------------------------------------------------------

message(STATUS "Configuring Linux Toolchain (GCC Variant)")

set(CMAKE_SYSTEM_NAME Linux)

# Determine target architecture (mirrors the macOS toolchain). _LINUX_ARCH holds
# the canonical processor name; the arch-specific compiler flags below derive
# from it. vcpkg sets VCPKG_TARGET_ARCHITECTURE only while building a port for a
# triplet — in the project's OWN configure scope it is empty, so fall back to the
# host architecture there. Without this fallback a native aarch64 build would
# default to x86_64 in the project scope and emit `-m64`, which an aarch64 gcc
# rejects ("unrecognized command-line option '-m64'").
if(VCPKG_TARGET_ARCHITECTURE STREQUAL "arm64")
    set(_LINUX_ARCH "aarch64")
elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
    set(_LINUX_ARCH "x86_64")
else()
    execute_process(
        COMMAND uname -m
        OUTPUT_VARIABLE _HOST_ARCH
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(_HOST_ARCH MATCHES "aarch64|arm64")
        set(_LINUX_ARCH "aarch64")
    else()
        set(_LINUX_ARCH "x86_64")
    endif()
endif()

set(CMAKE_SYSTEM_PROCESSOR ${_LINUX_ARCH})
message(STATUS "Target architecture: ${CMAKE_SYSTEM_PROCESSOR}")

# Find GCC compilers. The C and C++ searches share the same install
# locations, so the hint list is defined once and reused.
set(_GCC_BIN_HINTS
    "/usr/bin"
    "/usr/local/bin"
    "$ENV{GCC_ROOT}/bin"
)

find_program(CMAKE_C_COMPILER gcc
    HINTS ${_GCC_BIN_HINTS}
    NAMES gcc gcc-15 gcc-14 gcc-13 gcc-12 gcc-11 gcc-10
    REQUIRED
)

find_program(CMAKE_CXX_COMPILER g++
    HINTS ${_GCC_BIN_HINTS}
    NAMES g++ g++-15 g++-14 g++-13 g++-12 g++-11 g++-10
    REQUIRED
)

# Configure compiler flags. -m64 selects the 64-bit x86 ABI and is rejected by
# an aarch64 gcc, so it is x86-only; the arm host compiler already targets
# aarch64 natively and needs no equivalent flag.
if(_LINUX_ARCH STREQUAL "x86_64")
    set(CMAKE_C_FLAGS_INIT "-m64")
    set(CMAKE_CXX_FLAGS_INIT "-m64")
endif()

# Enable color diagnostics and better error messages
add_compile_options(-fdiagnostics-color=always)

# Opt-in raised-ISA baseline for release/benchmark builds (default OFF).
# x86_64 only — targets the x86-64-v2 microarchitecture level (SSE4.2/POPCNT)
# so the hot byte-scan and checksum loops can autovectorise. The arm64 baseline
# already exceeds this, so the flag is skipped there. Left OFF by default to
# keep the shipped binary runnable on generic hardware; only enable for builds
# validated against the benchmark suite.
if(ENABLE_NATIVE_ARCH AND _LINUX_ARCH STREQUAL "x86_64")
    add_compile_options(-march=x86-64-v2 -mtune=generic)
    message(STATUS "Raised-ISA baseline enabled (-march=x86-64-v2)")
endif()

# Link with pthread for multithreading
add_link_options(-pthread)

message(STATUS "Using GCC at: ${CMAKE_CXX_COMPILER}")
message("Linux GCC toolchain CMAKE_C_COMPILER = ${CMAKE_C_COMPILER}")
message("Linux GCC toolchain CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")