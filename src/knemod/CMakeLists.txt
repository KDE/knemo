include_directories( ../common )

set( knemo_SRCS
    main.cpp
    global.cpp
    interface.cpp
    interfaceicon.cpp
    interfaceplotterdialog.cpp
    interfacestatistics.cpp
    interfacestatisticsdialog.cpp
    interfacestatusdialog.cpp
    interfacetray.cpp
    knemodaemon.cpp
    plotterconfigdialog.cpp
    statisticsmodel.cpp
    statisticsview.cpp
    backends/backendbase.cpp
    ../common/data.cpp
    ../common/utils.cpp
    storage/sqlstorage.cpp
    storage/xmlstorage.cpp
    syncstats/externalstats.cpp
    syncstats/statsfactory.cpp
    syncstats/stats_vnstat.cpp
)

if ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
    set( knemo_SRCS ${knemo_SRCS} backends/netlinkbackend.cpp )
    if ( LIBIW_FOUND )
        set( knemo_SRCS ${knemo_SRCS} backends/netlinkbackend_wireless.cpp )
    endif( LIBIW_FOUND )
else ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
    set( knemo_SRCS ${knemo_SRCS} backends/bsdbackend.cpp )
endif ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )

ki18n_wrap_ui( knemo_SRCS
    interfacestatisticsdlg.ui
    interfacestatusdlg.ui
    plotterconfigdlg.ui
)

add_executable( knemo ${knemo_SRCS} )

target_link_libraries( knemo
    PUBLIC
    Qt5::Sql
    KF5::KIOWidgets
    KF5::DBusAddons
    KF5::GlobalAccel
    KF5::KDELibs4Support
    KF5::Notifications
    KF5::Plasma
    KF5::SignalPlotter
    KF5::XmlGui
    KF5::WindowSystem
    KF5::I18n
    ${LIBNL_LIBRARIES}
    ${LIBIW_LIBRARIES}
)


install( TARGETS knemo ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

install( FILES knemo.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFY5RCDIR} )
install( PROGRAMS knemo.desktop DESTINATION ${KDE_INSTALL_APPDIR} )
install( FILES knemo.desktop DESTINATION ${KDE_INSTALL_AUTOSTARTDIR} )

add_subdirectory( icons )
add_subdirectory( themes )
