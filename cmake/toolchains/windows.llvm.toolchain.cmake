#------------------------------------------------------------------------------
# Windows LLVM/Clang Toolchain Configuration
#------------------------------------------------------------------------------

message(STATUS "Configuring Windows Toolchain (LLVM/Clang Variant)")

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

# Use MSVC's link.exe rather than lld-link. lld-link cannot resolve
# compiler-synthesized DLL-exported members (e.g. implicit move constructors)
# which causes link failures with dynamic Boost libraries. clang-cl is fully
# ABI-compatible with MSVC so link.exe works without issues.
#
# Explicitly find MSVC's link.exe via VCToolsInstallDir to avoid picking up
# Git's /usr/bin/link.exe which shadows it on many systems.
find_program(CMAKE_LINKER NAMES link.exe
    PATHS "$ENV{VCToolsInstallDir}/bin/Hostx64/x64"
    NO_DEFAULT_PATH
    REQUIRED
)

# Compiler flags for MSVC compatibility
add_compile_options("/utf-8")
add_compile_options("/bigobj")

# Link with MSVC runtime
set(CMAKE_CXX_FLAGS_INIT "/MD")
set(CMAKE_C_FLAGS_INIT "/MD")

message(STATUS "Using LLVM/Clang at: ${CMAKE_CXX_COMPILER}")
