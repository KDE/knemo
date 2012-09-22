include_directories( ${KDE4_INCLUDES} ../common )

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
    signalplotter.cpp
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

kde4_add_ui_files( knemo_SRCS interfacestatisticsdlg.ui interfacestatusdlg.ui plotterconfigdlg.ui )
kde4_add_executable( knemo ${knemo_SRCS} )

target_link_libraries( knemo ${KDE4_KIO_LIBS} ${LIBIW_LIBRARIES} ${LIBNL_LIBRARIES} ${QT_QTSQL_LIBRARY} )

install( TARGETS knemo ${INSTALL_TARGETS_DEFAULT_ARGS} )

install( FILES knemo.notifyrc DESTINATION ${DATA_INSTALL_DIR}/knemo )
install( PROGRAMS knemo.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
install( FILES knemo.desktop DESTINATION ${AUTOSTART_INSTALL_DIR} )

add_subdirectory( icons )
add_subdirectory( themes )
