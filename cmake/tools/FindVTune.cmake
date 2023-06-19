# - Find VTune ittnotify.h
# Defines:
# VTune_FOUND
# VTune_INCLUDE_DIRS
# VTune_LIBRARIES

set(possible_vtune_dirs
  "$ENV{VTUNE_AMPLIFIER_XE_2013_DIR}/"
  "C:/Program Files (x86)/Intel/VTune Amplifier XE 2013/"
  "$HOME/intel/vtune_amplifier_xe_2013"
  "$ENV{VTUNE_AMPLIFIER_XE_2011_DIR}/"
  "C:/Program Files (x86)/Intel/VTune Amplifier XE 2011/"
  "$HOME/intel/vtune_amplifier_xe_2011"
  "$ENV{VTUNE_AMPLIFIER_XE_2011_DIR}/"
  "C:/Program Files (x86)/IntelSWTools/VTune Profiler 2020"
  )

find_path(VTune_INCLUDE_DIRS ittnotify.h
    PATHS ${possible_vtune_dirs}
    PATH_SUFFIXES include)

if (CMAKE_SIZEOF_VOID_P MATCHES "8")
  set(vtune_lib_dir lib64)
else()
  set(vtune_lib_dir lib32)
endif()

find_library(VTune_LIBRARIES libittnotify
    HINTS "${VTune_INCLUDE_DIRS}/.."
    PATHS ${possible_vtune_dirs}
    PATH_SUFFIXES ${vtune_lib_dir})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VTune DEFAULT_MSG VTune_LIBRARIES VTune_INCLUDE_DIRS)

if (VTune_FOUND)
  set(DEFAULT_COMPILE_DEFINITIONS ${DEFAULT_COMPILE_DEFINITIONS}
    VTUNE_SUPPORT_ENABLED
  )
endif(VTune_FOUND)
