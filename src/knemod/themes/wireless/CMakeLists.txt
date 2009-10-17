FILE( GLOB themes *.desktop)
install( FILES ${themes} DESTINATION ${DATA_INSTALL_DIR}/knemo/themes )

kde4_install_icons( ${ICON_INSTALL_DIR}  )
