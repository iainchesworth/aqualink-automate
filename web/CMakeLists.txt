#------------------------------------------------------------------------------
#
#
#
#
#------------------------------------------------------------------------------

set(SOURCE_WEB_ASSETS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assets)
set(DESTINATION_DIR ${CMAKE_CURRENT_BINARY_DIR})

add_custom_target(
	copy_web_assets 
	ALL
	COMMAND ${CMAKE_COMMAND} -E echo "Copying web assets..."
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${SOURCE_WEB_ASSETS_DIR} ${DESTINATION_DIR}
	COMMENT "Copying web assets to the static_web directory in the build folder"
)

add_dependencies(aqualink-automate copy_web_assets)
