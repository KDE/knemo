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

#include <QDebug>

#include <KAboutData>
#include <KConfigGroup>
#include <KLocale>
#include <KStandardDirs>

#include "config-knemo.h"
#include "knemodaemon.h"
#include "interface.h"
#include "backends/backendfactory.h"
#include "utils.h"

QString KNemoDaemon::sSelectedInterface = QString::null;

BackendBase *backend = NULL;

KNemoDaemon::KNemoDaemon()
    : QObject(),
      mConfig( KGlobal::config() ),
      mHaveInterfaces( false ),
      mColorVLines( 0x04FB1D ),
      mColorHLines( 0x04FB1D ),
      mColorIncoming( 0x1889FF ),
      mColorOutgoing( 0xFF7F08 ),
      mColorBackground( 0x313031 )
{
    backend = BackendFactory::backend();
    readConfig();
    QDBusConnection::sessionBus().registerObject("/knemo", this, QDBusConnection::ExportScriptableSlots);
    mPollTimer = new QTimer();
    connect( mPollTimer, SIGNAL( timeout() ), this, SLOT( updateInterfaces() ) );
    mPollTimer->start( mGeneralData.pollInterval * 1000 );
}

KNemoDaemon::~KNemoDaemon()
{
    mPollTimer->stop();
    delete mPollTimer;

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
    mGeneralData.saveInterval = clamp<int>(generalGroup.readEntry( "SaveInterval", 60 ), 0, 300 );
    mGeneralData.statisticsDir = generalGroup.readEntry( "StatisticsDir", KGlobal::dirs()->saveLocation( "data", "knemo/" ) );
    mGeneralData.toolTipContent = generalGroup.readEntry( "ToolTipContent", defaultTip );
    // If we already have an Interfaces key--even if its empty--then we
    // shouldn't try to set up a default interface
    if ( generalGroup.hasKey( "Interfaces" ) )
        mHaveInterfaces = true;
    QStringList interfaceList = generalGroup.readEntry( "Interfaces", QStringList() );

    // Remove interfaces that are no longer monitored
    foreach ( QString key, mInterfaceHash.keys() )
    {
        if ( !interfaceList.contains( key ) )
        {
            Interface *interface = mInterfaceHash.take( key );
            delete interface;
            backend->remove( key );
        }
    }

    // Add/update those that do need to be monitored
    foreach ( QString key, interfaceList )
    {
        Interface* iface;
        if ( !mInterfaceHash.contains( key ) )
        {
            const BackendData * data = backend->add( key );
            iface = new Interface( key, data, mGeneralData );
            mInterfaceHash.insert( key, iface );
            connect( backend, SIGNAL( updateComplete() ), iface, SLOT( activateMonitor() ) );
        }
        else
            iface = mInterfaceHash.value( key );
        iface->configChanged();
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
    if ( !backend )
        return;

    backend->update();
    if ( !mHaveInterfaces )
    {
        // If there's an interface for the default route, let's make that the
        // one to watch
        QString ifaceName = backend->getDefaultRouteIface( AF_INET );
        if ( ifaceName.isEmpty() )
            ifaceName = backend->getDefaultRouteIface( AF_INET6 );
        if ( !ifaceName.isEmpty() )
        {
            const BackendData* data = backend->add( ifaceName );
            Interface *iface = new Interface( ifaceName, data, mGeneralData );
            mInterfaceHash.insert( ifaceName, iface );
            connect( backend, SIGNAL( updateComplete() ), iface, SLOT( activateMonitor() ) );
            mHaveInterfaces = true;
            iface->configChanged();
        }
    }
}

static const char * const description =
    I18N_NOOP( "The KDE Network Monitor" );

void KNemoDaemon::createAboutData()
{
    mAboutData = new KAboutData( "knemo", 0, ki18n( "KNemo" ), KNEMO_VERSION,
                      ki18n( description ),
                      KAboutData::License_GPL_V2,
                      KLocalizedString(),
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
