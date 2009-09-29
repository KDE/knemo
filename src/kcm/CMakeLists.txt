include_directories( ${KDE4_INCLUDES} ../common )

set( kcm_knemo_SRCS
    configdialog.cpp
    ../common/utils.cpp
)

kde4_add_ui_files( kcm_knemo_SRCS configdlg.ui )
kde4_add_plugin( kcm_knemo ${kcm_knemo_SRCS} )

target_link_libraries( kcm_knemo ${KDE4_KIO_LIBS} ${KDE4_KNOTIFYCONFIG_LIBS} ${LIBNL_LIBRARY} )

install(TARGETS kcm_knemo DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES kcm_knemo.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

