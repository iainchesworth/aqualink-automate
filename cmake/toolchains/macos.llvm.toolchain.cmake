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

# Find LLVM/Clang compilers (prefer MacPorts LLVM 21, then Homebrew, then system)
find_program(CMAKE_C_COMPILER
    NAMES clang-21 clang
    HINTS
        "/opt/local/libexec/llvm-21/bin"
        "/opt/homebrew/bin"
        "/opt/homebrew/opt/llvm/bin"
        "/usr/local/bin"
        "/usr/local/opt/llvm/bin"
        "/opt/local/bin"
        "/usr/bin"
    REQUIRED
)

find_program(CMAKE_CXX_COMPILER
    NAMES clang++-21 clang++
    HINTS
        "/opt/local/libexec/llvm-21/bin"
        "/opt/homebrew/bin"
        "/opt/homebrew/opt/llvm/bin"
        "/usr/local/bin"
        "/usr/local/opt/llvm/bin"
        "/opt/local/bin"
        "/usr/bin"
    REQUIRED
)

# Use lld linker if available (for non-system LLVM)
find_program(CMAKE_LINKER
    NAMES ld.lld-21 ld.lld
    HINTS
        "/opt/local/libexec/llvm-21/bin"
        "/opt/homebrew/opt/llvm/bin"
        "/usr/local/opt/llvm/bin"
        "/opt/local/bin"
)

if(CMAKE_LINKER)
    set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld")
    set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=lld")
endif()

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
# This provides full C++23 support including <stacktrace> and unrestricted std::to_chars
get_filename_component(_LLVM_COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
get_filename_component(_LLVM_PREFIX "${_LLVM_COMPILER_DIR}" DIRECTORY)

set(_LLVM_LIBCXX_INCLUDE "${_LLVM_PREFIX}/include/c++/v1")
set(_LLVM_LIBCXX_LIB "${_LLVM_PREFIX}/lib")

if(EXISTS "${_LLVM_LIBCXX_INCLUDE}")
    message(STATUS "Found LLVM libc++ headers: ${_LLVM_LIBCXX_INCLUDE}")
    message(STATUS "Using LLVM libc++ runtime: ${_LLVM_LIBCXX_LIB}")

    # Use LLVM's libc++ headers instead of Apple SDK's
    string(APPEND CMAKE_CXX_FLAGS_INIT " -nostdinc++ -isystem ${_LLVM_LIBCXX_INCLUDE}")

    # Link against LLVM's libc++ runtime
    set(CMAKE_EXE_LINKER_FLAGS_INIT "${CMAKE_EXE_LINKER_FLAGS_INIT} -L${_LLVM_LIBCXX_LIB} -Wl,-rpath,${_LLVM_LIBCXX_LIB}")
    set(CMAKE_SHARED_LINKER_FLAGS_INIT "${CMAKE_SHARED_LINKER_FLAGS_INIT} -L${_LLVM_LIBCXX_LIB} -Wl,-rpath,${_LLVM_LIBCXX_LIB}")
else()
    message(STATUS "LLVM libc++ not found at ${_LLVM_LIBCXX_INCLUDE}, falling back to system libc++")
    # Fall back to system libc++ (Apple's SDK)
    add_compile_options(-stdlib=libc++)
    add_link_options(-stdlib=libc++)
endif()

# Enable color diagnostics
add_compile_options(-fcolor-diagnostics)

message(STATUS "Using LLVM/Clang at: ${CMAKE_CXX_COMPILER}")
message(STATUS "macOS deployment target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
message("macOS LLVM toolchain CMAKE_C_COMPILER = ${CMAKE_C_COMPILER}")
message("macOS LLVM toolchain CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")
