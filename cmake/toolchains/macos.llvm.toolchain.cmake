#------------------------------------------------------------------------------
# macOS LLVM/Clang Toolchain Configuration
#------------------------------------------------------------------------------

message(STATUS "Configuring macOS Toolchain (LLVM/Clang Variant)")

set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Find LLVM/Clang compilers (prefer Homebrew or MacPorts over system)
find_program(CMAKE_C_COMPILER clang
    HINTS
        "/opt/homebrew/bin"
        "/opt/homebrew/opt/llvm/bin"
        "/usr/local/bin"
        "/usr/local/opt/llvm/bin"
        "/opt/local/bin"
        "/usr/bin"
    NAMES clang clang-18 clang-17 clang-16 clang-15
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
    NAMES clang++ clang++-18 clang++-17 clang++-16 clang++-15
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

# Configure compiler flags
set(CMAKE_C_FLAGS_INIT "-arch x86_64")
set(CMAKE_CXX_FLAGS_INIT "-arch x86_64")

# Enable color diagnostics
add_compile_options(-fcolor-diagnostics)

# Use libc++ (default on macOS)
add_compile_options(-stdlib=libc++)
add_link_options(-stdlib=libc++)

message(STATUS "Using LLVM/Clang at: ${CMAKE_CXX_COMPILER}")
message(STATUS "macOS deployment target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
message("macOS LLVM toolchain CMAKE_C_COMPILER = ${CMAKE_C_COMPILER}")
message("macOS LLVM toolchain CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")