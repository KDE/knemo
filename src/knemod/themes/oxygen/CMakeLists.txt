ecm_install_icons(
    ICONS
    sc-status-knemo-network-error.svg
    sc-status-knemo-network-idle.svg
    sc-status-knemo-network-offline.svg
    sc-status-knemo-network-receive.svg
    sc-status-knemo-network-transmit-receive.svg
    sc-status-knemo-network-transmit.svg
    DESTINATION ${ICON_INSTALL_DIR}
    THEME oxygen )

install(
    FILES
    knemo.svgz
    DESTINATION ${DATA_INSTALL_DIR}/plasma/desktoptheme/oxygen/icons )
