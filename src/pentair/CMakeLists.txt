#------------------------------------------------------------------------------
#
#  PENTAIR EQUIPMENT SUPPORT LIBRARY
#  =================================
#
#------------------------------------------------------------------------------

add_library(libaqualink-pentair)

target_sources(
	libaqualink-pentair
	PRIVATE
	devices/pentair_device.cpp
	devices/pentair_device_id.cpp
	devices/pentair_vsp_pump_device.cpp
	equipment/pentair_equipment.cpp
	factories/pentair_message_factory.cpp
	generator/pentair_message_generator.cpp
	messages/pentair_message.cpp
	messages/pentair_message_unknown.cpp
	messages/pump/pentair_pump_message_power.cpp
	messages/pump/pentair_pump_message_speed.cpp
	messages/pump/pentair_pump_message_status.cpp
	options/options_pentair.cpp
	protocol/pentair_protocol_registration.cpp
	pentair.cpp
)

target_compile_features(libaqualink-pentair PUBLIC cxx_std_23)

target_include_directories(
	libaqualink-pentair
	PUBLIC
	"${PROJECT_SOURCE_DIR}/src"
	"${PROJECT_SOURCE_DIR}/src/pentair"
)

target_include_directories(
	libaqualink-pentair 
	SYSTEM 
	PUBLIC 	
	${Boost_INCLUDE_DIRS} 
	${magic_enum_INCLUDE_DIRS}
)

target_link_libraries(
	libaqualink-pentair
	PUBLIC
	Boost::boost 
	magic_enum::magic_enum
	PRIVATE
	libaqualink-automate
)
