#------------------------------------------------------------------------------
#
# 
# 
#
#------------------------------------------------------------------------------

# Toolchain settings go here (if required)
option(USE_LIBCXX "Use libc++ instead of libstdc++" ON)

if(USE_LIBCXX)
	set(LINUX_FLAGS_CXX "${LINUX_FLAGS_CXX} -stdlib=libc++") # for both compiler & linker
endif(USE_LIBCXX)

#------------------------------------------------------------------------------
#
# Chainloading the vcpkg.cmake file.  Note that this needs to be included
# as an include at the bottom of each of the "toolchain" cmake files.
#
#------------------------------------------------------------------------------

include(cmake/toolchains/chainload.vcpkg.toolchain.cmake)
