include(ExternalProject)

#------------------------------------------------------------------------------
#
#
#
#
#------------------------------------------------------------------------------

find_package(asio REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

add_library(Crow INTERFACE)
target_include_directories(Crow INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/crow/include")
target_link_libraries(Crow INTERFACE asio::asio OpenSSL::SSL OpenSSL::Crypto ZLIB::ZLIB)

#ExternalProject_Add(crowcpp
#	DOWNLOAD_COMMAND ""
#	SOURCE_DIR ${CMAKE_SOURCE_DIR}/deps/crowcpp
#	BINARY_DIR ${CMAKE_BINARY_DIR}/deps/crowcpp
#	INSTALL_DIR ${CMAKE_BINARY_DIR}/deps/crowcpp
#	CMAKE_ARGS
#		-DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/deps/vcpkg/scripts/buildsystems/vcpkg.cmake
#		-DCROW_BUILD_EXAMPLES=OFF
#		-DCROW_BUILD_TESTS=OFF
#		-DCROW_FEATURES=compression,ssl
#		-DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/deps/crowcpp
#)

#find_package(Crow REQUIRED CONFIG PATHS "${CMAKE_BINARY_DIR}/deps/crowcpp")

#set(Crow_INCLUDE_DIRS "${CMAKE_BINARY_DIR}/deps/crowcpp/include" PARENT_SCOPE)
