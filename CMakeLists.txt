cmake_minimum_required (VERSION 3.20 FATAL_ERROR)

set(VCPKG_ROOT "deps/vcpkg")
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

project(aqualink-automate VERSION 0.0.1 LANGUAGES C CXX)

option(ENABLE_TESTS "Enable unit tests" ON)

find_package(Boost REQUIRED COMPONENTS coroutine iostreams log program_options unit_test_framework)
find_package(magic_enum REQUIRED)

#------------------------------------------------------------------------------
#
#
#
#
#------------------------------------------------------------------------------

add_library(
	libaqualink-automate 
	src/developer/mock_serial_port.cpp
	src/equipment/equipment_base.cpp
	src/equipment/jandy/jandy_equipment.cpp
	src/errors/error_codes.cpp
	src/errors/message_error_codes.cpp
	src/errors/protocol_error_codes.cpp
	src/exceptions/exception_genericaqualinkexception.cpp
	src/exceptions/exception_optionparsingfailed.cpp
	src/exceptions/exception_optionshelporversion.cpp
	src/logging/global_logger.cpp
	src/logging/logging.cpp
	src/logging/logging_attributes.cpp
	src/logging/logging_channels.cpp
	src/logging/logging_formatter.cpp
	src/logging/logging_initialise.cpp 
	src/logging/logging_severity_filter.cpp
	src/logging/logging_severity_levels.cpp
	src/logging/formatters/logging_formatters.cpp 
	src/messages/jandy/formatters/jandy_message_formatters.cpp
	src/messages/jandy/messages/jandy_message.cpp
	src/messages/jandy/messages/jandy_message_ack.cpp
	src/messages/jandy/messages/jandy_message_message.cpp
	src/messages/jandy/messages/jandy_message_message_long.cpp
	src/messages/jandy/messages/jandy_message_probe.cpp
	src/messages/jandy/messages/jandy_message_status.cpp
	src/messages/jandy/messages/jandy_message_unknown.cpp
	src/messages/jandy/messages/aquarite/aquarite_message.cpp
	src/messages/jandy/messages/aquarite/aquarite_message_percent.cpp
	src/messages/jandy/messages/aquarite/aquarite_message_getid.cpp
	src/messages/jandy/messages/aquarite/aquarite_message_ppm.cpp
	src/messages/jandy/jandy_message_constants.cpp	
	src/messages/jandy/jandy_message_factory.cpp
	src/messages/jandy/jandy_message_generator.cpp
	src/messages/jandy/jandy_message_handler.cpp
	src/messages/jandy/jandy_message_processors.cpp
	src/messages/jandy/jandy_message_registration.cpp
	src/messages/jandy/jandy_message_type.cpp
	src/messages/message.cpp
	src/messages/message_generator.cpp
	src/messages/message_handler.cpp
	src/options/options_app_options.cpp
	src/options/options_developer_options.cpp
	src/options/options_initialise.cpp
	src/options/options_option_type.cpp
	src/options/options_serial_options.cpp
	src/options/options_settings.cpp
	src/options/options_web_options.cpp	
	src/options/validators/severity_level_validator.cpp 
	src/protocol/protocol_handler.cpp
	src/serial/serial_initialise.cpp
	src/serial/serial_operating_modes.cpp
	src/serial/serial_port.cpp
	src/serialization/serializable.cpp
	src/signals/signal_awaitable.cpp
	src/signals/signal_cleanup.cpp
	src/signals/signal_handler.cpp	
	src/utility/get_terminal_column_width.cpp
)

if(WIN32)
target_sources(
	libaqualink-automate 
	PRIVATE
	src/platform/utility/get_terminal_column_width_win32.cpp
)

target_compile_definitions(libaqualink-automate PUBLIC -D_WIN32_WINNT=0x0601 -DWINVER=0x0601 -DNOMINMAX -DWIN32_LEAN_AND_MEAN -DVC_EXTRALEAN)
endif(WIN32)

if(LINUX)
target_sources(
	libaqualink-automate 
	PRIVATE
	src/platform/utility/get_terminal_column_width_linux.cpp
)
endif(LINUX)

target_compile_features(libaqualink-automate PUBLIC cxx_std_23)
target_include_directories(libaqualink-automate PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_include_directories(libaqualink-automate SYSTEM PUBLIC ${Boost_INCLUDE_DIRS} ${magic_enum_INCLUDE_DIRS})

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

target_link_libraries(aqualink-automate PRIVATE libaqualink-automate Boost::boost Boost::coroutine Boost::iostreams Boost::log Boost::log_setup Boost::program_options)

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
	target_link_libraries(testaqualink-automate PRIVATE libaqualink-automate Boost::boost Boost::coroutine Boost::iostreams Boost::log Boost::log_setup Boost::program_options Boost::unit_test_framework)

	enable_testing()
	add_test(AqualinkAutomateTests testaqualink-automate)

endif(ENABLE_TESTS)
