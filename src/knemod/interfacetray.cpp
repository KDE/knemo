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
    QString title = i18n( "KNemo - %1", mInterface->ifaceName() );
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

QString InterfaceTray::formatTip( const QString& field, const QString& data, bool insertBreak )
{
    QString ltip;
    if (insertBreak)
        ltip = QLatin1String("<br>");
    return ltip += i18nc( "field: value", "<b>%1:</b> %2", field, data );
}

QString InterfaceTray::toolTipData()
{
    QString tipData;
    QString tipVal;
    int toolTipContent = generalSettings->toolTipContent;
    const BackendData * data = mInterface->backendData();
    if ( !data )
        return QString();

    tipData = QLatin1String("");
    tipVal = QLatin1String("");

    if ( toolTipContent & INTERFACE )
        tipData = formatTip( i18n( "Interface" ), mInterface->ifaceName(), !tipData.isEmpty() );

    if ( toolTipContent & STATUS )
    {
        if ( data->status & KNemoIface::Connected )
            tipVal = i18n( "Connected" );
        else if ( data->status & KNemoIface::Up )
            tipVal = i18n( "Disconnected" );
        else if ( data->status & KNemoIface::Available )
            tipVal += i18n( "Down" );
        else
            tipVal += i18n( "Unavailable" );
        tipData += formatTip( i18n( "Status" ), tipVal, !tipData.isEmpty() );
    }

    if ( data->status & KNemoIface::Connected &&
         toolTipContent & UPTIME )
    {
            tipData += formatTip( i18n( "Connection time"), mInterface->uptimeString(), !tipData.isEmpty() );
    }

    if ( data->status & KNemoIface::Up )
    {
        QStringList keys = data->addrData.keys();
        QString ip4Tip;
        QString ip6Tip;
        foreach ( QString key, keys )
        {
            AddrData addrData = data->addrData.value( key );

            if ( addrData.afType == AF_INET )
            {
                if ( toolTipContent & IP_ADDRESS )
                    ip4Tip += formatTip( i18n( "IPv4 Address"), key, !tipData.isEmpty() );
                if ( toolTipContent & SCOPE )
                    ip4Tip += formatTip( i18n( "Scope & Flags"), mScope.value( addrData.scope ) + addrData.ipv6Flags, !tipData.isEmpty() );
                if ( toolTipContent & BCAST_ADDRESS && !addrData.hasPeer )
                    ip4Tip += formatTip( i18n( "Broadcast Address"), addrData.broadcastAddress, !tipData.isEmpty() );
                else if ( toolTipContent & PTP_ADDRESS && addrData.hasPeer )
                    ip4Tip += formatTip( i18n( "PtP Address"), addrData.broadcastAddress, !tipData.isEmpty() );
            }
            else
            {
                if ( toolTipContent & IP_ADDRESS )
                    ip6Tip += formatTip( i18n( "IPv6 Address"), key, !tipData.isEmpty() );
                if ( toolTipContent & SCOPE )
                    ip4Tip += formatTip( i18n( "Scope & Flags"), mScope.value( addrData.scope ), !tipData.isEmpty() );
                if ( toolTipContent & PTP_ADDRESS && addrData.hasPeer )
                    ip4Tip += formatTip( i18n( "PtP Address"), addrData.broadcastAddress, !tipData.isEmpty() );
            }
        }
        tipData += ip4Tip + ip6Tip;

        if ( KNemoIface::Ethernet == data->interfaceType )
        {
            if ( toolTipContent & GATEWAY )
            {
                if ( !data->ip4DefaultGateway.isEmpty() )
                    tipData += formatTip( i18n( "IPv4 Default Gateway"), data->ip4DefaultGateway, !tipData.isEmpty() );
                if ( !data->ip6DefaultGateway.isEmpty() )
                    tipData += formatTip( i18n( "IPv6 Default Gateway"), data->ip6DefaultGateway, !tipData.isEmpty() );
            }
        }
    }

    if ( data->status & KNemoIface::Available )
    {
        if ( toolTipContent & HW_ADDRESS )
            tipData += formatTip( i18n( "MAC Address"), data->hwAddress, !tipData.isEmpty() );
        if ( toolTipContent & RX_PACKETS )
            tipData += formatTip( i18n( "Packets Received"), QString::number( data->rxPackets ), !tipData.isEmpty() );
        if ( toolTipContent & TX_PACKETS )
            tipData += formatTip( i18n( "Packets Sent"), QString::number( data->txPackets ), !tipData.isEmpty() );
        if ( toolTipContent & RX_BYTES )
            tipData += formatTip( i18n( "Bytes Received"), data->rxString, !tipData.isEmpty() );
        if ( toolTipContent & TX_BYTES )
            tipData += formatTip( i18n( "Bytes Sent"), data->txString, !tipData.isEmpty() );
    }

    if ( data->status & KNemoIface::Connected )
    {
        if ( toolTipContent & DOWNLOAD_SPEED )
            tipData += formatTip( i18n( "Download Speed"), mInterface->rxRateStr(), !tipData.isEmpty() );
        if ( toolTipContent & UPLOAD_SPEED )
            tipData += formatTip( i18n( "Upload Speed"), mInterface->txRateStr(), !tipData.isEmpty() );
    }

    if ( data->status & KNemoIface::Connected && data->isWireless )
    {
        if ( toolTipContent & ESSID )
            tipData += formatTip( i18n( "ESSID"), data->essid, !tipData.isEmpty() );
        if ( toolTipContent & MODE )
            tipData += formatTip( i18n( "Mode"), data->mode, !tipData.isEmpty() );
        if ( toolTipContent & FREQUENCY )
            tipData += formatTip( i18n( "Frequency"), data->frequency, !tipData.isEmpty() );
        if ( toolTipContent & BIT_RATE )
            tipData += formatTip( i18n( "Bit Rate"), data->bitRate, !tipData.isEmpty() );
        if ( toolTipContent & ACCESS_POINT )
            tipData += formatTip( i18n( "Access Point"), data->accessPoint, !tipData.isEmpty() );
        if ( toolTipContent & LINK_QUALITY )
            tipData += formatTip( i18n( "Link Quality"), data->linkQuality, !tipData.isEmpty() );
#ifdef __linux__
        if ( toolTipContent & NICK_NAME )
            tipData += formatTip( i18n( "Nickname"), data->nickName, !tipData.isEmpty() );
#endif
        if ( toolTipContent & ENCRYPTION )
        {
            if ( data->isEncrypted == true )
            {
                tipVal = i18n( "active" );
            }
            else
            {
                tipVal = i18n( "off" );
            }
            tipData += formatTip( i18n( "Encryption"), tipVal, !tipData.isEmpty() );
        }
    }
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
