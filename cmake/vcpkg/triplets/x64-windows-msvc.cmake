set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

# Force use of MSVC compiler (not Clang)
set(VCPKG_PLATFORM_TOOLSET "v143")

# Pass through MSVC environment variables so vcpkg port builds (including
# nmake-based ones like OpenSSL) can find headers, libraries, and tools.
set(VCPKG_ENV_PASSTHROUGH PATH INCLUDE LIB LIBPATH)
