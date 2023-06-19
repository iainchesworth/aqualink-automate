#------------------------------------------------------------------------------
#
# 
# 
#
#------------------------------------------------------------------------------

if(NOT DEFINED CMAKE_C_COMPILER)
set(CMAKE_C_COMPILER "clang.exe")
endif(NOT DEFINED CMAKE_C_COMPILER)

if(NOT DEFINED CMAKE_CXX_COMPILER)
set(CMAKE_CXX_COMPILER "clang++.exe")
endif(NOT DEFINED CMAKE_CXX_COMPILER)

# Detect compiler
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # If Clang, check the version
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "17.0.0")
    message(FATAL_ERROR "Clang version must be at least 17.0.0")
  else()
    message(FATAL_ERROR "Clang is not the specifed compiler...expected a Clang compiler")
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
