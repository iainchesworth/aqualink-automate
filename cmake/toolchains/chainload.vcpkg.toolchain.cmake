#------------------------------------------------------------------------------
#
# Chainloading the vcpkg.cmake file.  Note that this needs to be included
# as an include at the bottom of each of the "toolchain" cmake files.
#
#------------------------------------------------------------------------------

if(NOT DEFINED ENV{VCPKG_ROOT})
    # This is the expected default...don't publish anything to the user.
    set(VCPKG_ROOT "deps/vcpkg")
else()
    message(STATUS "VCPKG_ROOT is set to $ENV{VCPKG_ROOT}")
    set(VCPKG_ROOT $ENV{VCPKG_ROOT})
endif()

include("${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
