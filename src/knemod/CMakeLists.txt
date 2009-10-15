include_directories( ${KDE4_INCLUDES} ../common )

set( knemo_SRCS
    main.cpp
    interface.cpp
    interfaceicon.cpp
    interfacemonitor.cpp
    interfaceplotterdialog.cpp
    interfacestatistics.cpp
    interfacestatisticsdialog.cpp
    interfacestatusdialog.cpp
    interfacetray.cpp
    knemodaemon.cpp
    signalplotter.cpp
    ../common/utils.cpp
)

kde4_add_ui_files( knemo_SRCS interfacestatisticsdlg.ui interfacestatusdlg.ui )
kde4_add_executable( knemo ${knemo_SRCS} )

target_link_libraries( knemo knemo_backend ${LIBKNOTIFICATIONITEM-1_LIBRARIES} ${KDE4_KIO_LIBS} )

install( TARGETS knemo ${INSTALL_TARGETS_DEFAULT_ARGS} )

install( FILES knemo.notifyrc DESTINATION ${DATA_INSTALL_DIR}/knemo )
install( FILES knemo.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
install( FILES knemo.desktop DESTINATION ${AUTOSTART_INSTALL_DIR} )

add_subdirectory( backends )
add_subdirectory( icons )
add_subdirectory( pics )
