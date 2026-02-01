#------------------------------------------------------------------------------
# Windows GCC Toolchain Configuration (MinGW-w64)
#------------------------------------------------------------------------------

message(STATUS "Configuring Windows Toolchain (GCC/MinGW-w64 Variant)")

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Find MinGW-w64 GCC compilers
find_program(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc
    HINTS
        "C:/msys64/mingw64/bin"
        "C:/mingw64/bin"
        "$ENV{MINGW_ROOT}/bin"
        "$ENV{MSYS2_ROOT}/mingw64/bin"
    NAMES gcc x86_64-w64-mingw32-gcc
)

find_program(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++
    HINTS
        "C:/msys64/mingw64/bin"
        "C:/mingw64/bin"
        "$ENV{MINGW_ROOT}/bin"
        "$ENV{MSYS2_ROOT}/mingw64/bin"
    NAMES g++ x86_64-w64-mingw32-g++
)

# Find resource compiler
find_program(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres
    HINTS
        "C:/msys64/mingw64/bin"
        "C:/mingw64/bin"
        "$ENV{MINGW_ROOT}/bin"
        "$ENV{MSYS2_ROOT}/mingw64/bin"
    NAMES windres x86_64-w64-mingw32-windres
)

# Configure compiler flags
set(CMAKE_C_FLAGS_INIT "-m64")
set(CMAKE_CXX_FLAGS_INIT "-m64")

# Use static linking for portability
add_compile_options(-static-libgcc -static-libstdc++)
add_link_options(-static-libgcc -static-libstdc++)

# Enable UTF-8 source and execution character sets
add_compile_options(-finput-charset=utf-8 -fexec-charset=utf-8)

# Set up Ninja build tool path
set(CMAKE_NINJA_PATH "${CMAKE_CURRENT_LIST_DIR}/../../deps/vcpkg/downloads/tools/cmake-3.30.1-windows/cmake-3.30.1-windows-i386/bin/ninja.exe")
if(EXISTS "${CMAKE_NINJA_PATH}")
    set(CMAKE_MAKE_PROGRAM "${CMAKE_NINJA_PATH}")
    # Add Ninja to PATH for better availability
    get_filename_component(NINJA_DIR "${CMAKE_NINJA_PATH}" DIRECTORY)
    set(ENV{PATH} "${NINJA_DIR};$ENV{PATH}")
    message(STATUS "Using integrated Ninja at: ${CMAKE_NINJA_PATH}")
endif()

message(STATUS "Using MinGW-w64 GCC at: ${CMAKE_CXX_COMPILER}")
message("Windows GCC toolchain CMAKE_C_COMPILER = ${CMAKE_C_COMPILER}")
message("Windows GCC toolchain CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")
