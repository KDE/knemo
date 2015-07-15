FILE( GLOB themes *.desktop)
install( FILES ${themes} DESTINATION ${DATA_INSTALL_DIR}/knemo/themes )

ecm_install_icons(
    ICONS
    22-status-knemo-monitor-error.png
    22-status-knemo-monitor-idle.png
    22-status-knemo-monitor-offline.png
    22-status-knemo-monitor-receive.png
    22-status-knemo-monitor-transmit-receive.png
    22-status-knemo-monitor-transmit.png
    DESTINATION ${ICON_INSTALL_DIR} )

