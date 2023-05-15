#------------------------------------------------------------------------------
#
#
#
#
#------------------------------------------------------------------------------

if(WIN32)

	if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		include(cmake/platforms/linux.clang.cmake)
	endif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

endif(WIN32)

#------------------------------------------------------------------------------
#
#
#
#
#------------------------------------------------------------------------------

if(LINUX)

	if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		include(cmake/platforms/linux.clang.cmake)
	endif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

endif(LINUX)