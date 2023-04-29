cmake_minimum_required (VERSION 3.20 FATAL_ERROR)

set(VCPKG_ROOT "deps/vcpkg")
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
set(Boost_NO_WARN_NEW_VERSIONS 1)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS OFF)

if (POLICY CMP0141)
  # Enable Hot Reload for MSVC compilers if supported.
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(aqualink-automate VERSION 0.0.1 LANGUAGES C CXX)

option(ENABLE_TESTS "Enable unit tests" ON)
option(ENABLE_PROFILING "Enable profiling" ON)

find_package(Boost REQUIRED COMPONENTS coroutine iostreams log program_options system unit_test_framework)
find_package(magic_enum REQUIRED)
find_package(mqtt_cpp_iface CONFIG REQUIRED)

#------------------------------------------------------------------------------
#
#
#
#
#------------------------------------------------------------------------------

if(ENABLE_PROFILING)

find_package(Tracy CONFIG REQUIRED)

include(cmake/tools/profiling_uprof.cmake)
include(cmake/tools/profiling_vtune.cmake)

endif(ENABLE_PROFILING)

#------------------------------------------------------------------------------
#
#
#
#
#------------------------------------------------------------------------------

add_library(
	libaqualink-automate
	src/developer/mock_serial_port.cpp
	src/exceptions/exception_genericaqualinkexception.cpp
	src/exceptions/exception_optionparsingfailed.cpp
	src/exceptions/exception_optionshelporversion.cpp	
	src/interfaces/idevice.cpp 
	src/interfaces/iequipment.cpp
	src/interfaces/igenerator.cpp
	src/interfaces/imessage.cpp
	src/interfaces/imessagesignal.cpp
	src/interfaces/iprofiler.cpp
	src/interfaces/iprofilingunit.cpp
	src/interfaces/iserialport.cpp
	src/interfaces/iserializable.cpp
	src/jandy/devices/aquarite_device.cpp 
	src/jandy/devices/iaq_device.cpp 
	src/jandy/devices/jandy_device_types.cpp
	src/jandy/devices/onetouch_device.cpp 
	src/jandy/devices/pda_device.cpp 
	src/jandy/equipment/jandy_equipment.cpp
	src/jandy/errors/jandy_errors_messages.cpp
	src/jandy/errors/jandy_errors_protocol.cpp
	src/jandy/factories/jandy_message_factory.cpp
	src/jandy/factories/jandy_message_factory_registration.cpp
	src/jandy/formatters/jandy_message_formatters.cpp
	src/jandy/generator/jandy_message_generator.cpp
	src/jandy/jandy.cpp
	src/jandy/messages/aquarite/aquarite_message.cpp
	src/jandy/messages/aquarite/aquarite_message_getid.cpp
	src/jandy/messages/aquarite/aquarite_message_percent.cpp
	src/jandy/messages/aquarite/aquarite_message_ppm.cpp
	src/jandy/messages/iaq/iaq_message.cpp
	src/jandy/messages/iaq/iaq_message_control_ready.cpp
	src/jandy/messages/iaq/iaq_message_message_long.cpp
	src/jandy/messages/iaq/iaq_message_page_button.cpp
	src/jandy/messages/iaq/iaq_message_page_continue.cpp
	src/jandy/messages/iaq/iaq_message_page_end.cpp
	src/jandy/messages/iaq/iaq_message_page_message.cpp
	src/jandy/messages/iaq/iaq_message_page_start.cpp
	src/jandy/messages/iaq/iaq_message_poll.cpp
	src/jandy/messages/iaq/iaq_message_startup.cpp
	src/jandy/messages/iaq/iaq_message_table_message.cpp
	src/jandy/messages/pda/pda_message.cpp 
	src/jandy/messages/pda/pda_message_clear.cpp 
	src/jandy/messages/pda/pda_message_highlight.cpp 
	src/jandy/messages/pda/pda_message_highlight_chars.cpp 
	src/jandy/messages/pda/pda_message_shiftlines.cpp 
	src/jandy/messages/jandy_message.cpp
	src/jandy/messages/jandy_message_ack.cpp
	src/jandy/messages/jandy_message_constants.cpp
	src/jandy/messages/jandy_message_ids.cpp
	src/jandy/messages/jandy_message_message.cpp
	src/jandy/messages/jandy_message_message_long.cpp
	src/jandy/messages/jandy_message_probe.cpp
	src/jandy/messages/jandy_message_status.cpp	 
	src/jandy/messages/jandy_message_unknown.cpp
	src/jandy/types/jandy_types.cpp
	src/jandy/utility/screen_data_page.cpp
	src/jandy/utility/screen_data_page_updater.cpp
	src/jandy/utility/screen_data_page_updater_context.cpp
	src/jandy/utility/screen_data_page_updater_evshift.cpp
	src/jandy/utility/screen_data_page_updater_evupdate.cpp
	src/logging/formatters/logging_formatters.cpp
	src/logging/global_logger.cpp
	src/logging/logging.cpp
	src/logging/logging_attributes.cpp
	src/logging/logging_channels.cpp
	src/logging/logging_formatter.cpp
	src/logging/logging_initialise.cpp
	src/logging/logging_severity_filter.cpp
	src/logging/logging_severity_levels.cpp
	src/options/options_app_options.cpp
	src/options/options_developer_options.cpp
	src/options/options_initialise.cpp
	src/options/options_option_type.cpp
	src/options/options_serial_options.cpp
	src/options/options_settings.cpp
	src/options/options_web_options.cpp
	src/options/validators/profiler_type_validator.cpp
	src/options/validators/severity_level_validator.cpp
	src/profiling/noop_profiler.cpp
	src/profiling/profiling.cpp
	$<$<BOOL:${ENABLE_PROFILING}>:src/profiling/tracy_profiler.cpp>	
	$<$<AND:$<BOOL:${ENABLE_PROFILING}>,$<BOOL:${UProf_FOUND}>>:src/profiling/uprof_profiler.cpp>
	$<$<AND:$<BOOL:${ENABLE_PROFILING}>,$<BOOL:${VTune_FOUND}>>:src/profiling/vtune_profiler.cpp>
	src/profiling/factories/profiler_factory.cpp
	src/profiling/factories/profiler_factory_registration.cpp
	src/profiling/factories/profiling_unit_factory.cpp 
	src/profiling/factories/profiling_unit_factory_registration.cpp 
	src/profiling/formatters/profiling_formatters.cpp
	src/profiling/profiling_units/domain.cpp 
	src/profiling/profiling_units/frame.cpp 
	$<$<BOOL:${ENABLE_PROFILING}>:src/profiling/profiling_units/tracy_frame.cpp>	
	$<$<BOOL:${ENABLE_PROFILING}>:src/profiling/profiling_units/tracy_zone.cpp>	
	src/profiling/profiling_units/timed_zone.cpp 
	src/profiling/profiling_units/zone.cpp 
	src/profiling/types/profiling_types.cpp
	src/protocol/protocol_handler.cpp
	src/serial/serial_initialise.cpp
	src/serial/serial_operating_modes.cpp
	src/serial/serial_port.cpp
	src/signals/async_signal.cpp
	src/signals/async_slot.cpp
	src/signals/signal_awaitable.cpp
	src/signals/signal_cleanup.cpp
	src/signals/signal_handler.cpp
	src/utility/array_standard_formatter.cpp
	src/utility/circular_buffer_wrapping_iterator.cpp
	src/utility/get_terminal_column_width.cpp	
	src/utility/value_debouncer.cpp
)

if(MSVC AND NOT (CMAKE_C_COMPILER_ID STREQUAL "Intel"))
	target_compile_options (libaqualink-automate PRIVATE "/bigobj")
endif(MSVC AND NOT (CMAKE_C_COMPILER_ID STREQUAL "Intel"))

if(WIN32)
target_sources(
	libaqualink-automate 
	PRIVATE
	src/platform/utility/get_terminal_column_width_win32.cpp
)

target_compile_definitions(
	libaqualink-automate 
	PUBLIC 
	-D_SCL_SECURE_NO_WARNINGS
	-D_CRT_SECURE_NO_WARNINGS
	-D_WIN32_WINNT=0x0601 
	-DWINVER=0x0601 
	-DWIN32_LEAN_AND_MEAN 
	-DVC_EXTRALEAN
	-DNOGDI
	-DNOGDICAPMASKS
	-DNOMINMAX
)
endif(WIN32)

if(LINUX)
target_sources(
	libaqualink-automate 
	PRIVATE
	src/platform/utility/get_terminal_column_width_linux.cpp
)
endif(LINUX)

target_compile_features(libaqualink-automate PUBLIC cxx_std_20)

target_compile_definitions(
	libaqualink-automate 
	PUBLIC 
	$<$<BOOL:${ENABLE_PROFILING}>:TRACY_ENABLE>
)

target_include_directories(libaqualink-automate PUBLIC "${PROJECT_SOURCE_DIR}/src")

target_include_directories(
	libaqualink-automate 
	SYSTEM 
	PUBLIC 
	${Boost_INCLUDE_DIRS} 
	${magic_enum_INCLUDE_DIRS}
	$<$<AND:$<BOOL:${ENABLE_PROFILING}>,$<BOOL:${UProf_FOUND}>>:${UProf_INCLUDE_DIRS}>
	$<$<AND:$<BOOL:${ENABLE_PROFILING}>,$<BOOL:${VTune_FOUND}>>:${VTune_INCLUDE_DIRS}>
	$<$<BOOL:${ENABLE_PROFILING}>:${Tracy_INCLUDE_DIRS}>
)

#------------------------------------------------------------------------------
#
#
#
#
#------------------------------------------------------------------------------

add_executable(
	aqualink-automate
	src/aqualink-automate.cpp 
)

target_link_libraries(
	aqualink-automate 
	PRIVATE 
	"$<LINK_LIBRARY:WHOLE_ARCHIVE,libaqualink-automate>"
	Boost::boost 
	Boost::coroutine 
	Boost::iostreams 
	Boost::log 
	Boost::log_setup 
	Boost::program_options 
	magic_enum::magic_enum
	mqtt_cpp_iface::mqtt_cpp_iface 
	$<$<AND:$<BOOL:${ENABLE_PROFILING}>,$<BOOL:${UProf_FOUND}>>:${UProf_LIBRARIES}>
	$<$<AND:$<BOOL:${ENABLE_PROFILING}>,$<BOOL:${VTune_FOUND}>>:${VTune_LIBRARIES}>
	$<$<BOOL:${ENABLE_PROFILING}>:Tracy::TracyClient>
)

#------------------------------------------------------------------------------
#
#
#
#
#------------------------------------------------------------------------------

if(ENABLE_TESTS)

	add_executable(
		testaqualink-automate
		test/test_main.cpp
		test/messages/test_message_processing.cpp
		test/utilities/unit_test_ostream_support.cpp
	)

	target_compile_definitions(testaqualink-automate PRIVATE -DBOOST_TEST_DYN_LINK)
	target_include_directories(testaqualink-automate PUBLIC "${PROJECT_SOURCE_DIR}/test")

	target_link_libraries(
		testaqualink-automate 
		PRIVATE 
		"$<LINK_LIBRARY:WHOLE_ARCHIVE,libaqualink-automate>"
		Boost::boost 
		Boost::coroutine 
		Boost::iostreams 
		Boost::log 
		Boost::log_setup 
		Boost::program_options 
		Boost::unit_test_framework 
		magic_enum::magic_enum
	mqtt_cpp_iface::mqtt_cpp_iface 
		$<$<AND:$<BOOL:${ENABLE_PROFILING}>,$<BOOL:${UProf_FOUND}>>:${UProf_LIBRARIES}>
		$<$<AND:$<BOOL:${ENABLE_PROFILING}>,$<BOOL:${VTune_FOUND}>>:${VTune_LIBRARIES}>
		$<$<BOOL:${ENABLE_PROFILING}>:Tracy::TracyClient>
	)

	enable_testing()
	add_test(AqualinkAutomateTests testaqualink-automate)

endif(ENABLE_TESTS)
