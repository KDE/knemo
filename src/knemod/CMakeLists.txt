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
    backends/backendbase.cpp
    ../common/utils.cpp
)

if ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
    set( knemo_SRCS ${knemo_SRCS} backends/netlinkbackend.cpp )
else ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
    set( knemo_SRCS ${knemo_SRCS} backends/bsdbackend.cpp )
endif ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )

kde4_add_ui_files( knemo_SRCS interfacestatisticsdlg.ui interfacestatusdlg.ui plotterconfigdlg.ui )
kde4_add_executable( knemo ${knemo_SRCS} )

target_link_libraries( knemo ${KDE4_KIO_LIBS} ${LIBIW_LIBRARIES} ${LIBNL_LIBRARIES} )

install( TARGETS knemo ${INSTALL_TARGETS_DEFAULT_ARGS} )

install( FILES knemo.notifyrc DESTINATION ${DATA_INSTALL_DIR}/knemo )
install( FILES knemo.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
install( FILES knemo.desktop DESTINATION ${AUTOSTART_INSTALL_DIR} )

add_subdirectory( icons )
add_subdirectory( themes )
