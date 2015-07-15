FILE( GLOB themes *.desktop)
install( FILES ${themes} DESTINATION ${DATA_INSTALL_DIR}/knemo/themes )

ecm_install_icons(
    ICONS
    22-status-knemo-wireless-error.png
    22-status-knemo-wireless-idle.png
    22-status-knemo-wireless-offline.png
    22-status-knemo-wireless-receive.png
    22-status-knemo-wireless-transmit-receive.png
    22-status-knemo-wireless-transmit.png
    DESTINATION ${ICON_INSTALL_DIR} )

