#------------------------------------------------------------------------------
#
# 
# 
#
#------------------------------------------------------------------------------

# Detect compiler
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(FATAL_ERROR "Clang is not the specifed compiler...expected a Clang compiler")
endif()

# Toolchain settings go here (if required)

#------------------------------------------------------------------------------
#
# Chainloading the vcpkg.cmake file.  Note that this needs to be included
# as an include at the bottom of each of the "toolchain" cmake files.
#
#------------------------------------------------------------------------------

include(${CMAKE_CURRENT_LIST_DIR}/chainload.vcpkg.toolchain.cmake)
