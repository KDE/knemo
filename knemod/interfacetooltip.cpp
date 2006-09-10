/* This file is part of KNemo
   Copyright (C) 2004 Percy Leonhardt <percy@eris23.de>

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

#include <qrect.h>
#include <qstring.h>

#include <kdebug.h>
#include <klocale.h>
#include <kio/global.h>

#include "data.h"
#include "interface.h"
#include "interfacetooltip.h"

InterfaceToolTip::InterfaceToolTip( Interface* interface,
                                    QWidget* parent )
    : QToolTip( parent ),
      mInterface( interface )
{
    setupToolTipArray();
}

InterfaceToolTip::~InterfaceToolTip()
{
}

void InterfaceToolTip::maybeTip( const QPoint& )
{
    QRect rect( parentWidget()->rect() );
    if ( !rect.isValid() )
        return;

    QString tooltip;
    setupText( tooltip );
    tip( rect, tooltip );
}

void InterfaceToolTip::setupText( QString& text )
{
    int toolTipContent = mInterface->getGeneralData().toolTipContent;
    InterfaceData& data = mInterface->getData();

    text += "<table cellspacing=0 cellpadding=0 border=0>";
    if ( ( toolTipContent & ALIAS ) &&
         mInterface->getSettings().alias != QString::null )
        text += "<tr><th colspan=2 align=center>" + mInterface->getSettings().alias + "</th></tr>";
    if ( toolTipContent & INTERFACE )
        text += "<tr><td>" + mToolTips[0].first + "</td><td>" + mInterface->getName() + "</td></tr>";
    if ( data.available )
    {
        if ( toolTipContent & STATUS )
            text += "<tr><td>" + mToolTips[2].first + "</td><td>" + i18n( "Connection established." ) + "</td></tr>";
        if ( toolTipContent & UPTIME )
        {
            int upsecs = mInterface->getStartTime().secsTo( QDateTime::currentDateTime() );
            int updays = upsecs / 86400; // don't use QDateTime::daysTo() because
            // we only want complete days

            QString uptime;
            if ( updays == 1 )
                uptime = "1 day, ";
            else if ( updays > 1 )
                uptime = QString( "%1 days, " ).arg( updays );

            upsecs -= 86400 * updays; // we only want the seconds of today
            int hrs = upsecs / 3600;
            int mins = ( upsecs - hrs * 3600 ) / 60;
            int secs = upsecs - hrs * 3600 - mins * 60;
            QString time;
            time.sprintf( "%02d:%02d:%02d", hrs, mins, secs );
            uptime += time;
            text += "<tr><td>" + mToolTips[3].first + "</td><td>" + uptime + "</td></tr>";
        }
    }
    else if ( data.existing )
    {
        if ( toolTipContent & STATUS )
            text += "<tr><td>" + mToolTips[2].first + "</td><td>" + i18n( "Not connected." ) + "</td></tr>";
    }
    else
    {
        if ( toolTipContent & STATUS )
            text += "<tr><td>" + mToolTips[2].first + "</td><td>" + i18n( "Not existing." ) + "</td></tr>";
    }

    if ( data.available )
    {
        if ( toolTipContent & IP_ADDRESS )
            text += "<tr><td>" + mToolTips[4].first + "</td><td>" + data.ipAddress + "</td></tr>";
        if ( toolTipContent & SUBNET_MASK )
            text += "<tr><td>" + mToolTips[5].first + "</td><td>" + data.subnetMask + "</td></tr>";
        if ( mInterface->getType() == Interface::ETHERNET )
        {
            if ( toolTipContent & BCAST_ADDRESS )
                text += "<tr><td>" + mToolTips[18].first + "</td><td>" + data.broadcastAddress + "</td></tr>";
            if ( toolTipContent & GATEWAY )
                text += "<tr><td>" + mToolTips[19].first + "</td><td>" + data.defaultGateway + "</td></tr>";
            if ( toolTipContent & HW_ADDRESS )
                text += "<tr><td>" + mToolTips[6].first + "</td><td>" + data.hwAddress + "</td></tr>";
        }
        if ( mInterface->getType() == Interface::PPP )
        {
            if ( toolTipContent & PTP_ADDRESS )
                text += "<tr><td>" + mToolTips[7].first + "</td><td>" + data.ptpAddress + "</td></tr>";
        }
        if ( toolTipContent & RX_PACKETS )
            text += "<tr><td>" + mToolTips[8].first + "</td><td>" + QString::number( data.rxPackets ) + "</td></tr>";
        if ( toolTipContent & TX_PACKETS )
            text += "<tr><td>" + mToolTips[9].first + "</td><td>" + QString::number( data.txPackets ) + "</td></tr>";
        if ( toolTipContent & RX_BYTES )
            text += "<tr><td>" + mToolTips[10].first + "</td><td>" + data.rxString + "</td></tr>";
        if ( toolTipContent & TX_BYTES )
            text += "<tr><td>" + mToolTips[11].first + "</td><td>" + data.txString + "</td></tr>";
        if ( toolTipContent & DOWNLOAD_SPEED )
        {
            unsigned long bytesPerSecond = data.incomingBytes / mInterface->getGeneralData().pollInterval;
            text += "<tr><td>" + mToolTips[20].first + "</td><td>" + KIO::convertSize( bytesPerSecond ) + i18n( "/s" ) + "</td></tr>";
        }
        if ( toolTipContent & UPLOAD_SPEED )
        {
            unsigned long bytesPerSecond = data.outgoingBytes / mInterface->getGeneralData().pollInterval;
            text += "<tr><td>" + mToolTips[21].first + "</td><td>" + KIO::convertSize( bytesPerSecond ) + i18n( "/s" ) + "</td></tr>";
        }
    }

    if ( data.available && data.wirelessDevice )
    {
        WirelessData& wdata = mInterface->getWirelessData();
        if ( toolTipContent & ESSID )
            text += "<tr><td>" + mToolTips[12].first + "</td><td>" + wdata.essid + "</td></tr>";
        if ( toolTipContent & MODE )
            text += "<tr><td>" + mToolTips[13].first + "</td><td>" + wdata.mode + "</td></tr>";
        if ( toolTipContent & FREQUENCY )
            text += "<tr><td>" + mToolTips[14].first + "</td><td>" + wdata.frequency + "</td></tr>";
        if ( toolTipContent & BIT_RATE )
            text += "<tr><td>" + mToolTips[15].first + "</td><td>" + wdata.bitRate + "</td></tr>";
        if ( toolTipContent & ACCESS_POINT )
            text += "<tr><td>" + mToolTips[16].first + "</td><td>" + wdata.accessPoint + "</td></tr>";
        if ( toolTipContent & LINK_QUALITY )
            text += "<tr><td>" + mToolTips[17].first + "</td><td>" + wdata.linkQuality + "</td></tr>";
        if ( toolTipContent & NICK_NAME )
            text += "<tr><td>" + mToolTips[22].first + "</td><td>" + wdata.nickName + "</td></tr>";
        if ( toolTipContent & ENCRYPTION )
        {
            if ( wdata.encryption == true )
            {
                text += "<tr><td>" + mToolTips[23].first + "</td><td>" + i18n( "active" ) + "</td></tr>";
            }
            else
            {
                text += "<tr><td>" + mToolTips[23].first + "</td><td>" + i18n( "off" ) + "</td></tr>";
            }
        }
    }
    text += "</table>";
}

void InterfaceToolTip::setupToolTipArray()
{
    // Cannot make this data static as the i18n macro doesn't seem
    // to work when called to early i.e. before setting the catalogue.
    mToolTips[0] = QPair<QString, int>( i18n( "Interface" ), INTERFACE );
    mToolTips[1] = QPair<QString, int>( i18n( "Alias" ), ALIAS );
    mToolTips[2] = QPair<QString, int>( i18n( "Status" ), STATUS );
    mToolTips[3] = QPair<QString, int>( i18n( "Uptime" ), UPTIME );
    mToolTips[4] = QPair<QString, int>( i18n( "IP-Address" ), IP_ADDRESS );
    mToolTips[5] = QPair<QString, int>( i18n( "Subnet Mask" ), SUBNET_MASK );
    mToolTips[6] = QPair<QString, int>( i18n( "HW-Address" ), HW_ADDRESS );
    mToolTips[7] = QPair<QString, int>( i18n( "PtP-Address" ), PTP_ADDRESS );
    mToolTips[8] = QPair<QString, int>( i18n( "Packets Received" ), RX_PACKETS );
    mToolTips[9] = QPair<QString, int>( i18n( "Packets Sent" ), TX_PACKETS );
    mToolTips[10] = QPair<QString, int>( i18n( "Bytes Received" ), RX_BYTES );
    mToolTips[11] = QPair<QString, int>( i18n( "Bytes Sent" ), TX_BYTES );
    mToolTips[12] = QPair<QString, int>( i18n( "ESSID" ), ESSID );
    mToolTips[13] = QPair<QString, int>( i18n( "Mode" ), MODE );
    mToolTips[14] = QPair<QString, int>( i18n( "Frequency" ), FREQUENCY );
    mToolTips[15] = QPair<QString, int>( i18n( "Bit Rate" ), BIT_RATE );
    mToolTips[16] = QPair<QString, int>( i18n( "Access Point" ), ACCESS_POINT );
    mToolTips[17] = QPair<QString, int>( i18n( "Link Quality" ), LINK_QUALITY );
    mToolTips[18] = QPair<QString, int>( i18n( "Broadcast Address" ), BCAST_ADDRESS );
    mToolTips[19] = QPair<QString, int>( i18n( "Default Gateway" ), LINK_QUALITY );
    mToolTips[20] = QPair<QString, int>( i18n( "Download Speed" ), DOWNLOAD_SPEED );
    mToolTips[21] = QPair<QString, int>( i18n( "Upload Speed" ), UPLOAD_SPEED );
    mToolTips[22] = QPair<QString, int>( i18n( "Nickname" ), NICK_NAME );
    mToolTips[23] = QPair<QString, int>( i18n( "Encryption" ), ENCRYPTION );
    mToolTips[24] = QPair<QString, int>();
}

