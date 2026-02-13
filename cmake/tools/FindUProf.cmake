# - Find UProf AMDProfileController.h and AMDTActivityLogger.h
# Defines:
# UProf_FOUND
# UProf_INCLUDE_DIRS
# UProf_LIBRARIES
# UProf_ACTIVITY_LOGGER_FOUND
# UProf_ACTIVITY_LOGGER_LIBRARIES

set(possible_UProf_dirs
  "$ENV{AMD_UPROF_SDK_PATH}/"
  "C:/Program Files/AMD/AMDuProf/"
  "C:/Program Files/AMD/AMDuProfiler/"
  "/opt/AMDuProf_5.0/"
  "/opt/AMDuProf_4.2/"
  "/opt/AMDuProf_4.1/"
  "/opt/AMDuProf_4.0.341/"
  "/opt/AMDuProf_4.0.332/"
  )

find_path(UProf_INCLUDE_DIRS AMDProfileController.h
    PATHS ${possible_UProf_dirs}
    PATH_SUFFIXES include)

if (CMAKE_SIZEOF_VOID_P MATCHES "8")
  set(UProf_lib_dir x64)
else()
  set(UProf_lib_dir x32)
endif()

find_library(UProf_LIBRARIES AMDProfileController
    HINTS "${UProf_INCLUDE_DIRS}/../lib"
    PATHS ${possible_UProf_dirs}
    PATH_SUFFIXES ${UProf_lib_dir} lib/${UProf_lib_dir})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(UProf DEFAULT_MSG UProf_LIBRARIES UProf_INCLUDE_DIRS)

# Search for ActivityLogger component
find_path(UProf_ACTIVITY_LOGGER_INCLUDE_DIRS AMDTActivityLogger.h
    PATHS ${possible_UProf_dirs}
    PATH_SUFFIXES include)

find_library(UProf_ACTIVITY_LOGGER_LIBRARIES AMDTActivityLogger
    HINTS "${UProf_INCLUDE_DIRS}/../lib"
    PATHS ${possible_UProf_dirs}
    PATH_SUFFIXES ${UProf_lib_dir} lib/${UProf_lib_dir})

if (UProf_ACTIVITY_LOGGER_INCLUDE_DIRS AND UProf_ACTIVITY_LOGGER_LIBRARIES)
  set(UProf_ACTIVITY_LOGGER_FOUND TRUE CACHE BOOL "uProf ActivityLogger component found")
  # Merge ActivityLogger include dirs and libraries into the main variables
  list(APPEND UProf_INCLUDE_DIRS ${UProf_ACTIVITY_LOGGER_INCLUDE_DIRS})
  list(REMOVE_DUPLICATES UProf_INCLUDE_DIRS)
  list(APPEND UProf_LIBRARIES ${UProf_ACTIVITY_LOGGER_LIBRARIES})
else()
  set(UProf_ACTIVITY_LOGGER_FOUND FALSE CACHE BOOL "uProf ActivityLogger component found")
endif()

if (UProf_FOUND)
  set(DEFAULT_COMPILE_DEFINITIONS ${DEFAULT_COMPILE_DEFINITIONS}
    UProf_SUPPORT_ENABLED
  )
endif(UProf_FOUND)
