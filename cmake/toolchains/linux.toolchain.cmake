#------------------------------------------------------------------------------
#
# 
# 
#
#------------------------------------------------------------------------------

message(STATUS "Configuring Linux Toolchain")

# Toolchain settings go here (if required)

message(STATUS "C Compiler: ${CMAKE_C_COMPILER} (${CMAKE_C_COMPILER_ID})")
message(STATUS "CXX Compiler: ${CMAKE_CXX_COMPILER} (${CMAKE_CXX_COMPILER_ID})")


if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
  message(STATUS "Setting up usage of libc++")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -nostdinc++ -nostdlib++ -isystem /usr/lib/llvm-16/include/c++/v1")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++ -lc++abi -L /usr/lib/llvm-16/lib -Wl,-rpath,/usr/lib/llvm-16/lib")
endif()
