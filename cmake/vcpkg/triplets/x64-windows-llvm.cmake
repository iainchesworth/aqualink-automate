set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

# The following parameters enable vcpkg to work as the MSVC and LLVM toolsets
# are assumed to be available in the path.  Note that OpenSSL requires "nmake"

set(VCPKG_ENV_PASSTHROUGH PATH)
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../../toolchains/windows.llvm.toolchain.cmake")