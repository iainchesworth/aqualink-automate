set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

# Force use of MSVC compiler (not Clang)
set(VCPKG_PLATFORM_TOOLSET "v143")

# Use standard MSVC detection but ensure our custom settings
set(VCPKG_ENV_PASSTHROUGH PATH INCLUDE LIB LIBPATH)
# Only chainload minimal toolchain customization  
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../../toolchains/windows.msvc.toolchain.cmake")