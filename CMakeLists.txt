project( knemo )

set( KNEMO_VERSION 0.5.2 )

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

find_package( KDE4 REQUIRED )
include( CheckLibraryExists )
include( FindPackageHandleStandardArgs )
include(MacroOptionalAddSubdirectory)

set( ifconfig_FIND_REQUIRED TRUE )

find_program( PATH_IFCONFIG ifconfig )
find_program( PATH_IWCONFIG iwconfig )
find_program( PATH_IWLIST iwlist )
find_program( PATH_ROUTE route )
find_library( LIBIW_LIBRARY iw )

option (USE_KNOTIFICATIONITEM "If building against KDE 4.3, use the experimental KNotificationItem for the tray icon.  This is the default.  Set to false to compile the tray icon as a KSystemTrayIcon." TRUE )
if ( ${KDE_VERSION} VERSION_GREATER "4.2.84" AND USE_KNOTIFICATIONITEM )
    find_package(LibKNotificationItem-1 REQUIRED)
    macro_log_feature(LIBKNOTIFICATIONITEM-1_FOUND "LibKNotificationItem-1" "An experimental library" "svn://anonsvn.kde.org/home/kde/trunk/KDE/kdelibs/experimental" TRUE "" "LibKNotificationItem-1 is an unfinished library which is required by knemo.")
    set ( USE_KNOTIFICATIONITEM 1 )
else ( ${KDE_VERSION} VERSION_GREATER "4.2.84" AND USE_KNOTIFICATIONITEM )
    set ( LIBKNOTIFICATIONITEM-1_LIBRARIES "" )
    set ( USE_KNOTIFICATIONITEM false )
endif ( ${KDE_VERSION} VERSION_GREATER "4.2.84" AND USE_KNOTIFICATIONITEM )

find_package_handle_standard_args( ifconfig DEFAULT_MSG PATH_IFCONFIG )
find_package_handle_standard_args( iwconfig DEFAULT_MSG PATH_IWCONFIG )
find_package_handle_standard_args( route DEFAULT_MSG PATH_ROUTE )
find_package_handle_standard_args( libiw DEFAULT_MSG LIBIW_LIBRARY )

if ( LIBIW_LIBRARY )
    set ( HAVE_LIBIW 1 )
else ( LIBIW_LIBRARY )
    set ( LIBIW_LIBRARY "" )
endif ( LIBIW_LIBRARY )

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/config-knemo.h.cmake
                ${CMAKE_CURRENT_BINARY_DIR}/config-knemo.h
)

set( CMAKE_INCLUDE_CURRENT_DIR TRUE )

add_definitions( ${QT_DEFINITIONS} ${KDE4_DEFINITIONS} )

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

add_subdirectory( src )
