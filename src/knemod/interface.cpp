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

#include <math.h>

#include <KCalendarSystem>
#include <KColorScheme>
#include <KNotification>
#include <KWindowSystem>
#include <kio/global.h>

#include "backends/backendbase.h"
#include "utils.h"
#include "interface.h"
#include "interfaceplotterdialog.h"
#include "interfacestatistics.h"
#include "interfacestatusdialog.h"
#include "interfacestatisticsdialog.h"

Interface::Interface( const QString &ifname,
                      const BackendData* data,
                      const GeneralData& generalData )
    : QObject(),
      mType( KNemoIface::UnknownType ),
      mState( KNemoIface::UnknownState ),
      mPreviousState( KNemoIface::UnknownState ),
      mName( ifname ),
      mRealSec( 0.0 ),
      mUptime( 0 ),
      mUptimeString( "00:00:00" ),
      mRxRate( 0 ),
      mTxRate( 0 ),
      mIcon( this ),
      mStatistics( 0 ),
      mStatusDialog( 0 ),
      mStatisticsDialog(  0 ),
      mPlotterDialog( 0 ),
      mBackendData( data ),
      mGeneralData( generalData )
{
    mPlotterDialog = new InterfacePlotterDialog( mName );

    connect( &mIcon, SIGNAL( statisticsSelected() ),
             this, SLOT( showStatisticsDialog() ) );
}

Interface::~Interface()
{
    delete mStatusDialog;
    delete mPlotterDialog;
    delete mStatisticsDialog;
    delete mStatistics;
}

void Interface::configChanged()
{
    KSharedConfigPtr config = KGlobal::config();
    QString group( confg_interface );
    group += mName;
    KConfigGroup interfaceGroup( config, group );
    InterfaceSettings s;
    mSettings.alias = interfaceGroup.readEntry( conf_alias ).trimmed();
    mSettings.iconTheme = interfaceGroup.readEntry( conf_iconTheme, s.iconTheme );
    QStringList themeNames;
    QList<KNemoTheme> themes = findThemes();
    // Let's check that it's available
    foreach( KNemoTheme theme, themes )
        themeNames << theme.internalName;
    themeNames << NETLOAD_THEME;
    if ( !themeNames.contains( mSettings.iconTheme ) )
        mSettings.iconTheme = TEXT_THEME;
    mSettings.colorIncoming = interfaceGroup.readEntry( conf_colorIncoming, s.colorIncoming );
    mSettings.colorOutgoing = interfaceGroup.readEntry( conf_colorOutgoing, s.colorOutgoing );
    KColorScheme scheme(QPalette::Active, KColorScheme::View);
    mSettings.colorDisabled = interfaceGroup.readEntry( conf_colorDisabled, scheme.foreground( KColorScheme::InactiveText ).color() );
    mSettings.colorUnavailable = interfaceGroup.readEntry( conf_colorUnavailable, scheme.foreground( KColorScheme::InactiveText ).color() );
    mSettings.dynamicColor = interfaceGroup.readEntry( conf_dynamicColor, s.dynamicColor );
    mSettings.colorIncomingMax = interfaceGroup.readEntry( conf_colorIncomingMax, s.colorIncomingMax );
    mSettings.colorOutgoingMax = interfaceGroup.readEntry( conf_colorOutgoingMax, s.colorOutgoingMax );
    mSettings.barScale = interfaceGroup.readEntry( conf_barScale, s.barScale );
    mSettings.inMaxRate = interfaceGroup.readEntry( conf_inMaxRate, s.inMaxRate )*1024;
    mSettings.outMaxRate = interfaceGroup.readEntry( conf_outMaxRate, s.outMaxRate )*1024;
    mSettings.hideWhenDisconnected = interfaceGroup.readEntry( conf_hideWhenNotAvail, s.hideWhenDisconnected );
    mSettings.hideWhenUnavailable = interfaceGroup.readEntry( conf_hideWhenNotExist, s.hideWhenUnavailable );
    mSettings.activateStatistics = interfaceGroup.readEntry( conf_activateStatistics, s.activateStatistics );
    mSettings.trafficThreshold = clamp<unsigned int>(interfaceGroup.readEntry( conf_trafficThreshold, s.trafficThreshold ), 0, 1000 );
    mSettings.warnThreshold = clamp<double>(interfaceGroup.readEntry( conf_billingWarnThresh, s.warnThreshold ), 0.0, 9999.0 );
    mSettings.warnTotalTraffic = interfaceGroup.readEntry( conf_billingWarnRxTx, s.warnTotalTraffic );

    // TODO: Some of the calendars are a bit buggy, so default to Gregorian for now
    //mSettings.calendar = interfaceGroup.readEntry( conf_calendar, KGlobal::locale()->calendarType() );
    mSettings.calendar = interfaceGroup.readEntry( conf_calendar, "gregorian" );

    mSettings.customBilling = interfaceGroup.readEntry( conf_customBilling, s.customBilling );

    KCalendarSystem* calendar = KCalendarSystem::create( mSettings.calendar );
    QDate startDate = QDate::currentDate().addDays( 1 - calendar->day( QDate::currentDate() ) );
    mSettings.billingStart = interfaceGroup.readEntry( conf_billingStart, startDate );
    // No future start period
    if ( mSettings.billingStart > QDate::currentDate() )
        mSettings.billingStart = startDate;
    mSettings.billingMonths = clamp<int>(interfaceGroup.readEntry( conf_billingMonths, s.billingMonths ), 1, 6 );
    if ( mSettings.customBilling == false )
    {
        mSettings.billingMonths = 1;
        mSettings.billingStart = mSettings.billingStart.addDays( 1 - calendar->day( mSettings.billingStart ) );
    }
    mSettings.commands.clear();
    int numCommands = interfaceGroup.readEntry( conf_numCommands, s.numCommands );
    for ( int i = 0; i < numCommands; i++ )
    {
        QString entry;
        InterfaceCommand cmd;
        entry = QString( "%1%2" ).arg( conf_runAsRoot ).arg( i + 1 );
        cmd.runAsRoot = interfaceGroup.readEntry( entry, false );
        entry = QString( "%1%2" ).arg( conf_command ).arg( i + 1 );
        cmd.command = interfaceGroup.readEntry( entry );
        entry = QString( "%1%2" ).arg( conf_menuText ).arg( i + 1 );
        cmd.menuText = interfaceGroup.readEntry( entry );
        mSettings.commands.append( cmd );
    }

    // This prevents needless regeneration of icon when first shown in tray
    if ( mState == KNemoIface::UnknownState )
    {
        mState = mBackendData->status;
        mPreviousState = mState;
    }
    mIcon.configChanged();

    if ( mStatistics )
    {
        mStatistics->configChanged();
        if ( !mSettings.activateStatistics )
            stopStatistics();
    }
    else if ( mSettings.activateStatistics )
    {
        startStatistics();
    }

    if ( mStatisticsDialog != 0 )
        mStatisticsDialog->configChanged();
}

void Interface::processUpdate()
{
    mPreviousState = mState;
    unsigned int trafficThreshold = mSettings.trafficThreshold;
    mState = mBackendData->status;

    mRxRate = mBackendData->incomingBytes / mGeneralData.pollInterval;
    mTxRate = mBackendData->outgoingBytes / mGeneralData.pollInterval;

    QString title = mSettings.alias;
    if ( title.isEmpty() )
        title = mName;

    if ( mState & KNemoIface::Connected )
    {
        // the interface is connected, look for traffic
        if ( ( mBackendData->rxPackets - mBackendData->prevRxPackets ) > trafficThreshold )
            mState |= KNemoIface::RxTraffic;
        if ( ( mBackendData->txPackets - mBackendData->prevTxPackets ) > trafficThreshold )
            mState |= KNemoIface::TxTraffic;

        if ( mStatistics )
        {
            mStatistics->addRxBytes( mBackendData->incomingBytes );
            mStatistics->addTxBytes( mBackendData->outgoingBytes );
        }

        updateTime();

        if ( mPreviousState < KNemoIface::Connected )
        {
            QString connectedStr;
            if ( mBackendData->isWireless )
                connectedStr = i18n( "%1 is connected to %2", title, mBackendData->essid );
            else
                connectedStr = i18n( "%1 is connected", title );
            if ( mPreviousState != KNemoIface::UnknownState )
                KNotification::event( "connected", connectedStr );
        }
    }
    else if ( mState & KNemoIface::Available )
    {
        if ( mPreviousState & KNemoIface::Connected )
        {
            KNotification::event( "disconnected", i18n( "%1 has disconnected", title ) );
            if ( mType == KNemoIface::PPP )
                backend->clearTraffic( mName );
            resetUptime();
        }
        else if ( mPreviousState < KNemoIface::Available )
        {
            if ( mPreviousState != KNemoIface::UnknownState )
                KNotification::event( "available", i18n( "%1 is available", title ) );
        }
    }
    else if ( mState == KNemoIface::Unavailable &&
              mPreviousState > KNemoIface::Unavailable )
    {
        KNotification::event( "unavailable", i18n( "%1 is unavailable", title ) );
        if ( mType == KNemoIface::PPP )
            backend->clearTraffic( mName );
        resetUptime();
    }

    if ( mPreviousState != mState )
        mIcon.updateTrayStatus();

    if ( mPlotterDialog )
        mPlotterDialog->updatePlotter( mRxRate / 1024.0, mTxRate / 1024.0 );

    mIcon.updateToolTip();
    if ( mStatusDialog )
        mStatusDialog->updateDialog();
}

void Interface::resetUptime()
{
    mUptime = 0;
    mRealSec = 0.0;
    mUptimeString = "00:00:00";
    mRxRate = 0;
    mTxRate = 0;
}

void Interface::showStatusDialog( bool fromContextMenu )
{
    // Toggle the status dialog.
    // First click will show the status dialog, second will hide it.
    if ( mStatusDialog == 0L )
    {
        mStatusDialog = new InterfaceStatusDialog( this );
        if ( mStatistics != 0 )
        {
            connect( mStatistics, SIGNAL( currentEntryChanged() ),
                     mStatusDialog, SLOT( statisticsChanged() ) );
            mStatusDialog->statisticsChanged();
        }
    }

    activateOrHide( mStatusDialog, fromContextMenu );
}

void Interface::showSignalPlotter( bool fromContextMenu )
{
    // Toggle the signal plotter.
    activateOrHide( mPlotterDialog, fromContextMenu );
}

void Interface::showStatisticsDialog()
{
    if ( mStatisticsDialog == 0 )
    {
        mStatisticsDialog = new InterfaceStatisticsDialog( this );
        if ( mStatistics == 0 )
        {
            // should never happen but you never know...
            startStatistics();
        }
        connect( mStatisticsDialog, SIGNAL( clearStatistics() ), mStatistics, SLOT( clearStatistics() ) );
    }
    mStatisticsDialog->show();
}

void Interface::updateTime()
{
    mRealSec += mGeneralData.pollInterval;
    if ( mRealSec < 1.0 )
        return;

    mUptime += trunc( mRealSec );
    mRealSec -= trunc( mRealSec );

    time_t updays = mUptime / 86400;

    mUptimeString = i18np("1 day, ","%1 days, ",updays);

    mUptime -= 86400 * updays; // we only want the seconds of today
    int hrs = mUptime / 3600;
    int mins = ( mUptime - hrs * 3600 ) / 60;
    int secs = mUptime - hrs * 3600 - mins * 60;
    QString time;
    time.sprintf( "%02d:%02d:%02d", hrs, mins, secs );
    mUptimeString += time;
}

void Interface::startStatistics()
{
    mStatistics = new InterfaceStatistics( this );
    connect( mStatistics, SIGNAL( warnMonthlyTraffic( quint64 ) ),
             this, SLOT( warnMonthlyTraffic( quint64 ) ) );
    if ( mStatusDialog != 0 )
    {
        connect( mStatistics, SIGNAL( currentEntryChanged() ),
                 mStatusDialog, SLOT( statisticsChanged() ) );
        mStatusDialog->statisticsChanged();
    }
}

void Interface::stopStatistics()
{
    // this will close an open statistics dialog
    delete mStatisticsDialog;
    mStatisticsDialog = 0;

    delete mStatistics;
    mStatistics = 0;
}

void Interface::warnMonthlyTraffic( quint64 traffic )
{
    QString title = mSettings.alias;
    if ( title.isEmpty() )
        title = mName;

    KNotification::event( "exceededMonthlyTraffic",
                          i18n( "%1: Monthly traffic limit exceeded (currently %2)", title, KIO::convertSize( traffic ) )
                        );
}

// taken from ksystemtray.cpp
void Interface::activateOrHide( QWidget* widget, bool onlyActivate )
{
    if ( !widget )
	return;

    KWindowInfo info1 = KWindowSystem::windowInfo( widget->winId(), NET::XAWMState | NET::WMState );
    // mapped = visible (but possibly obscured)
    bool mapped = (info1.mappingState() == NET::Visible) && !info1.isMinimized();
//    - not mapped -> show, raise, focus
//    - mapped
//        - obscured -> raise, focus
//        - not obscured -> hide
    if( !mapped )
    {
        KWindowSystem::setOnDesktop( widget->winId(), KWindowSystem::currentDesktop() );
        widget->show();
        widget->raise();
    }
    else
    {
        QListIterator< WId > it (KWindowSystem::stackingOrder());
        it.toBack();
        while( it.hasPrevious() )
        {
            WId id = it.previous();
            if( id == widget->winId() )
                break;
            KWindowInfo info2 = KWindowSystem::windowInfo( id,
                NET::WMGeometry | NET::XAWMState | NET::WMState | NET::WMWindowType );
            if( info2.mappingState() != NET::Visible )
                continue; // not visible on current desktop -> ignore
            if( !info2.geometry().intersects( widget->geometry()))
                continue; // not obscuring the window -> ignore
            if( !info1.hasState( NET::KeepAbove ) && info2.hasState( NET::KeepAbove ))
                continue; // obscured by window kept above -> ignore
            NET::WindowType type = info2.windowType( NET::NormalMask | NET::DesktopMask
                | NET::DockMask | NET::ToolbarMask | NET::MenuMask | NET::DialogMask
                | NET::OverrideMask | NET::TopMenuMask | NET::UtilityMask | NET::SplashMask );
            if( type == NET::Dock || type == NET::TopMenu )
                continue; // obscured by dock or topmenu -> ignore
            widget->raise();
            KWindowSystem::activateWindow( widget->winId());
            return;
        }
        if ( !onlyActivate )
        {
            widget->hide();
        }
    }
}

#include "interface.moc"
