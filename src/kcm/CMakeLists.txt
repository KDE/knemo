include_directories( ../common )

set( kcm_knemo_SRCS
    configdialog.cpp
    themeconfig.cpp
    statsconfig.cpp
    warnconfig.cpp
    dateeditwidget.cpp
    kdatepickerpopup.cpp
    ../common/data.cpp
    ../common/utils.cpp
)

ki18n_wrap_ui( kcm_knemo_SRCS
    configdlg.ui
    themecfg.ui
    statscfg.ui
    warncfg.ui
)

add_library( kcm_knemo ${kcm_knemo_SRCS} )

target_link_libraries( kcm_knemo
    Qt5::DBus
    KF5::KDELibs4Support
    KF5::KCMUtils
    KF5::KIOWidgets
    KF5::NotifyConfig
    KF5::I18n
    ${LIBNL_LIBRARIES}
)

install(TARGETS kcm_knemo DESTINATION ${KDE_INSTALL_PLUGINDIR} )
install( FILES kcm_knemo.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )

