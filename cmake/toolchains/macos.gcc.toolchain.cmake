#------------------------------------------------------------------------------
# macOS GCC Toolchain Configuration
#------------------------------------------------------------------------------

message(STATUS "Configuring macOS Toolchain (GCC Variant)")

set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Find GCC compilers (prefer Homebrew or MacPorts)
find_program(CMAKE_C_COMPILER gcc
    HINTS
        "/opt/homebrew/bin"
        "/usr/local/bin"
        "/opt/local/bin"
    NAMES gcc gcc-13 gcc-12 gcc-11 gcc-10 gcc-9
    REQUIRED
)

find_program(CMAKE_CXX_COMPILER g++
    HINTS
        "/opt/homebrew/bin"
        "/usr/local/bin"
        "/opt/local/bin"
    NAMES g++ g++-13 g++-12 g++-11 g++-10 g++-9
    REQUIRED
)

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
    # GCC needs explicit SDK path
    add_compile_options(-isysroot ${CMAKE_OSX_SYSROOT})
    add_link_options(-isysroot ${CMAKE_OSX_SYSROOT})
endif()

# Configure compiler flags
set(CMAKE_C_FLAGS_INIT "-arch x86_64")
set(CMAKE_CXX_FLAGS_INIT "-arch x86_64")

# Enable color diagnostics
add_compile_options(-fdiagnostics-color=always)

# Use libstdc++ (GCC's standard library)
add_compile_options(-stdlib=libstdc++)
add_link_options(-stdlib=libstdc++)

# Link with pthread for multithreading
add_link_options(-pthread)

message(STATUS "Using GCC at: ${CMAKE_CXX_COMPILER}")
message(STATUS "macOS deployment target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
message("macOS GCC toolchain CMAKE_C_COMPILER = ${CMAKE_C_COMPILER}")
message("macOS GCC toolchain CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")