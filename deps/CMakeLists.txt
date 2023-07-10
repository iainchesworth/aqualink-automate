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
