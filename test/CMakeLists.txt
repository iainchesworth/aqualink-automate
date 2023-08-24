#------------------------------------------------------------------------------
#
#
#
#
#------------------------------------------------------------------------------

add_executable(
	testaqualink-automate
	unit/test_main.cpp 
	unit/concepts/test_concept_is_c_array.cpp 
	unit/devices/test_devices_id.cpp 
	unit/devices/test_devices_onetouch_pageprocessors.cpp 
	unit/devices/test_devices_screendatapage.cpp 
	unit/devices/test_devices_screendatapage_processor.cpp 
	unit/devices/test_devices_screenscraping.cpp 
	unit/devices/test_devices_types.cpp 
	unit/factories/test_factories_jandy_auxillary.cpp 
	unit/generators/test_jandy_message_generator.cpp 
	unit/generators/test_jandy_rawdata_generator.cpp 
	unit/http/test_api_equipment.cpp 
	unit/http/test_ws_equipment.cpp 
	unit/kernel/test_auxillary_traits.cpp 
	unit/logging/test_logging_validator_jandyemulateddevicetype.cpp 
	unit/logging/test_logging_validator_jandydeviceid.cpp 
	unit/logging/test_logging_validator_profiler.cpp 
	unit/logging/test_logging_validator_severity.cpp 
	unit/messages/test_message.cpp 
	unit/messages/test_message_ack.cpp 
	unit/messages/test_message_message.cpp 
	unit/messages/test_message_message_long.cpp 	 
	unit/messages/test_message_pda_highlight.cpp 
	unit/messages/test_message_probe.cpp 
	unit/messages/test_message_status.cpp 
	unit/messages/test_message_unknown.cpp 
	unit/mocks/mock_generator_messagetorawdata.cpp 
	unit/mocks/mock_generator_rawdatatomessage.cpp 
	unit/mocks/mock_serialport.cpp 
	unit/protocol/test_protocol_handler.cpp 
	unit/units/test_flow_rate.cpp 
	unit/units/test_units_orp.cpp 
	unit/units/test_units_ph.cpp 
	unit/utility/test_utility_auxillarystate.cpp 
	unit/utility/test_utility_chemistry.cpp 
	unit/utility/test_utility_jandychecksum.cpp 
	unit/utility/test_utility_signallingstatscounter.cpp 
	unit/utility/test_utility_temperature.cpp 
	unit/support/unit_test_hublocatorinjector.cpp 
	unit/support/unit_test_onetouchdevice.cpp 
	unit/support/unit_test_onetouchdevice_httpserver.cpp 
	unit/support/unit_test_ostream_support.cpp 
)

if(WIN32)
target_sources(testaqualink-automate PRIVATE unit/platform/test_main_win32.cpp)
target_compile_definitions(testaqualink-automate PRIVATE -DBOOST_TEST_DYN_LINK) # Boost Unit Test is dynamically linked under Windows.
endif(WIN32)

if(LINUX)
target_sources(testaqualink-automate PRIVATE unit/platform/test_main_linux.cpp)
endif(LINUX)

target_include_directories(testaqualink-automate PUBLIC "${PROJECT_SOURCE_DIR}/test/unit")

target_link_libraries(
	testaqualink-automate 
	PRIVATE 
	"$<LINK_LIBRARY:WHOLE_ARCHIVE,libaqualink-automate>"
	Boost::unit_test_framework
)

add_test(AqualinkAutomateUnitTests testaqualink-automate)

if(ENABLE_COVERAGE)
setup_target_for_coverage_lcov(
	NAME coverage-testaqualink-automate 
	EXECUTABLE ctest -j ${PROCESSOR_COUNT} 
	DEPENDENCIES testaqualink-automate
	EXCLUDE 
		"${PROJECT_SOURCE_DIR}/deps/*"
		"/usr/include/*"
		"include/*"
)
endif(ENABLE_COVERAGE)

if(ENABLE_SANITIZERS)
add_sanitizers(testaqualink-automate)
endif(ENABLE_SANITIZERS)

#------------------------------------------------------------------------------
#
#
#
#
#------------------------------------------------------------------------------

if(ENABLE_BENCHMARKS)

add_executable(
	perfaqualink-automate
	performance/test_main.cpp 
	performance/test_serialization.cpp 
)

target_include_directories(perfaqualink-automate PUBLIC "${PROJECT_SOURCE_DIR}/test/performance")

target_link_libraries(
	perfaqualink-automate 
	PRIVATE 
	"$<LINK_LIBRARY:WHOLE_ARCHIVE,libaqualink-automate>"
	benchmark::benchmark 
	benchmark::benchmark_main
)

add_test(AqualinkAutomatePerfTests perfaqualink-automate)

if(ENABLE_SANITIZERS)
add_sanitizers(perfaqualink-automate)
endif(ENABLE_SANITIZERS)

endif(ENABLE_BENCHMARKS)
