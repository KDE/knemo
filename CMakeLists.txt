project( knemo )

set( KNEMO_VERSION 0.5.2 )

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

set( CMAKE_VERBOSE_MAKEFILE TRUE )
set( CMAKE_INCLUDE_CURRENT_DIR TRUE )

add_definitions( ${QT_DEFINITIONS} ${KDE4_DEFINITIONS} )

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )
link_directories( ${KDE4_LIB_DIR} )

add_subdirectory( src )
