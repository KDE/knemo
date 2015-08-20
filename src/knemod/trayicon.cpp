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

#include "trayicon.h"
#include "global.h"
#include "interface.h"
#include "knemodaemon.h"
#include "utils.h"

#ifdef __linux__
#include <netlink/netlink.h>
#endif

#include <sys/socket.h>
#include <netdb.h>
#include <kio/global.h>
#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QPixmapCache>
#include <KAboutData>
#include <KConfigGroup>
#include <KHelpMenu>
#include <KLocalizedString>
#include <KMessageBox>
#include <KProcess>
#include <KSharedConfig>

#include <QToolTip>
#include <QHelpEvent>

#define SHRINK_MAX 0.75
#define HISTSIZE_STORE 0.5

TrayIcon::TrayIcon( Interface* interface, const QString &id, QWidget* parent ) :
    KStatusNotifierItem( id, parent )
{
    mInterface = interface;
    setToolTipIconByName( QLatin1String("knemo") );
    setCategory(Hardware);
    setStatus(Active);
    connect(this, SIGNAL(secondaryActivateRequested(QPoint)), this, SLOT(togglePlotter()));

    // Cannot make this data static as the i18n macro doesn't seem
    // to work when called to early i.e. before setting the catalogue.
    mScope.insert( RT_SCOPE_NOWHERE, i18nc( "ipv6 address scope", "none" ) );
    mScope.insert( RT_SCOPE_HOST, i18nc( "ipv6 address scope", "host" ) );
    mScope.insert( RT_SCOPE_LINK, i18nc( "ipv6 address scope", "link" ) );
    mScope.insert( RT_SCOPE_SITE, i18nc( "ipv6 address scope", "site" ) );
    mScope.insert( RT_SCOPE_UNIVERSE, i18nc( "ipv6 address scope", "global" ) );

    // Replace the standard quit action with ours
    QAction *quitAction = new QAction(this);
    quitAction->setText(KStatusNotifierItem::tr("Quit"));
    quitAction->setIcon(QIcon::fromTheme(QLatin1String("application-exit")));
    QObject::connect(quitAction, SIGNAL(triggered()), this, SLOT(slotQuit()));
    addAction(QLatin1String("quit"), quitAction);

    statusAction = new QAction( i18n( "Show &Status Dialog" ), this );
    plotterAction = new QAction( QIcon::fromTheme( QLatin1String("utilities-system-monitor") ),
                       i18n( "Show &Traffic Plotter" ), this );
    statisticsAction = new QAction( QIcon::fromTheme( QLatin1String("view-statistics") ),
                          i18n( "Show St&atistics" ), this );
    configAction = new QAction( QIcon::fromTheme( QLatin1String("configure") ),
                       i18n( "&Configure KNemo..." ), this );

    connect( statusAction, SIGNAL( triggered() ),
             mInterface, SLOT( showStatusDialog() ) );
    connect( plotterAction, SIGNAL( triggered() ),
             mInterface, SLOT( showSignalPlotter() ) );
    connect( statisticsAction, SIGNAL( triggered() ),
             mInterface, SLOT( showStatisticsDialog() ) );
    connect( configAction, SIGNAL( triggered() ),
             this, SLOT( showConfigDialog() ) );

    QMenu* menu = contextMenu();
    // FIXME: title for QMenu?
    //menu->addTitle( QIcon::fromTheme( QLatin1String("knemo") ), i18n( "KNemo - %1", title ) );
    menu->addAction( statusAction );
    menu->addAction( plotterAction );
    menu->addAction( configAction );
    KHelpMenu* helpMenu( new KHelpMenu( menu, KAboutData::applicationData(), false ) );
    menu->addMenu( helpMenu->menu() )->setIcon( QIcon::fromTheme( QLatin1String("help-contents") ) );

    configChanged();
}

TrayIcon::~TrayIcon()
{
}

void TrayIcon::showConfigDialog()
{
    KNemoDaemon::sSelectedInterface = mInterface->ifaceName();

    KProcess process;
    process << QLatin1String("kcmshell5") << QLatin1String("kcm_knemo");
    process.startDetached();
}

void TrayIcon::slotQuit()
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

void TrayIcon::activate(const QPoint&)
{
    mInterface->showStatusDialog( false );
}

void TrayIcon::togglePlotter()
{
     mInterface->showSignalPlotter( false );
}

void TrayIcon::configChanged()
{
    m_maxRate = mInterface->settings().maxRate;

    histSize = HISTSIZE_STORE/generalSettings->pollInterval;
    if ( histSize < 2 )
        histSize = 3;

    for ( int i=0; i < histSize; i++ )
    {
        m_rxHist.append( 0 );
        m_txHist.append( 0 );
    }

    // Add/remove statisticsAction
    QMenu* menu = contextMenu();
    if ( mInterface->settings().activateStatistics )
        menu->insertAction( configAction, statisticsAction );
    else
        menu->removeAction( statisticsAction );

    // Now force update the icon
    if ( mInterface->settings().iconTheme == TEXT_THEME )
         updateTextIcon( true );
    else if ( mInterface->settings().iconTheme == NETLOAD_THEME )
         updateBarIcon( true );
    else
        updateImgIcon( mInterface->ifaceState() );
}

void TrayIcon::processUpdate()
{
    if ( mInterface->previousIfaceState() != mInterface->ifaceState() &&
         mInterface->settings().iconTheme != TEXT_THEME &&
         mInterface->settings().iconTheme != NETLOAD_THEME )
    {
        updateImgIcon( mInterface->ifaceState() );
    }

    m_rxHist.prepend( mInterface->rxRate() );
    m_txHist.prepend( mInterface->txRate() );
    while ( m_rxHist.count() > histSize )
    {
        m_rxHist.removeLast();
        m_txHist.removeLast();
    }
    if ( mInterface->settings().iconTheme == TEXT_THEME )
        updateTextIcon();
    else if ( mInterface->settings().iconTheme == NETLOAD_THEME )
        updateBarIcon();
    QString currentTip;
    QString title = i18n( "KNemo - %1", mInterface->ifaceName() );
    if ( toolTipTitle() != title )
        setToolTipTitle( title );
    currentTip = toolTipData();
    if ( currentTip != toolTipSubTitle() )
        setToolTipSubTitle( currentTip );
}

bool TrayIcon::conStatusChanged()
{
    int mask = (KNemoIface::Unavailable | KNemoIface::Available | KNemoIface::Connected );
    return (mInterface->backendData()->status & mask) != (mInterface->backendData()->prevStatus & mask);
}

/*
 * ImgIcon
 */

void TrayIcon::updateImgIcon( int status )
{
    QString iconName;
    if ( mInterface->settings().iconTheme == SYSTEM_THEME )
        iconName = QStringLiteral("network-");
    else
        iconName = QLatin1String("knemo-") + mInterface->settings().iconTheme + QLatin1Char('-');

    // Now set the correct icon depending on the status of the interface.
    if ( ( status & KNemoIface::RxTraffic ) &&
         ( status & KNemoIface::TxTraffic ) )
    {
        iconName += ICON_RX_TX;
    }
    else if ( status & KNemoIface::RxTraffic )
    {
        iconName += ICON_RX;
    }
    else if ( status & KNemoIface::TxTraffic )
    {
        iconName += ICON_TX;
    }
    else if ( status & KNemoIface::Connected )
    {
        iconName += ICON_IDLE;
    }
    else if ( status & KNemoIface::Available )
    {
        iconName += ICON_OFFLINE;
    }
    else
    {
        iconName += ICON_ERROR;
    }
    setIconByName( iconName );
}

/*
 * Bar Icon
 */

QList<qreal> TrayIcon::barLevels( QList<unsigned long>& rxHist, QList<unsigned long>& txHist )
{
    // get the average rate of the two, then calculate based on the higher
    // return ratios for both.
    unsigned long histTotal = 0;
    foreach( unsigned long j, rxHist )
    {
        histTotal += j;
    }
    unsigned long rxAvgRate = histTotal / histSize;

    histTotal = 0;
    foreach( unsigned long j, txHist )
    {
        histTotal += j;
    }
    unsigned long txAvgRate = histTotal / histSize;

    // Adjust the scale of the bar icons when we don't have a fixed ceiling.
    if ( !mInterface->settings().barScale )
    {
        QList<unsigned long>rxSortedMax( rxHist );
        QList<unsigned long>txSortedMax( txHist );
        qSort( rxSortedMax );
        qSort( txSortedMax );
        int multiplier = 1024;
        if ( generalSettings->useBitrate )
            multiplier = 1000;
        if( qMax(rxAvgRate, txAvgRate) > m_maxRate )
        {
            m_maxRate = qMax(rxAvgRate, txAvgRate);
        }
        else if( qMax( rxSortedMax.last(), txSortedMax.last() ) < m_maxRate * SHRINK_MAX
                && m_maxRate * SHRINK_MAX >= multiplier )
        {
            m_maxRate *= SHRINK_MAX;
        }
    }

    QList<qreal> levels;
    qreal rxLevel = static_cast<double>(rxAvgRate)/m_maxRate;
    if ( rxLevel > 1.0 )
        rxLevel = 1.0;
    levels.append(rxLevel);
    qreal txLevel = static_cast<double>(txAvgRate)/m_maxRate;
    if ( txLevel > 1.0 )
        txLevel = 1.0;
    levels.append(txLevel);
    return levels;
}

void TrayIcon::updateBarIcon( bool force )
{
    // Has color changed?
    if ( conStatusChanged() )
    {
        force = true;
    }

    QSize iconSize = getIconSize();

    // If either of the bar heights have changed since the last time, we have
    // to do an update.
    QList<qreal> levels = barLevels( m_rxHist, m_txHist );
    int barHeight = static_cast<int>(round(iconSize.height() * levels.at(0)) + 0.5);
    if ( barHeight != m_rxPrevBarHeight )
    {
        force = true;
        m_rxPrevBarHeight = barHeight;
    }
    barHeight = static_cast<int>(round(iconSize.height() * levels.at(1)) + 0.5);
    if ( barHeight != m_txPrevBarHeight )
    {
        force = true;
        m_txPrevBarHeight = barHeight;
    }

    if ( !force )
        return;

    const BackendData * data = mInterface->backendData();

    setIconByPixmap( genBarIcon( levels.at(0), levels.at(1), data->status ) );
    QPixmapCache::clear();
}

/*
 * Text Icon
 */

QString TrayIcon::compactTrayText(unsigned long data )
{
    QString dataString;
    // Space is tight, so no space between number and units, and the complete
    // string should be no more than 4 chars.
    /* Visually confusing to display bytes
    if ( bytes < 922 ) // 922B = 0.9K
        byteString = i18n( "%1B", bytes );
    */
    double multiplier = 1024;
    if ( generalSettings->useBitrate )
        multiplier = 1000;

    int precision = 0;
    if ( data < multiplier*9.95 ) // < 9.95K
    {
        precision = 1;
    }
    if ( data < multiplier*999.5 ) // < 999.5K
    {
        if ( generalSettings->useBitrate )
            dataString = i18n( "%1k", QString::number( data/multiplier, 'f', precision ) );
        else
            dataString = i18n( "%1K", QString::number( data/multiplier, 'f', precision ) );
        return dataString;
    }

    if ( data < pow(multiplier, 2)*9.95 ) // < 9.95M
        precision = 1;
    if ( data < pow(multiplier, 2)*999.5 ) // < 999.5M
    {
        dataString = i18n( "%1M", QString::number( data/pow(multiplier, 2), 'f', precision ) );
        return dataString;
    }

    if ( data < pow(multiplier, 3)*9.95 ) // < 9.95G
        precision = 1;
    // xgettext: no-c-format
    dataString = i18n( "%1G", QString::number( data/pow(multiplier, 3), 'f', precision) );
    return dataString;
}

void TrayIcon::updateTextIcon( bool force )
{
    // Has color changed?
    if ( conStatusChanged() )
    {
        force = true;
    }

    // Has text changed?
    QString byteText = compactTrayText( mInterface->rxRate() );
    if ( byteText != m_rxText )
    {
        force = true;
        m_rxText = byteText;
    }
    byteText = compactTrayText( mInterface->txRate() );
    if ( byteText != m_txText )
    {
        force = true;
        m_txText = byteText;
    }

    if ( !force )
        return;

    setIconByPixmap( genTextIcon(m_rxText, m_txText, plasmaTheme->smallestFont(), mInterface->backendData()->status) );
    QPixmapCache::clear();
}

/*
 * Tooltip
 */

QString TrayIcon::formatTip( const QString& field, const QString& data, bool insertBreak )
{
    QString ltip;
    if (insertBreak)
        ltip = QLatin1String("<br>");
    return ltip += i18nc( "field: value", "<b>%1:</b> %2", field, data );
}

QString TrayIcon::toolTipData()
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

#include "moc_trayicon.cpp"
