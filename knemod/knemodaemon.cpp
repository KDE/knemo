/* This file is part of KNemo
   Copyright (C) 2004, 2006 Percy Leonhardt <percy@eris23.de>

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

#include <qtimer.h>
#include <qstrlist.h>

#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kconfig.h>
#include <kprocess.h>
#include <kinstance.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>

#include "knemodaemon.h"
#include "interface.h"
#include "backendbase.h"
#include "daemonregistry.h"

QString KNemoDaemon::sSelectedInterface = QString::null;

extern "C" {
   KDE_EXPORT KDEDModule *create_knemod(const QCString &name) {
       return new KNemoDaemon(name);
   }
}

KNemoDaemon::KNemoDaemon( const QCString& name )
    : KDEDModule( name ),
      mColorVLines( 0x04FB1D ),
      mColorHLines( 0x04FB1D ),
      mColorIncoming( 0x1889FF ),
      mColorOutgoing( 0xFF7F08 ),
      mColorBackground( 0x313031 ),
      mInstance( new KInstance( "knemo" ) ),
      mNotifyInstance( new KNotifyClient::Instance( mInstance ) )
{
    KGlobal::locale()->insertCatalogue( "knemod" );
    readConfig();

    // select the backend from the config file
    KConfig* config = new KConfig( "knemorc", true );
    config->setGroup( "General" );
    mBackendName = config->readEntry( "Backend", "Nettools" );
    delete config;

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
        i = 0; // use the first backend (Nettools)
    }
    mBackend = ( *DaemonRegistry[i].function )( mInterfaceDict );

    mInterfaceDict.setAutoDelete( true );

    mPollTimer = new QTimer();
    connect( mPollTimer, SIGNAL( timeout() ), this, SLOT( updateInterfaces() ) );
    mPollTimer->start( mGeneralData.pollInterval * 1000 );
}

KNemoDaemon::~KNemoDaemon()
{
    mPollTimer->stop();
    delete mPollTimer;
    delete mBackend;
    delete mNotifyInstance;
    delete mInstance;

    QDictIterator<Interface> it( mInterfaceDict );
    for ( ; it.current(); )
    {
        mInterfaceDict.remove( it.currentKey() );
        // 'remove' already advanced the iterator to the next item
    }
}

void KNemoDaemon::readConfig()
{
    KConfig* config = new KConfig( "knemorc", true );

    config->setGroup( "General" );
    mGeneralData.pollInterval = config->readNumEntry( "PollInterval", 1 );
    mGeneralData.saveInterval = config->readNumEntry( "SaveInterval", 60 );
    mGeneralData.statisticsDir = config->readEntry( "StatisticsDir", KGlobal::dirs()->saveLocation( "data", "knemo/" ) );
    mGeneralData.toolTipContent = config->readNumEntry( "ToolTipContent", 2 );
    QStrList list;
    int numEntries = config->readListEntry( "Interfaces", list );

    if ( numEntries == 0 )
        return;

    char* interface;
    for ( interface = list.first(); interface; interface = list.next() )
    {
        Interface* iface = new Interface( interface, mGeneralData, mPlotterSettings );
        QString group( "Interface_" );
        group += interface;
        if ( config->hasGroup( group ) )
        {
            config->setGroup( group );
            InterfaceSettings& settings = iface->getSettings();
            settings.alias = config->readEntry( "Alias" );
            settings.iconSet = config->readNumEntry( "IconSet", 0 );
            settings.customCommands = config->readBoolEntry( "CustomCommands" );
            settings.hideWhenNotAvailable = config->readBoolEntry( "HideWhenNotAvailable" );
            settings.hideWhenNotExisting = config->readBoolEntry( "HideWhenNotExisting" );
            settings.activateStatistics = config->readBoolEntry( "ActivateStatistics" );
            settings.trafficThreshold = config->readNumEntry( "TrafficThreshold", 0 );
            if ( settings.customCommands )
            {
                int numCommands = config->readNumEntry( "NumCommands" );
                for ( int i = 0; i < numCommands; i++ )
                {
                    QString entry;
                    InterfaceCommand cmd;
                    entry = QString( "RunAsRoot%1" ).arg( i + 1 );
                    cmd.runAsRoot = config->readBoolEntry( entry );
                    entry = QString( "Command%1" ).arg( i + 1 );
                    cmd.command = config->readEntry( entry );
                    entry = QString( "MenuText%1" ).arg( i + 1 );
                    cmd.menuText = config->readEntry( entry );
                    settings.commands.append( cmd );
                }
            }
            iface->configChanged(); // important to activate the statistics
        }
        mInterfaceDict.insert( interface, iface );
    }

    // load the settings for the plotter
    config->setGroup( "PlotterSettings" );
    mPlotterSettings.pixel = config->readNumEntry( "Pixel", 1 );
    mPlotterSettings.count = config->readNumEntry( "Count", 5 );
    mPlotterSettings.distance = config->readNumEntry( "Distance", 30 );
    mPlotterSettings.fontSize = config->readNumEntry( "FontSize", 8 );
    mPlotterSettings.minimumValue = config->readNumEntry( "MinimumValue", 0 );
    mPlotterSettings.maximumValue = config->readNumEntry( "MaximumValue", 1 );
    mPlotterSettings.labels = config->readBoolEntry( "Labels", true );
    mPlotterSettings.topBar = config->readBoolEntry( "TopBar", false );
    mPlotterSettings.showIncoming = config->readBoolEntry( "ShowIncoming", true );
    mPlotterSettings.showOutgoing = config->readBoolEntry( "ShowOutgoing", true );
    mPlotterSettings.verticalLines = config->readBoolEntry( "VerticalLines", true );
    mPlotterSettings.horizontalLines = config->readBoolEntry( "HorizontalLines", true );
    mPlotterSettings.automaticDetection = config->readBoolEntry( "AutomaticDetection", true );
    mPlotterSettings.verticalLinesScroll = config->readBoolEntry( "VerticalLinesScroll", true );
    mPlotterSettings.colorVLines = config->readColorEntry( "ColorVLines", &mColorVLines );
    mPlotterSettings.colorHLines = config->readColorEntry( "ColorHLines", &mColorHLines );
    mPlotterSettings.colorIncoming = config->readColorEntry( "ColorIncoming", &mColorIncoming );
    mPlotterSettings.colorOutgoing = config->readColorEntry( "ColorOutgoing", &mColorOutgoing );
    mPlotterSettings.colorBackground = config->readColorEntry( "ColorBackground", &mColorBackground );

    delete config;
}

void KNemoDaemon::reparseConfiguration()
{
    // Read the settings from the config file.
    QDict<InterfaceSettings> settingsDict;
    KConfig* config = new KConfig( "knemorc", false );

    config->setGroup( "General" );
    mGeneralData.pollInterval = config->readNumEntry( "PollInterval", 1 );
    mGeneralData.saveInterval = config->readNumEntry( "SaveInterval", 60 );
    mGeneralData.statisticsDir = config->readEntry( "StatisticsDir", KGlobal::dirs()->saveLocation( "data", "knemo/" ) );
    mGeneralData.toolTipContent = config->readNumEntry( "ToolTipContent", 2 );

    // restart the timer with the new interval
    mPollTimer->changeInterval( mGeneralData.pollInterval * 1000 );

    // select the backend from the config file
    QString backend = config->readEntry( "Backend", "Nettools" );

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
            delete mBackend;
            mBackend = ( *DaemonRegistry[i].function )( mInterfaceDict );
        }
    }

    QStrList list;
    int numEntries = config->readListEntry( "Interfaces", list );

    if ( numEntries == 0 )
        return;

    char* interface;
    for ( interface = list.first(); interface; interface = list.next() )
    {
        QString group( "Interface_" );
        group += interface;
        InterfaceSettings* settings = new InterfaceSettings();
        if ( config->hasGroup( group ) )
        {
            config->setGroup( group );
            settings->alias = config->readEntry( "Alias" );
            settings->iconSet = config->readNumEntry( "IconSet", 0 );
            settings->customCommands = config->readBoolEntry( "CustomCommands" );
            settings->hideWhenNotAvailable = config->readBoolEntry( "HideWhenNotAvailable" );
            settings->hideWhenNotExisting = config->readBoolEntry( "HideWhenNotExisting" );
            settings->activateStatistics = config->readBoolEntry( "ActivateStatistics" );
            settings->trafficThreshold = config->readNumEntry( "TrafficThreshold", 0 );
            if ( settings->customCommands )
            {
                int numCommands = config->readNumEntry( "NumCommands" );
                for ( int i = 0; i < numCommands; i++ )
                {
                    QString entry;
                    InterfaceCommand cmd;
                    entry = QString( "RunAsRoot%1" ).arg( i + 1 );
                    cmd.runAsRoot = config->readBoolEntry( entry );
                    entry = QString( "Command%1" ).arg( i + 1 );
                    cmd.command = config->readEntry( entry );
                    entry = QString( "MenuText%1" ).arg( i + 1 );
                    cmd.menuText = config->readEntry( entry );
                    settings->commands.append( cmd );
                }
            }
        }
        settingsDict.insert( interface, settings );
    }

    // load the settings for the plotter
    config->setGroup( "PlotterSettings" );
    mPlotterSettings.pixel = config->readNumEntry( "Pixel", 1 );
    mPlotterSettings.count = config->readNumEntry( "Count", 5 );
    mPlotterSettings.distance = config->readNumEntry( "Distance", 30 );
    mPlotterSettings.fontSize = config->readNumEntry( "FontSize", 8 );
    mPlotterSettings.minimumValue = config->readNumEntry( "MinimumValue", 0 );
    mPlotterSettings.maximumValue = config->readNumEntry( "MaximumValue", 1 );
    mPlotterSettings.labels = config->readBoolEntry( "Labels", true );
    mPlotterSettings.topBar = config->readBoolEntry( "TopBar", false );
    mPlotterSettings.showIncoming = config->readBoolEntry( "ShowIncoming", true );
    mPlotterSettings.showOutgoing = config->readBoolEntry( "ShowOutgoing", true );
    mPlotterSettings.verticalLines = config->readBoolEntry( "VerticalLines", true );
    mPlotterSettings.horizontalLines = config->readBoolEntry( "HorizontalLines", true );
    mPlotterSettings.automaticDetection = config->readBoolEntry( "AutomaticDetection", true );
    mPlotterSettings.verticalLinesScroll = config->readBoolEntry( "VerticalLinesScroll", true );
    mPlotterSettings.colorVLines = config->readColorEntry( "ColorVLines", &mColorVLines );
    mPlotterSettings.colorHLines = config->readColorEntry( "ColorHLines", &mColorHLines );
    mPlotterSettings.colorIncoming = config->readColorEntry( "ColorIncoming", &mColorIncoming );
    mPlotterSettings.colorOutgoing = config->readColorEntry( "ColorOutgoing", &mColorOutgoing );
    mPlotterSettings.colorBackground = config->readColorEntry( "ColorBackground", &mColorBackground );

    // Remove all interfaces that the user deleted from
    // the internal list.
    QDictIterator<Interface> it( mInterfaceDict );
    for ( ; it.current(); )
    {
        if ( settingsDict.find( it.currentKey() ) == 0 )
        {
            config->deleteGroup( "Interface_" + it.currentKey() );
            mInterfaceDict.remove( it.currentKey() );
            // 'remove' already advanced the iterator to the next item
        }
        else
            ++it;
    }
    config->sync();
    delete config;

    // Update all other interfaces and add new ones.
    QDictIterator<InterfaceSettings> setIt( settingsDict );
    for ( ; setIt.current(); ++setIt )
    {
        Interface* iface;
        if ( mInterfaceDict.find( setIt.currentKey() ) == 0 )
        {
            iface = new Interface( setIt.currentKey(), mGeneralData, mPlotterSettings );
            mInterfaceDict.insert( setIt.currentKey(), iface );
        }
        else
            iface = mInterfaceDict[setIt.currentKey()];

        InterfaceSettings& settings = iface->getSettings();
        settings.alias = setIt.current()->alias;
        settings.iconSet = setIt.current()->iconSet;
        settings.customCommands = setIt.current()->customCommands;
        settings.hideWhenNotAvailable = setIt.current()->hideWhenNotAvailable;
        settings.hideWhenNotExisting = setIt.current()->hideWhenNotExisting;
        settings.activateStatistics = setIt.current()->activateStatistics;
        settings.trafficThreshold = setIt.current()->trafficThreshold;
        settings.commands = setIt.current()->commands;
        iface->configChanged();
    }
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
    mBackend->update();
}

#include "knemodaemon.moc"
