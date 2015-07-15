FILE( GLOB themes *.desktop)
install( FILES ${themes} DESTINATION ${DATA_INSTALL_DIR}/knemo/themes )

ecm_install_icons(
    ICONS
    sc-status-knemo-monochrome-error.svgz
    sc-status-knemo-monochrome-idle.svgz
    sc-status-knemo-monochrome-offline.svgz
    sc-status-knemo-monochrome-receive.svgz
    sc-status-knemo-monochrome-transmit-receive.svgz
    sc-status-knemo-monochrome-transmit.svgz
    DESTINATION ${ICON_INSTALL_DIR} )

