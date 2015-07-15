FILE( GLOB themes *.desktop)
install( FILES ${themes} DESTINATION ${DATA_INSTALL_DIR}/knemo/themes )

ecm_install_icons(
    ICONS
    22-status-knemo-modem-error.png
    22-status-knemo-modem-idle.png
    22-status-knemo-modem-offline.png
    22-status-knemo-modem-receive.png
    22-status-knemo-modem-transmit-receive.png
    22-status-knemo-modem-transmit.png
    DESTINATION ${ICON_INSTALL_DIR} )

