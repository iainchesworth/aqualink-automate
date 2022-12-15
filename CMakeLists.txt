cmake_minimum_required (VERSION 3.20 FATAL_ERROR)

set(VCPKG_ROOT "deps/vcpkg")
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

project(aqualink-automate VERSION 0.0.1 LANGUAGES C CXX)

find_package(Boost REQUIRED COMPONENTS coroutine log program_options)

add_executable(
	aqualink-automate 
	src/aqualink-automate.cpp 
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
	src/messages/message.cpp
	src/messages/message_types.cpp
	src/options/options_app_options.cpp
	src/options/options_initialise.cpp
	src/options/options_option_type.cpp
	src/options/options_serial_options.cpp
	src/options/options_web_options.cpp
	src/protocol/protocol_handler.cpp
	src/protocol/protocol_jandy.cpp
	src/serial/serial_port.cpp 
	src/serialization/serializable.cpp
	src/signals/signal_awaitable.cpp
	src/signals/signal_handler.cpp
	src/utility/describe_enumerators_as_array.cpp
	src/utility/get_terminal_column_width.cpp
)

if(WIN32)
target_sources(
	aqualink-automate
	PRIVATE
	src/platform/utility/get_terminal_column_width_win32.cpp
)

target_compile_definitions(aqualink-automate PRIVATE -D_WIN32_WINNT=0x0601 -DWINVER=0x0601)
endif(WIN32)

if(LINUX)
target_sources(
	aqualink-automate
	PRIVATE
	src/platform/utility/get_terminal_column_width_linux.cpp
)
endif(LINUX)

target_compile_features(aqualink-automate PRIVATE cxx_std_23)
target_include_directories(aqualink-automate PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(aqualink-automate PRIVATE Boost::boost Boost::coroutine Boost::log Boost::log_setup Boost::program_options)
