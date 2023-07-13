#------------------------------------------------------------------------------
#
#
#
#
#------------------------------------------------------------------------------

include(FetchContent)

#------------------------------------------------------------------------------
#
#
#
#
#------------------------------------------------------------------------------

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)

add_library(Cinatra INTERFACE)
target_include_directories(Cinatra INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/cinatra/include")
target_compile_definitions(Cinatra INTERFACE CINATRA_ENABLE_GZIP CINATRA_ENABLE_SSL)
target_link_libraries(Cinatra INTERFACE Threads::Threads OpenSSL::SSL OpenSSL::Crypto ZLIB::ZLIB)

# Cinatra depends on codecvt_utf8<wchar_t> which is marked deprecated in C++17; this triggers 
# warnings when compiling using Clang with the MSVC frontend....silence those warnings 

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    target_compile_definitions(Cinatra INTERFACE _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
  elseif (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
    # Do nothing as this is a non-MSVC variant.
  endif()
endif()
