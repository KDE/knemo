/* This file is part of KNemo
   Copyright (C) 2004, 2005 Percy Leonhardt <percy@eris23.de>
   Copyright (C) 2009, 2010 John Stamp <jstamp@users.sourceforge.net>

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

#include "global.h"
#include "interfacetray.h"

#ifdef __linux__
#include <netlink/netlink.h>
#endif

#include <sys/socket.h>
#include <netdb.h>
#include <kio/global.h>
#include <QAction>
#include <QApplication>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>
#include <KSharedConfig>
#include <KStandardAction>

#include <QToolTip>
#include <QHelpEvent>

InterfaceTray::InterfaceTray( Interface* interface, const QString &id, QWidget* parent ) :
    KStatusNotifierItem( id, parent )
{
    mInterface = interface;
    setToolTipIconByName( QLatin1String("knemo") );
    setCategory(Hardware);
    setStatus(Active);
    connect(this, SIGNAL(secondaryActivateRequested(QPoint)), this, SLOT(togglePlotter()));
    setupMappings();
    QAction *quitAction = new QAction(this);
    quitAction->setText(KStatusNotifierItem::tr("Quit"));
    quitAction->setIcon(QIcon::fromTheme(QLatin1String("application-exit")));
    QObject::connect(quitAction, SIGNAL(triggered()), this, SLOT(slotQuit()));
    // Replace the standard quit action
    addAction(QLatin1String("quit"), quitAction);
}

InterfaceTray::~InterfaceTray()
{
}

void InterfaceTray::updateToolTip()
{
    QString currentTip;
    QString title = mInterface->settings().alias;
    if ( title.isEmpty() )
        title = mInterface->ifaceName();
    title = i18n( "KNemo - %1", title );
    if ( toolTipTitle() != title )
        setToolTipTitle( title );
    currentTip = toolTipData();
    if ( currentTip != toolTipSubTitle() )
        setToolTipSubTitle( currentTip );
}

void InterfaceTray::slotQuit()
{
    int autoStart = KMessageBox::questionYesNoCancel(0, i18n("Should KNemo start automatically when you login?"),
                                                     i18n("Automatically Start KNemo?"), KGuiItem(i18n("Start")),
                                                     KGuiItem(i18n("Do Not Start")), KStandardGuiItem::cancel(), QLatin1String("StartAutomatically"));

    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup generalGroup( config, confg_general );
    if ( autoStart == KMessageBox::Yes ) {
        generalGroup.writeEntry( conf_autoStart, true );
    } else if ( autoStart == KMessageBox::No) {
        generalGroup.writeEntry( conf_autoStart, false );
    } else  // cancel chosen; don't quit
        return;
    config->sync();

    qApp->quit();
}

void InterfaceTray::activate(const QPoint&)
{
    mInterface->showStatusDialog( false );
}

void InterfaceTray::togglePlotter()
{
     mInterface->showSignalPlotter( false );
}

QString InterfaceTray::toolTipData()
{
    QString tipData;
    int toolTipContent = generalSettings->toolTipContent;
    const BackendData * data = mInterface->backendData();
    if ( !data )
        return QString();
    QString leftTags = QStringLiteral("<tr><td style='padding-right:1em; white-space:nowrap;'>");
    QString centerTags = QStringLiteral("</td><td style='white-space:nowrap;'>");
    QString rightTags = QStringLiteral("</td></tr>");

    tipData = QStringLiteral("<table cellspacing='2'>");

    if ( toolTipContent & INTERFACE )
        tipData += leftTags + i18n( "Interface" ) + centerTags + mInterface->ifaceName() + rightTags;

    if ( toolTipContent & STATUS )
    {
        tipData += leftTags + i18n( "Status" ) + centerTags;
        if ( data->status & KNemoIface::Connected )
            tipData += i18n( "Connected" );
        else if ( data->status & KNemoIface::Up )
            tipData += i18n( "Disconnected" );
        else if ( data->status & KNemoIface::Available )
            tipData += i18n( "Down" );
        else
            tipData += i18n( "Unavailable" );
        tipData += rightTags;
    }

    if ( data->status & KNemoIface::Connected &&
         toolTipContent & UPTIME )
    {
            tipData += leftTags + i18n( "Connection time" ) + centerTags + mInterface->uptimeString() + rightTags ;
    }

    if ( data->status & KNemoIface::Up )
    {
        QStringList keys = data->addrData.keys();
        QString ip4Tip;
        QString ip6Tip;
        QString ptpaddress = i18n( "PtP Address" );
        QString scope = i18n( "Scope & Flags" );
        foreach ( QString key, keys )
        {
            AddrData addrData = data->addrData.value( key );

            if ( addrData.afType == AF_INET )
            {
                if ( toolTipContent & IP_ADDRESS )
                    ip4Tip += leftTags + i18n( "IPv4 Address" ) + centerTags + key + rightTags;
                if ( toolTipContent & SCOPE )
                    ip4Tip += leftTags + scope + centerTags + mScope.value( addrData.scope ) + addrData.ipv6Flags + rightTags;
                if ( toolTipContent & BCAST_ADDRESS && !addrData.hasPeer )
                    ip4Tip += leftTags + i18n( "Broadcast Address" ) + centerTags + addrData.broadcastAddress + rightTags;
                else if ( toolTipContent & PTP_ADDRESS && addrData.hasPeer )
                    ip4Tip += leftTags + ptpaddress + centerTags + addrData.broadcastAddress + rightTags;
            }
            else
            {
                if ( toolTipContent & IP_ADDRESS )
                    ip6Tip += leftTags + i18n( "IPv6 Address" ) + centerTags + key + rightTags;
                if ( toolTipContent & SCOPE )
                    ip6Tip += leftTags + scope + centerTags + mScope.value( addrData.scope ) + rightTags;
                if ( toolTipContent & PTP_ADDRESS && addrData.hasPeer )
                    ip6Tip += leftTags + ptpaddress + centerTags + addrData.broadcastAddress + rightTags;
            }
        }
        tipData += ip4Tip + ip6Tip;

        if ( KNemoIface::Ethernet == data->interfaceType )
        {
            if ( toolTipContent & GATEWAY )
            {
                if ( !data->ip4DefaultGateway.isEmpty() )
                    tipData += leftTags + i18n( "IPv4 Default Gateway" ) + centerTags + data->ip4DefaultGateway + rightTags;
                if ( !data->ip6DefaultGateway.isEmpty() )
                    tipData += leftTags + i18n( "IPv6 Default Gateway" ) + centerTags + data->ip6DefaultGateway + rightTags;
            }
        }
    }

    if ( data->status & KNemoIface::Available )
    {
        if ( toolTipContent & HW_ADDRESS )
            tipData += leftTags + i18n( "MAC Address" ) + centerTags + data->hwAddress + rightTags;
        if ( toolTipContent & RX_PACKETS )
            tipData += leftTags + i18n( "Packets Received" ) + centerTags + QString::number( data->rxPackets ) + rightTags;
        if ( toolTipContent & TX_PACKETS )
            tipData += leftTags + i18n( "Packets Sent" ) + centerTags + QString::number( data->txPackets ) + rightTags;
        if ( toolTipContent & RX_BYTES )
            tipData += leftTags + i18n( "Bytes Received" ) + centerTags + data->rxString + rightTags;
        if ( toolTipContent & TX_BYTES )
            tipData += leftTags + i18n( "Bytes Sent" ) + centerTags + data->txString + rightTags;
    }

    if ( data->status & KNemoIface::Connected )
    {
        if ( toolTipContent & DOWNLOAD_SPEED )
            tipData += leftTags + i18n( "Download Speed" ) + centerTags + mInterface->rxRateStr() + rightTags;
        if ( toolTipContent & UPLOAD_SPEED )
            tipData += leftTags + i18n( "Upload Speed" ) + centerTags + mInterface->txRateStr() + rightTags;
    }

    if ( data->status & KNemoIface::Connected && data->isWireless )
    {
        if ( toolTipContent & ESSID )
            tipData += leftTags + i18n( "ESSID" ) + centerTags + data->essid + rightTags;
        if ( toolTipContent & MODE )
            tipData += leftTags + i18n( "Mode" ) + centerTags + data->mode + rightTags;
        if ( toolTipContent & FREQUENCY )
            tipData += leftTags + i18n( "Frequency" ) + centerTags + data->frequency + rightTags;
        if ( toolTipContent & BIT_RATE )
            tipData += leftTags + i18n( "Bit Rate" ) + centerTags + data->bitRate + rightTags;
        if ( toolTipContent & ACCESS_POINT )
            tipData += leftTags + i18n( "Access Point" ) + centerTags + data->accessPoint + rightTags;
        if ( toolTipContent & LINK_QUALITY )
            tipData += leftTags + i18n( "Link Quality" ) + centerTags + data->linkQuality + rightTags;
#ifdef __linux__
        if ( toolTipContent & NICK_NAME )
            tipData += leftTags + i18n( "Nickname" ) + centerTags + data->nickName + rightTags;
#endif
        if ( toolTipContent & ENCRYPTION )
        {
            QString encryption = i18n( "Encryption" );
            if ( data->isEncrypted == true )
            {
                tipData += leftTags + encryption + centerTags + i18n( "active" ) + rightTags;
            }
            else
            {
                tipData += leftTags + encryption + centerTags + i18n( "off" ) + rightTags;
            }
        }
    }
    tipData += QLatin1String("</table>");
    return tipData;
}

void InterfaceTray::setupMappings()
{
    // Cannot make this data static as the i18n macro doesn't seem
    // to work when called to early i.e. before setting the catalogue.
    mScope.insert( RT_SCOPE_NOWHERE, i18nc( "ipv6 address scope", "none" ) );
    mScope.insert( RT_SCOPE_HOST, i18nc( "ipv6 address scope", "host" ) );
    mScope.insert( RT_SCOPE_LINK, i18nc( "ipv6 address scope", "link" ) );
    mScope.insert( RT_SCOPE_SITE, i18nc( "ipv6 address scope", "site" ) );
    mScope.insert( RT_SCOPE_UNIVERSE, i18nc( "ipv6 address scope", "global" ) );
}

#include "moc_interfacetray.cpp"
