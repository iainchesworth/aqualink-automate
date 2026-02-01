#------------------------------------------------------------------------------
# Windows Common Toolchain Configuration
#------------------------------------------------------------------------------

message(STATUS "Configuring Windows Toolchain (Common)")

if(NOT WIN32)
  message(FATAL_ERROR "Windows Common toolchain: This setup is for Windows only.")
endif()

set(TOOLCHAIN_ARCH "x64" CACHE STRING "Windows target architecture (x64|x86)")
set(_valid_arch x64 x86)
if(NOT TOOLCHAIN_ARCH IN_LIST _valid_arch)
  message(FATAL_ERROR "TOOLCHAIN_ARCH must be one of: x64, x86")
endif()

# Resolve ProgramFiles and ProgramFiles(x86) safely (no $ENV{ProgramFiles(x86)})
set(WIN_PF "$ENV{ProgramFiles}")
set(WIN_PF86 "")
execute_process(
  COMMAND cmd /c "echo %ProgramFiles(x86)%"
  OUTPUT_VARIABLE WIN_PF86
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(NOT WIN_PF86)
  # 32-bit Windows fallback
  set(WIN_PF86 "${WIN_PF}")
endif()

cmake_path(CONVERT "${WIN_PF}"   TO_CMAKE_PATH_LIST WIN_PF)
cmake_path(CONVERT "${WIN_PF86}" TO_CMAKE_PATH_LIST WIN_PF86)

set(WIN_VSWHERE "${CMAKE_CURRENT_LIST_DIR}/../tools/vswhere/vswhere.exe")
set(WIN_VS_INSTALL "")

if(NOT EXISTS "${WIN_VSWHERE}")
  message(FATAL_ERROR "Windows Common toolchain: vswhere not found; cannot proceed!")
else()
  message(STATUS "Using vswhere at: ${WIN_VSWHERE}")
  execute_process(
    COMMAND "${WIN_VSWHERE}"
            -latest -prerelease 
            -products * 
            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 
            -property installationPath
    OUTPUT_VARIABLE WIN_VS_INSTALL
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()

set(WIN_VC_TOOLS_DIR "")
if(WIN_VS_INSTALL)
  cmake_path(CONVERT "${WIN_VS_INSTALL}" TO_CMAKE_PATH_LIST WIN_VS_INSTALL)
  file(GLOB _vc_tools_dirs "${WIN_VS_INSTALL}/VC/Tools/MSVC/*")
  list(SORT _vc_tools_dirs COMPARE NATURAL ORDER DESCENDING)
  if(_vc_tools_dirs)
    list(GET _vc_tools_dirs 0 WIN_VC_TOOLS_DIR)
  endif()
endif()

set(WIN_VC_INCLUDE_DIR "")
set(WIN_VC_LIB_DIR "")
if(WIN_VC_TOOLS_DIR)
  set(WIN_VC_INCLUDE_DIR "${WIN_VC_TOOLS_DIR}/include")
  set(WIN_VC_LIB_DIR     "${WIN_VC_TOOLS_DIR}/lib/${TOOLCHAIN_ARCH}")
endif()

set(WINSDK_ROOT "${WIN_PF86}/Windows Kits/10")
cmake_path(CONVERT "${WINSDK_ROOT}" TO_CMAKE_PATH_LIST WINSDK_ROOT)

set(WINSDK_BIN_DIR "")
set(WINSDK_INCLUDE_VER "")
set(WINSDK_LIB_VER "")

if(EXISTS "${WINSDK_ROOT}")
  file(GLOB _ws_bin "${WINSDK_ROOT}/bin/*")
  list(SORT _ws_bin COMPARE NATURAL ORDER DESCENDING)
  if(_ws_bin)
    list(GET _ws_bin 0 WINSDK_BIN_DIR)
  endif()

  file(GLOB _ws_inc "${WINSDK_ROOT}/include/*")
  list(SORT _ws_inc COMPARE NATURAL ORDER DESCENDING)
  if(_ws_inc)
    list(GET _ws_inc 0 _ws_inc_dir)
    get_filename_component(WINSDK_INCLUDE_VER "${_ws_inc_dir}" NAME)
  endif()

  file(GLOB _ws_lib "${WINSDK_ROOT}/lib/*")
  list(SORT _ws_lib COMPARE NATURAL ORDER DESCENDING)
  if(_ws_lib)
    list(GET _ws_lib 0 _ws_lib_dir)
    get_filename_component(WINSDK_LIB_VER "${_ws_lib_dir}" NAME)
  endif()
endif()

unset(CMAKE_RC_COMPILER CACHE)
unset(CMAKE_MT CACHE)

find_program(WIN_RC_EXE NAMES rc
  HINTS "${WINSDK_BIN_DIR}/${TOOLCHAIN_ARCH}" "${WINSDK_BIN_DIR}")
if(WIN_RC_EXE)
  set(CMAKE_RC_COMPILER "${WIN_RC_EXE}" CACHE FILEPATH "" FORCE)
endif()

find_program(WIN_MT_EXE NAMES mt
  HINTS "${WINSDK_BIN_DIR}/${TOOLCHAIN_ARCH}" "${WINSDK_BIN_DIR}")
if(WIN_MT_EXE)
  set(CMAKE_MT "${WIN_MT_EXE}" CACHE FILEPATH "" FORCE)
endif()

set(WIN_INCLUDE_DIRS "")
set(WIN_LIB_DIRS "")

if(EXISTS "${WIN_VC_INCLUDE_DIR}")
  list(APPEND WIN_INCLUDE_DIRS "${WIN_VC_INCLUDE_DIR}")
endif()

if(WINSDK_INCLUDE_VER)
  list(APPEND WIN_INCLUDE_DIRS
    "${WINSDK_ROOT}/include/${WINSDK_INCLUDE_VER}/ucrt"
    "${WINSDK_ROOT}/include/${WINSDK_INCLUDE_VER}/um"
    "${WINSDK_ROOT}/include/${WINSDK_INCLUDE_VER}/shared"
    "${WINSDK_ROOT}/include/${WINSDK_INCLUDE_VER}/winrt"
    "${WINSDK_ROOT}/include/${WINSDK_INCLUDE_VER}/cppwinrt"
  )
endif()

if(EXISTS "${WIN_VC_LIB_DIR}")
  list(APPEND WIN_LIB_DIRS "${WIN_VC_LIB_DIR}")
endif()
if(WINSDK_LIB_VER)
  list(APPEND WIN_LIB_DIRS
    "${WINSDK_ROOT}/lib/${WINSDK_LIB_VER}/ucrt/${TOOLCHAIN_ARCH}"
    "${WINSDK_ROOT}/lib/${WINSDK_LIB_VER}/um/${TOOLCHAIN_ARCH}"
  )
endif()

set(WIN_EXPORT_ENV ON CACHE BOOL "Export Windows INCLUDE/LIB env for non-DevPrompt shells")
if(WIN_EXPORT_ENV)
  set(_INC_NATIVE "${WIN_INCLUDE_DIRS}")
  set(_LIB_NATIVE "${WIN_LIB_DIRS}")

  cmake_path(CONVERT "${_INC_NATIVE}" TO_NATIVE_PATH_LIST _INC_NATIVE)
  cmake_path(CONVERT "${_LIB_NATIVE}" TO_NATIVE_PATH_LIST _LIB_NATIVE)

  list(JOIN _INC_NATIVE ";" _JOINED_INC_NATIVE)
  list(JOIN _LIB_NATIVE ";" _JOINED_LIB_NATIVE)

  set(ENV{INCLUDE} "${_JOINED_INC_NATIVE}")
  set(ENV{LIB}     "${_JOINED_LIB_NATIVE}")
endif()

message(STATUS "Windows Common toolchain VS install = ${WIN_VS_INSTALL}")
message(STATUS "Windows Common toolchain VC tools = ${WIN_VC_TOOLS_DIR}")
message(STATUS "Windows Common toolchain WinSDK inc = ${WINSDK_INCLUDE_VER}")
message(STATUS "Windows Common toolchain WinSDK lib = ${WINSDK_LIB_VER}")
