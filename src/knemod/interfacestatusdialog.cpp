/* This file is part of KNemo
   Copyright (C) 2004, 2006 Percy Leonhardt <percy@eris23.de>
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

#include <kio/global.h>
#include <sys/socket.h>
#include <netdb.h>
#include <QAbstractItemView>
#include <KGlobalSettings>

#ifdef __linux__
  #include <netlink/netlink.h>
#endif

#include "global.h"
#include "interface.h"
#include "interfacestatistics.h"
#include "interfacestatusdialog.h"
#include "statisticsmodel.h"

InterfaceStatusDialog::InterfaceStatusDialog( Interface* interface, QWidget* parent )
    : KDialog( parent ),
      mWasShown( false ),
      mSetPos( true ),
      mConfig( KGlobal::config() ),
      mInterface( interface )
{
    setCaption( i18nc( "interface name", "%1 Interface Status", interface->ifaceName() ) );
    setButtons( None );

    ui.setupUi( mainWidget() );
    configChanged();

    // FreeBSD doesn't have these
#ifndef __linux__
    ui.addrLabel->hide();
    ui.textLabelAddrLabel->hide();
    ui.textLabelNickNameL->hide();
    ui.textLabelNickName->hide();
#endif

    connect( ui.comboBoxIP, SIGNAL( currentIndexChanged(int) ), this, SLOT( updateDialog() ) );

    updateDialog();
    const BackendData * data = mInterface->backendData();
    if ( !data )
        return;
    if ( !data->isWireless )
    {
        QWidget* wirelessTab = ui.tabWidget->widget( 2 );
        ui.tabWidget->removeTab( 2 );
        delete wirelessTab;
    }

    // Restore window size and position.
    KConfig *config = mConfig.data();
    KConfigGroup interfaceGroup( config, confg_interface + mInterface->ifaceName() );
    if ( interfaceGroup.hasKey( conf_statusPos ) )
    {
        QPoint p = interfaceGroup.readEntry( conf_statusPos, QPoint() );
        // See comment in event()
        mSetPos = false;
        move( p );
    }
    if ( interfaceGroup.hasKey( conf_statusSize ) )
    {
        QSize s = interfaceGroup.readEntry( conf_statusSize, QSize() );
        resize( s );
    }
    else
        resize( sizeHint() );

    statisticsChanged();
}

InterfaceStatusDialog::~InterfaceStatusDialog()
{
    if ( mWasShown )
    {
        KConfig *config = mConfig.data();
        KConfigGroup interfaceGroup( config, confg_interface + mInterface->ifaceName() );
        interfaceGroup.writeEntry( conf_statusPos, pos() );
        interfaceGroup.writeEntry( conf_statusSize, size() );
        config->sync();
    }
}

bool InterfaceStatusDialog::event( QEvent *e )
{
    /* If we do not explicitly call size() and move() at least once then
     * hiding and showing the dialog will cause it to forget its previous
     * size and position. */
    if ( e->type() == QEvent::Move )
    {
        if ( mSetPos && !pos().isNull() )
        {
            mSetPos = false;
            move( pos() );
        }
    }
    else if ( e->type() == QEvent::Show )
    {
        mWasShown = true;
        updateDialog();
    }

    return KDialog::event( e );
}

void InterfaceStatusDialog::updateDialog()
{
    if ( isHidden() )
        return;

    const BackendData* data = mInterface->backendData();
    if ( !data )
        return;
    InterfaceSettings& settings = mInterface->settings();

    // connection tab
    ui.textLabelInterface->setText( mInterface->ifaceName() );
    ui.textLabelAlias->setText( settings.alias );
    ui.textLabelUptime->setText( mInterface->uptimeString() );

    if ( data->status & KNemoIface::Connected )
        ui.textLabelStatus->setText( i18n( "Connected" ) );
    else if ( data->status & KNemoIface::Up )
        ui.textLabelStatus->setText( i18n( "Disconnected" ) );
    else if ( data->status & KNemoIface::Available )
        ui.textLabelStatus->setText( i18n( "Down" ) );
    else
        ui.textLabelStatus->setText( i18n( "Unavailable" ) );

    ui.groupBoxStatistics->setEnabled( mInterface->settings().activateStatistics );

    if ( data->status & KNemoIface::Available )
    {
        doAvailable( data );
        if ( data->status & KNemoIface::Up )
        {
            doUp( data );
            if ( data->status & KNemoIface::Connected )
                doConnected( data );
        }
    }

    if ( data->status < KNemoIface::Connected )
    {
        doDisconnected( data );
        if ( data->status < KNemoIface::Up )
        {
            doDown();
            if ( data->status < KNemoIface::Available )
                doUnavailable();
        }
    }
}

void InterfaceStatusDialog::doAvailable( const BackendData* data )
{
    if ( data->interfaceType == KNemoIface::Ethernet )
    {
        ui.macText->setText( data->hwAddress );
        ui.macLabel->show();
        ui.macText->show();
    }
    else
    {
        ui.gatewayLabel->hide();
        ui.gatewayText->hide();
        ui.macLabel->hide();
        ui.macText->hide();
    }

    ui.textLabelPacketsSend->setText( QString::number( data->txPackets ) );
    ui.textLabelPacketsReceived->setText( QString::number( data->rxPackets ) );
    ui.textLabelBytesSend->setText( data->txString );
    ui.textLabelBytesReceived->setText( data->rxString );
    ui.textLabelSpeedSend->setText( mInterface->txRateStr() );
    ui.textLabelSpeedReceived->setText( mInterface->rxRateStr() );
}

void InterfaceStatusDialog::doConnected( const BackendData *data )
{
    ui.groupBoxCurrentConnection->setEnabled( true );
    if ( data->isWireless )
    {
        // wireless tab
        ui.textLabelESSID->setText( data->essid );
        ui.textLabelAccessPoint->setText( data->accessPoint );
        ui.textLabelNickName->setText( data->nickName );
        ui.textLabelMode->setText( data->mode );
        ui.textLabelFreqChannel->setText( data->frequency + " [" + data->channel + "]" );
        ui.textLabelBitRate->setText( data->bitRate );
        ui.textLabelLinkQuality->setText( data->linkQuality );
        if ( data->isEncrypted == true )
        {
            ui.textLabelEncryption->setText( i18n( "active" ) );
        }
        else
        {
            ui.textLabelEncryption->setText( i18n( "off" ) );
        }
    }
}

void InterfaceStatusDialog::doUp( const BackendData *data )
{
    // ip tab

    // Simpler to just clear and re-insert items in the combo box.
    // But then if we're selecting, the highlighted item would get
    // cleared each poll period.
    int i = 0;
    QStringList keys = data->addrData.keys();
    while ( i < ui.comboBoxIP->count() )
    {
        if ( keys.contains( ui.comboBoxIP->itemText( i ) ) )
            i++;
        else
            ui.comboBoxIP->removeItem( i );
    }
    QFont f = KGlobalSettings::generalFont();
    QFontMetrics fm( f );
    int w = 0;
    int keyCounter = 0;
    foreach( QString key, keys )
    {
        // Combo box preserves order in map
        if ( ui.comboBoxIP->findText( key ) < 0 )
            ui.comboBoxIP->insertItem( keyCounter, key );
        keyCounter++;
        if ( fm.width( key ) > w )
            w = fm.width( key );
    }
    ui.comboBoxIP->setMinimumWidth( w + 35 );

    AddrData addrData = data->addrData.value( ui.comboBoxIP->currentText() );

#ifdef __linux__
    if ( addrData.label.isEmpty() )
        ui.textLabelAddrLabel->clear();
    else
        ui.textLabelAddrLabel->setText( addrData.label );
#endif

    if ( ui.comboBoxIP->count() > 0 )
    {
        QString scope;
        switch ( addrData.scope )
        {
            case RT_SCOPE_UNIVERSE:
                scope = i18nc( "ipv6 address scope", "global" );
                break;
            case RT_SCOPE_SITE:
                scope = i18nc( "ipv6 address scope", "site" );
                break;
            case RT_SCOPE_LINK:
                scope = i18nc( "ipv6 address scope", "link" );
                break;
            case RT_SCOPE_HOST:
                scope = i18nc( "ipv6 address scope", "host" );
                break;
            case RT_SCOPE_NOWHERE:
                scope = i18nc( "ipv6 address scope", "none" );
                break;
        }
        scope += addrData.ipv6Flags;
        ui.textLabelScope->setText( scope );

        if ( data->interfaceType == KNemoIface::Ethernet )
        {
            if ( addrData.scope != RT_SCOPE_HOST )
            {
                if ( addrData.afType == AF_INET )
                    ui.gatewayText->setText( data->ip4DefaultGateway );
                else
                    ui.gatewayText->setText( data->ip6DefaultGateway );
                ui.gatewayLabel->show();
                ui.gatewayText->show();
            }
            else
            {
                ui.gatewayLabel->hide();
                ui.gatewayText->hide();
            }
        }

        ui.broadcastLabel->setText( i18n( "Broadcast Address:" ) );
        if ( addrData.scope != RT_SCOPE_HOST )
        {
            ui.broadcastText->setText( addrData.broadcastAddress );
            if ( addrData.hasPeer )
                ui.broadcastLabel->setText( i18n( "PtP Address:" ) );
            ui.broadcastLabel->show();
            ui.broadcastText->show();
        }
        else
        {
            ui.broadcastLabel->hide();
            ui.broadcastText->hide();
        }
    }
    ui.groupBoxIP->setEnabled( (ui.comboBoxIP->count() > 0) );

    // traffic tab
}

void InterfaceStatusDialog::doDisconnected( const BackendData *data )
{
    ui.groupBoxCurrentConnection->setEnabled( false );
    if ( data->isWireless )
    {
        ui.textLabelESSID->setText( QString::null );
        ui.textLabelAccessPoint->setText( QString::null );
        ui.textLabelNickName->setText( QString::null );
        ui.textLabelMode->setText( QString::null );
        ui.textLabelFreqChannel->setText( QString::null );
        ui.textLabelBitRate->setText( QString::null );
        ui.textLabelLinkQuality->setText( QString::null );
        ui.textLabelEncryption->setText( QString::null );
    }
}

void InterfaceStatusDialog::doDown()
{
    // clear IP group
    ui.groupBoxIP->setEnabled( false );
    ui.comboBoxIP->clear();
    ui.textLabelAddrLabel->setText( QString::null );
    ui.textLabelScope->setText( QString::null );
    ui.broadcastText->setText( QString::null );
    ui.gatewayText->setText( QString::null );
}

void InterfaceStatusDialog::doUnavailable()
{
    ui.macText->setText( QString::null );

    // clear current connection group
    ui.textLabelPacketsSend->setText( QString::null );
    ui.textLabelPacketsReceived->setText( QString::null );
    ui.textLabelBytesSend->setText( QString::null );
    ui.textLabelBytesReceived->setText( QString::null );
    ui.textLabelSpeedSend->setText( QString::null );
    ui.textLabelSpeedReceived->setText( QString::null );
}

void InterfaceStatusDialog::configChanged()
{
    bool billText = false;
    if ( mInterface->settings().activateStatistics )
    {
        foreach ( StatsRule rule, mInterface->settings().statsRules )
        {
            if ( rule.periodCount != 1 ||
                 rule.periodUnits != KNemoStats::Month ||
                 mInterface->ifaceStatistics()->calendar()->day( rule.startDate ) != 1 )
            {
                billText = true;
            }
        }
    }

    ui.textLabelBill->setVisible( billText );
    ui.textLabelBillSent->setVisible( billText );
    ui.textLabelBillReceived->setVisible( billText );
    ui.textLabelBillTotal->setVisible( billText );
}

void InterfaceStatusDialog::statisticsChanged()
{
    InterfaceStatistics *stat = mInterface->ifaceStatistics();
    if ( stat == 0 )
        return;

    StatisticsModel * statistics = stat->getStatistics( KNemoStats::Day );
    ui.textLabelTodaySent->setText( statistics->txText() );
    ui.textLabelTodayReceived->setText( statistics->rxText() );
    ui.textLabelTodayTotal->setText( statistics->totalText() );

    statistics = stat->getStatistics( KNemoStats::Month );
    ui.textLabelMonthSent->setText( statistics->txText() );
    ui.textLabelMonthReceived->setText( statistics->rxText() );
    ui.textLabelMonthTotal->setText( statistics->totalText() );

    statistics = stat->getStatistics( KNemoStats::BillPeriod );
    ui.textLabelBillSent->setText( statistics->txText() );
    ui.textLabelBillReceived->setText( statistics->rxText() );
    ui.textLabelBillTotal->setText( statistics->totalText() );

    statistics = stat->getStatistics( KNemoStats::Year );
    ui.textLabelYearSent->setText( statistics->txText() );
    ui.textLabelYearReceived->setText( statistics->rxText() );
    ui.textLabelYearTotal->setText( statistics->totalText() );
}

#include "interfacestatusdialog.moc"
