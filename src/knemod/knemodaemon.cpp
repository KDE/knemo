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

#include <QSqlDatabase>
#include <QtDBus/QDBusConnection>
#include <QTimer>

#include <KAboutData>
#include <KConfigGroup>
#include <KLocale>
#include <KMessageBox>
#include <KStandardDirs>

#include "config-knemo.h"
#include "global.h"
#include "knemodaemon.h"
#include "interface.h"
#include "backends/backendfactory.h"
#include "utils.h"

QString KNemoDaemon::sSelectedInterface = QString::null;

BackendBase *backend = NULL;
GeneralSettings *generalSettings = NULL;

KNemoDaemon::KNemoDaemon()
    : QObject(),
      mConfig( KGlobal::config() ),
      mHaveInterfaces( false )
{
    generalSettings = new GeneralSettings();
    backend = BackendFactory::backend();
    QDBusConnection::sessionBus().registerObject("/knemo", this, QDBusConnection::ExportScriptableSlots);
    mPollTimer = new QTimer();
    connect( mPollTimer, SIGNAL( timeout() ), this, SLOT( updateInterfaces() ) );
    readConfig();
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
    delete generalSettings;
}

void KNemoDaemon::readConfig()
{
    mPollTimer->stop();
    KConfig *config = mConfig.data();

    // For when reparseConfiguration() is called
    config->reparseConfiguration();

    // General
    GeneralSettings g;
    KConfigGroup generalGroup( config, confg_general );
    generalSettings->pollInterval = clamp<double>(generalGroup.readEntry( conf_pollInterval, g.pollInterval ), 0.1, 2.0 );
    generalSettings->pollInterval = validatePoll( generalSettings->pollInterval );
    generalSettings->useBitrate = generalGroup.readEntry( conf_useBitrate, g.useBitrate );
    generalSettings->saveInterval = clamp<int>(generalGroup.readEntry( conf_saveInterval, g.saveInterval ), 0, 300 );
    generalSettings->statisticsDir = generalGroup.readEntry( conf_statisticsDir, g.statisticsDir );
    generalSettings->toolTipContent = generalGroup.readEntry( conf_toolTipContent, g.toolTipContent );
    // If we already have an Interfaces key--even if its empty--then we
    // shouldn't try to set up a default interface
    if ( generalGroup.hasKey( conf_interfaces ) )
        mHaveInterfaces = true;
    QStringList interfaceList = generalGroup.readEntry( conf_interfaces, QStringList() );

    // Remove interfaces that are no longer monitored
    foreach ( QString key, mInterfaceHash.keys() )
    {
        if ( !interfaceList.contains( key ) )
        {
            Interface *interface = mInterfaceHash.take( key );
            delete interface;
            backend->remove( key );

            // If knemo is running while config removes an interface to monitor,
            // it will keep the interface and plotter groups. Delete them here.
            KConfigGroup interfaceGroup( config, QString( confg_interface + key ) );
            KConfigGroup plotterGroup( config, QString( confg_plotter + key ) );
            interfaceGroup.deleteGroup();
            plotterGroup.deleteGroup();
            config->sync();
        }
    }

    if ( !mHaveInterfaces )
    {
        QString ifaceName = backend->getDefaultRouteIface( AF_INET );
        if ( ifaceName.isEmpty() )
            ifaceName = backend->getDefaultRouteIface( AF_INET6 );
        if ( !ifaceName.isEmpty() )
        {
            interfaceList << ifaceName;
            mHaveInterfaces = true;
        }
    }

    // Add/update those that do need to be monitored
    QStringList newIfaces;
    foreach ( QString key, interfaceList )
    {
        if ( !mInterfaceHash.contains( key ) )
        {
            const BackendData * data = backend->add( key );
            Interface *iface = new Interface( key, data );
            mInterfaceHash.insert( key, iface );
            newIfaces << key;
        }
    }

    // Now (re)config interfaces, but new interfaces need extra work so
    // they don't show bogus icon traffic states on startup.
    updateInterfaces();
    foreach( QString key, interfaceList )
    {
        Interface *iface = mInterfaceHash.value( key );
        iface->configChanged();

        if ( newIfaces.contains( key ) )
        {
            backend->updatePackets( key );
            iface->processUpdate();
            connect( backend, SIGNAL( updateComplete() ), iface, SLOT( processUpdate() ) );
        }
    }

    bool statsActivated = false;
    foreach ( Interface *iface, mInterfaceHash )
    {
        if ( iface->getSettings().activateStatistics )
            statsActivated = true;
    }
    if ( statsActivated )
    {
        QStringList drivers = QSqlDatabase::drivers();
        if ( !drivers.contains( "QSQLITE" ) )
        {
            KMessageBox::sorry( 0, i18n( "The Qt4 SQLite database plugin is not available.\n"
                                         "Please install it to store traffic statistics." ) );
        }
    }

    mPollTimer->start( generalSettings->pollInterval * 1000 );
}

void KNemoDaemon::reparseConfiguration()
{
    readConfig();
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
    backend->update();
}

static const char * const description =
    I18N_NOOP( "The KDE Network Monitor" );

void KNemoDaemon::createAboutData()
{
    mAboutData = new KAboutData( "knemo", 0, ki18n( "KNemo" ), KNEMO_VERSION,
                      ki18n( description ),
                      KAboutData::License_GPL_V2,
                      KLocalizedString(),
                      ki18n( "Copyright (C) 2004, 2005, 2006 Percy Leonhardt\nCopyright (C) 2009 John Stamp\n\nSignal plotter taken from KSysGuard\nCopyright (C) 2006 - 2009 John Tapsell" ),
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
