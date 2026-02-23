# OpenCppCoverage-based code coverage for MSVC builds on Windows
#
# USAGE:
#
# 1. Copy this file into your cmake modules path.
#
# 2. Add the following line to your CMakeLists.txt (best inside an if-condition
#    using a CMake option() to enable it just optionally):
#      include(OpenCppCoverage)
#
# 3. No special compiler flags are needed. OpenCppCoverage instruments using
#    PDB debug information, so a Debug build (with /Zi) is sufficient.
#
# 4. Use the functions described below to create a custom make target which
#    runs your test executable and produces a code coverage report.
#
# 5. Build a Debug build:
#      cmake --preset config-windows-msvc-coverage
#      cmake --build --preset build-windows-msvc-coverage
#

include(CMakeParseArguments)

option(CODE_COVERAGE_VERBOSE "Verbose information" FALSE)

# Check prereqs
find_program(OPENCPPCOVERAGE_PATH NAMES OpenCppCoverage OpenCppCoverage.exe)

if(NOT OPENCPPCOVERAGE_PATH)
  message(FATAL_ERROR "OpenCppCoverage not found! Install via: choco install opencppcoverage\n"
    "Or download from: https://github.com/OpenCppCoverage/OpenCppCoverage/releases")
endif()

# Check that the compiler is MSVC
get_property(LANGUAGES GLOBAL PROPERTY ENABLED_LANGUAGES)
foreach(LANG ${LANGUAGES})
  if(NOT "${CMAKE_${LANG}_COMPILER_ID}" STREQUAL "MSVC")
    if(NOT "${CMAKE_${LANG}_COMPILER_ID}" MATCHES "(Apple)?[Cc]lang")
      message(WARNING "OpenCppCoverage is designed for MSVC builds. "
        "Current ${LANG} compiler: ${CMAKE_${LANG}_COMPILER_ID}")
    endif()
  endif()
endforeach()

get_property(GENERATOR_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT (CMAKE_BUILD_TYPE STREQUAL "Debug" OR GENERATOR_IS_MULTI_CONFIG))
  message(WARNING "Code coverage results with an optimised (non-Debug) build may be misleading")
endif()

# Defines a target for running and collecting code coverage information
# using OpenCppCoverage. Builds dependencies, runs the given executable
# and outputs reports.
#
# NOTE! The executable should always have a ZERO as exit code otherwise
# the coverage generation will not complete.
#
# setup_target_for_coverage_opencppcoverage(
#     NAME testrunner_coverage                    # New target name
#     EXECUTABLE ctest -j ${PROCESSOR_COUNT}      # Executable in PROJECT_BINARY_DIR
#     DEPENDENCIES testrunner                     # Dependencies to build first
#     SOURCES "${PROJECT_SOURCE_DIR}/src"          # Source directories to include
#     EXCLUDE "${PROJECT_SOURCE_DIR}/deps"         # Patterns to exclude
#     MODULES "${PROJECT_BINARY_DIR}"              # Module directories to include
#     OUTPUT_FORMAT html                           # Output format: html or cobertura
# )
function(setup_target_for_coverage_opencppcoverage)

  set(options NONE)
  set(oneValueArgs BASE_DIRECTORY NAME OUTPUT_FORMAT)
  set(multiValueArgs EXCLUDE EXECUTABLE EXECUTABLE_ARGS DEPENDENCIES SOURCES MODULES)
  cmake_parse_arguments(Coverage "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  # Set base directory (as absolute path), or default to PROJECT_SOURCE_DIR
  if(DEFINED Coverage_BASE_DIRECTORY)
    get_filename_component(BASEDIR ${Coverage_BASE_DIRECTORY} ABSOLUTE)
  else()
    set(BASEDIR ${PROJECT_SOURCE_DIR})
  endif()

  # Set output format or default to html
  if(NOT DEFINED Coverage_OUTPUT_FORMAT)
    set(Coverage_OUTPUT_FORMAT "html")
  endif()

  # OpenCppCoverage requires native Windows backslash paths.
  # Use cmake_path(NATIVE_PATH) to convert all paths before passing them.

  # Build source filter arguments (--sources)
  set(SOURCE_ARGS "")
  foreach(SRC ${Coverage_SOURCES})
    get_filename_component(SRC_ABS ${SRC} ABSOLUTE BASE_DIR ${BASEDIR})
    cmake_path(NATIVE_PATH SRC_ABS SRC_NATIVE)
    list(APPEND SOURCE_ARGS "--sources" "${SRC_NATIVE}")
  endforeach()
  if(NOT Coverage_SOURCES)
    cmake_path(NATIVE_PATH BASEDIR BASEDIR_NATIVE)
    list(APPEND SOURCE_ARGS "--sources" "${BASEDIR_NATIVE}\\src")
  endif()

  # Build module filter arguments (--modules)
  set(MODULE_ARGS "")
  foreach(MOD ${Coverage_MODULES})
    get_filename_component(MOD_ABS ${MOD} ABSOLUTE BASE_DIR ${BASEDIR})
    cmake_path(NATIVE_PATH MOD_ABS MOD_NATIVE)
    list(APPEND MODULE_ARGS "--modules" "${MOD_NATIVE}")
  endforeach()

  # Build exclude arguments (--excluded_sources)
  set(EXCLUDE_ARGS "")
  foreach(EXCL ${Coverage_EXCLUDE} ${COVERAGE_EXCLUDES})
    get_filename_component(EXCL_ABS ${EXCL} ABSOLUTE BASE_DIR ${BASEDIR})
    cmake_path(NATIVE_PATH EXCL_ABS EXCL_NATIVE)
    list(APPEND EXCLUDE_ARGS "--excluded_sources" "${EXCL_NATIVE}")
  endforeach()

  # Determine export type flag
  if(Coverage_OUTPUT_FORMAT STREQUAL "cobertura")
    set(EXPORT_ARGS "--export_type=cobertura:${Coverage_NAME}.xml")
    set(COVERAGE_BYPRODUCTS ${Coverage_NAME}.xml)
    set(COVERAGE_REPORT_MSG "${Coverage_OUTPUT_FORMAT} code coverage report saved in ${Coverage_NAME}.xml.")
  else()
    set(EXPORT_ARGS "--export_type=html:${Coverage_NAME}")
    set(COVERAGE_BYPRODUCTS ${Coverage_NAME}/index.html)
    set(COVERAGE_REPORT_MSG "Open ./${Coverage_NAME}/index.html in your browser to view the coverage report.")
  endif()

  if(CODE_COVERAGE_VERBOSE)
    message(STATUS "Executed command report")
    message(STATUS "Command to generate OpenCppCoverage report:")
    message(STATUS "  ${OPENCPPCOVERAGE_PATH}")
    message(STATUS "    ${SOURCE_ARGS}")
    message(STATUS "    ${MODULE_ARGS}")
    message(STATUS "    ${EXCLUDE_ARGS}")
    message(STATUS "    ${EXPORT_ARGS}")
    message(STATUS "    --cover_children")
    message(STATUS "    -- ${Coverage_EXECUTABLE} ${Coverage_EXECUTABLE_ARGS}")
  endif()

  add_custom_target(${Coverage_NAME}
    COMMAND ${OPENCPPCOVERAGE_PATH}
      ${SOURCE_ARGS}
      ${MODULE_ARGS}
      ${EXCLUDE_ARGS}
      ${EXPORT_ARGS}
      --cover_children
      -- ${Coverage_EXECUTABLE} ${Coverage_EXECUTABLE_ARGS}

    BYPRODUCTS ${COVERAGE_BYPRODUCTS}
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    DEPENDS ${Coverage_DEPENDENCIES}
    VERBATIM
    COMMENT "Running OpenCppCoverage to produce ${Coverage_OUTPUT_FORMAT} code coverage report."
  )

  # Show info where to find the report
  add_custom_command(TARGET ${Coverage_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E true
    COMMENT "${COVERAGE_REPORT_MSG}"
  )

endfunction()
