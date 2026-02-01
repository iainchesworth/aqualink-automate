#------------------------------------------------------------------------------
# Windows LLVM/Clang Toolchain Configuration
#------------------------------------------------------------------------------

message(STATUS "Configuring Windows Toolchain (LLVM/Clang Variant)")

# include("${CMAKE_CURRENT_LIST_DIR}/windows.common.toolchain.cmake")

# Find clang-cl.exe (MSVC-compatible clang frontend)
find_program(CMAKE_C_COMPILER clang-cl.exe
    HINTS
        "C:/Program Files/LLVM/bin"
        "$ENV{ProgramFiles}/LLVM/bin"
        "C:/Program Files (x86)/LLVM/bin"
        #"$ENV{ProgramFiles(x86)}/LLVM/bin"
    REQUIRED
)

find_program(CMAKE_CXX_COMPILER clang-cl.exe
    HINTS
        "C:/Program Files/LLVM/bin"
        "$ENV{ProgramFiles}/LLVM/bin"
        "C:/Program Files (x86)/LLVM/bin"
        #"$ENV{ProgramFiles(x86)}/LLVM/bin"
    REQUIRED
)

# Use lld-link as linker (compatible with MSVC)
find_program(CMAKE_LINKER lld-link.exe
    HINTS
        "C:/Program Files/LLVM/bin"
        "$ENV{ProgramFiles}/LLVM/bin"
        "C:/Program Files (x86)/LLVM/bin"
        #"$ENV{ProgramFiles(x86)}/LLVM/bin"
)

# Compiler flags for MSVC compatibility
add_compile_options("/utf-8")
add_compile_options("/bigobj")

# Link with MSVC runtime
set(CMAKE_CXX_FLAGS_INIT "${_include_flags} /MD")
set(CMAKE_C_FLAGS_INIT "${_include_flags} /MD")

# Set up Ninja build tool path
set(CMAKE_NINJA_PATH "${CMAKE_CURRENT_LIST_DIR}/../../deps/vcpkg/downloads/tools/cmake-3.30.1-windows/cmake-3.30.1-windows-i386/bin/ninja.exe")
if(EXISTS "${CMAKE_NINJA_PATH}")
    set(CMAKE_MAKE_PROGRAM "${CMAKE_NINJA_PATH}")
    # Add Ninja to PATH for better availability
    get_filename_component(NINJA_DIR "${CMAKE_NINJA_PATH}" DIRECTORY)
    set(ENV{PATH} "${NINJA_DIR};$ENV{PATH}")
    message(STATUS "Using integrated Ninja at: ${CMAKE_NINJA_PATH}")
endif()

message(STATUS "Using LLVM/Clang at: ${CMAKE_CXX_COMPILER}")
message(STATUS "MSVC include directory: ${WIN_VC_INCLUDE_DIR}")
message(STATUS "Windows SDK include version: ${WINSDK_INCLUDE_VER}")
message("Windows LLVM toolchain CMAKE_C_COMPILER = ${CMAKE_C_COMPILER}")
message("Windows LLVM toolchain CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")
