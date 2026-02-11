set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

# Only build release variants of dependencies.  The LLVM toolchain forces
# /MD (release CRT) for all build types — including Debug — because ASan
# requires the release runtime.  Linking our /MD code against /MDd debug
# DLLs causes CRT mismatch (different _ITERATOR_DEBUG_LEVEL, different
# runtime DLLs) which corrupts std::string operations across DLL boundaries
# and manifests as false ASan positives during static initialisation.
set(VCPKG_BUILD_TYPE release)

# Let vcpkg build dependencies with its default MSVC toolchain (clang-cl
# produces MSVC-compatible binaries so the resulting libraries link fine).
# The main project's LLVM toolchain is set via VCPKG_CHAINLOAD_TOOLCHAIN_FILE
# in CMakePresets.json, which only affects the project build — not vcpkg ports.
#
# Pass through MSVC environment variables so ports that need headers, libs,
# and tools (e.g. OpenSSL requires nmake) can find them.
set(VCPKG_ENV_PASSTHROUGH PATH INCLUDE LIB LIBPATH)
