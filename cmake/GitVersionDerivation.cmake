# GitVersionDerivation.cmake
#
# Derives semver version components from git tags at configure time.
# Include this BEFORE project() in the root CMakeLists.txt.
#
# Runs: git describe --tags --match "v*" --always HEAD
# Parses tag format: v<MAJOR>.<MINOR>.<PATCH>[-<prerelease>]
#   where prerelease is e.g. alpha.1, beta.2, rc.1
#
# Sets the following variables:
#   DERIVED_VERSION_MAJOR      -- Major version number
#   DERIVED_VERSION_MINOR      -- Minor version number
#   DERIVED_VERSION_PATCH      -- Patch version number
#   DERIVED_VERSION_PRERELEASE -- Prerelease label (e.g. "beta.2") or empty
#   DERIVED_VERSION_SEMVER     -- M.M.P (for CMake project() VERSION)
#   DERIVED_VERSION_FULL       -- M.M.P[-prerelease] (display version)
#
# Fallback: 0.0.0-dev when git is unavailable or no v* tag matches.

function(DeriveVersionFromGit)
    set(_fallback_major 0)
    set(_fallback_minor 0)
    set(_fallback_patch 0)
    set(_fallback_prerelease "dev")

    find_package(Git QUIET)
    if(NOT GIT_FOUND)
        message(STATUS "GitVersionDerivation: git not found, using fallback 0.0.0-dev")
        set(DERIVED_VERSION_MAJOR ${_fallback_major} PARENT_SCOPE)
        set(DERIVED_VERSION_MINOR ${_fallback_minor} PARENT_SCOPE)
        set(DERIVED_VERSION_PATCH ${_fallback_patch} PARENT_SCOPE)
        set(DERIVED_VERSION_PRERELEASE "${_fallback_prerelease}" PARENT_SCOPE)
        set(DERIVED_VERSION_SEMVER "${_fallback_major}.${_fallback_minor}.${_fallback_patch}" PARENT_SCOPE)
        set(DERIVED_VERSION_FULL "${_fallback_major}.${_fallback_minor}.${_fallback_patch}-${_fallback_prerelease}" PARENT_SCOPE)
        return()
    endif()

    execute_process(
        COMMAND "${GIT_EXECUTABLE}" describe --tags --match "v*" --always HEAD
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE _git_result
        OUTPUT_VARIABLE _git_describe
        ERROR_VARIABLE _git_error
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(NOT _git_result EQUAL 0)
        message(STATUS "GitVersionDerivation: git describe failed, using fallback 0.0.0-dev")
        set(DERIVED_VERSION_MAJOR ${_fallback_major} PARENT_SCOPE)
        set(DERIVED_VERSION_MINOR ${_fallback_minor} PARENT_SCOPE)
        set(DERIVED_VERSION_PATCH ${_fallback_patch} PARENT_SCOPE)
        set(DERIVED_VERSION_PRERELEASE "${_fallback_prerelease}" PARENT_SCOPE)
        set(DERIVED_VERSION_SEMVER "${_fallback_major}.${_fallback_minor}.${_fallback_patch}" PARENT_SCOPE)
        set(DERIVED_VERSION_FULL "${_fallback_major}.${_fallback_minor}.${_fallback_patch}-${_fallback_prerelease}" PARENT_SCOPE)
        return()
    endif()

    message(STATUS "GitVersionDerivation: git describe output: ${_git_describe}")

    # Try to match: v<MAJOR>.<MINOR>.<PATCH>[-<prerelease>][-<distance>-g<hash>]
    # Examples:
    #   v1.0.0                    -> 1.0.0
    #   v1.0.0-beta.2             -> 1.0.0, prerelease=beta.2
    #   v1.0.0-15-gabcdef1        -> 1.0.0 (distance suffix ignored)
    #   v1.0.0-beta.2-3-gabcdef1  -> 1.0.0, prerelease=beta.2
    if(_git_describe MATCHES "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)(-([a-zA-Z]+\\.[0-9]+))?(-[0-9]+-g[0-9a-f]+)?$")
        set(_major "${CMAKE_MATCH_1}")
        set(_minor "${CMAKE_MATCH_2}")
        set(_patch "${CMAKE_MATCH_3}")
        set(_prerelease "${CMAKE_MATCH_5}")

        set(DERIVED_VERSION_MAJOR ${_major} PARENT_SCOPE)
        set(DERIVED_VERSION_MINOR ${_minor} PARENT_SCOPE)
        set(DERIVED_VERSION_PATCH ${_patch} PARENT_SCOPE)
        set(DERIVED_VERSION_PRERELEASE "${_prerelease}" PARENT_SCOPE)
        set(DERIVED_VERSION_SEMVER "${_major}.${_minor}.${_patch}" PARENT_SCOPE)

        if(_prerelease)
            set(DERIVED_VERSION_FULL "${_major}.${_minor}.${_patch}-${_prerelease}" PARENT_SCOPE)
            message(STATUS "GitVersionDerivation: ${_major}.${_minor}.${_patch}-${_prerelease}")
        else()
            set(DERIVED_VERSION_FULL "${_major}.${_minor}.${_patch}" PARENT_SCOPE)
            message(STATUS "GitVersionDerivation: ${_major}.${_minor}.${_patch}")
        endif()
    else()
        # No matching v* tag — bare hash or unrecognised format
        message(STATUS "GitVersionDerivation: no v* tag match (got '${_git_describe}'), using fallback 0.0.0-dev")
        set(DERIVED_VERSION_MAJOR ${_fallback_major} PARENT_SCOPE)
        set(DERIVED_VERSION_MINOR ${_fallback_minor} PARENT_SCOPE)
        set(DERIVED_VERSION_PATCH ${_fallback_patch} PARENT_SCOPE)
        set(DERIVED_VERSION_PRERELEASE "${_fallback_prerelease}" PARENT_SCOPE)
        set(DERIVED_VERSION_SEMVER "${_fallback_major}.${_fallback_minor}.${_fallback_patch}" PARENT_SCOPE)
        set(DERIVED_VERSION_FULL "${_fallback_major}.${_fallback_minor}.${_fallback_patch}-${_fallback_prerelease}" PARENT_SCOPE)
    endif()
endfunction()

DeriveVersionFromGit()
