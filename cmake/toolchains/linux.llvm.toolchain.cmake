#------------------------------------------------------------------------------
# Linux LLVM/Clang Toolchain Configuration
#------------------------------------------------------------------------------

message(STATUS "Configuring Linux Toolchain (LLVM/Clang Variant)")

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Find LLVM/Clang compilers
find_program(CMAKE_C_COMPILER
    NAMES clang-21 clang clang-18 clang-17 clang-16 clang-15
    HINTS
        "/usr/bin"
        "/usr/local/bin"
        "/opt/llvm/bin"
        "$ENV{LLVM_ROOT}/bin"
    REQUIRED
)

find_program(CMAKE_CXX_COMPILER
    NAMES clang++-21 clang++ clang++-18 clang++-17 clang++-16 clang++-15
    HINTS
        "/usr/bin"
        "/usr/local/bin"
        "/opt/llvm/bin"
        "$ENV{LLVM_ROOT}/bin"
    REQUIRED
)

# Use lld linker if available
find_program(CMAKE_LINKER
    NAMES ld.lld-21 ld.lld ld.lld-18 ld.lld-17 ld.lld-16 ld.lld-15
    HINTS
        "/usr/bin"
        "/usr/local/bin"
        "/opt/llvm/bin"
        "$ENV{LLVM_ROOT}/bin"
)

if(CMAKE_LINKER)
    set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld")
    set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=lld")
endif()

# Configure compiler flags
set(CMAKE_C_FLAGS_INIT "-m64")
set(CMAKE_CXX_FLAGS_INIT "-m64")

# Enable color diagnostics and better error messages
add_compile_options(-fcolor-diagnostics -fdiagnostics-show-template-tree)

# Use GCC's libstdc++ (default for Clang on Linux). libc++ lacks C++23
# <stacktrace> support so we cannot use it until that is resolved.
add_link_options(-lstdc++exp)

message(STATUS "Using LLVM/Clang at: ${CMAKE_CXX_COMPILER}")
if(CMAKE_LINKER)
    message(STATUS "Using LLD linker at: ${CMAKE_LINKER}")
endif()
message("Linux LLVM toolchain CMAKE_C_COMPILER = ${CMAKE_C_COMPILER}")
message("Linux LLVM toolchain CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")