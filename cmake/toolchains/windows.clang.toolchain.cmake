#------------------------------------------------------------------------------
#
# 
# 
#
#------------------------------------------------------------------------------

# Toolchain settings go here (if required)

if(NOT DEFINED CMAKE_C_COMPILER)
set(CMAKE_C_COMPILER "clang.exe")
endif(NOT DEFINED CMAKE_C_COMPILER)

if(NOT DEFINED CMAKE_CXX_COMPILER)
set(CMAKE_CXX_COMPILER "clang++.exe")
endif(NOT DEFINED CMAKE_CXX_COMPILER)

#------------------------------------------------------------------------------
#
# Chainloading the vcpkg.cmake file.  Note that this needs to be included
# as an include at the bottom of each of the "toolchain" cmake files.
#
#------------------------------------------------------------------------------

include(${CMAKE_CURRENT_LIST_DIR}/chainload.vcpkg.toolchain.cmake)
