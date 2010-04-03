project( knemo )

set( KNEMO_VERSION 0.6.1+svn )

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

find_package( KDE4 REQUIRED )
include( CheckLibraryExists )
include( FindPackageHandleStandardArgs )
include(MacroOptionalAddSubdirectory)

if ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
    find_package( Libnl )
    macro_log_feature( LIBNL_FOUND "libnl" "Linux netlink sockets library" "http://people.suug.ch/~tgr/libnl/" TRUE "" "" )

    find_package( Libiw )
    macro_log_feature( LIBIW_FOUND "libiw" "Linux Wireless Extensions library" "http://www.hpl.hp.com/personal/Jean_Tourrilhes/Linux/Tools.html" FALSE "" "" )

    if ( LIBIW_FOUND )
        set ( HAVE_LIBIW 1 )
    endif ( LIBIW_FOUND )

endif ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )

option (LEGACY_TRAY_ICON "Force the tray icon to be compiled as a KSystemTrayIcon." FALSE )
if ( NOT LEGACY_TRAY_ICON )
    if ( ${KDE_VERSION} VERSION_GREATER "4.3.80" )
        set ( HAVE_KSTATUSNOTIFIERITEM 1 )
    endif ( ${KDE_VERSION} VERSION_GREATER "4.3.80" )
endif ( NOT LEGACY_TRAY_ICON )

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/config-knemo.h.cmake
                ${CMAKE_CURRENT_BINARY_DIR}/config-knemo.h
)

macro_display_feature_log()

set( CMAKE_INCLUDE_CURRENT_DIR TRUE )

add_definitions( ${QT_DEFINITIONS} ${KDE4_DEFINITIONS} )

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

add_subdirectory( src )
