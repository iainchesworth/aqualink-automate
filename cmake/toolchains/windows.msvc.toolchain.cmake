#------------------------------------------------------------------------------
# Windows MSVC Toolchain Configuration 
# Minimal customization that works with vcpkg standard detection
#------------------------------------------------------------------------------

message(STATUS "Configuring Windows Toolchain (MSVC Variant)")

# Only set our specific customizations, let vcpkg handle standard MSVC detection
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/bigobj>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/bigobj>")

# Set up Ninja build tool path
set(CMAKE_NINJA_PATH "${CMAKE_CURRENT_LIST_DIR}/../../deps/vcpkg/downloads/tools/cmake-3.30.1-windows/cmake-3.30.1-windows-i386/bin/ninja.exe")
if(EXISTS "${CMAKE_NINJA_PATH}")
    set(CMAKE_MAKE_PROGRAM "${CMAKE_NINJA_PATH}")
    message(STATUS "Using integrated Ninja at: ${CMAKE_NINJA_PATH}")
endif()

# AddressSanitizer support (disabled by default due to compatibility issues)
# Uncomment the following lines if you want to enable AddressSanitizer
# set(CMAKE_CXX_FLAGS_INIT "/fsanitize=address")
# set(CMAKE_C_FLAGS_INIT "/fsanitize=address")