FILE( GLOB themes *.desktop)
install( FILES ${themes} DESTINATION ${DATA_INSTALL_DIR}/knemo/themes )

add_subdirectory( breeze )
add_subdirectory( breeze-dark )
add_subdirectory( hicolor )
add_subdirectory( oxygen )
