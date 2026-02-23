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

