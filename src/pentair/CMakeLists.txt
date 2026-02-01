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
	options/options_pentair.cpp
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
