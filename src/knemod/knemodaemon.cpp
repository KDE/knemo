/* This file is part of KNemo
   Copyright (C) 2004, 2006 Percy Leonhardt <percy@eris23.de>
   Copyright (C) 2009 John Stamp <jstamp@users.sourceforge.net>

   KNemo is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   KNemo is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <QtDBus/QDBusConnection>
#include <QTimer>

#include <KAboutData>
#include <KConfigGroup>
#include <KLocale>
#include <KStandardDirs>

#include "config.h"
#include "knemodaemon.h"
#include "interface.h"
#include "backends/backendbase.h"
#include "backends/daemonregistry.h"

QString KNemoDaemon::sSelectedInterface = QString::null;

KNemoDaemon::KNemoDaemon()
    : QObject(),
      mConfig( KGlobal::config() ),
      mHaveInterfaces( false ),
      mColorVLines( 0x04FB1D ),
      mColorHLines( 0x04FB1D ),
      mColorIncoming( 0x1889FF ),
      mColorOutgoing( 0xFF7F08 ),
      mColorBackground( 0x313031 ),
      mBackend( 0 )
{
    readConfig();
    QDBusConnection::sessionBus().registerObject("/knemo", this, QDBusConnection::ExportScriptableSlots);

    bool foundBackend = false;
    int i;
    for ( i = 0; DaemonRegistry[i].name != QString::null; i++ )
    {
        if ( DaemonRegistry[i].name == mBackendName )
        {
            foundBackend = true;
            break;
        }
    }

    if ( !foundBackend )
    {
        i = 0; // use the first backend (Sys)
    }
    mBackend = ( *DaemonRegistry[i].function )( mInterfaceHash );

    mPollTimer = new QTimer();
    connect( mPollTimer, SIGNAL( timeout() ), this, SLOT( updateInterfaces() ) );
    mPollTimer->start( mGeneralData.pollInterval * 1000 );
}

KNemoDaemon::~KNemoDaemon()
{
    mPollTimer->stop();
    delete mPollTimer;
    if ( mBackend )
        delete mBackend;

    foreach ( QString key, mInterfaceHash.keys() )
    {
        Interface *interface = mInterfaceHash.take( key );
        delete interface;
    }
}

void KNemoDaemon::readConfig()
{
    KConfig *config = mConfig.data();

    // For when reparseConfiguration() is called
    config->reparseConfiguration();

    // General
    KConfigGroup generalGroup( config, "General" );
    mGeneralData.pollInterval = clamp<int>(generalGroup.readEntry( "PollInterval", 1 ), 1, 60 );
    mGeneralData.saveInterval = clamp<int>(generalGroup.readEntry( "SaveInterval", 60 ), 1, 300 );
    mGeneralData.statisticsDir = generalGroup.readEntry( "StatisticsDir", KGlobal::dirs()->saveLocation( "data", "knemo/" ) );
    mGeneralData.toolTipContent = generalGroup.readEntry( "ToolTipContent", 2 );
    // If we already have an Interfaces key--even if its empty--then we
    // shouldn't try to set up a default interface
    if ( generalGroup.hasKey( "Interfaces" ) )
        mHaveInterfaces = true;
    QStringList interfaceList = generalGroup.readEntry( "Interfaces", QStringList() );
    QString backend = generalGroup.readEntry( "Backend", "Sys" );

    if ( mBackendName != backend )
    {
        bool foundBackend = false;
        mBackendName = backend;
        int i;
        for ( i = 0; DaemonRegistry[i].name != QString::null; i++ )
        {
            if ( DaemonRegistry[i].name == backend )
            {
                foundBackend = true;
                break;
            }
        }

        if ( foundBackend )
        {
            if ( mBackend )
                delete mBackend;
            mBackend = ( *DaemonRegistry[i].function )( mInterfaceHash );
        }
    }

    // Plotter
    KConfigGroup plotterGroup( config, "PlotterSettings" );
    mPlotterSettings.pixel = clamp<int>(plotterGroup.readEntry( "Pixel", 1 ), 1, 50 );
    mPlotterSettings.distance = clamp<int>(plotterGroup.readEntry( "Distance", 30 ), 10, 120 );
    mPlotterSettings.fontSize = clamp<int>(plotterGroup.readEntry( "FontSize", 8 ), 5, 24 );
    mPlotterSettings.minimumValue = clamp<int>(plotterGroup.readEntry( "MinimumValue", 0 ), 0, 49999 );
    mPlotterSettings.maximumValue = clamp<int>(plotterGroup.readEntry( "MaximumValue", 1 ), 1, 50000 );
    mPlotterSettings.labels = plotterGroup.readEntry( "Labels", true );
    mPlotterSettings.bottomBar = plotterGroup.readEntry( "BottomBar", false );
    mPlotterSettings.showIncoming = plotterGroup.readEntry( "ShowIncoming", true );
    mPlotterSettings.showOutgoing = plotterGroup.readEntry( "ShowOutgoing", true );
    mPlotterSettings.verticalLines = plotterGroup.readEntry( "VerticalLines", true );
    mPlotterSettings.horizontalLines = plotterGroup.readEntry( "HorizontalLines", true );
    mPlotterSettings.automaticDetection = plotterGroup.readEntry( "AutomaticDetection", true );
    mPlotterSettings.verticalLinesScroll = plotterGroup.readEntry( "VerticalLinesScroll", true );
    mPlotterSettings.colorVLines = plotterGroup.readEntry( "ColorVLines", mColorVLines );
    mPlotterSettings.colorHLines = plotterGroup.readEntry( "ColorHLines", mColorHLines );
    mPlotterSettings.colorIncoming = plotterGroup.readEntry( "ColorIncoming", mColorIncoming );
    mPlotterSettings.colorOutgoing = plotterGroup.readEntry( "ColorOutgoing", mColorOutgoing );
    mPlotterSettings.colorBackground = plotterGroup.readEntry( "ColorBackground", mColorBackground );

    // Interfaces
    QHash< QString, InterfaceSettings *> settingsHash;
    foreach ( QString interface, interfaceList )
    {
        QString group( "Interface_" );
        group += interface;
        InterfaceSettings* settings = new InterfaceSettings();
        if ( config->hasGroup( group ) )
        {
            KConfigGroup interfaceGroup( config, group );
            settings->alias = interfaceGroup.readEntry( "Alias" );
            settings->iconSet = interfaceGroup.readEntry( "IconSet", "monitor" );
            settings->customCommands = interfaceGroup.readEntry( "CustomCommands", false );
            settings->hideWhenNotAvailable = interfaceGroup.readEntry( "HideWhenNotAvailable",false );
            settings->hideWhenNotExisting = interfaceGroup.readEntry( "HideWhenNotExisting", false );
            settings->activateStatistics = interfaceGroup.readEntry( "ActivateStatistics", false );
            settings->trafficThreshold = clamp<int>(interfaceGroup.readEntry( "TrafficThreshold", 0 ), 0, 1000 );
            if ( settings->customCommands )
            {
                int numCommands = interfaceGroup.readEntry( "NumCommands", 0 );
                for ( int i = 0; i < numCommands; i++ )
                {
                    QString entry;
                    InterfaceCommand cmd;
                    entry = QString( "RunAsRoot%1" ).arg( i + 1 );
                    cmd.runAsRoot = interfaceGroup.readEntry( entry, false );
                    entry = QString( "Command%1" ).arg( i + 1 );
                    cmd.command = interfaceGroup.readEntry( entry );
                    entry = QString( "MenuText%1" ).arg( i + 1 );
                    cmd.menuText = interfaceGroup.readEntry( entry );
                    settings->commands.append( cmd );
                }
            }
        }
        settingsHash.insert( interface, settings );
    }

    // Remove interfaces that are no longer monitored
    foreach ( QString key, mInterfaceHash.keys() )
    {
        if ( !settingsHash.contains( key ) )
        {
            Interface *interface = mInterfaceHash.take( key );
            delete interface;
        }
    }

    // Add/update those that do need to be monitored
    foreach ( QString key, settingsHash.keys() )
    {
        Interface* iface;
        if ( !mInterfaceHash.contains( key ) )
        {
            iface = new Interface( key, mGeneralData, mPlotterSettings );
            mInterfaceHash.insert( key, iface );
        }
        else
            iface = mInterfaceHash.value( key );

        InterfaceSettings& settings = iface->getSettings();
        settings.alias = settingsHash.value( key )->alias;
        settings.iconSet = settingsHash.value( key )->iconSet;
        settings.customCommands = settingsHash.value( key )->customCommands;
        settings.hideWhenNotAvailable = settingsHash.value( key )->hideWhenNotAvailable;
        settings.hideWhenNotExisting = settingsHash.value( key )->hideWhenNotExisting;
        settings.activateStatistics = settingsHash.value( key )->activateStatistics;
        settings.trafficThreshold = settingsHash.value( key )->trafficThreshold;
        settings.commands = settingsHash.value( key )->commands;
        iface->configChanged();  // important to activate the statistics
    }
}

void KNemoDaemon::reparseConfiguration()
{
    mPollTimer->stop();
    readConfig();
    // restart the timer with the new interval
    mPollTimer->start( mGeneralData.pollInterval * 1000 );
}

QString KNemoDaemon::getSelectedInterface()
{
    // Reset the variable to avoid preselecting an interface when
    // the user opens the control center module from the control
    // center afterwards.
    QString tmp = sSelectedInterface;
    sSelectedInterface = QString::null;

    return tmp;
}

void KNemoDaemon::updateInterfaces()
{
    if ( !mBackend )
        return;

    if ( !mHaveInterfaces )
    {
        // If there's an interface for the default route, let's make that the
        // one to watch
        QString ifaceName = mBackend->getDefaultRouteIface();
        if ( !ifaceName.isEmpty() )
        {
            Interface *iface = new Interface( ifaceName, mGeneralData, mPlotterSettings );
            mInterfaceHash.insert( ifaceName, iface );

            InterfaceSettings& settings = iface->getSettings();
            settings.iconSet = "monitor";
            settings.alias = ifaceName;
            mHaveInterfaces = true;
            iface->configChanged();
        }
    }

    mBackend->update();
}

static const char * const description =
    I18N_NOOP( "The KDE Network Monitor" );

void KNemoDaemon::createAboutData()
{
    mAboutData = new KAboutData( "knemo", 0, ki18n( "KNemo" ), KNEMO_VERSION,
                      ki18n( description ),
                      KAboutData::License_GPL_V2,
                      ki18n( "" ),
                      ki18n( "Copyright (C) 2004, 2005, 2006 Percy Leonhardt\nCopyright (C) 2009 John Stamp\n\nSignal plotter taken from KSysGuard\nCopyright (C) 1999 - 2002, Chris Schlaeger\nCopyright (C) 2006 John Tapsell" ),
                      "http://extragear.kde.org/apps/knemo/",
                      "jstamp@users.sourceforge.net");

    mAboutData->addAuthor( ki18n( "Percy Leonhardt" ), ki18n( "Original Author" ),
                    "percy@eris23.de" );
    mAboutData->addAuthor( ki18n( "John Stamp" ), ki18n( "Current maintainer" ),
                    "jstamp@users.sourceforge.net" );
    mAboutData->addCredit( ki18n( "Michael Olbrich" ), ki18n( "Threshold support" ),
                    "michael.olbrich@gmx.net" );
    mAboutData->addCredit( ki18n( "Chris Schlaeger" ), ki18n( "Signal plotter" ),
                    "cs@kde.org" );
    mAboutData->addCredit( ki18n( "John Tapsell" ), ki18n( "Signal plotter" ),
                    "tapsell@kde.org" );
}

void KNemoDaemon::destroyAboutData()
{
    delete mAboutData;
    mAboutData = NULL;
}

KAboutData* KNemoDaemon::mAboutData;

KAboutData* KNemoDaemon::aboutData()
{
    return mAboutData;
}

#include "knemodaemon.moc"
