#------------------------------------------------------------------------------
#
# 
# 
#
#------------------------------------------------------------------------------

# Detect compiler
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # If GCC, check the version
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "13.0.0")
    message(FATAL_ERROR "GCC version must be at least 13.0.0")
  endif()
endif()

# Toolchain settings go here (if required)

#------------------------------------------------------------------------------
#
# Chainloading the vcpkg.cmake file.  Note that this needs to be included
# as an include at the bottom of each of the "toolchain" cmake files.
#
#------------------------------------------------------------------------------

include(${CMAKE_CURRENT_LIST_DIR}/chainload.vcpkg.toolchain.cmake)
