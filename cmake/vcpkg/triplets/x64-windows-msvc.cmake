# Linkage policy: dynamic CRT + dynamic dependency libraries. This is the
# distribution default — packaging (ZIP/NSIS) ships the runtime DLLs and an
# all-dynamic build keeps install/upgrade footprints small. A static-CRT /
# static-library variant (which would remove the per-call DLL import thunks on
# Boost/OpenSSL/zlib hot paths) was evaluated and intentionally DEFERRED: it
# would need its own non-distributable internal release/benchmark triplet +
# preset rather than changing this shipped triplet. Keep dynamic here.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

# Chainload policy (see also the Linux/macOS triplets and CMakePresets.json):
#   - The project's own build always chainloads its toolchain via
#     VCPKG_CHAINLOAD_TOOLCHAIN_FILE in CMakePresets.json.
#   - The Linux/macOS triplets ALSO set VCPKG_CHAINLOAD_TOOLCHAIN_FILE so that
#     vcpkg builds the dependency ports with the very same gcc/clang the project
#     uses. On Windows we deliberately do NOT chainload into the port builds:
#     vcpkg builds the ports with its default MSVC toolset (and clang-cl for the
#     LLVM triplet, which is MSVC-ABI compatible), which is required by ports
#     that drive their own build systems (e.g. OpenSSL's nmake build needs the
#     unmodified MSVC environment). Hence the documented Windows/Unix asymmetry.

# Force use of MSVC compiler (not Clang)
set(VCPKG_PLATFORM_TOOLSET "v143")

# Pass through MSVC environment variables so vcpkg port builds (including
# nmake-based ones like OpenSSL) can find headers, libraries, and tools.
set(VCPKG_ENV_PASSTHROUGH PATH INCLUDE LIB LIBPATH)
