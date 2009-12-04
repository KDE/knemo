project( knemo )

set( KNEMO_VERSION 0.6.0+svn )

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

option (USE_KNOTIFICATIONITEM "If building against KDE 4.3, use the experimental KNotificationItem for the tray icon." FALSE )
if ( USE_KNOTIFICATIONITEM )
    if ( ${KDE_VERSION} VERSION_GREATER "4.3.66" )
    #it's in kdeui now
    else ( ${KDE_VERSION} VERSION_GREATER "4.3.66" )
        if ( ${KDE_VERSION} VERSION_GREATER "4.2.84" )
            find_package(LibKNotificationItem-1)
            macro_log_feature(LIBKNOTIFICATIONITEM-1_FOUND "LibKNotificationItem-1" "An experimental library for a new way to handle the system tray" "svn://anonsvn.kde.org/home/kde/branches/KDE/4.3/kdelibs/experimental" FALSE "" "Lets you compile KNemo's tray icon as a KNotificationItem")
        else ( ${KDE_VERSION} VERSION_GREATER "4.2.84" )
            #not available in 4.2; force off
            set( USE_KNOTIFICATIONITEM 0 )
        endif ( ${KDE_VERSION} VERSION_GREATER "4.2.84" )
    endif ( ${KDE_VERSION} VERSION_GREATER "4.3.66" )
endif ( USE_KNOTIFICATIONITEM )

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/config-knemo.h.cmake
                ${CMAKE_CURRENT_BINARY_DIR}/config-knemo.h
)

macro_display_feature_log()

set( CMAKE_INCLUDE_CURRENT_DIR TRUE )

add_definitions( ${QT_DEFINITIONS} ${KDE4_DEFINITIONS} )

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

add_subdirectory( src )
