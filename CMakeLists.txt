cmake_minimum_required (VERSION 3.31 FATAL_ERROR)

if(POLICY CMP0053)
  cmake_policy(SET CMP0053 NEW)
endif()
if(POLICY CMP0077)
  # Allow preset/command-line variables to override option() defaults
  cmake_policy(SET CMP0077 NEW)
endif()
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

set(Boost_NO_WARN_NEW_VERSIONS	 ON) # suppress the warning about unknown dependencies for new Boost versions
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Generate compile_commands.json for clang-tidy
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" "${CMAKE_SOURCE_DIR}/cmake/tools/" "${CMAKE_SOURCE_DIR}/cmake/tools/coverage" "${CMAKE_SOURCE_DIR}/deps")

# Define options BEFORE project() to ensure preset values take precedence
# over any vcpkg toolchain cache interference
option(ENABLE_BENCHMARKS "Enable performance tests" ON)
option(ENABLE_COVERAGE "Enable coverage of unit tests" OFF)
option(ENABLE_TESTS "Enable unit tests" ON)
option(ENABLE_PROFILING "DevTools: Enable profiling tooling" ON)
option(ENABLE_CLANG_TIDY "DevTools: Enable static analysis using clang-tidy" OFF)
option(ENABLE_PVS_STUDIO "DevTools: Enable static analysis using PVS-Studio" OFF)
option(ENABLE_ASIO_TRACKING "DevTools: Debug using ASIO connection tracking" OFF)
option(ENABLE_SANITIZERS "DevTools: Enable sanitizer tooling" OFF)

project(
	aqualink-automate
	VERSION 0.0.1
	DESCRIPTION "Control Jandy Aqualink RS pool equipment via common IoT integrations"
	HOMEPAGE_URL "https://github.com/iainchesworth/aqualink-automate"
	LANGUAGES C CXX)

if(ENABLE_SANITIZERS AND ENABLE_PROFILING)
  message(FATAL_ERROR "ENABLE_SANITIZERS and ENABLE_PROFILING cannot both be ON. "
    "Profiling instrumentation (Tracy) is incompatible with sanitizer runtimes. "
    "Disable one of them.")
endif()

option(USE_SONARQUBE "Use SonarQube for coverage of unit tests" OFF)

#------------------------------------------------------------------------------
#
#
#
#
#------------------------------------------------------------------------------

add_subdirectory(deps)

find_package(benchmark REQUIRED)
find_package(Boost REQUIRED COMPONENTS graph iostreams log log_setup program_options regex system unit_test_framework url)
find_package(magic_enum REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(tl-expected REQUIRED)
find_package(ZLIB REQUIRED)

#------------------------------------------------------------------------------
#
#
#
#
#------------------------------------------------------------------------------

if(ENABLE_COVERAGE)

if(MSVC)
  include(OpenCppCoverage)
  # No compiler flags needed - OpenCppCoverage uses PDB debug information
else()
  include(CodeCoverage)
  append_coverage_compiler_flags()
endif()

endif(ENABLE_COVERAGE)

#------------------------------------------------------------------------------
#
#
#
#
#------------------------------------------------------------------------------

if(ENABLE_PROFILING)

find_package(Tracy CONFIG REQUIRED)

find_package(UProf)
find_package(VTune)

endif(ENABLE_PROFILING)

#------------------------------------------------------------------------------
#
#
#
#
#------------------------------------------------------------------------------

if(ENABLE_SANITIZERS)

option(SANITIZE_ADDRESS "DevTools: Sanitizers -> Use AddressSanitizer" OFF)
option(SANITIZE_MEMORY "DevTools: Sanitizers -> Use MemorySanitizer" OFF)
option(SANITIZE_THREAD "DevTools: Sanitizers -> Use ThreadSanitizer" OFF)
option(SANITIZE_UNDEFINED "DevTools: Sanitizers -> Use UndefinedBehaviourSanitizer" OFF)

include(Sanitizers)

# On non-Windows platforms, Clang uses static ASan by default which can cause linking issues.
# Use shared ASan runtime instead. This flag is not valid on Windows (clang-cl).
if(NOT WIN32)
    add_compile_options("$<$<CXX_COMPILER_ID:Clang>:-shared-libasan>")
endif()

endif(ENABLE_SANITIZERS)

#------------------------------------------------------------------------------
#
#
#
#
#------------------------------------------------------------------------------

# Template the git commit information prior to building the application.
set(GIT_FAIL_IF_NONZERO_EXIT false)
set(PRE_CONFIGURE_FILE "src/core/version/version_git.constants.cpp.in")
set(POST_CONFIGURE_FILE "${CMAKE_CURRENT_BINARY_DIR}/src/core/version/version_git.constants.cpp")
include(GitWatcher)

# Template the versioning information prior to building the application.
set(VERSION_CMAKE_SOURCE_FILE "src/core/version/version_cmake.constants.cpp.in")
set(VERSION_CMAKE_DEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/src/core/version/version_cmake.constants.cpp")
include(CMakeVersionWatcher)

# Mark the source files as generated so that the add_XXXXX command doesn't complain
set_source_files_properties(${POST_CONFIGURE_FILE} PROPERTIES GENERATED 1)
set_source_files_properties(${VERSION_CMAKE_DEST_FILE} PROPERTIES GENERATED 1)

#------------------------------------------------------------------------------
#
#
#
#
#------------------------------------------------------------------------------

add_subdirectory(src)

#------------------------------------------------------------------------------
#
#    STATIC ANALYSIS CONFIGURATION
#    =============================
#
#------------------------------------------------------------------------------

if(ENABLE_CLANG_TIDY)

find_package(ClangTidy)

# Apply clang-tidy to both the library and executable
set_target_properties(libaqualink-automate PROPERTIES CXX_CLANG_TIDY "${CMAKE_CXX_CLANG_TIDY}")
set_target_properties(aqualink-automate PROPERTIES CXX_CLANG_TIDY "${CMAKE_CXX_CLANG_TIDY}")

endif(ENABLE_CLANG_TIDY)

if(ENABLE_PVS_STUDIO)

include(PVS-Studio)

pvs_studio_add_target(
	ALL
	TARGET aqualink-automate.analyze
	ANALYZE aqualink-automate libaqualink-automate 
	OUTPUT
	LOG "${CMAKE_CURRENT_BINARY_DIR}/aqualink-automate.pvs-studio.log"
	FORMAT fullhtml 
	MODE 64+GA+OP+CS+MISRA+AUTOSAR+OWASP
	SOURCES "${CMAKE_SOURCE_DIR}/src"
	ARGS --exclude-path "${CMAKE_SOURCE_DIR}/build"
)

endif(ENABLE_PVS_STUDIO)

#------------------------------------------------------------------------------
#
#
#
#
#------------------------------------------------------------------------------

if(ENABLE_TESTS)

enable_testing()
add_subdirectory(test)

endif(ENABLE_TESTS)

#------------------------------------------------------------------------------
#
#
#
#
#------------------------------------------------------------------------------

if(ENABLE_SANITIZERS)

target_enable_sanitizers(libaqualink-automate)
target_enable_sanitizers(aqualink-automate)

endif(ENABLE_SANITIZERS)

#------------------------------------------------------------------------------
#
#
#
#
#------------------------------------------------------------------------------

include(CPackConfig)
