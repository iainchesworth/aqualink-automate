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

# Cinatra depends on codecvt_utf8<wchar_t> which is marked deprecated in C++17; this triggers 
# warnings when compiling using Clang with the MSVC frontend....silence those warnings 

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    target_compile_definitions(Cinatra INTERFACE _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
  elseif (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
    # Do nothing as this is a non-MSVC variant.
  endif()
endif()
