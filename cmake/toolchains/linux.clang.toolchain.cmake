#------------------------------------------------------------------------------
#
# 
# 
#
#------------------------------------------------------------------------------

# Toolchain settings go here (if required)
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
