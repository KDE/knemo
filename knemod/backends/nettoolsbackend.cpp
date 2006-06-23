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

#include <qmap.h>
#include <qregexp.h>
#include <qstringlist.h>

#include <kdebug.h>
#include <kprocess.h>
#include <kio/global.h>

#include "nettoolsbackend.h"

#include "config.h"

NetToolsBackend::NetToolsBackend( QDict<Interface>& interfaceDict )
    : QObject(),
      mRouteProcess(0L),
      mIfconfigProcess(0L),
      mIwconfigProcess(0L),
      mInterfaceDict( interfaceDict )
{
}

NetToolsBackend::~NetToolsBackend()
{
    if ( mRouteProcess )
    {
        mRouteProcess->kill();
        delete mRouteProcess;
    }
    if ( mIfconfigProcess )
    {
        mIfconfigProcess->kill();
        delete mIfconfigProcess;
    }
    if ( mIwconfigProcess )
    {
        mIwconfigProcess->kill();
        delete mIwconfigProcess;
    }
}

void NetToolsBackend::checkConfig()
{
    if ( !mIfconfigProcess )
    {
        mIfconfigStdout = QString::null;
        mIfconfigProcess = new KProcess();
        mIfconfigProcess->setEnvironment( "LANG", "C" );
        mIfconfigProcess->setEnvironment( "LC_ALL", "C" );
        *mIfconfigProcess << PATH_IFCONFIG << "-a";
        connect( mIfconfigProcess,  SIGNAL( receivedStdout( KProcess*, char*, int ) ),
                 this, SLOT( ifconfigProcessStdout( KProcess*, char*, int ) ) );
        connect( mIfconfigProcess,  SIGNAL( processExited( KProcess* ) ),
                 this, SLOT( ifconfigProcessExited( KProcess* ) ) );

        if ( !mIfconfigProcess->start( KProcess::NotifyOnExit, KProcess::Stdout ) )
        {
            delete mIfconfigProcess;
            mIfconfigProcess = 0L;
        }
    }

#ifdef PATH_IWCONFIG
    if ( !mIwconfigProcess )
    {
        mIwconfigStdout = QString::null;
        mIwconfigProcess = new KProcess();
        mIwconfigProcess->setEnvironment( "LANG", "C" );
        mIwconfigProcess->setEnvironment( "LC_ALL", "C" );
        *mIwconfigProcess << PATH_IWCONFIG;
        connect( mIwconfigProcess,  SIGNAL( receivedStdout( KProcess*, char*, int ) ),
                 this, SLOT( iwconfigProcessStdout( KProcess*, char*, int ) ) );
        connect( mIwconfigProcess,  SIGNAL( receivedStderr( KProcess*, char*, int ) ),
                 this, SLOT( iwconfigProcessStdout( KProcess*, char*, int ) ) );
        connect( mIwconfigProcess,  SIGNAL( processExited( KProcess* ) ),
                 this, SLOT( iwconfigProcessExited( KProcess* ) ) );

        if ( !mIwconfigProcess->start( KProcess::NotifyOnExit, KProcess::AllOutput ) )
        {
            delete mIwconfigProcess;
            mIwconfigProcess = 0L;
        }
    }
#endif

#ifdef PATH_ROUTE
    if ( !mRouteProcess )
    {
        mRouteStdout = QString::null;
        mRouteProcess = new KProcess();
        mRouteProcess->setEnvironment( "LANG", "C" );
        mRouteProcess->setEnvironment( "LC_ALL", "C" );
        *mRouteProcess << PATH_ROUTE << "-n";
        connect( mRouteProcess,  SIGNAL( receivedStdout( KProcess*, char*, int ) ),
                 this, SLOT( routeProcessStdout( KProcess*, char*, int ) ) );
        connect( mRouteProcess,  SIGNAL( receivedStderr( KProcess*, char*, int ) ),
                 this, SLOT( routeProcessStdout( KProcess*, char*, int ) ) );
        connect( mRouteProcess,  SIGNAL( processExited( KProcess* ) ),
                 this, SLOT( routeProcessExited( KProcess* ) ) );

        if ( !mRouteProcess->start( KProcess::NotifyOnExit, KProcess::AllOutput ) )
        {
            delete mRouteProcess;
            mRouteProcess = 0L;
        }
    }
#endif
}

void NetToolsBackend::routeProcessExited( KProcess* process )
{
    if ( process == mRouteProcess )
    {
        mRouteProcess->deleteLater(); // we're in a slot connected to mRouteProcess
        mRouteProcess = 0L;
        parseRouteOutput();
    }
}

void NetToolsBackend::routeProcessStdout( KProcess*, char* buffer, int buflen )
{
    mRouteStdout += QString::fromLatin1( buffer, buflen );
}

void NetToolsBackend::ifconfigProcessExited( KProcess* process )
{
    if ( process == mIfconfigProcess )
    {
        delete mIfconfigProcess;
        mIfconfigProcess = 0L;
        parseIfconfigOutput();
    }
}

void NetToolsBackend::ifconfigProcessStdout( KProcess*, char* buffer, int buflen )
{
    mIfconfigStdout += QString::fromLatin1( buffer, buflen );
}

void NetToolsBackend::iwconfigProcessExited( KProcess* process )
{
    if ( process == mIwconfigProcess )
    {
        delete mIwconfigProcess;
        mIwconfigProcess = 0L;
        parseIwconfigOutput();
    }
}

void NetToolsBackend::iwconfigProcessStdout( KProcess*, char* buffer, int buflen )
{
    mIwconfigStdout += QString::fromLatin1( buffer, buflen );
}

void NetToolsBackend::parseIfconfigOutput()
{
    /* mIfconfigStdout contains the complete output of 'ifconfig' which we
     * are going to parse here.
     */
    QMap<QString, QString> configs;
    QStringList ifList = QStringList::split( "\n\n", mIfconfigStdout );
    QStringList::Iterator it;
    for ( it = ifList.begin(); it != ifList.end(); ++it )
    {
        int index = ( *it ).find( ' ' );
        if ( index == -1 )
            continue;
        QString key = ( *it ).left( index );
        configs[key] = ( *it ).mid( index );
    }

    /* We loop over the interfaces the user wishs to monitor.
     * If we find the interface in the output of 'ifconfig'
     * we update its data, otherwise we mark it as
     * 'not existing'.
     */
    QDictIterator<Interface> ifIt( mInterfaceDict );
    for ( ; ifIt.current(); ++ifIt )
    {
        QString key = ifIt.currentKey();
        Interface* interface = ifIt.current();

        if ( configs.find( key ) == configs.end() )
        {
            // The interface does not exist. Meaning the driver
            // isn't loaded and/or the interface has not been created.
            interface->getData().existing = false;
            interface->getData().available = false;
        }
        // JJ 2005-07-18: use RUNNING instead of UP to detect whether interface is connected
        else if ( !configs[key].contains( "inet " ) ||
                  !configs[key].contains( "RUNNING" ) )
        {
            // The interface is up or has an IP assigned but not both
            interface->getData().existing = true;
            interface->getData().available = false;
        }
        else
        {
            // ...determine the type of the interface
            if ( configs[key].contains( "Ethernet" ) )
                interface->setType( Interface::ETHERNET );
            else
                interface->setType( Interface::PPP );

            // Update the interface.
            interface->getData().existing = true;
            interface->getData().available = true;
            updateInterfaceData( configs[key], interface->getData(), interface->getType() );
        }
        interface->activateMonitor();
    }
}

void NetToolsBackend::updateInterfaceData( QString& config, InterfaceData& data, int type )
{
    QRegExp regExp( ".*RX.*:(\\d+).*:\\d+.*:\\d+.*:\\d+" );
    if ( regExp.search( config ) > -1 )
        data.rxPackets = regExp.cap( 1 ).toULong();

    regExp.setPattern( ".*TX.*:(\\d+).*:\\d+.*:\\d+.*:\\d+" );
    if ( regExp.search( config ) > -1 )
        data.txPackets = regExp.cap( 1 ).toULong();

    regExp.setPattern( "RX bytes:(\\d+)\\s*\\(\\d+\\.\\d+\\s*\\w+\\)" );
    if ( regExp.search( config ) > -1 )
    {
        // We count the traffic on ourself to avoid an overflow after
        // 4GB of traffic.
        unsigned long currentRxBytes = regExp.cap( 1 ).toULong();
        if ( currentRxBytes < data.prevRxBytes )
        {
            // there was an overflow
            data.rxBytes += 0x7FFFFFFF - data.prevRxBytes;
            data.prevRxBytes = 0L;
        }
        if ( data.rxBytes == 0L )
        {
            // on startup set to currently received bytes
            data.rxBytes = currentRxBytes;
            // this is new: KNemo only counts the traffic transfered
            // while it is running. Important to not falsify statistics!
            data.prevRxBytes = currentRxBytes;
        }
        else
            // afterwards only add difference to previous number of bytes
            data.rxBytes += currentRxBytes - data.prevRxBytes;

        data.incomingBytes = currentRxBytes - data.prevRxBytes;
        data.prevRxBytes = currentRxBytes;
        data.rxString = KIO::convertSize( data.rxBytes );
    }

    regExp.setPattern( "TX bytes:(\\d+)\\s*\\(\\d+\\.\\d+\\s*\\w+\\)" );
    if ( regExp.search( config ) > -1 )
    {
        // We count the traffic on ourself to avoid an overflow after
        // 4GB of traffic.
        unsigned long currentTxBytes = regExp.cap( 1 ).toULong();
        if ( currentTxBytes < data.prevTxBytes )
        {
            // there was an overflow
            data.txBytes += 0x7FFFFFFF - data.prevTxBytes;
            data.prevTxBytes = 0L;
        }
        if ( data.txBytes == 0L )
        {
            // on startup set to currently transmitted bytes
            data.txBytes = currentTxBytes;
            // this is new: KNemo only counts the traffic transfered
            // while it is running. Important to not falsify statistics!
            data.prevTxBytes = currentTxBytes;
        }
        else
            // afterwards only add difference to previous number of bytes
            data.txBytes += currentTxBytes - data.prevTxBytes;

        data.outgoingBytes = currentTxBytes - data.prevTxBytes;
        data.prevTxBytes = currentTxBytes;
        data.txString = KIO::convertSize( data.txBytes );
    }

    regExp.setPattern( "inet\\s+\\w+:(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})" );
    if ( regExp.search( config ) > -1 )
        data.ipAddress = regExp.cap( 1 );

    regExp.setPattern( "(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}).*(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}).*(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})" );
    if ( regExp.search( config ) > -1 )
    {
        data.broadcastAddress = regExp.cap( 2 );
        data.subnetMask = regExp.cap( 3 );
    }

    if ( type == Interface::ETHERNET )
    {
        regExp.setPattern( "(.{2}:.{2}:.{2}:.{2}:.{2}:.{2})" );
        if ( regExp.search( config ) > -1 )
            data.hwAddress = regExp.cap( 1 );
    }
    else if (  type == Interface::PPP )
    {
        regExp.setPattern( "(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}).*(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}).*(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})" );
        if ( regExp.search( config ) > -1 )
            data.ptpAddress = regExp.cap( 2 );
    }
}

void NetToolsBackend::parseIwconfigOutput()
{
    /* mIwconfigStdout contains the complete output of 'iwconfig' which we
     * are going to parse here.
     */
    QMap<QString, QString> configs;
    QStringList ifList = QStringList::split( "\n\n", mIwconfigStdout );
    QStringList::Iterator it;
    for ( it = ifList.begin(); it != ifList.end(); ++it )
    {
        int index = ( *it ).find( ' ' );
        if ( index == -1 )
            continue;
        QString key = ( *it ).left( index );
        configs[key] = ( *it ).mid( index );
    }

    /* We loop over the interfaces the user wishs to monitor.
     * If we find the interface in the output of 'iwconfig'
     * we update its data.
     */
    QDictIterator<Interface> ifIt( mInterfaceDict );
    for ( ; ifIt.current(); ++ifIt )
    {
        QString key = ifIt.currentKey();
        Interface* interface = ifIt.current();

        if ( configs.find( key ) == configs.end() )
        {
            // The interface was not found.
            continue;
        }
        else if ( configs[key].contains( "no wireless extensions" ) )
        {
            // The interface isn't a wireless device.
            interface->getData().wirelessDevice = false;
        }
        else
        {
            // Update the wireless data of the interface.
            interface->getData().wirelessDevice = true;
            updateWirelessData( configs[key], interface->getWirelessData() );
        }
    }
}

void NetToolsBackend::updateWirelessData( QString& config, WirelessData& data )
{
    QRegExp regExp( "ESSID:\"?([^\"]*)\"?" );
    if ( regExp.search( config ) > -1 )
        data.essid = regExp.cap( 1 );

    regExp.setPattern( "Mode:(\\w*)" );
    if ( regExp.search( config ) > -1 )
        data.mode = regExp.cap( 1 );

    regExp.setPattern( "Frequency:([\\w|\\.]*)" );
    if ( regExp.search( config ) > -1 )
        data.frequency = regExp.cap( 1 );
    else
    {
        regExp.setPattern( "Channel:(\\d*)" );
        if ( regExp.search( config ) > -1 )
            data.channel = regExp.cap( 1 );
    }

    regExp.setPattern( "Bit Rate[=:]([\\w/]*)" );
    if ( regExp.search( config ) > -1 )
        data.bitRate = regExp.cap( 1 );

    regExp.setPattern( "Signal level.(-?\\d+\\s*\\w+)" );
    if ( regExp.search( config ) > -1 )
        data.signal = regExp.cap( 1 );

    regExp.setPattern( "Noise level.(-?\\d+\\s*\\w+)" );
    if ( regExp.search( config ) > -1 )
        data.noise = regExp.cap( 1 );

    regExp.setPattern( "Link Quality[=:]([\\d/]*)" );
    if ( regExp.search( config ) > -1 )
        data.linkQuality = regExp.cap( 1 );
}

void NetToolsBackend::parseRouteOutput()
{
    /* mRouteStdout contains the complete output of 'route' which we
     * are going to parse here.
     */
    QMap<QString, QStringList> configs;
    QStringList routeList = QStringList::split( "\n", mRouteStdout );
    QStringList::Iterator it;
    for ( it = routeList.begin(); it != routeList.end(); ++it )
    {
        QStringList routeParameter = QStringList::split( " ", *it );
        if ( routeParameter.count() < 8 ) // no routing entry
            continue;
        if ( routeParameter[0] != "0.0.0.0" ) // no default route
            continue;
        configs[routeParameter[7]] = routeParameter;
    }

    /* We loop over the interfaces the user wishs to monitor.
     * If we find the interface in the output of 'route' we update
     * the data of the interface.
     */
    QDictIterator<Interface> ifIt( mInterfaceDict );
    for ( ; ifIt.current(); ++ifIt )
    {
        QString key = ifIt.currentKey();
        Interface* interface = ifIt.current();

        if ( configs.find( key ) != configs.end() )
        {
            // Update the default gateway.
            QStringList routeParameter = configs[key];
            interface->getData().defaultGateway = routeParameter[1];
        }
        else
        {
            // Reset the default gateway.
            interface->getData().defaultGateway = QString::null;
        }
    }
}
