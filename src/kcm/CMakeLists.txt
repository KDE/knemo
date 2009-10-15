include_directories( ${KDE4_INCLUDES} ../common )

set( kcm_knemo_SRCS
    configdialog.cpp
    ../common/utils.cpp
    kdateedit.cpp
    kdatepickerpopup.cpp
)

kde4_add_ui_files( kcm_knemo_SRCS configdlg.ui )
kde4_add_plugin( kcm_knemo ${kcm_knemo_SRCS} )

target_link_libraries( kcm_knemo knemo_backend ${KDE4_KIO_LIBS} ${KDE4_KNOTIFYCONFIG_LIBS} )

install(TARGETS kcm_knemo DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES kcm_knemo.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

