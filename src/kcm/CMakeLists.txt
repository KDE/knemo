include_directories( ${KDE4_INCLUDES} ../common )

set( kcm_knemo_SRCS
    configdialog.cpp
    themeconfig.cpp
    statsconfig.cpp
    warnconfig.cpp
    kdateedit.cpp
    kdatepickerpopup.cpp
    ../common/data.cpp
    ../common/utils.cpp
)

kde4_add_ui_files( kcm_knemo_SRCS configdlg.ui themecfg.ui statscfg.ui warncfg.ui )
kde4_add_plugin( kcm_knemo ${kcm_knemo_SRCS} )

target_link_libraries( kcm_knemo ${KDE4_KIO_LIBS} ${KDE4_KNOTIFYCONFIG_LIBS} ${LIBNL_LIBRARIES} )

install(TARGETS kcm_knemo DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES kcm_knemo.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

