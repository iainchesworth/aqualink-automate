# Hardened "diagnostic" profiling triplet (see docs/profiling.md).
#
# Identical to x64-linux-gcc, but builds the Tracy client with
# TRACY_ONLY_LOCALHOST + TRACY_NO_BROADCAST so the profiler's TCP data port
# binds to loopback only and no UDP discovery beacon is emitted. These are
# COMPILE-TIME Tracy macros (not runtime env vars), so they must be baked into
# the prebuilt TracyClient here rather than set by the application.
#
# NOTE: a distinct triplet changes the vcpkg ABI, so the first configure with
# this triplet rebuilds the dependency set into its own installed tree.
include("${CMAKE_CURRENT_LIST_DIR}/x64-linux-gcc.cmake")

if(PORT STREQUAL "tracy")
    string(APPEND VCPKG_CXX_FLAGS " -DTRACY_ONLY_LOCALHOST -DTRACY_NO_BROADCAST")
    string(APPEND VCPKG_C_FLAGS " -DTRACY_ONLY_LOCALHOST -DTRACY_NO_BROADCAST")
endif()
