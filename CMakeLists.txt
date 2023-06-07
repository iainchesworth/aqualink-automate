cmake_minimum_required (VERSION 3.20 FATAL_ERROR)

set(Boost_NO_WARN_NEW_VERSIONS 1)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" "${CMAKE_SOURCE_DIR}/cmake/tools/" "${CMAKE_SOURCE_DIR}/deps/sanitizers-cmake/cmake")

project(
	aqualink-automate 
	VERSION 0.0.1 
	DESCRIPTION "Control Jandy Aqualink RS pool equipment via common IoT integrations" 
    HOMEPAGE_URL "https://github.com/iainchesworth/aqualink-automate"
	LANGUAGES C CXX)

option(ENABLE_TESTS "Enable unit tests" ON)
option(ENABLE_PROFILING "DevTools: Enable profiling tooling" ON)
option(ENABLE_CLANG_TIDY "DevTools: Enable Clang-Tidy analysis" ON)
cmake_dependent_option(ENABLE_SANITIZERS "DevTools: Enable sanitizer tooling" ON "NOT ENABLE_PROFILING" OFF)

find_package(Boost REQUIRED COMPONENTS coroutine graph iostreams log log_setup program_options regex system unit_test_framework)
find_package(Crow CONFIG REQUIRED)
find_package(magic_enum REQUIRED)
find_package(mqtt_cpp_iface REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(OpenSSL REQUIRED)

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

find_package(Sanitizers)

endif(ENABLE_SANITIZERS)

#------------------------------------------------------------------------------
#
#
#
#
#------------------------------------------------------------------------------

if(ENABLE_CLANG_TIDY)

find_package(ClangTidy)

endif(ENABLE_CLANG_TIDY)

#------------------------------------------------------------------------------
#
#
#
#
#------------------------------------------------------------------------------

# Template the git commit information prior to building the application.
set(GIT_FAIL_IF_NONZERO_EXIT false)
set(PRE_CONFIGURE_FILE "src/version/version_git.constants.cpp.in")
set(POST_CONFIGURE_FILE "${CMAKE_CURRENT_BINARY_DIR}/src/version/version_git.constants.cpp")
include(GitWatcher)

# Template the versioning information prior to building the application. 
set(VERSION_CMAKE_SOURCE_FILE "src/version/version_cmake.constants.cpp.in")
set(VERSION_CMAKE_DEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/src/version/version_cmake.constants.cpp")
configure_file(${VERSION_CMAKE_SOURCE_FILE} ${VERSION_CMAKE_DEST_FILE})

# Mark the source files as generated so that the add_XXXXX command doesn't complain
set_source_files_properties(${POST_CONFIGURE_FILE} PROPERTIES GENERATED 1)
set_source_files_properties(${VERSION_CMAKE_DEST_FILE} PROPERTIES GENERATED 1)

#------------------------------------------------------------------------------
#
#
#
#
#------------------------------------------------------------------------------

add_library(
	libaqualink-automate
	src/certificates/certificate_pemformat.cpp 
	src/certificates/certificate_staticdata.cpp 
	src/certificates/certificate_management.cpp 
	src/certificates/certificate_verification.cpp 
	src/developer/mock_serial_port.cpp
	src/exceptions/exception_genericaqualinkexception.cpp
	src/exceptions/exception_optionparsingfailed.cpp
	src/exceptions/exception_optionshelporversion.cpp 
	src/exceptions/exception_options_conflictingoptions.cpp 
	src/exceptions/exception_options_missingdependency.cpp 
	src/exceptions/exception_webserver.cpp 
	src/http/crow_custom_logger.cpp 
	src/http/webroute_jandyequipment_buttons.cpp 
	src/http/webroute_jandyequipment_stats.cpp 
	src/http/webroute_jandyequipment_version.cpp 
	src/http/webroute_page_index.cpp 
	src/http/webroute_page_jandyequipment.cpp 
	src/http/webroute_page_version.cpp 
	src/http/webroute_version.cpp 
	src/interfaces/icertificate.cpp 
	src/interfaces/idevice.cpp 
	src/interfaces/iequipment.cpp	
	src/interfaces/imessage.cpp
	src/interfaces/imessagesignal_recv.cpp
	src/interfaces/imessagesignal_send.cpp
	src/interfaces/iprofiler.cpp
	src/interfaces/iprofilingunit.cpp	 
	src/interfaces/iserializable.cpp
	src/interfaces/iserialport.cpp
	src/interfaces/iwebroute.cpp 
	src/interfaces/iwebsocket.cpp 
	src/jandy/config/jandy_config.cpp
	src/jandy/config/jandy_config_auxillary.cpp
	src/jandy/config/jandy_config_remotepowercenter.cpp
	src/jandy/devices/aquarite_device.cpp 
	src/jandy/devices/iaq_device.cpp 
	src/jandy/devices/jandy_controller.cpp 
	src/jandy/devices/jandy_device.cpp 
	src/jandy/devices/jandy_device_id.cpp 
	src/jandy/devices/jandy_device_types.cpp
	src/jandy/devices/jandy_emulated_device_types.cpp 
	src/jandy/devices/capabilities/emulated.cpp 
	src/jandy/devices/capabilities/scrapeable.cpp 
	src/jandy/devices/capabilities/screen.cpp 
	src/jandy/devices/keypad_device.cpp 
	src/jandy/devices/onetouch_device.cpp 
	src/jandy/devices/pda_device.cpp 
	src/jandy/devices/keypad/keypad_messageprocessors.cpp 
	src/jandy/devices/keypad/keypad_pageprocessors.cpp 
	src/jandy/devices/onetouch/onetouch_messageprocessors.cpp 
	src/jandy/devices/onetouch/onetouch_pageprocessors.cpp 
	src/jandy/devices/onetouch/onetouch_screenscraping.cpp 
	src/jandy/devices/pda/pda_messageprocessors.cpp 	 
	src/jandy/devices/pda/pda_pageprocessors.cpp 
	src/jandy/equipment/jandy_equipment.cpp
	src/jandy/equipment/jandy_equipment_modes.cpp 
	src/jandy/equipment/jandy_equipment_types.cpp 
	src/jandy/equipment/jandy_equipment_versions.cpp 
	src/jandy/errors/jandy_errors_messages.cpp
	src/jandy/errors/jandy_errors_protocol.cpp
	src/jandy/errors/string_conversion_errors.cpp
	src/jandy/factories/jandy_message_factory.cpp
	src/jandy/factories/jandy_message_factory_registration.cpp
	src/jandy/formatters/jandy_device_formatters.cpp 
	src/jandy/formatters/jandy_message_formatters.cpp 
	src/jandy/formatters/screen_data_page_formatter.cpp
	src/jandy/generator/jandy_message_generator.cpp
	src/jandy/generator/jandy_message_generator_buffercleanup.cpp
	src/jandy/generator/jandy_message_generator_buffervalidation.cpp
	src/jandy/generator/jandy_message_generator_packetprocessing.cpp
	src/jandy/generator/jandy_message_generator_packetvalidation.cpp
	src/jandy/generator/jandy_message_generator_startendsequence.cpp
	src/jandy/generator/jandy_rawdata_generator.cpp 
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
	src/jandy/messages/jandy_message_message_loop_start.cpp
	src/jandy/messages/jandy_message_probe.cpp
	src/jandy/messages/jandy_message_status.cpp	 
	src/jandy/messages/jandy_message_unknown.cpp
	src/jandy/types/jandy_types.cpp
	src/jandy/utility/filtered_slot.cpp 
	src/jandy/utility/jandy_checksum.cpp 
	src/jandy/utility/screen_data_page.cpp
	src/jandy/utility/screen_data_page_graph.cpp 
	src/jandy/utility/screen_data_page_graph/screen_data_page_graph_edge.cpp 
	src/jandy/utility/screen_data_page_graph/screen_data_page_graph_traverse.cpp 
	src/jandy/utility/screen_data_page_graph/screen_data_page_graph_validator.cpp
	src/jandy/utility/screen_data_page_graph/screen_data_page_graph_vertex.cpp 
	src/jandy/utility/screen_data_page_processor.cpp 
	src/jandy/utility/screen_data_page_updater.cpp
	src/jandy/utility/screen_data_page_updater/screen_data_page_updater_context.cpp
	src/jandy/utility/screen_data_page_updater/screen_data_page_updater_evhighlight.cpp 
	src/jandy/utility/screen_data_page_updater/screen_data_page_updater_evhighlightchars.cpp 
	src/jandy/utility/screen_data_page_updater/screen_data_page_updater_evshift.cpp
	src/jandy/utility/screen_data_page_updater/screen_data_page_updater_evupdate.cpp
	src/jandy/utility/slot_connection_manager.cpp
	src/jandy/utility/string_manipulation.cpp 
	src/jandy/utility/string_conversion/auxillary_state.cpp 
	src/jandy/utility/string_conversion/temperature.cpp 
	src/jandy/utility/string_conversion/timeout_duration.cpp 
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
	src/options/options_emulated_device_options.cpp 
	src/options/options_initialise.cpp
	src/options/options_option_type.cpp
	src/options/options_serial_options.cpp
	src/options/options_settings.cpp
	src/options/options_web_options.cpp
	src/options/helpers/conflicting_options_helper.cpp 
	src/options/helpers/option_dependency_helper.cpp 
	src/options/validators/jandy_device_id_validator.cpp 
	src/options/validators/jandy_emulated_device_type_validator.cpp
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
	$<$<BOOL:${ENABLE_PROFILING}>:src/profiling/profiling_units/tracy_zone_datamap.cpp>	
	src/profiling/profiling_units/timed_zone.cpp 
	src/profiling/profiling_units/zone.cpp 
	src/profiling/types/profiling_types.cpp
	src/protocol/protocol_handler.cpp 
	src/protocol/protocol_handler_read.cpp 
	src/protocol/protocol_handler_write.cpp 
	src/serial/serial_initialise.cpp
	src/serial/serial_operating_modes.cpp
	src/serial/serial_port.cpp
	src/serial/serial_types.cpp
	src/signals/async_signal.cpp
	src/signals/async_slot.cpp
	src/signals/signal_awaitable.cpp
	src/signals/signal_cleanup.cpp
	src/signals/signal_handler.cpp
	src/utility/array_standard_formatter.cpp
	src/utility/circular_buffer_wrapping_iterator.cpp
	src/utility/get_terminal_column_width.cpp	
	src/utility/value_debouncer.cpp 
	src/version/version.cpp 
	src/version/version_cmake.cpp 
	src/version/version_git.cpp 
	${CMAKE_CURRENT_BINARY_DIR}/src/version/version_cmake.constants.cpp # Generated by the configure_file command in the versioning section
	${CMAKE_CURRENT_BINARY_DIR}/src/version/version_git.constants.cpp   # Generated by the GitWatcher.cmake in the versioning section
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

target_compile_features(libaqualink-automate PUBLIC cxx_std_23)

target_compile_definitions(
	libaqualink-automate 
	PUBLIC 
	CROW_ENABLE_COMPRESSION 
	CROW_ENABLE_SSL 
	$<$<BOOL:${ENABLE_PROFILING}>:TRACY_ENABLE>
)

target_include_directories(libaqualink-automate PUBLIC "${PROJECT_SOURCE_DIR}/src" "${CMAKE_CURRENT_BINARY_DIR}/src")

target_include_directories(
	libaqualink-automate 
	SYSTEM 
	PUBLIC 
	${Boost_INCLUDE_DIRS} 
	${Crow_INCLUDE_DIRS} 
	${magic_enum_INCLUDE_DIRS}
	${nlohmann_json_INCLUDE_DIRS}
	${OpenSSL_INCLUDE_DIRS}
	$<$<AND:$<BOOL:${ENABLE_PROFILING}>,$<BOOL:${UProf_FOUND}>>:${UProf_INCLUDE_DIRS}>
	$<$<AND:$<BOOL:${ENABLE_PROFILING}>,$<BOOL:${VTune_FOUND}>>:${VTune_INCLUDE_DIRS}>
	$<$<BOOL:${ENABLE_PROFILING}>:${Tracy_INCLUDE_DIRS}>
)

target_link_libraries(
	libaqualink-automate 
	PUBLIC
	Boost::boost 
	Boost::coroutine 
	Boost::iostreams 
	Boost::log 
	Boost::log_setup 
	Boost::program_options 
	Crow::Crow
	magic_enum::magic_enum
	mqtt_cpp_iface::mqtt_cpp_iface 
	nlohmann_json::nlohmann_json
	OpenSSL::SSL 
	OpenSSL::Crypto
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

add_executable(
	aqualink-automate
	src/aqualink-automate.cpp 
)

target_link_libraries(
	aqualink-automate 
	PRIVATE 
	"$<LINK_LIBRARY:WHOLE_ARCHIVE,libaqualink-automate>"
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
		test/devices/test_devices_id.cpp 
		test/devices/test_devices_screenscraping.cpp 
		test/devices/test_devices_types.cpp 
		test/generators/test_jandy_message_generator.cpp 
		test/generators/test_jandy_rawdata_generator.cpp 
		test/logging/test_logging_validator_jandyemulateddevicetype.cpp 
		test/logging/test_logging_validator_jandydeviceid.cpp 
		test/logging/test_logging_validator_profiler.cpp 
		test/logging/test_logging_validator_severity.cpp 
		test/messages/test_message_processing.cpp 
		test/mocks/mock_generator_messagetorawdata.cpp 
		test/mocks/mock_generator_rawdatatomessage.cpp 
		test/mocks/mock_serialport.cpp 
		test/protocol/test_protocol_handler.cpp 
		test/utility/test_utility_auxillarystate.cpp 
		test/utility/test_utility_jandychecksum.cpp 
		test/utility/test_utility_temperature.cpp 
		test/support/unit_test_ostream_support.cpp 
	)

	target_compile_definitions(testaqualink-automate PRIVATE -DBOOST_TEST_DYN_LINK)
	target_include_directories(testaqualink-automate PUBLIC "${PROJECT_SOURCE_DIR}/test")

	target_link_libraries(
		testaqualink-automate 
		PRIVATE 
		"$<LINK_LIBRARY:WHOLE_ARCHIVE,libaqualink-automate>"
		Boost::unit_test_framework
	)

	enable_testing()
	add_test(AqualinkAutomateTests testaqualink-automate)

endif(ENABLE_TESTS)

#------------------------------------------------------------------------------
#
#
#
#
#------------------------------------------------------------------------------

if(ENABLE_SANITIZERS)

add_sanitizers(libaqualink-automate)
add_sanitizers(aqualink-automate)
add_sanitizers(testaqualink-automate)

endif(ENABLE_SANITIZERS)

#------------------------------------------------------------------------------
#
#
#
#
#------------------------------------------------------------------------------

set(SOURCE_WEB_ASSETS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assets/web)
set(DESTINATION_DIR ${CMAKE_CURRENT_BINARY_DIR}/)

add_custom_target(
	copy_web_assets 
	ALL
	COMMAND ${CMAKE_COMMAND} -E echo "Copying web assets..."
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${SOURCE_WEB_ASSETS_DIR} ${DESTINATION_DIR}
	COMMENT "Copying web assets to the static_web directory in the build folder"
)

#------------------------------------------------------------------------------
#
#
#
#
#------------------------------------------------------------------------------

include(CPackConfig)
