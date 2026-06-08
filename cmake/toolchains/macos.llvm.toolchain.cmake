#------------------------------------------------------------------------------
# macOS LLVM/Clang Toolchain Configuration
#------------------------------------------------------------------------------

message(STATUS "Configuring macOS Toolchain (LLVM/Clang Variant)")

set(CMAKE_SYSTEM_NAME Darwin)

# Determine target architecture from vcpkg triplet or auto-detect.
# _MACOS_ARCH holds the canonical clang/Mach-O arch name (arm64 / x86_64); the
# raw -arch compiler flag and CMAKE_OSX_ARCHITECTURES are both derived from it.
if(VCPKG_TARGET_ARCHITECTURE STREQUAL "arm64")
    set(CMAKE_SYSTEM_PROCESSOR arm64)
    set(_MACOS_ARCH "arm64")
elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
    set(CMAKE_SYSTEM_PROCESSOR x86_64)
    set(_MACOS_ARCH "x86_64")
else()
    # Auto-detect host architecture
    execute_process(
        COMMAND uname -m
        OUTPUT_VARIABLE _HOST_ARCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(_HOST_ARCH STREQUAL "arm64")
        set(CMAKE_SYSTEM_PROCESSOR arm64)
        set(_MACOS_ARCH "arm64")
    else()
        set(CMAKE_SYSTEM_PROCESSOR x86_64)
        set(_MACOS_ARCH "x86_64")
    endif()
endif()

set(_MACOS_ARCH_FLAG "-arch ${_MACOS_ARCH}")

# Drive the architecture through CMAKE_OSX_ARCHITECTURES as well as the raw
# -arch flag. Without this CMake selects the SDK/host architecture for its own
# checks while only the compile flags carry -arch, risking an arch/SDK mismatch
# (e.g. cross-building x86_64 on an arm64 host).
set(CMAKE_OSX_ARCHITECTURES "${_MACOS_ARCH}" CACHE STRING "Target macOS architecture")

message(STATUS "Target architecture: ${CMAKE_SYSTEM_PROCESSOR}")

# Find LLVM/Clang compilers (prefer Homebrew LLVM, then MacPorts, then system).
# Both searches share the same install locations, so the hint list is defined
# once and reused.
set(_LLVM_BIN_HINTS
    "/opt/homebrew/opt/llvm/bin"
    "/usr/local/opt/llvm/bin"
    "/opt/local/libexec/llvm-21/bin"
    "/opt/local/bin"
    "/opt/homebrew/bin"
    "/usr/local/bin"
    "/usr/bin"
)

find_program(CMAKE_C_COMPILER
    NAMES clang-21 clang
    HINTS ${_LLVM_BIN_HINTS}
    REQUIRED
)

find_program(CMAKE_CXX_COMPILER
    NAMES clang++-21 clang++
    HINTS ${_LLVM_BIN_HINTS}
    REQUIRED
)

# Note: We intentionally do NOT use LLD on macOS. While LLVM ships ld64.lld
# (Mach-O linker), autotools-based vcpkg ports (e.g. libbacktrace) use libtool
# which generates -Wl,-soname (ELF) instead of -Wl,-install_name (Mach-O) when
# it detects lld, causing link failures. Apple's system linker works fine.

# Set minimum macOS version (13.3 required for full std::to_chars support)
set(CMAKE_OSX_DEPLOYMENT_TARGET "13.3" CACHE STRING "Minimum macOS version")

# Find and set SDK path
execute_process(
    COMMAND xcrun --show-sdk-path
    OUTPUT_VARIABLE CMAKE_OSX_SYSROOT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

if(CMAKE_OSX_SYSROOT)
    message(STATUS "Using macOS SDK: ${CMAKE_OSX_SYSROOT}")
endif()

# Configure compiler flags for target architecture
set(CMAKE_C_FLAGS_INIT "${_MACOS_ARCH_FLAG}")
set(CMAKE_CXX_FLAGS_INIT "${_MACOS_ARCH_FLAG}")

# Detect LLVM's own libc++ and use it instead of Apple's SDK libc++
# This provides full C++23 support including unrestricted std::to_chars
get_filename_component(_LLVM_COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
get_filename_component(_LLVM_PREFIX "${_LLVM_COMPILER_DIR}" DIRECTORY)

message(STATUS "LLVM compiler dir: ${_LLVM_COMPILER_DIR}")
message(STATUS "LLVM prefix: ${_LLVM_PREFIX}")

set(_LLVM_LIBCXX_INCLUDE "${_LLVM_PREFIX}/include/c++/v1")
set(_LLVM_LIBCXX_LIB "${_LLVM_PREFIX}/lib")

if(EXISTS "${_LLVM_LIBCXX_INCLUDE}")
    message(STATUS "Found LLVM libc++ headers: ${_LLVM_LIBCXX_INCLUDE}")
    message(STATUS "Using LLVM libc++ runtime: ${_LLVM_LIBCXX_LIB}")

    # Use LLVM's libc++ headers instead of Apple SDK's.
    # Disable vendor availability annotations so features like std::from_chars
    # are not gated on the macOS deployment target — we link against LLVM's
    # own libc++ runtime which has all symbols available.
    string(APPEND CMAKE_CXX_FLAGS_INIT " -nostdinc++ -isystem ${_LLVM_LIBCXX_INCLUDE} -D_LIBCPP_DISABLE_AVAILABILITY")

    # Link against LLVM's libc++ runtime
    set(CMAKE_EXE_LINKER_FLAGS_INIT "-L${_LLVM_LIBCXX_LIB}/c++ -L${_LLVM_LIBCXX_LIB} -Wl,-rpath,${_LLVM_LIBCXX_LIB}/c++ -Wl,-rpath,${_LLVM_LIBCXX_LIB}")
    set(CMAKE_SHARED_LINKER_FLAGS_INIT "-L${_LLVM_LIBCXX_LIB}/c++ -L${_LLVM_LIBCXX_LIB} -Wl,-rpath,${_LLVM_LIBCXX_LIB}/c++ -Wl,-rpath,${_LLVM_LIBCXX_LIB}")
else()
    message(WARNING "LLVM libc++ not found at ${_LLVM_LIBCXX_INCLUDE}")
    message(WARNING "CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")
    message(WARNING "Derived prefix = ${_LLVM_PREFIX}")
    # Fall back to system libc++ (Apple's SDK) — C++23 features may be unavailable
    add_compile_options(-stdlib=libc++)
    add_link_options(-stdlib=libc++)
endif()

# Enable color diagnostics
add_compile_options(-fcolor-diagnostics)

# Opt-in raised-ISA baseline for release/benchmark builds (default OFF).
# x86_64 only — targets the x86-64-v2 microarchitecture level so the hot
# byte-scan and checksum loops can autovectorise. The arm64 baseline already
# exceeds this, so the flag is skipped there. Left OFF by default to keep the
# shipped binary runnable on generic hardware; only enable for builds
# validated against the benchmark suite.
if(ENABLE_NATIVE_ARCH AND _MACOS_ARCH STREQUAL "x86_64")
    add_compile_options(-march=x86-64-v2 -mtune=generic)
    message(STATUS "Raised-ISA baseline enabled (-march=x86-64-v2)")
endif()

message(STATUS "Using LLVM/Clang at: ${CMAKE_CXX_COMPILER}")
message(STATUS "macOS deployment target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
message("macOS LLVM toolchain CMAKE_C_COMPILER = ${CMAKE_C_COMPILER}")
message("macOS LLVM toolchain CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")
