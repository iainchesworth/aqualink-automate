#------------------------------------------------------------------------------
#
# Chainloading the vcpkg.cmake file.  Note that this needs to be included
# as an include at the bottom of each of the "toolchain" cmake files.
#
#------------------------------------------------------------------------------
include(${CMAKE_CURRENT_LIST_DIR}/../../deps/vcpkg/scripts/buildsystems/vcpkg.cmake)
