project( knemo )

set( KNEMO_VERSION 0.5.2 )

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

find_package( KDE4 REQUIRED )
include( CheckLibraryExists )
include( FindPackageHandleStandardArgs )
include(MacroOptionalAddSubdirectory)

if ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
    find_library( LIBNL_LIBRARY nl REQUIRED )
    find_library( LIBIW_LIBRARY iw )

    find_package_handle_standard_args( libnl DEFAULT_MSG LIBNL_LIBRARY )
    find_package_handle_standard_args( libiw DEFAULT_MSG LIBIW_LIBRARY )
endif ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )

option (USE_KNOTIFICATIONITEM "If building against KDE 4.3, use the experimental KNotificationItem for the tray icon." FALSE )
if ( ${KDE_VERSION} VERSION_GREATER "4.3.66" AND USE_KNOTIFICATIONITEM )
#it's in kdeui now
elseif ( ${KDE_VERSION} VERSION_GREATER "4.2.84" AND USE_KNOTIFICATIONITEM )
    find_package(LibKNotificationItem-1 REQUIRED)
    macro_log_feature(LIBKNOTIFICATIONITEM-1_FOUND "LibKNotificationItem-1" "An experimental library" "svn://anonsvn.kde.org/home/kde/trunk/KDE/kdelibs/experimental" TRUE "" "LibKNotificationItem-1 is an unfinished library which is required by knemo.")
    set ( USE_KNOTIFICATIONITEM 1 )
endif ( ${KDE_VERSION} VERSION_GREATER "4.3.66" AND USE_KNOTIFICATIONITEM )

if ( NOT LIBNL_LIBRARY )
    set ( LIBNL_LIBRARY "" )
endif ( NOT LIBNL_LIBRARY )

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
