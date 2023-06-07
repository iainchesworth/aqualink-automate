# - Find UProf AMDProfileController.h
# Defines:
# UProf_FOUND
# UProf_INCLUDE_DIRS
# UProf_LIBRARIES

set(possible_UProf_dirs
  "$ENV{AMD_UPROF_SDK_PATH}/"
  "C:/Program Files/AMD/AMDuProf/"
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
    PATH_SUFFIXES ${UProf_lib_dir})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(UProf DEFAULT_MSG UProf_LIBRARIES UProf_INCLUDE_DIRS)

if (UProf_FOUND)
  set(DEFAULT_COMPILE_DEFINITIONS ${DEFAULT_COMPILE_DEFINITIONS}
    UProf_SUPPORT_ENABLED
  )
endif(UProf_FOUND)
