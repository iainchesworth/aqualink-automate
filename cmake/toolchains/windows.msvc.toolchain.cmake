#------------------------------------------------------------------------------
#
# 
# 
#
#------------------------------------------------------------------------------

# Toolchain settings go here (if required)

if(NOT DEFINED CMAKE_C_COMPILER)
set(CMAKE_C_COMPILER "cl.exe")
endif(NOT DEFINED CMAKE_C_COMPILER)

if(NOT DEFINED CMAKE_CXX_COMPILER)
set(CMAKE_CXX_COMPILER "cl.exe")
endif(NOT DEFINED CMAKE_CXX_COMPILER)

if (POLICY CMP0141)
  # Enable Hot Reload for MSVC compilers if supported.
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

#------------------------------------------------------------------------------
#
# Chainloading the vcpkg.cmake file.  Note that this needs to be included
# as an include at the bottom of each of the "toolchain" cmake files.
#
#------------------------------------------------------------------------------

include(${CMAKE_CURRENT_LIST_DIR}/chainload.vcpkg.toolchain.cmake)