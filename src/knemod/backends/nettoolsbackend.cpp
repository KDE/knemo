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

#include <KProcess>
#include <kio/global.h>

#include "config-knemo.h"
#include "nettoolsbackend.h"

NetToolsBackend::NetToolsBackend( QHash<QString, Interface *>& interfaces )
    : QObject(),
      BackendBase( interfaces ),
      mRouteProcess(0L),
      mIfconfigProcess(0L),
      mIwconfigProcess(0L)
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

BackendBase* NetToolsBackend::createInstance( QHash<QString, Interface *>& interfaces )
{
    return new NetToolsBackend( interfaces );
}

void NetToolsBackend::update()
{
    if ( !mIfconfigProcess )
    {
        mIfconfigStdout = QString::null;
        mIfconfigProcess = new KProcess();
        mIfconfigProcess->setEnv( "LANG", "C" );
        mIfconfigProcess->setEnv( "LC_ALL", "C" );
        mIfconfigProcess->setOutputChannelMode( KProcess::OnlyStdoutChannel );
        *mIfconfigProcess << PATH_IFCONFIG << "-a";
        connect( mIfconfigProcess,  SIGNAL( readyReadStandardOutput() ) ,
                 this, SLOT( ifconfigProcessStdout() ) );
        connect( mIfconfigProcess,  SIGNAL( finished( int, QProcess::ExitStatus ) ),
                 this, SLOT( ifconfigProcessExited( int, QProcess::ExitStatus ) ) );

        mIfconfigProcess->start();
    }

#ifdef PATH_IWCONFIG
    if ( !mIwconfigProcess )
    {
        mIwconfigStdout = QString::null;
        mIwconfigProcess = new KProcess();
        mIwconfigProcess->setOutputChannelMode( KProcess::MergedChannels );
        mIwconfigProcess->setEnv( "LANG", "C" );
        mIwconfigProcess->setEnv( "LC_ALL", "C" );
        *mIwconfigProcess << PATH_IWCONFIG;
        connect( mIwconfigProcess,  SIGNAL( readyReadStandardOutput() ),
                 this, SLOT( iwconfigProcessStdout() ) );
        connect( mIwconfigProcess,  SIGNAL( finished( int, QProcess::ExitStatus ) ),
                 this, SLOT( iwconfigProcessExited( int, QProcess::ExitStatus ) ) );

        mIwconfigProcess->start();
    }
#endif

#ifdef PATH_ROUTE
    if ( !mRouteProcess )
    {
        mRouteStdout = QString::null;
        mRouteProcess = new KProcess();
        mRouteProcess->setOutputChannelMode( KProcess::MergedChannels );
        mRouteProcess->setEnv( "LANG", "C" );
        mRouteProcess->setEnv( "LC_ALL", "C" );
        *mRouteProcess << PATH_ROUTE << "-n";
        connect( mRouteProcess,  SIGNAL( readyReadStandardOutput() ),
                 this, SLOT( routeProcessStdout() ) );
        connect( mRouteProcess,  SIGNAL( finished( int, QProcess::ExitStatus ) ),
                 this, SLOT( routeProcessExited( int, QProcess::ExitStatus ) ) );

        mRouteProcess->start();
    }
#endif
}

QString NetToolsBackend::getDefaultRouteIface()
{
    QString iface;
#ifdef PATH_ROUTE
    KProcess droute;
    droute.setOutputChannelMode( KProcess::MergedChannels );
    droute.setEnv( "LANG", "C" );
    droute.setEnv( "LC_ALL", "C" );
    droute << PATH_ROUTE << "-n";
    droute.execute();
    QString routeStdout = droute.readAllStandardOutput();

    QStringList routeList = routeStdout.split( "\n" );
    QStringListIterator it( routeList );
    while ( it.hasNext() )
    {
        QStringList routeParameter = it.next().split( " ", QString::SkipEmptyParts );
        if ( routeParameter.count() < 8 ) // no routing entry
            continue;
        if ( routeParameter[0] != "0.0.0.0" ) // no default route
            continue;
        if ( routeParameter[7] == "lo" )
            continue;
        iface = routeParameter[7]; // default route interface
        break;
    }
#endif
    return iface;
}

void NetToolsBackend::routeProcessExited( int, QProcess::ExitStatus )
{
    mRouteProcess->deleteLater(); // we're in a slot connected to mRouteProcess
    mRouteProcess = 0L;
    parseRouteOutput();
}

void NetToolsBackend::routeProcessStdout()
{
    mRouteStdout += mRouteProcess->readAllStandardOutput();
}

void NetToolsBackend::ifconfigProcessExited( int, QProcess::ExitStatus )
{
    mIfconfigProcess->deleteLater();
    mIfconfigProcess = 0L;
    parseIfconfigOutput();
}

void NetToolsBackend::ifconfigProcessStdout()
{
    mIfconfigStdout += mIfconfigProcess->readAllStandardOutput();
}

void NetToolsBackend::iwconfigProcessExited( int, QProcess::ExitStatus )
{
    mIwconfigProcess->deleteLater();
    mIwconfigProcess = 0L;
    parseIwconfigOutput();
}

void NetToolsBackend::iwconfigProcessStdout()
{
    mIwconfigStdout += mIwconfigProcess->readAllStandardOutput();
}

void NetToolsBackend::parseIfconfigOutput()
{
    /* mIfconfigStdout contains the complete output of 'ifconfig' which we
     * are going to parse here.
     */
    QMap<QString, QString> configs;
    QStringList ifList = mIfconfigStdout.split( "\n\n" );
    QStringList::Iterator it;
    for ( it = ifList.begin(); it != ifList.end(); ++it )
    {
        int index = ( *it ).indexOf( ' ' );
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
    foreach ( QString key, mInterfaces.keys() )
    {
        Interface *interface = mInterfaces.value( key );
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
    }
    updateComplete();
}

void NetToolsBackend::updateInterfaceData( QString& config, InterfaceData& data, int type )
{
    QRegExp regExp( ".*RX.*:(\\d+).*:\\d+.*:\\d+.*:\\d+" );
    if ( regExp.indexIn( config ) > -1 )
        data.rxPackets = regExp.cap( 1 ).toULong();

    regExp.setPattern( ".*TX.*:(\\d+).*:\\d+.*:\\d+.*:\\d+" );
    if ( regExp.indexIn( config ) > -1 )
        data.txPackets = regExp.cap( 1 ).toULong();

    regExp.setPattern( "RX bytes:(\\d+)\\s*\\(\\d+\\.\\d+\\s*\\w+\\)" );
    if ( regExp.indexIn( config ) > -1 )
    {
        // We count the traffic on ourself to avoid an overflow after
        // 4GB of traffic.
        unsigned long currentRxBytes = regExp.cap( 1 ).toULong();
        if ( currentRxBytes < data.prevRxBytes )
        {
            // there was an overflow
            if ( type == Interface::ETHERNET )
            {
                // This makes data counting more accurate but will not work
                // for interfaces that reset the transfered data to zero
                // when deactivated like ppp does.
                data.rxBytes += 0xFFFFFFFF - data.prevRxBytes;
            }
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
    if ( regExp.indexIn( config ) > -1 )
    {
        // We count the traffic on ourself to avoid an overflow after
        // 4GB of traffic.
        unsigned long currentTxBytes = regExp.cap( 1 ).toULong();
        if ( currentTxBytes < data.prevTxBytes )
        {
            // there was an overflow
            if ( type == Interface::ETHERNET )
            {
                // This makes data counting more accurate but will not work
                // for interfaces that reset the transfered data to zero
                // when deactivated like ppp does.
                data.txBytes += 0xFFFFFFFF - data.prevTxBytes;
            }
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
    if ( regExp.indexIn( config ) > -1 )
        data.ipAddress = regExp.cap( 1 );

    regExp.setPattern( "(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}).*(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}).*(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})" );
    if ( regExp.indexIn( config ) > -1 )
    {
        data.broadcastAddress = regExp.cap( 2 );
        data.subnetMask = regExp.cap( 3 );
    }

    if ( type == Interface::ETHERNET )
    {
        regExp.setPattern( "(.{2}:.{2}:.{2}:.{2}:.{2}:.{2})" );
        if ( regExp.indexIn( config ) > -1 )
            data.hwAddress = regExp.cap( 1 );
    }
    else if (  type == Interface::PPP )
    {
        regExp.setPattern( "(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}).*(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}).*(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})" );
        if ( regExp.indexIn( config ) > -1 )
            data.ptpAddress = regExp.cap( 2 );
    }
}

void NetToolsBackend::parseIwconfigOutput()
{
    /* mIwconfigStdout contains the complete output of 'iwconfig' which we
     * are going to parse here.
     */
    QMap<QString, QString> configs;
    QStringList ifList = mIwconfigStdout.split( "\n\n" );
    QStringList::Iterator it;
    for ( it = ifList.begin(); it != ifList.end(); ++it )
    {
        int index = ( *it ).indexOf( ' ' );
        if ( index == -1 )
            continue;
        QString key = ( *it ).left( index );
        configs[key] = ( *it ).mid( index );
    }

    /* We loop over the interfaces the user wishs to monitor.
     * If we find the interface in the output of 'iwconfig'
     * we update its data.
     */
    foreach ( QString key, mInterfaces.keys() )
    {
        Interface* interface = mInterfaces.value( key );

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
            updateWirelessData( key, configs[key], interface->getWirelessData() );
        }
    }
}

void NetToolsBackend::updateWirelessData( QString& ifaceName, QString& config, WirelessData& data )
{
    QRegExp regExp( "ESSID:([^\"][\\S]*)" );
    if ( regExp.indexIn( config ) > -1 )
        data.essid = regExp.cap( 1 );
    else
    {
        regExp.setPattern( "ESSID:\"([^\"]*)" );
        if ( regExp.indexIn( config ) > -1 )
            data.essid = regExp.cap( 1 );
        else
            data.essid = QString::null;
    }

    regExp.setPattern( "Mode:(\\w*)" );
    if ( regExp.indexIn( config ) > -1 )
        data.mode = regExp.cap( 1 );

    regExp.setPattern( "Frequency:([\\w|\\.]*\\s*\\w*)" );
    if ( regExp.indexIn( config ) > -1 )
    {
        data.frequency = regExp.cap( 1 );
        data.channel = "-";
    }
    else
    {
        data.frequency = "-";
        regExp.setPattern( "Channel:(\\d*)" );
        if ( regExp.indexIn( config ) > -1 )
            data.channel = regExp.cap( 1 );
        else
            data.channel = "-";
    }

    regExp.setPattern( "Bit Rate[=:](\\d*\\s*[\\w/]*)" );
    if ( regExp.indexIn( config ) > -1 )
        data.bitRate = regExp.cap( 1 );

    regExp.setPattern( "(.{2}:.{2}:.{2}:.{2}:.{2}:.{2})" );
    if ( regExp.indexIn( config ) > -1 )
        data.accessPoint = regExp.cap( 1 );
    else
        data.accessPoint.clear();

    regExp.setPattern( "Nickname:\"(\\w*)\"" );
    if ( regExp.indexIn( config ) > -1 )
        data.nickName = regExp.cap( 1 );

    regExp.setPattern( "Link Quality[=:]([\\d]*)(\\/([\\d]+))?" );
    if ( regExp.indexIn( config ) > -1 )
    {
        if ( regExp.numCaptures() == 3 && !regExp.cap( 3 ).isEmpty() )
        {
            int maxQual = regExp.cap( 3 ).toInt();
            if ( maxQual > 0 )
                data.linkQuality = QString( "%1%" ).arg( 100 * regExp.cap( 1 ).toInt() / maxQual );
            else
                data.linkQuality = "0";
        }
        else
            data.linkQuality = regExp.cap( 1 );
    }

#ifdef PATH_IWLIST
    if ( data.accessPoint != data.prevAccessPoint )
    {
        /* Reset encryption status for new access point */
        data.encryption = false;
        data.prevAccessPoint = data.accessPoint;
    }
    /* We only use left-over wireless scans to prevent doing a new scan every
     * polling period.  This requires that we run iwlist once per wireless
     * device.  If our current access point disappears from the results then
     * parseWirelessEncryption will use the last encryption status until the
     * results are updated again. */
    KProcess iwlistProcess;
    iwlistProcess.setOutputChannelMode( KProcess::MergedChannels );
    iwlistProcess.setEnv( "LANG", "C" );
    iwlistProcess.setEnv( "LC_ALL", "C" );
    iwlistProcess << PATH_IWLIST << ifaceName << "scan" << "last";

    iwlistProcess.execute();
    QString iwlistOutput = iwlistProcess.readAllStandardOutput();
    parseWirelessEncryption( iwlistOutput, data );
#endif
}

void NetToolsBackend::parseWirelessEncryption( QString& config, WirelessData& data )
{
    QStringList apList = config.split( "Cell [0-9]{2} - ", QString::SkipEmptyParts );
    foreach( QString ap, apList )
    {
        QRegExp regExp( "Address: (.{2}:.{2}:.{2}:.{2}:.{2}:.{2})" );
        if ( regExp.indexIn( ap ) > -1 && regExp.cap( 1 ) == data.accessPoint )
        {
            regExp.setPattern( "Encryption key:" );
            if ( regExp.indexIn( ap ) > -1 )
            {
                regExp.setPattern( "Encryption key:off" );
                if ( regExp.indexIn( ap ) > -1 )
                    data.encryption = false;
                else
                    data.encryption = true;
                return;
            }
        }
    }
}

void NetToolsBackend::parseRouteOutput()
{
    /* mRouteStdout contains the complete output of 'route' which we
     * are going to parse here.
     */
    QHash<QString, QStringList> configs;
    QStringList routeList = mRouteStdout.split( "\n" );
    foreach ( QString it, routeList )
    {
        QStringList routeParameter = it.split( " ", QString::SkipEmptyParts );
        if ( routeParameter.count() < 8 ) // no routing entry
            continue;
        if ( routeParameter[0] != "0.0.0.0" ) // no default route
            continue;
        configs.insert( routeParameter[7], routeParameter );
    }

    /* We loop over the interfaces the user wishs to monitor.
     * If we find the interface in the output of 'route' we update
     * the data of the interface.
     */
    foreach ( QString key, mInterfaces.keys() )
    {
        Interface* interface = mInterfaces.value( key );

        if ( configs.contains( key ) )
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
