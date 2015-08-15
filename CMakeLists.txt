project( knemo )

cmake_minimum_required(VERSION 2.8.12)

find_package (ECM REQUIRED NO_MODULE)
set (CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/modules)

set( KNEMO_VERSION 0.7.6+svn )

include(KDEInstallDirs)
include(KDEFrameworkCompilerSettings)
include (KDECMakeSettings)
include(FeatureSummary)
include(ECMInstallIcons)

find_package(Qt5 REQUIRED COMPONENTS
    Core
    DBus
    Sql
    Widgets
)

find_package(KF5 REQUIRED COMPONENTS
    Config
    ConfigWidgets
    CoreAddons
    GlobalAccel
    GuiAddons
    DBusAddons
    I18n
    KCMUtils
    KDELibs4Support
    KIO
    Notifications
    NotifyConfig
    Plasma
    SysGuard
    WidgetsAddons
    WindowSystem
    XmlGui
)

include( CheckLibraryExists )
include( FindPackageHandleStandardArgs )

option (NO_WIRELESS_SUPPORT "Disable support for wireless devices." FALSE )
if ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
    find_package( Libnl REQUIRED )

    if ( NOT NO_WIRELESS_SUPPORT )
        find_package( Libiw )
        set_package_properties( "libiw" PROPERTIES DESCRIPTION "Linux Wireless Extensions library" URL "http://www.hpl.hp.com/personal/Jean_Tourrilhes/Linux/Tools.html" TYPE RECOMMENDED )

        if ( LIBIW_FOUND )
            set ( HAVE_LIBIW 1 )
        endif ( LIBIW_FOUND )
    endif ( NOT NO_WIRELESS_SUPPORT )

endif ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/config-knemo.h.cmake
                ${CMAKE_CURRENT_BINARY_DIR}/config-knemo.h
)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

set( CMAKE_INCLUDE_CURRENT_DIR TRUE )

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

add_subdirectory( src )
