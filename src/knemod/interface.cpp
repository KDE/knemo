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
#include <kdeversion.h>
#include <kio/global.h>

#include "backends/backendbase.h"
#include "global.h"
#include "utils.h"
#include "interface.h"
#include "interfaceplotterdialog.h"
#include "interfacestatistics.h"
#include "interfacestatusdialog.h"
#include "interfacestatisticsdialog.h"

Interface::Interface( const QString &ifname,
                      const BackendData* data )
    : QObject(),
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
      mBackendData( data )
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
    mSettings.iconFont = interfaceGroup.readEntry( conf_iconFont, s.iconFont );
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
    mSettings.warnRules.clear();
    int warnRuleCount = interfaceGroup.readEntry( conf_warnRules, 0 );
    for ( int i = 0; i < warnRuleCount; ++i )
    {
        group = QString( "%1%2 #%3" ).arg( confg_warnRule ).arg( mName ).arg( i );
        if ( config->hasGroup( group ) )
        {
            KConfigGroup warnGroup( config, group );
            WarnRule warn;
            warn.periodUnits = clamp<int>(warnGroup.readEntry( conf_warnPeriodUnits, warn.periodUnits ), KNemoStats::Hour, KNemoStats::Year );
            warn.periodCount = clamp<int>(warnGroup.readEntry( conf_warnPeriodCount, warn.periodUnits ), 1, 1000 );
            warn.trafficType = clamp<int>(warnGroup.readEntry( conf_warnTrafficType, warn.trafficType ), KNemoStats::PeakOffpeak, KNemoStats::Offpeak );
            warn.trafficDirection = clamp<int>(warnGroup.readEntry( conf_warnTrafficDirection, warn.trafficDirection ), KNemoStats::TrafficIn, KNemoStats::TrafficTotal );
            warn.trafficUnits = clamp<int>(warnGroup.readEntry( conf_warnTrafficUnits, warn.trafficUnits ), KNemoStats::UnitB, KNemoStats::UnitG );
            warn.threshold = clamp<double>(warnGroup.readEntry( conf_warnThreshold, warn.threshold ), 0.0, 9999.0 );
            warn.customText = warnGroup.readEntry( conf_warnCustomText, warn.customText ).trimmed();

            mSettings.warnRules << warn;
        }
    }

    QString defaultCal;
    if ( KDE::versionMajor() >= 4 && KDE::versionMinor() >= 4 )
        defaultCal = KGlobal::locale()->calendarType();
    else
        defaultCal = "gregorian";
    mSettings.calendar = interfaceGroup.readEntry( conf_calendar, defaultCal );

    mSettings.statsRules.clear();
    int statsRuleCount = interfaceGroup.readEntry( conf_statsRules, 0 );
    KCalendarSystem *testCal = KCalendarSystem::create( mSettings.calendar );
    for ( int i = 0; i < statsRuleCount; ++i )
    {
        group = QString( "%1%2 #%3" ).arg( confg_statsRule ).arg( mName ).arg( i );
        if ( config->hasGroup( group ) )
        {
            KConfigGroup statsGroup( config, group );
            StatsRule rule;

            rule.startDate = statsGroup.readEntry( conf_statsStartDate, QDate() );
            rule.periodUnits = clamp<int>(statsGroup.readEntry( conf_statsPeriodUnits, rule.periodUnits ), KNemoStats::Day, KNemoStats::Year );
            rule.periodCount = clamp<int>(statsGroup.readEntry( conf_statsPeriodCount, rule.periodCount ), 1, 1000 );
            rule.logOffpeak = statsGroup.readEntry( conf_logOffpeak,rule.logOffpeak );
            rule.offpeakStartTime = QTime::fromString( statsGroup.readEntry( conf_offpeakStartTime, rule.offpeakStartTime.toString( Qt::ISODate ) ), Qt::ISODate );
            rule.offpeakEndTime = QTime::fromString( statsGroup.readEntry( conf_offpeakEndTime, rule.offpeakEndTime.toString( Qt::ISODate ) ), Qt::ISODate );
            rule.weekendIsOffpeak = statsGroup.readEntry( conf_weekendIsOffpeak, rule.weekendIsOffpeak );
            rule.weekendDayStart = clamp<int>(statsGroup.readEntry( conf_weekendDayStart, rule.weekendDayStart ), 1, testCal->daysInWeek( QDate::currentDate() ) );
            rule.weekendDayEnd = clamp<int>(statsGroup.readEntry( conf_weekendDayEnd, rule.weekendDayEnd ), 1, testCal->daysInWeek( QDate::currentDate() ) );
            rule.weekendTimeStart = QTime::fromString( statsGroup.readEntry( conf_weekendTimeStart, rule.weekendTimeStart.toString( Qt::ISODate ) ), Qt::ISODate );
            rule.weekendTimeEnd = QTime::fromString( statsGroup.readEntry( conf_weekendTimeEnd, rule.weekendTimeEnd.toString( Qt::ISODate ) ), Qt::ISODate );
            if ( rule.isValid( testCal ) )
            {
                mSettings.statsRules << rule;
            }
        }
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

    if ( mStatusDialog )
        mStatusDialog->configChanged();
    if ( mStatisticsDialog != 0 )
        mStatisticsDialog->configChanged();
    if ( mPlotterDialog )
        mPlotterDialog->useBitrate( generalSettings->useBitrate );
}

void Interface::processUpdate()
{
    mPreviousState = mState;
    unsigned int trafficThreshold = mSettings.trafficThreshold;
    mState = mBackendData->status;

    int units = 1;
    if ( generalSettings->useBitrate )
        units = 8;
    mRxRate = mBackendData->incomingBytes * units / generalSettings->pollInterval;
    mTxRate = mBackendData->outgoingBytes * units / generalSettings->pollInterval;
    mRxRateStr = formattedRate( mRxRate, generalSettings->useBitrate );
    mTxRateStr = formattedRate( mTxRate, generalSettings->useBitrate );

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
            if ( mBackendData->interfaceType == KNemoIface::PPP )
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
        backend->clearTraffic( mName );
        resetUptime();
    }

    if ( mPreviousState != mState )
        mIcon.updateTrayStatus();

    if ( mPlotterDialog )
        mPlotterDialog->updatePlotter( mRxRate, mTxRate );

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
    mRxRateStr = formattedRate( mRxRate, generalSettings->useBitrate );
    mTxRateStr = formattedRate( mTxRate, generalSettings->useBitrate );
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
    mRealSec += generalSettings->pollInterval;
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
    connect( mStatistics, SIGNAL( warnTraffic( QString, quint64, quint64 ) ),
             this, SLOT( warnTraffic( QString, quint64, quint64 ) ) );
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

void Interface::warnTraffic( QString warnText, quint64 threshold, quint64 current )
{
    if ( !warnText.isEmpty() )
    {
        warnText = warnText.replace( QRegExp("%i"), mName );
        warnText = warnText.replace( QRegExp("%a"), mSettings.alias );
        warnText = warnText.replace( QRegExp("%t"), KIO::convertSize( threshold ) );
        warnText = warnText.replace( QRegExp("%c"), KIO::convertSize( threshold ) );
    }
    else
    {
        warnText = i18n( "<table><tr><td style='padding-right:0.2em;'>%1:</td>"
                                "<td>Exceeded traffic limit of %2\n"
                                "(currently %3)</td></tr></table>",
                                mName,
                                KIO::convertSize( threshold ),
                                KIO::convertSize( current ) );
    }
    KNotification::event( "exceededTraffic", warnText );
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
