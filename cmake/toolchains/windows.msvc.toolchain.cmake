#------------------------------------------------------------------------------
# Windows MSVC Toolchain Configuration 
# Minimal customization that works with vcpkg standard detection
#------------------------------------------------------------------------------

message(STATUS "Configuring Windows Toolchain (MSVC Variant)")

# Only set our specific customizations, let vcpkg handle standard MSVC detection.
# This toolchain is selected only when MSVC (cl.exe) is the active compiler for
# both languages, so the flags are applied unconditionally rather than guarded
# per-language by a redundant $<C/CXX_COMPILER_ID:MSVC> generator expression.
add_compile_options("/utf-8")
add_compile_options("/bigobj")

# Opt-in raised-ISA baseline for release/benchmark builds (default OFF).
# Enables AVX2 codegen for the hot byte-scan and checksum loops. Left OFF by
# default to keep the shipped binary runnable on generic x86-64 hardware; only
# enable for builds validated against the benchmark suite.
if(ENABLE_NATIVE_ARCH)
    add_compile_options("/arch:AVX2")
    message(STATUS "Raised-ISA baseline enabled (/arch:AVX2)")
endif()

