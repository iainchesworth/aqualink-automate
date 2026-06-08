#------------------------------------------------------------------------------
# Windows LLVM/Clang Toolchain Configuration
#------------------------------------------------------------------------------

message(STATUS "Configuring Windows Toolchain (LLVM/Clang Variant)")

# Find clang-cl.exe (MSVC-compatible clang frontend).
# Both the C and C++ searches use the same install locations, so the hint
# list is defined once and shared rather than copy-pasted per language.
set(_LLVM_BIN_HINTS
    "C:/Program Files/LLVM/bin"
    "$ENV{ProgramFiles}/LLVM/bin"
    "C:/Program Files (x86)/LLVM/bin"
)

find_program(CMAKE_C_COMPILER clang-cl.exe
    HINTS ${_LLVM_BIN_HINTS}
    REQUIRED
)

find_program(CMAKE_CXX_COMPILER clang-cl.exe
    HINTS ${_LLVM_BIN_HINTS}
    REQUIRED
)

# Use MSVC's link.exe rather than lld-link. lld-link cannot resolve
# compiler-synthesized DLL-exported members (e.g. implicit move constructors)
# which causes link failures with dynamic Boost libraries. clang-cl is fully
# ABI-compatible with MSVC so link.exe works without issues.
#
# Explicitly find MSVC's link.exe via VCToolsInstallDir to avoid picking up
# Git's /usr/bin/link.exe which shadows it on many systems.
find_program(CMAKE_LINKER NAMES link.exe
    PATHS "$ENV{VCToolsInstallDir}/bin/Hostx64/x64"
    NO_DEFAULT_PATH
    REQUIRED
)

# Compiler flags for MSVC compatibility
add_compile_options("/utf-8")
add_compile_options("/bigobj")

# CRT / optimisation flag handling — intentional divergence from windows.msvc.
#
# The MSVC toolchain leaves the CRT and standard /O//Zi//Od flags entirely to
# CMake's defaults. Here we override them explicitly for a single reason:
# sanitizer (ASan) builds require the *release* dynamic CRT (/MD, never /MDd)
# in every configuration, otherwise ASan and the debug CRT's runtime checks
# conflict at link time. Because the /MD requirement forces us off CMake's
# per-config defaults for the runtime, we also pin the matching /Zi//Ob//Od//O2
# flags here so a Debug build still gets full debug info and an unoptimised
# build, and a Release build still gets /O2. CMAKE_MSVC_RUNTIME_LIBRARY pins the
# runtime selection so CMake does not re-add /MDd or /MTd on top of these.
set(CMAKE_CXX_FLAGS_INIT "/MD")
set(CMAKE_C_FLAGS_INIT "/MD")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "/MD /Zi /Ob0 /Od")
set(CMAKE_C_FLAGS_DEBUG_INIT "/MD /Zi /Ob0 /Od")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "/MD /O2 /Ob2 /DNDEBUG")
set(CMAKE_C_FLAGS_RELEASE_INIT "/MD /O2 /Ob2 /DNDEBUG")

# Prevent CMake from adding /MDd or /MTd
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

# Opt-in raised-ISA baseline for release/benchmark builds (default OFF).
# Enables AVX2 codegen for the hot byte-scan and checksum loops. Left OFF by
# default to keep the shipped binary runnable on generic x86-64 hardware; only
# enable for builds validated against the benchmark suite.
if(ENABLE_NATIVE_ARCH)
    add_compile_options("/arch:AVX2")
    message(STATUS "Raised-ISA baseline enabled (/arch:AVX2)")
endif()

message(STATUS "Using LLVM/Clang at: ${CMAKE_CXX_COMPILER}")
