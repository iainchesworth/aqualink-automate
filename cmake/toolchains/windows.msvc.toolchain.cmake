#------------------------------------------------------------------------------
#
# 
# 
#
#------------------------------------------------------------------------------

# Toolchain settings go here (if required)

#------------------------------------------------------------------------------
#
# Chainloading the vcpkg.cmake file.  Note that this needs to be included
# as an include at the bottom of each of the "toolchain" cmake files.
#
#------------------------------------------------------------------------------

message(STATUS "Chainloading vcpkg.cmake...")
include(${CMAKE_CURRENT_LIST_DIR}/chainload.vcpkg.toolchain.cmake)
