#------------------------------------------------------------------------------
# macOS LLVM/Clang Toolchain Configuration
#------------------------------------------------------------------------------

message(STATUS "Configuring macOS Toolchain (LLVM/Clang Variant)")

set(CMAKE_SYSTEM_NAME Darwin)

# Determine target architecture from vcpkg triplet or auto-detect
if(VCPKG_TARGET_ARCHITECTURE STREQUAL "arm64")
    set(CMAKE_SYSTEM_PROCESSOR arm64)
    set(_MACOS_ARCH_FLAG "-arch arm64")
elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
    set(CMAKE_SYSTEM_PROCESSOR x86_64)
    set(_MACOS_ARCH_FLAG "-arch x86_64")
else()
    # Auto-detect host architecture
    execute_process(
        COMMAND uname -m
        OUTPUT_VARIABLE _HOST_ARCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(_HOST_ARCH STREQUAL "arm64")
        set(CMAKE_SYSTEM_PROCESSOR arm64)
        set(_MACOS_ARCH_FLAG "-arch arm64")
    else()
        set(CMAKE_SYSTEM_PROCESSOR x86_64)
        set(_MACOS_ARCH_FLAG "-arch x86_64")
    endif()
endif()

message(STATUS "Target architecture: ${CMAKE_SYSTEM_PROCESSOR}")

# Find LLVM/Clang compilers (prefer Homebrew or MacPorts over system)
find_program(CMAKE_C_COMPILER clang
    HINTS
        "/opt/homebrew/bin"
        "/opt/homebrew/opt/llvm/bin"
        "/usr/local/bin"
        "/usr/local/opt/llvm/bin"
        "/opt/local/bin"
        "/usr/bin"
    NAMES clang clang-21 clang-20 clang-19 clang-18 clang-17
    REQUIRED
)

find_program(CMAKE_CXX_COMPILER clang++
    HINTS
        "/opt/homebrew/bin"
        "/opt/homebrew/opt/llvm/bin"
        "/usr/local/bin"
        "/usr/local/opt/llvm/bin"
        "/opt/local/bin"
        "/usr/bin"
    NAMES clang++ clang++-21 clang++-20 clang++-19 clang++-18 clang++-17
    REQUIRED
)

# Use lld linker if available (for non-system LLVM)
find_program(CMAKE_LINKER ld.lld
    HINTS
        "/opt/homebrew/opt/llvm/bin"
        "/usr/local/opt/llvm/bin"
        "/opt/local/bin"
)

if(CMAKE_LINKER)
    set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld")
    set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=lld")
endif()

# Set minimum macOS version
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")

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

# Enable color diagnostics
add_compile_options(-fcolor-diagnostics)

# Use libc++ (default on macOS)
add_compile_options(-stdlib=libc++)
add_link_options(-stdlib=libc++)

message(STATUS "Using LLVM/Clang at: ${CMAKE_CXX_COMPILER}")
message(STATUS "macOS deployment target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
message("macOS LLVM toolchain CMAKE_C_COMPILER = ${CMAKE_C_COMPILER}")
message("macOS LLVM toolchain CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")