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

#include <qtimer.h>
#include <qlabel.h>
#include <qstring.h>
#include <qgroupbox.h>
#include <qdatetime.h>
#include <qtabwidget.h>

#include <kdebug.h>
#include <kconfig.h>
#include <klocale.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kactivelabel.h>
#include <kio/global.h>

#include "data.h"
#include "interface.h"
#include "interfacestatistics.h"
#include "interfacestatusdialog.h"

InterfaceStatusDialog::InterfaceStatusDialog( Interface* interface, QWidget* parent, const char* name )
    : InterfaceStatusDlg( parent, name ),
      mPosInitialized( false ),
      mInterface( interface )
{
    setIcon( SmallIcon( "knemo" ) );
    setCaption( interface->getName() + " " + i18n( "Interface Status" ) );
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
        QWidget* wirelessTab = tabWidget->page( 2 );
        tabWidget->removePage( wirelessTab );
        delete wirelessTab;
    }

    if ( !interface->getSettings().activateStatistics )
    {
        setStatisticsGroupEnabled( false );
    }

    // Restore window size and position.
    KConfig* config = new KConfig( "knemorc", false );
    if ( config->hasGroup( "Interface_" + mInterface->getName() ) )
    {
        config->setGroup( "Interface_" + mInterface->getName() );
        if ( config->hasKey( "StatusX" ) && config->hasKey( "StatusY" ) )
        {
            mPos.setX( config->readNumEntry( "StatusX" ) );
            mPos.setY( config->readNumEntry( "StatusY" ) );
            mPosInitialized = true;
        }
        if ( config->hasKey( "StatusWidth" ) && config->hasKey( "StatusHeight" ) )
            resize( config->readNumEntry( "StatusWidth" ),
                    config->readNumEntry( "StatusHeight" ) );
    }
    delete config;

    statisticsChanged();

    mTimer = new QTimer();
    connect( mTimer, SIGNAL( timeout() ), this, SLOT( updateDialog() ) );
    mTimer->start( 1000 );
}

InterfaceStatusDialog::~InterfaceStatusDialog()
{
    mTimer->stop();
    delete mTimer;

    // Store window size and position.
    KConfig* config = new KConfig( "knemorc", false );
    if ( config->hasGroup( "Interface_" + mInterface->getName() ) )
    {
        config->setGroup( "Interface_" + mInterface->getName() );
        config->writeEntry( "StatusX", x() );
        config->writeEntry( "StatusY", y() );
        config->writeEntry( "StatusWidth", width() );
        config->writeEntry( "StatusHeight", height() );
        config->sync();
    }
    delete config;
}

void InterfaceStatusDialog::hide()
{
    mPos = pos();
    mPosInitialized = true;
    QDialog::hide();
}

void InterfaceStatusDialog::show()
{
    QDialog::show();
    /**
     * mPosInitialized should always be true, except when
     * starting KNemo for the very first time.
     */
    if ( mPosInitialized )
        move( mPos );
}

void InterfaceStatusDialog::setStatisticsGroupEnabled( bool enabled )
{
    groupBoxStatistics->setEnabled( enabled );
}

void InterfaceStatusDialog::updateDialog()
{
    InterfaceData& data = mInterface->getData();
    InterfaceSettings& settings = mInterface->getSettings();

    // connection tab
    textLabelInterface->setText( mInterface->getName() );
    textLabelAlias->setText( settings.alias );
    if ( data.available )
    {
        textLabelStatus->setText( i18n( "Connection established." ) );
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
        textLabelUptime->setText( uptime );
    }
    else if ( data.existing )
    {
        textLabelStatus->setText( i18n( "Not connected." ) );
        textLabelUptime->setText( "00:00:00" );
    }
    else
    {
        textLabelStatus->setText( i18n( "Not existing." ) );
        textLabelUptime->setText( "00:00:00" );
    }

    if ( data.available )
    {
        // ip tab
        textLabelIP->setText( data.ipAddress );
        textLabelSubnet->setText( data.subnetMask );
        if ( mInterface->getType() == Interface::ETHERNET )
        {
            variableLabel1->setText( i18n( "Broadcast Address:" ) );
            variableText1->setText( data.broadcastAddress );
            variableLabel2->setText( i18n( "Default Gateway:" ) );
            variableText2->setText( data.defaultGateway );
            variableLabel3->setText( i18n( "HW-Address:" ) );
            variableText3->setText( data.hwAddress );
        }
        else if ( mInterface->getType() == Interface::PPP )
        {
            variableLabel1->setText( i18n( "PtP-Address:" ) );
            variableText1->setText( data.ptpAddress );
            variableLabel2->setText( QString::null );
            variableText2->setText( QString::null );
            variableLabel3->setText( QString::null );
            variableText3->setText( QString::null );
        }
        else
        {
            // shouldn't happen
            variableLabel1->setText( QString::null );
            variableText1->setText( QString::null );
            variableLabel2->setText( QString::null );
            variableText2->setText( QString::null );
            variableLabel3->setText( QString::null );
            variableText3->setText( QString::null );
        }

        // traffic tab
        textLabelPacketsSend->setText( QString::number( data.txPackets ) );
        textLabelPacketsReceived->setText( QString::number( data.rxPackets ) );
        textLabelBytesSend->setText( data.txString );
        textLabelBytesReceived->setText( data.rxString );
        unsigned long bytesPerSecond = data.outgoingBytes / mInterface->getGeneralData().pollInterval;
        textLabelSpeedSend->setText( KIO::convertSize( bytesPerSecond  ) + i18n( "/s" ) );
        bytesPerSecond = data.incomingBytes / mInterface->getGeneralData().pollInterval;
        textLabelSpeedReceived->setText( KIO::convertSize( bytesPerSecond ) + i18n( "/s" ) );

        if ( data.wirelessDevice )
        {
            WirelessData& wdata = mInterface->getWirelessData();

            // wireless tab
            textLabelESSID->setText( wdata.essid );
            textLabelAccessPoint->setText( wdata.accessPoint );
            textLabelNickName->setText( wdata.nickName );
            textLabelMode->setText( wdata.mode );
            textLabelFreqChannel->setText( wdata.frequency + " [" + wdata.channel + "]" );
            textLabelBitRate->setText( wdata.bitRate );
            textLabelLinkQuality->setText( wdata.linkQuality + "%" );
            if ( wdata.encryption == true )
            {
                textLabelEncryption->setText( i18n( "active" ) );
            }
            else
            {
                textLabelEncryption->setText( i18n( "off" ) );
            }
        }
    }
}

void InterfaceStatusDialog::enableNetworkGroups( int )
{
    groupBoxIP->setEnabled( true );
    groupBoxCurrentConnection->setEnabled( true );
}

void InterfaceStatusDialog::disableNetworkGroups( int )
{
    groupBoxIP->setEnabled( false );
    groupBoxCurrentConnection->setEnabled( false );

    // clear IP group
    textLabelIP->setText( QString::null );
    textLabelSubnet->setText( QString::null );
    variableText1->setText( QString::null );
    variableText2->setText( QString::null );
    variableText3->setText( QString::null );

    // clear current connection group
    textLabelPacketsSend->setText( QString::null );
    textLabelPacketsReceived->setText( QString::null );
    textLabelBytesSend->setText( QString::null );
    textLabelBytesReceived->setText( QString::null );
    textLabelSpeedSend->setText( QString::null );
    textLabelSpeedReceived->setText( QString::null );

    // clear wireless tab
    textLabelESSID->setText( QString::null );
    textLabelAccessPoint->setText( QString::null );
    textLabelNickName->setText( QString::null );
    textLabelMode->setText( QString::null );
    textLabelFreqChannel->setText( QString::null );
    textLabelBitRate->setText( QString::null );
    textLabelLinkQuality->setText( QString::null );
    textLabelEncryption->setText( QString::null );
}

void InterfaceStatusDialog::statisticsChanged()
{
    InterfaceStatistics* statistics = mInterface->getStatistics();

    if ( statistics == 0 )
    {
        return;
    }

    const StatisticEntry* entry = statistics->getCurrentDay();
    textLabelTodaySent->setText( KIO::convertSize( entry->txBytes ) );
    textLabelTodayReceived->setText( KIO::convertSize( entry->rxBytes ) );
    textLabelTodayTotal->setText( KIO::convertSize( entry->txBytes + entry->rxBytes ) );

    entry = statistics->getCurrentMonth();
    textLabelMonthSent->setText( KIO::convertSize( entry->txBytes ) );
    textLabelMonthReceived->setText( KIO::convertSize( entry->rxBytes ) );
    textLabelMonthTotal->setText( KIO::convertSize( entry->txBytes + entry->rxBytes ) );

    entry = statistics->getCurrentYear();
    textLabelYearSent->setText( KIO::convertSize( entry->txBytes ) );
    textLabelYearReceived->setText( KIO::convertSize( entry->rxBytes ) );
    textLabelYearTotal->setText( KIO::convertSize( entry->txBytes + entry->rxBytes ) );
}

#include "interfacestatusdialog.moc"
