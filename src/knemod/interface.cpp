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

#include <QTimer>

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
      mPlotterTimer( 0 ),
      mIcon( this ),
      mStatistics( 0 ),
      mStatusDialog( 0 ),
      mStatisticsDialog(  0 ),
      mPlotterDialog( 0 ),
      mBackendData( data ),
      mGeneralData( generalData )
{
    mPlotterDialog = new InterfacePlotterDialog( mName );
    mPlotterTimer = new QTimer();

    connect( mPlotterTimer, SIGNAL( timeout() ),
             this, SLOT( updatePlotter() ) );
    connect( &mIcon, SIGNAL( statisticsSelected() ),
             this, SLOT( showStatisticsDialog() ) );
}

Interface::~Interface()
{
    delete mStatusDialog;
    delete mPlotterDialog;
    if ( mPlotterTimer != 0 )
    {
        mPlotterTimer->stop();
        delete mPlotterTimer;
    }
    if ( mStatistics != 0 )
    {
        // this will also delete a possibly open statistics dialog
        stopStatistics();
    }
}

void Interface::configChanged()
{
    KSharedConfigPtr config = KGlobal::config();
    QString group( "Interface_" );
    group += mName;
    KConfigGroup interfaceGroup( config, group );
    mSettings.alias = interfaceGroup.readEntry( "Alias" ).trimmed();
    mSettings.iconTheme = interfaceGroup.readEntry( "IconSet", "monitor" );
    QStringList themeNames;
    QList<KNemoTheme> themes = findThemes();
    // Let's check that it's available
    foreach( KNemoTheme theme, themes )
        themeNames << theme.internalName;
    if ( !themeNames.contains( mSettings.iconTheme ) )
        mSettings.iconTheme = TEXT_THEME;
    mSettings.colorIncoming = interfaceGroup.readEntry( "ColorIncoming", QColor( 0x1889FF ) );
    mSettings.colorOutgoing = interfaceGroup.readEntry( "ColorOutgoing", QColor( 0xFF7F08 ) );
    KColorScheme scheme(QPalette::Active, KColorScheme::View);
    mSettings.colorDisabled = interfaceGroup.readEntry( "ColorDisabled", scheme.foreground( KColorScheme::InactiveText ).color() );
    mSettings.customCommands = interfaceGroup.readEntry( "CustomCommands", false );
    mSettings.hideWhenDisconnected = interfaceGroup.readEntry( "HideWhenNotAvailable",false );
    mSettings.hideWhenUnavailable = interfaceGroup.readEntry( "HideWhenNotExisting", false );
    mSettings.activateStatistics = interfaceGroup.readEntry( "ActivateStatistics", false );
    mSettings.trafficThreshold = clamp<int>(interfaceGroup.readEntry( "TrafficThreshold", 0 ), 0, 1000 );
    mSettings.warnThreshold = clamp<double>(interfaceGroup.readEntry( "BillingWarnThreshold", 0.0 ), 0.0, 9999.0 );
    mSettings.warnTotalTraffic = interfaceGroup.readEntry( "BillingWarnRxTx", false );

    // TODO: Some of the calendars are a bit buggy, so default to Gregorian for now
    //mSettings.calendar = interfaceGroup.readEntry( "Calendar", KGlobal::locale()->calendarType() );
    mSettings.calendar = interfaceGroup.readEntry( "Calendar", "gregorian" );

    KCalendarSystem* calendar = KCalendarSystem::create( mSettings.calendar );
    QDate startDate = QDate::currentDate().addDays( 1 - calendar->day( QDate::currentDate() ) );
    mSettings.billingStart = interfaceGroup.readEntry( "BillingStart", startDate );
    // No future start period
    if ( mSettings.billingStart > QDate::currentDate() )
        mSettings.billingStart = startDate;
    mSettings.billingMonths = clamp<int>(interfaceGroup.readEntry( "BillingMonths", 0 ), 0, 6 );
    if ( mSettings.customCommands )
    {
        int numCommands = interfaceGroup.readEntry( "NumCommands", 0 );
        for ( int i = 0; i < numCommands; i++ )
        {
            QString entry;
            InterfaceCommand cmd;
            entry = QString( "RunAsRoot%1" ).arg( i + 1 );
            cmd.runAsRoot = interfaceGroup.readEntry( entry, false );
            entry = QString( "Command%1" ).arg( i + 1 );
            cmd.command = interfaceGroup.readEntry( entry );
            entry = QString( "MenuText%1" ).arg( i + 1 );
            cmd.menuText = interfaceGroup.readEntry( entry );
            mSettings.commands.append( cmd );
        }
    }

    // This prevents needless regeneration of icon when first shown in tray
    if ( mState == KNemoIface::UnknownState )
    {
        mState = mBackendData->status;
        mPreviousState = mState;
    }
    mIcon.configChanged( mSettings.colorIncoming, mSettings.colorOutgoing, mSettings.colorDisabled );

    /*if ( mPlotterDialog != 0L )
    {
        mPlotterDialog->configChanged();
    }*/

    if ( mStatistics != 0 )
    {
        mStatistics->configChanged();
    }

    if ( mSettings.activateStatistics && mStatistics == 0 )
    {
        // user turned on statistics
        startStatistics();
    }
    else if ( !mSettings.activateStatistics && mStatistics != 0 )
    {
        // user turned off statistics
        stopStatistics();
    }

    if ( mStatusDialog )
    {
        mStatusDialog->setStatisticsGroupEnabled( mSettings.activateStatistics );
    }
}

void Interface::processUpdate()
{
    mPreviousState = mState;
    int trafficThreshold = mSettings.trafficThreshold;
    mState = mBackendData->status;

    QString title = mSettings.alias;
    if ( title.isEmpty() )
        title = mName;

    if ( mState & KNemoIface::Connected )
    {
        // the interface is connected, look for traffic
        if ( ( mBackendData->rxPackets - mBackendData->prevRxPackets ) > (unsigned int) trafficThreshold )
            mState |= KNemoIface::RxTraffic;
        if ( ( mBackendData->txPackets - mBackendData->prevTxPackets ) > (unsigned int) trafficThreshold )
            mState |= KNemoIface::TxTraffic;
    }

    if ( mStatistics != 0 )
    {
        // update the statistics
        if ( mBackendData->incomingBytes > 0 )
            mStatistics->addIncomingData( mBackendData->incomingBytes );
        if ( mBackendData->outgoingBytes > 0 )
            mStatistics->addOutgoingData( mBackendData->outgoingBytes );
    }

    backend->updatePackets( mName );

    if ( mState & KNemoIface::Connected &&
         mPreviousState < KNemoIface::Connected )
    {
        setStartTime();
        QString connectedStr = i18n( "Connected" );
        if ( mBackendData->isWireless )
            connectedStr = i18n( "Connected to %1", mBackendData->essid );
        if ( mPreviousState != KNemoIface::UnknownState )
            KNotification::event( "connected",
                                  title + ": " + connectedStr );
        if ( mStatusDialog )
            mStatusDialog->enableNetworkGroups();
    }
    else if ( mState == KNemoIface::Available )
    {
        if ( mPreviousState > KNemoIface::Available )
        {
            KNotification::event( "disconnected",
                                  title + ": " + i18n( "Disconnected" ) );
            if ( mStatusDialog )
                mStatusDialog->disableNetworkGroups();
            if ( mType == KNemoIface::PPP )
                backend->clearTraffic( mName );
        }
        else if ( mPreviousState < KNemoIface::Available )
        {
            if ( mPreviousState != KNemoIface::UnknownState )
                KNotification::event( "available",
                                      title + ": " + i18n( "Available" ) );
        }
    }
    else if ( mState == KNemoIface::Unavailable &&
              mPreviousState > KNemoIface::Unavailable )
    {
        KNotification::event( "unavailable",
                              title + ": " + i18n( "Unavailable" ) );
        if ( mStatusDialog )
            mStatusDialog->disableNetworkGroups();
        if ( mType == KNemoIface::PPP )
            backend->clearTraffic( mName );
    }

    if ( mPreviousState != mState )
        mIcon.updateTrayStatus();

    // The tooltip and status dialog always get updated
    updateDetails();
}

void Interface::setStartTime()
{
    mUptime = 0;
    mUptimeString = "00:00:00";
    if ( !mPlotterTimer->isActive() )
        mPlotterTimer->start( 1000 );
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
    if ( mPlotterDialog->isVisible() && !mPlotterTimer->isActive() )
        mPlotterTimer->start( 1000 );
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
        connect( mStatistics, SIGNAL( dayStatisticsChanged() ),
                 mStatisticsDialog, SLOT( updateDays() ) );
        connect( mStatistics, SIGNAL( weekStatisticsChanged() ),
                 mStatisticsDialog, SLOT( updateWeeks() ) );
        connect( mStatistics, SIGNAL( monthStatisticsChanged( bool ) ),
                 mStatisticsDialog, SLOT( updateMonths( bool ) ) );
        connect( mStatistics, SIGNAL( yearStatisticsChanged() ),
                 mStatisticsDialog, SLOT( updateYears() ) );
        connect( mStatistics, SIGNAL( currentEntryChanged() ),
                 mStatisticsDialog, SLOT( updateCurrentEntry() ) );
        connect( mStatisticsDialog, SIGNAL( clearStatistics() ),
                 mStatistics, SLOT( clearStatistics() ) );

        /* We need to show the dialog before we update the stats.  That way
         * the viewport doesn't get messed up when we scroll to the most recent
         * item. */
        mStatisticsDialog->show();

        mStatisticsDialog->updateDays();
        mStatisticsDialog->updateWeeks();
        mStatisticsDialog->updateMonths( false );
        mStatisticsDialog->updateYears();
    }
    else
        mStatisticsDialog->show();
}

void Interface::updateDetails()
{
    mUptime += mGeneralData.pollInterval;
    QString uptime;
    if ( mBackendData->status & KNemoIface::Connected )
    {
        time_t upsecs = mUptime;
        time_t updays = upsecs / 86400;

        uptime = i18np("1 day, ","%1 days, ",updays);

        upsecs -= 86400 * updays; // we only want the seconds of today
        int hrs = upsecs / 3600;
        int mins = ( upsecs - hrs * 3600 ) / 60;
        int secs = upsecs - hrs * 3600 - mins * 60;
        QString time;
        time.sprintf( "%02d:%02d:%02d", hrs, mins, secs );
        uptime += time;
    }
    mUptimeString = uptime;

    mIcon.updateToolTip();
    if ( mStatusDialog )
        mStatusDialog->updateDialog();
}

void Interface::updatePlotter()
{
    if ( mPlotterDialog )
    {
        double outgoingBytes = mBackendData->outgoingBytes / 1024.0 / (double) mGeneralData.pollInterval;
        double incomingBytes = mBackendData->incomingBytes / 1024.0 / (double) mGeneralData.pollInterval;
        mPlotterDialog->updatePlotter( incomingBytes, outgoingBytes );
    }
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

    mStatistics->saveStatistics();

    delete mStatistics;
    mStatistics = 0;
}

void Interface::warnMonthlyTraffic( quint64 traffic )
{
    QString title = mSettings.alias;
    if ( title.isEmpty() )
        title = mName;

    KNotification::event( "exceededMonthlyTraffic",
                          title + ": " +
                          i18n( "Monthly traffic limit exceeded (currently %1)" ).arg( KIO::convertSize( traffic ) )
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
