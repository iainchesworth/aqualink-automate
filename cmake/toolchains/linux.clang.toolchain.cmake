#------------------------------------------------------------------------------
#
# 
# 
#
#------------------------------------------------------------------------------

# Detect compiler
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # If GCC, check the version
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "17.0.0")
    message(FATAL_ERROR "GCC version must be at least 17.0.0")
  else()
    message(FATAL_ERROR "GCC is not the specifed compiler...expected a GCC compiler")
  endif()
endif()

# Toolchain settings go here (if required)

if(NOT DEFINED CMAKE_C_COMPILER)
set(CMAKE_C_COMPILER "clang-17")
endif(NOT DEFINED CMAKE_C_COMPILER)

if(NOT DEFINED CMAKE_CXX_COMPILER)
set(CMAKE_CXX_COMPILER "clang++-17")
endif(NOT DEFINED CMAKE_CXX_COMPILER)

option(USE_LIBCXX "Use libc++ instead of libstdc++" ON)

if(USE_LIBCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++") # for both compiler & linker
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lc++abi")
endif(USE_LIBCXX)

#------------------------------------------------------------------------------
#
# Chainloading the vcpkg.cmake file.  Note that this needs to be included
# as an include at the bottom of each of the "toolchain" cmake files.
#
#------------------------------------------------------------------------------

include(${CMAKE_CURRENT_LIST_DIR}/chainload.vcpkg.toolchain.cmake)
