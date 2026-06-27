# - Find VTune ittnotify.h
# Defines:
# VTune_FOUND
# VTune_INCLUDE_DIRS
# VTune_LIBRARIES

set(possible_vtune_dirs
  "$ENV{VTUNE_PROFILER_DIR}/"
  "$ENV{VTUNE_AMPLIFIER_XE_2013_DIR}/"
  "C:/Program Files (x86)/Intel/oneAPI/vtune/latest/"
  "C:/Program Files (x86)/Intel/VTune Profiler 2025/"
  "C:/Program Files (x86)/Intel/VTune Profiler 2024/"
  "C:/Program Files (x86)/IntelSWTools/VTune Profiler 2020"
  "C:/Program Files (x86)/Intel/VTune Amplifier XE 2013/"
  "$HOME/intel/oneapi/vtune/latest"
  "$HOME/intel/vtune_amplifier_xe_2013"
  "/opt/intel/oneapi/vtune/latest"
  )

find_path(VTune_INCLUDE_DIRS ittnotify.h
    PATHS ${possible_vtune_dirs}
    PATH_SUFFIXES include sdk/src/ittnotify)

if (CMAKE_SIZEOF_VOID_P MATCHES "8")
  set(vtune_lib_dir lib64)
else()
  set(vtune_lib_dir lib32)
endif()

find_library(VTune_LIBRARIES libittnotify
    HINTS "${VTune_INCLUDE_DIRS}/.."
    PATHS ${possible_vtune_dirs}
    PATH_SUFFIXES ${vtune_lib_dir} lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VTune DEFAULT_MSG VTune_LIBRARIES VTune_INCLUDE_DIRS)

# NOTE: the VTUNE_SUPPORT_ENABLED compile definition is applied (gated on
# VTune_FOUND) by src/core/CMakeLists.txt via a generator expression — this
# module only sets VTune_FOUND / VTune_INCLUDE_DIRS / VTune_LIBRARIES.
