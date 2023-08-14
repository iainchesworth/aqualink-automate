#------------------------------------------------------------------------------
#
# 
# 
#
#------------------------------------------------------------------------------

message(STATUS "Configuring Windows Toolchain")

# Toolchain settings go here (if required)

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/bigobj>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/bigobj>")
