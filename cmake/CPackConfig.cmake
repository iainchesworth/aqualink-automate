set(CPACK_PACKAGE_DIRECTORY "${CMAKE_SOURCE_DIR}/packages")
set(CPACK_STRIP_FILES ON)
set(CPACK_THREADS 0)

include(GNUInstallDirs)

# ---------------------------------------------------------------------------
# Runtime-dependency bundling
# ===========================
# The shipped triplets link the vcpkg dependencies (Boost/OpenSSL/sqlite3/zlib)
# DYNAMICALLY (see cmake/vcpkg/triplets/*). Those libraries live in the build
# tree under vcpkg_installed/<triplet>/{lib,bin} and are NOT system packages, so
# every distributable package must carry them or the installed binary fails to
# start ("error while loading shared libraries ...").
#
#   - Windows : fixup_bundle copies the dependency DLLs next to the .exe, and
#               InstallRequiredSystemLibraries adds the MSVC runtime DLLs.
#   - Linux   : the .so files are installed into a PRIVATE lib dir and the
#               executable carries an $ORIGIN-relative RPATH so the loader finds
#               them without polluting the system library path.
#   - macOS   : the .dylib files are installed into the same private lib dir and
#               the executable carries an @loader_path-relative RPATH. (vcpkg
#               builds @rpath-clean dylibs.)
# ---------------------------------------------------------------------------

set(AQ_VCPKG_LIB_DIR "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/lib")
set(AQ_VCPKG_BIN_DIR "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/bin")

# Private directory (relative to the install prefix) that holds the bundled
# vcpkg runtime libraries on Linux/macOS. A package-specific subdirectory keeps
# the vendored libraries out of the shared system library path.
set(AQ_PRIVATE_LIBDIR "${CMAKE_INSTALL_LIBDIR}/aqualink-automate")

if("${CMAKE_SYSTEM_NAME}" MATCHES "Windows")

    # Windows is not an FHS platform: keep the package self-contained with the
    # web/ssl assets sitting beside the executable in bin/ (the runtime resolves
    # them relative to the executable — see platform/windows/application_defaults.cpp).
    set(AQ_WEB_DESTINATION "bin/web")
    set(AQ_SSL_DESTINATION "bin/ssl")
    set(AQ_EXAMPLES_DESTINATION "examples")

    # The executable MUST be in the same component (Runtime) as the fixup_bundle
    # install(CODE) below. NSIS does a per-component install, so if the exe were in
    # the default (Unspecified) component, fixup_bundle would run during the Runtime
    # component install with the exe not yet staged -> "fixup_bundle: not a valid
    # bundle". (ZIP is monolithic so it happened to work regardless.)
    install(TARGETS aqualink-automate RUNTIME DESTINATION bin COMPONENT Runtime)

    set(BUNDLE_APPS \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/bin/aqualink-automate${CMAKE_EXECUTABLE_SUFFIX})
    install(
        CODE
        "
            include(BundleUtilities)
            fixup_bundle(
                \"${BUNDLE_APPS}\"
                \"\"
                \"${AQ_VCPKG_BIN_DIR}\"
            )
        "
        COMPONENT Runtime
    )

    # Bundle the MSVC C/C++ runtime DLLs (vcruntime140.dll, msvcp140.dll, ...).
    # fixup_bundle treats these System32 DLLs as "system" and skips them, so a
    # clean machine without the VC++ redistributable would fail to launch.
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION bin)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP FALSE)
    include(InstallRequiredSystemLibraries)

    # Windows installer
    set(CPACK_GENERATOR "ZIP;NSIS" CACHE STRING "Package targets")

    # NSIS-specific configuration
    set(CPACK_NSIS_DISPLAY_NAME "Aqualink Automate")
    set(CPACK_NSIS_PACKAGE_NAME "Aqualink Automate")
    set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MODIFY_PATH OFF)

elseif("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")

    # FHS-aligned, relocatable layout. The executable resolves its assets
    # relative to itself (../share/aqualink-automate/...) via boost::dll, so the
    # same install tree works whether unpacked from the .tgz anywhere or
    # installed by the .deb/.rpm under /usr.
    set(AQ_WEB_DESTINATION "${CMAKE_INSTALL_DATADIR}/aqualink-automate/web")
    set(AQ_SSL_DESTINATION "${CMAKE_INSTALL_DATADIR}/aqualink-automate/ssl")
    set(AQ_EXAMPLES_DESTINATION "${CMAKE_INSTALL_DOCDIR}/examples")

    # $ORIGIN is the directory of the executable (bin/); the bundled libraries
    # live in the private lib dir. The literal "$ORIGIN" must reach the linker,
    # so it is NOT a CMake ${} expansion.
    set_target_properties(aqualink-automate PROPERTIES
        INSTALL_RPATH "$ORIGIN/../${AQ_PRIVATE_LIBDIR}")

    install(TARGETS aqualink-automate RUNTIME DESTINATION bin)

    install(DIRECTORY "${AQ_VCPKG_LIB_DIR}/"
        DESTINATION ${AQ_PRIVATE_LIBDIR}
        COMPONENT Runtime
        FILES_MATCHING
            PATTERN "*.so*"
            PATTERN "pkgconfig" EXCLUDE
            PATTERN "cmake" EXCLUDE)

    # System installation packages for unix systems
    set(CPACK_GENERATOR "TGZ;DEB;RPM" CACHE STRING "Package targets")

    # The dependency libraries are bundled privately, so dpkg-shlibdeps must NOT
    # try to resolve them against system packages (it would fail / mis-declare).
    # Declare only the genuine system runtime dependencies explicitly.
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libstdc++6, libgcc-s1")
    set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
    # RPM's automatic dependency scanner sees the bundled .so as both provided
    # (by the private lib dir) and required (by the binary), so it self-resolves.
    set(CPACK_RPM_FILE_NAME "RPM-DEFAULT")

 else()

    # macOS — relocatable bundle mirroring the Linux layout.
    set(AQ_WEB_DESTINATION "${CMAKE_INSTALL_DATADIR}/aqualink-automate/web")
    set(AQ_SSL_DESTINATION "${CMAKE_INSTALL_DATADIR}/aqualink-automate/ssl")
    set(AQ_EXAMPLES_DESTINATION "${CMAKE_INSTALL_DOCDIR}/examples")

    set_target_properties(aqualink-automate PROPERTIES
        INSTALL_RPATH "@loader_path/../${AQ_PRIVATE_LIBDIR}")

    install(TARGETS aqualink-automate RUNTIME DESTINATION bin)

    install(DIRECTORY "${AQ_VCPKG_LIB_DIR}/"
        DESTINATION ${AQ_PRIVATE_LIBDIR}
        COMPONENT Runtime
        FILES_MATCHING
            PATTERN "*.dylib"
            PATTERN "pkgconfig" EXCLUDE
            PATTERN "cmake" EXCLUDE)

    # macOS installer
    set(CPACK_GENERATOR "TGZ;DragNDrop" CACHE STRING "Package targets")

    # DMG-specific configuration
    set(CPACK_DMG_VOLUME_NAME "Aqualink Automate")
    set(CPACK_DMG_FORMAT "UDZO")

endif()

# ---------------------------------------------------------------------------
# Asset installs (common; destinations chosen per-platform above)
# ---------------------------------------------------------------------------

install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets/ssl/ DESTINATION ${AQ_SSL_DESTINATION} COMPONENT SslAssets)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets/web/ DESTINATION ${AQ_WEB_DESTINATION} COMPONENT WebAssets)

# Stamp the installed service worker's cache name with the build version (mirrors
# the dev-tree POST_BUILD step in src/CMakeLists.txt) so a packaged upgrade purges
# the previous version's offline shell on activate. Declared after the web-asset
# directory install so it runs once those files are staged.
install(CODE "
    execute_process(COMMAND \"${CMAKE_COMMAND}\"
        -D \"SW_JS=\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${AQ_WEB_DESTINATION}/sw.js\"
        -D \"AQ_VERSION=${PROJECT_VERSION}\"
        -P \"${CMAKE_SOURCE_DIR}/cmake/stamp_sw_version.cmake\")
" COMPONENT WebAssets)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/examples/
    DESTINATION ${AQ_EXAMPLES_DESTINATION}
    COMPONENT ExampleConfigs
    FILES_MATCHING PATTERN "*.conf")

#------------------------------------------------------------------------------
#
#    PACKAGE METADATA
#
#------------------------------------------------------------------------------

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_CONTACT "Iain Chesworth")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${CMAKE_PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_VENDOR "")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_PACKAGE_CHECKSUM SHA512)
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

# DEB/RPM architecture and metadata defaults. These come BEFORE the prerelease
# block so a prerelease build can override CPACK_RPM_PACKAGE_RELEASE.
#
# The architecture is DERIVED from the build's target processor (set by the
# toolchain: x86_64 or aarch64) rather than hardcoded, so a native arm64 build
# is labelled arm64/aarch64 — otherwise an arm64 .deb would be stamped amd64 and
# apt would refuse to install it on a Raspberry Pi. DEB-DEFAULT/RPM-DEFAULT embed
# this architecture in the package filename, so the two arches never collide.
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    set(AQ_ARCH_LABEL "arm64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
    set(CPACK_RPM_PACKAGE_ARCHITECTURE "aarch64")
else()
    set(AQ_ARCH_LABEL "amd64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
endif()
set(CPACK_DEBIAN_PACKAGE_SECTION "net")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_RPM_PACKAGE_RELEASE 1)

# Encode any prerelease label (alpha/beta/rc) so prerelease packages do not
# collide with — and sort BEFORE — their eventual final release.
if(PROJECT_VERSION_PRERELEASE)
    # Archive/installer filenames (ZIP/TGZ/NSIS/DMG) carry the label directly.
    set(CPACK_PACKAGE_FILE_NAME
        "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-${PROJECT_VERSION_PRERELEASE}-${CMAKE_SYSTEM_NAME}")
    # Debian: '~' sorts a prerelease lower than the final (1.0.0~beta.1 < 1.0.0).
    set(CPACK_DEBIAN_PACKAGE_VERSION "${PROJECT_VERSION}~${PROJECT_VERSION_PRERELEASE}")
    # RPM: encode the prerelease in the Release field (0.<pre>) so it sorts
    # before the final release's Release value (1).
    set(CPACK_RPM_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_RPM_PACKAGE_RELEASE "0.${PROJECT_VERSION_PRERELEASE}")
endif()

# Disambiguate the per-arch Linux archive (TGZ). The DEB/RPM names already embed
# the architecture (via *-DEFAULT + CPACK_*_PACKAGE_ARCHITECTURE) and ignore
# CPACK_PACKAGE_FILE_NAME, but the TGZ name is "<name>-<version>[-<pre>]-Linux"
# with NO arch — so an x64 and an arm64 build would produce identically named
# tarballs that clobber each other when every platform's packages are gathered
# into a single GitHub release. Fold the arch into CPACK_PACKAGE_FILE_NAME on
# Linux (this drives the TGZ name; deb/rpm are unaffected — verified empirically:
# CPACK_ARCHIVE_FILE_NAME is NOT honoured by the archive generator here, the TGZ
# follows CPACK_PACKAGE_FILE_NAME). Windows/macOS keep their single-arch names.
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    if(PROJECT_VERSION_PRERELEASE)
        set(CPACK_PACKAGE_FILE_NAME
            "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-${PROJECT_VERSION_PRERELEASE}-${CMAKE_SYSTEM_NAME}-${AQ_ARCH_LABEL}")
    else()
        set(CPACK_PACKAGE_FILE_NAME
            "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${AQ_ARCH_LABEL}")
    endif()
endif()

#------------------------------------------------------------------------------
#
#
#
#
#------------------------------------------------------------------------------

include(CPack)

# Add a custom target to enable CPack to be run under Visual Studio
add_custom_target(
	pack-${PROJECT_NAME}
	COMMAND ${CMAKE_CPACK_COMMAND} -C $<CONFIGURATION> --config ${CPACK_OUTPUT_CONFIG_FILE}
	COMMENT "Running CPack. Please wait..."
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
)

set_target_properties(pack-${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD 1)
