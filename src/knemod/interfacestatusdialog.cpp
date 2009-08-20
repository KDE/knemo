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

#include <kio/global.h>

#include "data.h"
#include "interface.h"
#include "interfacestatistics.h"
#include "interfacestatusdialog.h"

InterfaceStatusDialog::InterfaceStatusDialog( Interface* interface, QWidget* parent )
    : KDialog( parent ),
      mSetPos( true ),
      mConfig( KGlobal::config() ),
      mInterface( interface )
{
    setCaption( interface->getName() + " " + i18n( "Interface Status" ) );
    setButtons( Close );

    ui.setupUi( mainWidget() );

    updateDialog();
    if ( interface->getData().available )
    {
        enableNetworkGroups( 0 );
    }
    else
    {
        disableNetworkGroups( 0 );
    }
    if ( !interface->getData().wirelessDevice )
    {
        QWidget* wirelessTab = ui.tabWidget->widget( 2 );
        ui.tabWidget->removeTab( 2 );
        delete wirelessTab;
    }

    if ( !interface->getSettings().activateStatistics )
    {
        setStatisticsGroupEnabled( false );
    }

    // Restore window size and position.
    KConfig *config = mConfig.data();
    KConfigGroup interfaceGroup( config, "Interface_" + mInterface->getName() );
    if ( interfaceGroup.hasKey( "StatusPos" ) )
    {
        QPoint p = interfaceGroup.readEntry( "StatusPos", QPoint() );
        // See comment in event()
        mSetPos = false;
        move( p );
    }
    if ( interfaceGroup.hasKey( "StatusSize" ) )
    {
        QSize s = interfaceGroup.readEntry( "StatusSize", QSize() );
        resize( s );
    }
    else
        resize( sizeHint() );

    statisticsChanged();
}

InterfaceStatusDialog::~InterfaceStatusDialog()
{
    KConfig *config = mConfig.data();
    KConfigGroup interfaceGroup( config, "Interface_" + mInterface->getName() );
    interfaceGroup.writeEntry( "StatusPos", pos() );
    interfaceGroup.writeEntry( "StatusSize", size() );
    config->sync();
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

    return KDialog::event( e );
}

void InterfaceStatusDialog::setStatisticsGroupEnabled( bool enabled )
{
    ui.groupBoxStatistics->setEnabled( enabled );
}

void InterfaceStatusDialog::updateDialog()
{
    InterfaceData& data = mInterface->getData();
    InterfaceSettings& settings = mInterface->getSettings();

    // connection tab
    ui.textLabelInterface->setText( mInterface->getName() );
    ui.textLabelAlias->setText( settings.alias );
    if ( data.available )
    {
        ui.textLabelStatus->setText( i18n( "Connected" ) );
        time_t upsecs = mInterface->getUptime();
        time_t updays = upsecs / 86400;

        QString uptime = i18np("1 day, ","%1 days, ",updays);

        upsecs -= 86400 * updays; // we only want the seconds of today
        int hrs = upsecs / 3600;
        int mins = ( upsecs - hrs * 3600 ) / 60;
        int secs = upsecs - hrs * 3600 - mins * 60;
        QString time;
        time.sprintf( "%02d:%02d:%02d", hrs, mins, secs );
        uptime += time;
        ui.textLabelUptime->setText( uptime );
    }
    else if ( data.existing )
    {
        ui.textLabelStatus->setText( i18n( "Disconnected" ) );
        ui.textLabelUptime->setText( "00:00:00" );
    }
    else
    {
        ui.textLabelStatus->setText( i18n( "Nonexistent" ) );
        ui.textLabelUptime->setText( "00:00:00" );
    }

    if ( data.available )
    {
        // ip tab
        ui.textLabelIP->setText( data.ipAddress );
        ui.textLabelSubnet->setText( data.subnetMask );
        if ( mInterface->getType() == Interface::ETHERNET )
        {
            ui.variableLabel1->setText( i18n( "Broadcast Address:" ) );
            ui.variableText1->setText( data.broadcastAddress );
            ui.variableLabel2->setText( i18n( "Default Gateway:" ) );
            ui.variableText2->setText( data.defaultGateway );
            ui.variableLabel3->setText( i18n( "HW-Address:" ) );
            ui.variableText3->setText( data.hwAddress );
        }
        else if ( mInterface->getType() == Interface::PPP )
        {
            ui.variableLabel1->setText( i18n( "PtP-Address:" ) );
            ui.variableText1->setText( data.ptpAddress );
            ui.variableLabel2->setText( QString::null );
            ui.variableText2->setText( QString::null );
            ui.variableLabel3->setText( QString::null );
            ui.variableText3->setText( QString::null );
        }
        else
        {
            // shouldn't happen
            ui.variableLabel1->setText( QString::null );
            ui.variableText1->setText( QString::null );
            ui.variableLabel2->setText( QString::null );
            ui.variableText2->setText( QString::null );
            ui.variableLabel3->setText( QString::null );
            ui.variableText3->setText( QString::null );
        }

        // traffic tab
        ui.textLabelPacketsSend->setText( QString::number( data.txPackets ) );
        ui.textLabelPacketsReceived->setText( QString::number( data.rxPackets ) );
        ui.textLabelBytesSend->setText( data.txString );
        ui.textLabelBytesReceived->setText( data.rxString );
        unsigned long bytesPerSecond = data.outgoingBytes / mInterface->getGeneralData().pollInterval;
        ui.textLabelSpeedSend->setText( KIO::convertSize( bytesPerSecond  ) + i18n( "/s" ) );
        bytesPerSecond = data.incomingBytes / mInterface->getGeneralData().pollInterval;
        ui.textLabelSpeedReceived->setText( KIO::convertSize( bytesPerSecond ) + i18n( "/s" ) );

        if ( data.wirelessDevice )
        {
            WirelessData& wdata = mInterface->getWirelessData();

            // wireless tab
            ui.textLabelESSID->setText( wdata.essid );
            ui.textLabelAccessPoint->setText( wdata.accessPoint );
            ui.textLabelNickName->setText( wdata.nickName );
            ui.textLabelMode->setText( wdata.mode );
            ui.textLabelFreqChannel->setText( wdata.frequency + " [" + wdata.channel + "]" );
            ui.textLabelBitRate->setText( wdata.bitRate );
            ui.textLabelLinkQuality->setText( wdata.linkQuality );
            if ( wdata.encryption == true )
            {
                ui.textLabelEncryption->setText( i18n( "active" ) );
            }
            else
            {
                ui.textLabelEncryption->setText( i18n( "off" ) );
            }
        }
    }
}

void InterfaceStatusDialog::enableNetworkGroups( int )
{
    ui.groupBoxIP->setEnabled( true );
    ui.groupBoxCurrentConnection->setEnabled( true );
}

void InterfaceStatusDialog::disableNetworkGroups( int )
{
    ui.groupBoxIP->setEnabled( false );
    ui.groupBoxCurrentConnection->setEnabled( false );

    // clear IP group
    ui.textLabelIP->setText( QString::null );
    ui.textLabelSubnet->setText( QString::null );
    ui.variableText1->setText( QString::null );
    ui.variableText2->setText( QString::null );
    ui.variableText3->setText( QString::null );

    // clear current connection group
    ui.textLabelPacketsSend->setText( QString::null );
    ui.textLabelPacketsReceived->setText( QString::null );
    ui.textLabelBytesSend->setText( QString::null );
    ui.textLabelBytesReceived->setText( QString::null );
    ui.textLabelSpeedSend->setText( QString::null );
    ui.textLabelSpeedReceived->setText( QString::null );

    // clear wireless tab
    if ( mInterface->getData().wirelessDevice )
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

void InterfaceStatusDialog::statisticsChanged()
{
    InterfaceStatistics* statistics = mInterface->getStatistics();

    if ( statistics == 0 )
    {
        return;
    }

    const StatisticEntry* entry = statistics->getCurrentDay();
    ui.textLabelTodaySent->setText( KIO::convertSize( entry->txBytes ) );
    ui.textLabelTodayReceived->setText( KIO::convertSize( entry->rxBytes ) );
    ui.textLabelTodayTotal->setText( KIO::convertSize( entry->txBytes + entry->rxBytes ) );

    entry = statistics->getCurrentMonth();
    ui.textLabelMonthSent->setText( KIO::convertSize( entry->txBytes ) );
    ui.textLabelMonthReceived->setText( KIO::convertSize( entry->rxBytes ) );
    ui.textLabelMonthTotal->setText( KIO::convertSize( entry->txBytes + entry->rxBytes ) );

    entry = statistics->getCurrentYear();
    ui.textLabelYearSent->setText( KIO::convertSize( entry->txBytes ) );
    ui.textLabelYearReceived->setText( KIO::convertSize( entry->rxBytes ) );
    ui.textLabelYearTotal->setText( KIO::convertSize( entry->txBytes + entry->rxBytes ) );
}

#include "interfacestatusdialog.moc"
