FILE( GLOB themes *.desktop)
install( FILES ${themes} DESTINATION ${DATA_INSTALL_DIR}/knemo/themes )

ecm_install_icons(
    ICONS
    22-status-knemo-network-error.png
    22-status-knemo-network-idle.png
    22-status-knemo-network-offline.png
    22-status-knemo-network-receive.png
    22-status-knemo-network-transmit-receive.png
    22-status-knemo-network-transmit.png
    DESTINATION ${ICON_INSTALL_DIR} )

