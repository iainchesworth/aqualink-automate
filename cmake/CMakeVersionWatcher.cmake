# CMakeVersionWatcher.cmake
#
# This file defines a target that regenerates version_cmake.constants.cpp
# at build time (not just configure time). This ensures the version info
# is always up-to-date, following the same pattern as GitWatcher.cmake.
#
# Required variables (set before including this file):
#   VERSION_CMAKE_SOURCE_FILE -- The .in template file path.
#   VERSION_CMAKE_DEST_FILE   -- The output configured file path.

function(ConfigureVersionFile)
    # Read the cached project variables written at configure time.
    include("${VERSION_CMAKE_VARS_FILE}")

    configure_file("${VERSION_CMAKE_SOURCE_FILE}" "${VERSION_CMAKE_DEST_FILE}" @ONLY)
endfunction()

function(SetupCMakeVersionMonitoring)
    # Convert to absolute paths for build-time invocation.
    get_filename_component(_abs_source "${VERSION_CMAKE_SOURCE_FILE}" ABSOLUTE)
    get_filename_component(_abs_dest "${VERSION_CMAKE_DEST_FILE}" ABSOLUTE)

    # Write project variables to a cache file that the build-time script can include.
    # This avoids issues with spaces in values when passing via -D command-line args.
    set(_vars_file "${CMAKE_CURRENT_BINARY_DIR}/cmake_version_vars.cmake")
    file(WRITE "${_vars_file}"
        "set(PROJECT_NAME \"${PROJECT_NAME}\")\n"
        "set(PROJECT_VERSION \"${PROJECT_VERSION}\")\n"
        "set(PROJECT_DESCRIPTION \"${PROJECT_DESCRIPTION}\")\n"
        "set(PROJECT_HOMEPAGE_URL \"${PROJECT_HOMEPAGE_URL}\")\n"
        "set(PROJECT_VERSION_MAJOR \"${PROJECT_VERSION_MAJOR}\")\n"
        "set(PROJECT_VERSION_MINOR \"${PROJECT_VERSION_MINOR}\")\n"
        "set(PROJECT_VERSION_PATCH \"${PROJECT_VERSION_PATCH}\")\n"
    )

    add_custom_target(check_cmake_version
        ALL
        DEPENDS ${_abs_source}
        BYPRODUCTS ${_abs_dest}
        COMMENT "Regenerating CMake version constants..."
        COMMAND
            ${CMAKE_COMMAND}
            -D_BUILD_TIME_CHECK_CMAKE_VERSION=TRUE
            -DVERSION_CMAKE_SOURCE_FILE=${_abs_source}
            -DVERSION_CMAKE_DEST_FILE=${_abs_dest}
            -DVERSION_CMAKE_VARS_FILE=${_vars_file}
            -P "${CMAKE_CURRENT_LIST_FILE}")
endfunction()

function(Main_CMakeVersion)
    if(_BUILD_TIME_CHECK_CMAKE_VERSION)
        ConfigureVersionFile()
    else()
        SetupCMakeVersionMonitoring()
    endif()
endfunction()

Main_CMakeVersion()
