project( knemo )

set( KNEMO_VERSION 0.7.4 )

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

set(KDE_MIN_VERSION "4.7.0")
find_package( KDE4 REQUIRED )
include( CheckLibraryExists )
include( FindPackageHandleStandardArgs )
include(MacroOptionalAddSubdirectory)

set( QT_USE_QTSQL TRUE )
if ( NOT QT_QTSQL_FOUND )
    message( FATAL_ERROR "The QtSql development package could not be found. Please install libQtSql." )
endif( NOT QT_QTSQL_FOUND )

find_package(LibKSignalPlotter REQUIRED)

option (NO_WIRELESS_SUPPORT "Disable support for wireless devices." FALSE )
if ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
    find_package( Libnl REQUIRED )

    if ( NOT NO_WIRELESS_SUPPORT )
        find_package( Libiw )
        macro_log_feature( LIBIW_FOUND "libiw" "Linux Wireless Extensions library" "http://www.hpl.hp.com/personal/Jean_Tourrilhes/Linux/Tools.html" FALSE "" "" )

        if ( LIBIW_FOUND )
            set ( HAVE_LIBIW 1 )
        endif ( LIBIW_FOUND )
    endif ( NOT NO_WIRELESS_SUPPORT )

endif ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/config-knemo.h.cmake
                ${CMAKE_CURRENT_BINARY_DIR}/config-knemo.h
)

macro_display_feature_log()

set( CMAKE_INCLUDE_CURRENT_DIR TRUE )

add_definitions( ${QT_DEFINITIONS} ${KDE4_DEFINITIONS} )

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

add_subdirectory( src )
