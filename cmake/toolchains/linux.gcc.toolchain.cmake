#------------------------------------------------------------------------------
# Linux GCC Toolchain Configuration
#------------------------------------------------------------------------------

message(STATUS "Configuring Linux Toolchain (GCC Variant)")

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Find GCC compilers
find_program(CMAKE_C_COMPILER gcc
    HINTS
        "/usr/bin"
        "/usr/local/bin"
        "$ENV{GCC_ROOT}/bin"
    NAMES gcc gcc-13 gcc-12 gcc-11 gcc-10
    REQUIRED
)

find_program(CMAKE_CXX_COMPILER g++
    HINTS
        "/usr/bin"
        "/usr/local/bin"
        "$ENV{GCC_ROOT}/bin"
    NAMES g++ g++-13 g++-12 g++-11 g++-10
    REQUIRED
)

# Configure compiler flags
set(CMAKE_C_FLAGS_INIT "-m64")
set(CMAKE_CXX_FLAGS_INIT "-m64")

# Enable color diagnostics and better error messages
add_compile_options(-fdiagnostics-color=always)

# Link with pthread for multithreading
add_link_options(-pthread)

message(STATUS "Using GCC at: ${CMAKE_CXX_COMPILER}")
message("Linux GCC toolchain CMAKE_C_COMPILER = ${CMAKE_C_COMPILER}")
message("Linux GCC toolchain CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")