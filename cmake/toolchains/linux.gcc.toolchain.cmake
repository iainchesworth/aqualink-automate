#------------------------------------------------------------------------------
# Linux GCC Toolchain Configuration
#------------------------------------------------------------------------------

message(STATUS "Configuring Linux Toolchain (GCC Variant)")

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

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

# Configure compiler flags
set(CMAKE_C_FLAGS_INIT "-m64")
set(CMAKE_CXX_FLAGS_INIT "-m64")

# Enable color diagnostics and better error messages
add_compile_options(-fdiagnostics-color=always)

# Opt-in raised-ISA baseline for release/benchmark builds (default OFF).
# Targets the x86-64-v2 microarchitecture level (SSE4.2/POPCNT) so the hot
# byte-scan and checksum loops can autovectorise. Left OFF by default to keep
# the shipped binary runnable on generic x86-64 hardware; only enable for
# builds validated against the benchmark suite.
if(ENABLE_NATIVE_ARCH)
    add_compile_options(-march=x86-64-v2 -mtune=generic)
    message(STATUS "Raised-ISA baseline enabled (-march=x86-64-v2)")
endif()

# Link with pthread for multithreading
add_link_options(-pthread)

message(STATUS "Using GCC at: ${CMAKE_CXX_COMPILER}")
message("Linux GCC toolchain CMAKE_C_COMPILER = ${CMAKE_C_COMPILER}")
message("Linux GCC toolchain CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")