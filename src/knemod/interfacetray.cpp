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
#include <KActionCollection>
#include <KApplication>
#include <KConfigGroup>
#include <KMessageBox>
#include <KStandardAction>

#include <QToolTip>
#include <QHelpEvent>

#ifdef USE_KNOTIFICATIONITEM
InterfaceTray::InterfaceTray( Interface* interface, const QString &id, QWidget* parent ) : KNotificationItem( id, parent )
#else
InterfaceTray::InterfaceTray( Interface* interface, const QString &, QWidget* parent ) : KSystemTrayIcon( parent )
#endif
{
    mInterface = interface;
#ifdef USE_KNOTIFICATIONITEM
    setToolTipIconByName( "knemo" );
    setCategory(Hardware);
    setStatus(Active);
    connect(this, SIGNAL(secondaryActivateRequested(QPoint)), this, SLOT(togglePlotter()));
#else
    connect( this, SIGNAL( activated( QSystemTrayIcon::ActivationReason ) ),
             this, SLOT( iconActivated( QSystemTrayIcon::ActivationReason ) ) );
#endif
    setupToolTipArray();
    // remove quit action added by KSystemTrayIcon
    actionCollection()->removeAction( actionCollection()->action( KStandardAction::name( KStandardAction::Quit ) ) );
    KStandardAction::quit( this, SLOT( slotQuit() ), actionCollection() );
}

InterfaceTray::~InterfaceTray()
{
}

void InterfaceTray::updateToolTip()
{
#ifdef USE_KNOTIFICATIONITEM
    QString title = mInterface->getSettings().alias;
    if ( title.isEmpty() )
        title = mInterface->getName();
    setToolTipTitle( i18n( "KNemo - " ) + title );
    setToolTipSubTitle( toolTipData() );
#else
    QPoint pos = QCursor::pos();
    /* If a tooltip is already visible and the global cursor position is in
     * our rect then it must be our tooltip, right? */
    if ( !QToolTip::text().isEmpty() && geometry().contains( pos ) )
    {
        /* Sure.  Update its text in case any data changed. */
        setToolTip( toolTipData() );
        QToolTip::showText( pos, toolTip() );
    }
#endif
}

void InterfaceTray::slotQuit()
{
    int autoStart = KMessageBox::questionYesNoCancel(0, i18n("Should KNemo start automatically when you login?"),
                                                     i18n("Automatically Start KNemo?"), KGuiItem(i18n("Start")),
                                                     KGuiItem(i18n("Do Not Start")), KStandardGuiItem::cancel(), "StartAutomatically");

    KConfig *config = KGlobal::config().data();
    KConfigGroup generalGroup( config, "General");
    if ( autoStart == KMessageBox::Yes ) {
        generalGroup.writeEntry("AutoStart", true);
    } else if ( autoStart == KMessageBox::No) {
        generalGroup.writeEntry("AutoStart", false);
    } else  // cancel chosen; don't quit
        return;
    config->sync();

    kapp->quit();
}

#ifdef USE_KNOTIFICATIONITEM
void InterfaceTray::activate(const QPoint&)
{
    mInterface->showStatusDialog();
}

void InterfaceTray::togglePlotter()
{
     mInterface->showSignalPlotter( true );
}
#else
void InterfaceTray::iconActivated( QSystemTrayIcon::ActivationReason reason )
{
    switch (reason)
    {
     case QSystemTrayIcon::Trigger:
         mInterface->showStatusDialog();
         break;
     case QSystemTrayIcon::MiddleClick:
         mInterface->showSignalPlotter( true );
         break;
     default:
         ;
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
#endif

QString InterfaceTray::toolTipData()
{
    QString tipData;
    int toolTipContent = mInterface->getGeneralData().toolTipContent;
    InterfaceData& data = mInterface->getData();
    QString leftTags = "<tr><td style='padding-right:1em; white-space:nowrap;'>";
    QString centerTags = "</td><td style='white-space:nowrap;'>";
    QString rightTags = "</td></tr>";

    tipData = "<table cellspacing='2'>";

#ifndef USE_KNOTIFICATIONITEM
    if ( ( toolTipContent & ALIAS ) &&
         !mInterface->getSettings().alias.isEmpty() )
        tipData += "<tr><th colspan='2' style='text-align:center;'>" + mInterface->getSettings().alias + "</th></tr>";
#endif
    if ( toolTipContent & INTERFACE )
        tipData += leftTags + mToolTips.value( INTERFACE ) + centerTags + mInterface->getName() + rightTags;
    if ( data.available )
    {
        if ( toolTipContent & STATUS )
            tipData += leftTags + mToolTips.value( STATUS ) + centerTags + i18n( "Connected" ) + rightTags;
    }
    else if ( data.existing )
    {
        if ( toolTipContent & STATUS )
            tipData += leftTags + mToolTips.value( STATUS ) + centerTags + i18n( "Disconnected" ) + rightTags;
    }
    else
    {
        if ( toolTipContent & STATUS )
            tipData += leftTags + mToolTips.value( STATUS ) + centerTags + i18n( "Nonexistent" ) + rightTags;
    }

    if ( ! mInterface->getData().existing )
        tipData += leftTags + i18n( "Nonexistent" ) + centerTags + rightTags;
    else if ( ! mInterface->getData().available )
        tipData += leftTags + i18n( "Disconnected" ) + centerTags + rightTags;

    if ( data.available )
    {
        if ( toolTipContent & UPTIME )
            tipData += leftTags + mToolTips.value( UPTIME ) + centerTags + mInterface->getUptimeString() + rightTags ;
        if ( toolTipContent & IP_ADDRESS )
            tipData += leftTags + mToolTips.value( IP_ADDRESS ) + centerTags + data.ipAddress + rightTags;
        if ( toolTipContent & SUBNET_MASK )
            tipData += leftTags + mToolTips.value( SUBNET_MASK ) + centerTags + data.subnetMask + rightTags;
        if ( mInterface->getType() == Interface::ETHERNET )
        {
            if ( toolTipContent & BCAST_ADDRESS )
                tipData += leftTags + mToolTips.value( BCAST_ADDRESS ) + centerTags + data.broadcastAddress + rightTags;
            if ( toolTipContent & GATEWAY )
                tipData += leftTags + mToolTips.value( GATEWAY ) + centerTags + data.defaultGateway + rightTags;
            if ( toolTipContent & HW_ADDRESS )
                tipData += leftTags + mToolTips.value( HW_ADDRESS ) + centerTags + data.hwAddress + rightTags;
        }
        if ( mInterface->getType() == Interface::PPP )
        {
            if ( toolTipContent & PTP_ADDRESS )
                tipData += leftTags + mToolTips.value( PTP_ADDRESS ) + centerTags + data.ptpAddress + rightTags;
        }
        if ( toolTipContent & RX_PACKETS )
            tipData += leftTags + mToolTips.value( RX_PACKETS ) + centerTags + QString::number( data.rxPackets ) + rightTags;
        if ( toolTipContent & TX_PACKETS )
            tipData += leftTags + mToolTips.value( TX_PACKETS ) + centerTags + QString::number( data.txPackets ) + rightTags;
        if ( toolTipContent & RX_BYTES )
            tipData += leftTags + mToolTips.value( RX_BYTES ) + centerTags + data.rxString + rightTags;
        if ( toolTipContent & TX_BYTES )
            tipData += leftTags + mToolTips.value( TX_BYTES ) + centerTags + data.txString + rightTags;
        if ( toolTipContent & DOWNLOAD_SPEED )
        {
            unsigned long bytesPerSecond = data.incomingBytes / mInterface->getGeneralData().pollInterval;
            tipData += leftTags + mToolTips.value( DOWNLOAD_SPEED ) + centerTags + KIO::convertSize( bytesPerSecond ) + i18n( "/s" ) + rightTags;
        }
        if ( toolTipContent & UPLOAD_SPEED )
        {
            unsigned long bytesPerSecond = data.outgoingBytes / mInterface->getGeneralData().pollInterval;
            tipData += leftTags + mToolTips.value( UPLOAD_SPEED ) + centerTags + KIO::convertSize( bytesPerSecond ) + i18n( "/s" ) + rightTags;
        }
    }

    if ( data.available && data.wirelessDevice )
    {
        WirelessData& wdata = mInterface->getWirelessData();
        if ( toolTipContent & ESSID )
            tipData += leftTags + mToolTips.value( ESSID ) + centerTags + wdata.essid + rightTags;
        if ( toolTipContent & MODE )
            tipData += leftTags + mToolTips.value( MODE ) + centerTags + wdata.mode + rightTags;
        if ( toolTipContent & FREQUENCY )
            tipData += leftTags + mToolTips.value( FREQUENCY ) + centerTags + wdata.frequency + rightTags;
        if ( toolTipContent & BIT_RATE )
            tipData += leftTags + mToolTips.value( BIT_RATE ) + centerTags + wdata.bitRate + rightTags;
        if ( toolTipContent & ACCESS_POINT )
            tipData += leftTags + mToolTips.value( ACCESS_POINT ) + centerTags + wdata.accessPoint + rightTags;
        if ( toolTipContent & LINK_QUALITY )
            tipData += leftTags + mToolTips.value( LINK_QUALITY ) + centerTags + wdata.linkQuality + rightTags;
        if ( toolTipContent & NICK_NAME )
            tipData += leftTags + mToolTips.value( NICK_NAME ) + centerTags + wdata.nickName + rightTags;
        if ( toolTipContent & ENCRYPTION )
        {
            if ( wdata.encryption == true )
            {
                tipData += leftTags + mToolTips.value( ENCRYPTION ) + centerTags + i18n( "active" ) + rightTags;
            }
            else
            {
                tipData += leftTags + mToolTips.value( ENCRYPTION ) + centerTags + i18n( "off" ) + rightTags;
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
#ifndef USE_KNOTIFICATIONITEM
    mToolTips.insert( ALIAS, i18n( "Alias" ) );
#endif
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
