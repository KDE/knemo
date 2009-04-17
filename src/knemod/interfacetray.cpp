/* This file is part of KNemo
   Copyright (C) 2004, 2005 Percy Leonhardt <percy@eris23.de>
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

#include "interfacetray.h"

#include <kio/global.h>
#include <KLocale>

#include <QToolTip>
#include <QHelpEvent>

InterfaceTray::InterfaceTray( Interface* interface, const QString& icon, QWidget* parent ) : KSystemTrayIcon( icon, parent )
{
    mInterface = interface;
    setupToolTipArray();
}

InterfaceTray::~InterfaceTray()
{
}

void InterfaceTray::updateToolTip()
{
    QPoint pos = QCursor::pos();
    /* If a tooltip is already visible and the global cursor position is in
     * our rect then it must be our tooltip, right? */
    if ( !QToolTip::text().isEmpty() && geometry().contains( pos ) )
    {
        /* Sure.  Update its text in case any data changed. */
        setToolTip( toolTipData() );
        QToolTip::showText( pos, toolTip() );
    }
}

bool InterfaceTray::event( QEvent *e )
{
    if (e->type() == QEvent::ToolTip) {
         QHelpEvent *helpEvent = static_cast<QHelpEvent *>(e);
         setToolTip( toolTipData() );
         QToolTip::showText(helpEvent->globalPos(), toolTip() );
         return true;
    }
    else
        return KSystemTrayIcon::event(e);
}

QString InterfaceTray::toolTipData()
{
    QString tipData;
    int toolTipContent = mInterface->getGeneralData().toolTipContent;
    InterfaceData& data = mInterface->getData();

    tipData = "<table cellspacing='2'>";

    if ( ( toolTipContent & ALIAS ) &&
         mInterface->getSettings().alias != QString::null )
        tipData += "<tr><th colspan='2' align='center'>" + mInterface->getSettings().alias + "</th></tr>";
    if ( toolTipContent & INTERFACE )
        tipData += "<tr><td style='padding-right:1em'>" + mToolTips.value( INTERFACE ) + "</td><td>" + mInterface->getName() + "</td></tr>";
    if ( data.available )
    {
        if ( toolTipContent & STATUS )
            tipData += "<tr><td style='padding-right:1em'>" + mToolTips.value( STATUS ) + "</td><td>" + i18n( "Connection established." ) + "</td></tr>";
    }
    else if ( data.existing )
    {
        if ( toolTipContent & STATUS )
            tipData += "<tr><td style='padding-right:1em'>" + mToolTips.value( STATUS ) + "</td><td>" + i18n( "Not connected." ) + "</td></tr>";
    }
    else
    {
        if ( toolTipContent & STATUS )
            tipData += "<tr><td style='padding-right:1em'>" + mToolTips.value( STATUS ) + "</td><td>" + i18n( "Not existing." ) + "</td></tr>";
    }

    if ( toolTipContent & UPTIME )
        tipData += "<tr><td style='padding-right:1em'>" + mToolTips.value( UPTIME ) + "</td><td>" + mInterface->getUptimeString() + "</td></tr>" ;

    if ( data.available )
    {
        if ( toolTipContent & IP_ADDRESS )
            tipData += "<tr><td style='padding-right:1em'>" + mToolTips.value( IP_ADDRESS ) + "</td><td>" + data.ipAddress + "</td></tr>";
        if ( toolTipContent & SUBNET_MASK )
            tipData += "<tr><td style='padding-right:1em'>" + mToolTips.value( SUBNET_MASK ) + "</td><td>" + data.subnetMask + "</td></tr>";
        if ( mInterface->getType() == Interface::ETHERNET )
        {
            if ( toolTipContent & BCAST_ADDRESS )
                tipData += "<tr><td style='padding-right:1em'>" + mToolTips.value( BCAST_ADDRESS ) + "</td><td>" + data.broadcastAddress + "</td></tr>";
            if ( toolTipContent & GATEWAY )
                tipData += "<tr><td style='padding-right:1em'>" + mToolTips.value( GATEWAY ) + "</td><td>" + data.defaultGateway + "</td></tr>";
            if ( toolTipContent & HW_ADDRESS )
                tipData += "<tr><td style='padding-right:1em'>" + mToolTips.value( HW_ADDRESS ) + "</td><td>" + data.hwAddress + "</td></tr>";
        }
        if ( mInterface->getType() == Interface::PPP )
        {
            if ( toolTipContent & PTP_ADDRESS )
                tipData += "<tr><td style='padding-right:1em'>" + mToolTips.value( PTP_ADDRESS ) + "</td><td>" + data.ptpAddress + "</td></tr>";
        }
        if ( toolTipContent & RX_PACKETS )
            tipData += "<tr><td style='padding-right:1em'>" + mToolTips.value( RX_PACKETS ) + "</td><td>" + QString::number( data.rxPackets ) + "</td></tr>";
        if ( toolTipContent & TX_PACKETS )
            tipData += "<tr><td style='padding-right:1em'>" + mToolTips.value( TX_PACKETS ) + "</td><td>" + QString::number( data.txPackets ) + "</td></tr>";
        if ( toolTipContent & RX_BYTES )
            tipData += "<tr><td style='padding-right:1em'>" + mToolTips.value( RX_BYTES ) + "</td><td>" + data.rxString + "</td></tr>";
        if ( toolTipContent & TX_BYTES )
            tipData += "<tr><td style='padding-right:1em'>" + mToolTips.value( TX_BYTES ) + "</td><td>" + data.txString + "</td></tr>";
        if ( toolTipContent & DOWNLOAD_SPEED )
        {
            unsigned long bytesPerSecond = data.incomingBytes / mInterface->getGeneralData().pollInterval;
            tipData += "<tr><td style='padding-right:1em'>" + mToolTips.value( DOWNLOAD_SPEED ) + "</td><td>" + KIO::convertSize( bytesPerSecond ) + i18n( "/s" ) + "</td></tr>";
        }
        if ( toolTipContent & UPLOAD_SPEED )
        {
            unsigned long bytesPerSecond = data.outgoingBytes / mInterface->getGeneralData().pollInterval;
            tipData += "<tr><td style='padding-right:1em'>" + mToolTips.value( UPLOAD_SPEED ) + "</td><td>" + KIO::convertSize( bytesPerSecond ) + i18n( "/s" ) + "</td></tr>";
        }
    }

    if ( data.available && data.wirelessDevice )
    {
        WirelessData& wdata = mInterface->getWirelessData();
        if ( toolTipContent & ESSID )
            tipData += "<tr><td style='padding-right:1em'>" + mToolTips.value( ESSID ) + "</td><td>" + wdata.essid + "</td></tr>";
        if ( toolTipContent & MODE )
            tipData += "<tr><td style='padding-right:1em'>" + mToolTips.value( MODE ) + "</td><td>" + wdata.mode + "</td></tr>";
        if ( toolTipContent & FREQUENCY )
            tipData += "<tr><td style='padding-right:1em'>" + mToolTips.value( FREQUENCY ) + "</td><td>" + wdata.frequency + "</td></tr>";
        if ( toolTipContent & BIT_RATE )
            tipData += "<tr><td style='padding-right:1em'>" + mToolTips.value( BIT_RATE ) + "</td><td>" + wdata.bitRate + "</td></tr>";
        if ( toolTipContent & ACCESS_POINT )
            tipData += "<tr><td style='padding-right:1em'>" + mToolTips.value( ACCESS_POINT ) + "</td><td>" + wdata.accessPoint + "</td></tr>";
        if ( toolTipContent & LINK_QUALITY )
            tipData += "<tr><td style='padding-right:1em'>" + mToolTips.value( LINK_QUALITY ) + "</td><td>" + wdata.linkQuality + "</td></tr>";
        if ( toolTipContent & NICK_NAME )
            tipData += "<tr><td style='padding-right:1em'>" + mToolTips.value( NICK_NAME ) + "</td><td>" + wdata.nickName + "</td></tr>";
        if ( toolTipContent & ENCRYPTION )
        {
            if ( wdata.encryption == true )
            {
                tipData += "<tr><td style='padding-right:1em'>" + mToolTips.value( ENCRYPTION ) + "</td><td>" + i18n( "active" ) + "</td></tr>";
            }
            else
            {
                tipData += "<tr><td style='padding-right:1em'>" + mToolTips.value( ENCRYPTION ) + "</td><td>" + i18n( "off" ) + "</td></tr>";
            }
        }
    }
    tipData += "</table>";
    return tipData;
}

void InterfaceTray::setupToolTipArray()
{
    // Cannot make this data static as the i18n macro doesn't seem
    // to work when called to early i.e. before setting the catalogue.
    mToolTips.insert( INTERFACE, i18n( "Interface" ) );
    mToolTips.insert( ALIAS, i18n( "Alias" ) );
    mToolTips.insert( STATUS, i18n( "Status" ) );
    mToolTips.insert( UPTIME, i18n( "Uptime" ) );
    mToolTips.insert( IP_ADDRESS, i18n( "IP-Address" ) );
    mToolTips.insert( SUBNET_MASK, i18n( "Subnet Mask" ) );
    mToolTips.insert( HW_ADDRESS, i18n( "HW-Address" ) );
    mToolTips.insert( PTP_ADDRESS, i18n( "PtP-Address" ) );
    mToolTips.insert( RX_PACKETS, i18n( "Packets Received" ) );
    mToolTips.insert( TX_PACKETS, i18n( "Packets Sent" ) );
    mToolTips.insert( RX_BYTES, i18n( "Bytes Received" ) );
    mToolTips.insert( TX_BYTES, i18n( "Bytes Sent" ) );
    mToolTips.insert( ESSID, i18n( "ESSID" ) );
    mToolTips.insert( MODE, i18n( "Mode" ) );
    mToolTips.insert( FREQUENCY, i18n( "Frequency" ) );
    mToolTips.insert( BIT_RATE, i18n( "Bit Rate" ) );
    mToolTips.insert( ACCESS_POINT, i18n( "Access Point" ) );
    mToolTips.insert( LINK_QUALITY, i18n( "Link Quality" ) );
    mToolTips.insert( BCAST_ADDRESS, i18n( "Broadcast Address" ) );
    mToolTips.insert( GATEWAY, i18n( "Default Gateway" ) );
    mToolTips.insert( DOWNLOAD_SPEED, i18n( "Download Speed" ) );
    mToolTips.insert( UPLOAD_SPEED, i18n( "Upload Speed" ) );
    mToolTips.insert( NICK_NAME, i18n( "Nickname" ) );
    mToolTips.insert( ENCRYPTION, i18n( "Encryption" ) );
}
