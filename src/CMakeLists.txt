#------------------------------------------------------------------------------
#
#  APPLICATION COMPONENTS
#  ======================
#
#------------------------------------------------------------------------------

add_subdirectory(core)
add_subdirectory(jandy)
add_subdirectory(pentair)

#------------------------------------------------------------------------------
#
#  APPLICATION EXECUTABLE
#  ======================
#
#------------------------------------------------------------------------------

add_executable(aqualink-automate)

target_sources(
	aqualink-automate
	PRIVATE
	aqualink-automate.cpp 
)

target_link_libraries(
	aqualink-automate 
	PRIVATE
	libaqualink-automate
	libaqualink-jandy
	libaqualink-pentair
)

#------------------------------------------------------------------------------
#
#  APPLICATION WEB SERVER ASSETS
#  =============================
#
#------------------------------------------------------------------------------

add_custom_command(
	TARGET aqualink-automate 
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E echo "Copying web assets to build folder..."
	COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${CMAKE_SOURCE_DIR}/assets/web" "${CMAKE_BINARY_DIR}/templates"
)

add_custom_command(
	TARGET aqualink-automate 
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E echo "Copying SSL assets to build folder..."
	COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${CMAKE_SOURCE_DIR}/assets/ssl" "${CMAKE_BINARY_DIR}/ssl"
)
