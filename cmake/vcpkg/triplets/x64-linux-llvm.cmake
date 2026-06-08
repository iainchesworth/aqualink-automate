# Linkage policy: dynamic runtime + dynamic dependency libraries. This is the
# distribution default — packaging (TGZ/DEB/RPM) integrates with the system's
# shared libraries. A static-library variant (which would remove the PLT/GOT
# indirection on Boost/OpenSSL/zlib hot paths) was evaluated and intentionally
# DEFERRED: it would need its own non-distributable internal release/benchmark
# triplet + preset rather than changing this shipped triplet. Keep dynamic here.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CMAKE_SYSTEM_NAME Linux)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

set(VCPKG_ENV_PASSTHROUGH PATH)

# Chainload policy (see also the Windows triplets and CMakePresets.json):
# The project build always chainloads its toolchain via
# VCPKG_CHAINLOAD_TOOLCHAIN_FILE in CMakePresets.json. On Linux/macOS we ALSO
# chainload the same toolchain into the vcpkg port builds here, so the
# dependencies are compiled with the identical gcc/clang (and flags) as the
# project — this avoids ABI/stdlib mismatches with libstdc++/libc++. The
# Windows triplets deliberately omit this so vcpkg builds ports with the
# default MSVC toolset (required by ports like OpenSSL); that asymmetry is
# intentional.
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../../toolchains/linux.llvm.toolchain.cmake")