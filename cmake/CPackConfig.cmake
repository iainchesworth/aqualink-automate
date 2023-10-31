set(CPACK_PACKAGE_DIRECTORY "${CMAKE_SOURCE_DIR}/packages")
set(CPACK_STRIP_FILES ON)
set(CPACK_THREADS 0)

if("${CMAKE_SYSTEM_NAME}" MATCHES "Windows")

    install(TARGETS aqualink-automate)

    set(BUNDLE_APPS \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/bin/aqualink-automate${CMAKE_EXECUTABLE_SUFFIX})
    install(
        CODE 
        "
            include(BundleUtilities)
            fixup_bundle(
                \"${BUNDLE_APPS}\" 
                \"\" 
                \"${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/bin\"
            )
        "
        COMPONENT Runtime
    )

    install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets/ssl/ DESTINATION bin/ssl COMPONENT SslAssets)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets/web/ DESTINATION bin/web COMPONENT WebAssets)

    # Windows installer
    set(CPACK_GENERATOR "ZIP" CACHE STRING "Package targets")

elseif("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")

    include(GNUInstallDirs)

    install(TARGETS aqualink-automate)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets/ssl/ DESTINATION ./ssl COMPONENT SslAssets)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets/web/ DESTINATION ./web COMPONENT WebAssets)

    # System installation packages for unix systems
    set(CPACK_GENERATOR "TGZ;DEB;RPM" CACHE STRING "Package targets")

 else()

    install(TARGETS aqualink-automate)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets/ssl/ DESTINATION ./ssl COMPONENT SslAssets)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets/web/ DESTINATION ./web COMPONENT WebAssets)

    # Default (portable package for any platform)
    set(CPACK_GENERATOR "ZIP;TGZ" CACHE STRING "Package targets")

endif()

#------------------------------------------------------------------------------
#
#
#
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

#------------------------------------------------------------------------------
#
#
#
#
#------------------------------------------------------------------------------

set(CPACK_DEBIAN_PACKAGE_DEPENDS "libboost-dev, libssl")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "all")
set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_RPM_PACKAGE_RELEASE 1)
set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")

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
