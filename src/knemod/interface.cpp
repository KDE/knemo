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

#include <KWindowSystem>

#include "interface.h"
#include "interfaceplotterdialog.h"
#include "interfacestatistics.h"
#include "interfacestatusdialog.h"
#include "interfacestatisticsdialog.h"

Interface::Interface( QString ifname,
                      const GeneralData& generalData,
                      const PlotterSettings& plotterSettings )
    : QObject(),
      mType( UNKNOWN_TYPE ),
      mState( UNKNOWN_STATE ),
      mName( ifname ),
      mPlotterTimer( 0 ),
      mIcon( this ),
      mStatistics( 0 ),
      mStatusDialog( 0 ),
      mStatisticsDialog(  0 ),
      mPlotterDialog( 0 ),
      mGeneralData( generalData ),
      mPlotterSettings( plotterSettings )
{
    mPlotterDialog = new InterfacePlotterDialog( mPlotterSettings, mName );
    mPlotterTimer = new QTimer();

    connect( mPlotterTimer, SIGNAL( timeout() ),
             this, SLOT( updatePlotter() ) );
    connect( &mMonitor, SIGNAL( statusChanged( int ) ),
             &mIcon, SLOT( updateStatus( int ) ) );
    connect( &mMonitor, SIGNAL( available( int ) ),
             &mIcon, SLOT( updateTrayStatus( int ) ) );
    connect( &mMonitor, SIGNAL( notAvailable( int ) ),
             &mIcon, SLOT( updateTrayStatus( int ) ) );
    connect( &mMonitor, SIGNAL( notExisting( int ) ),
             &mIcon, SLOT( updateTrayStatus( int ) ) );
    connect( &mMonitor, SIGNAL( updateDetails() ),
             this, SLOT( updateDetails() ) );
    connect( &mMonitor, SIGNAL( available( int ) ),
             this, SLOT( setStartTime( int ) ) );
    connect( &mMonitor, SIGNAL( statusChanged( int ) ),
             this, SLOT( resetData( int ) ) );
    connect( &mIcon, SIGNAL( statisticsSelected() ),
             this, SLOT( showStatisticsDialog() ) );
}

Interface::~Interface()
{
    if ( mStatusDialog != 0 )
    {
        delete mStatusDialog;
    }
    if ( mPlotterDialog != 0 )
    {
        delete mPlotterDialog;
    }
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
    // UNKNOWN_STATE to avoid notification
    mIcon.updateTrayStatus( UNKNOWN_STATE );
    // handle changed iconset by user
    mIcon.updateStatus( mState );
    mIcon.updateMenu();

    if ( mPlotterDialog != 0L )
    {
        mPlotterDialog->configChanged();
    }

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

void Interface::activateMonitor()
{
    mMonitor.checkStatus( this );
}

void Interface::setStartTime( int )
{
    mUptime = 0;
    mUptimeString = "00:00:00";
    if ( !mPlotterTimer->isActive() )
        mPlotterTimer->start( 1000 );
}

void Interface::showStatusDialog()
{
    // Toggle the status dialog.
    // First click will show the status dialog, second will hide it.
    if ( mStatusDialog == 0L )
    {
        mStatusDialog = new InterfaceStatusDialog( this );
        connect( &mMonitor, SIGNAL( available( int ) ),
                 mStatusDialog, SLOT( enableNetworkGroups( int ) ) );
        connect( &mMonitor, SIGNAL( notAvailable( int ) ),
                 mStatusDialog, SLOT( disableNetworkGroups( int ) ) );
        connect( &mMonitor, SIGNAL( notExisting( int ) ),
                 mStatusDialog, SLOT( disableNetworkGroups( int ) ) );
        if ( mStatistics != 0 )
        {
            connect( mStatistics, SIGNAL( currentEntryChanged() ),
                     mStatusDialog, SLOT( statisticsChanged() ) );
            mStatusDialog->statisticsChanged();
        }
        activateOrHide( mStatusDialog, true );
    }
    else
    {
        // Toggle the status dialog.
        activateOrHide( mStatusDialog );
    }
}

void Interface::showSignalPlotter( bool wasMiddleButton )
{
    if ( wasMiddleButton )
    {
        // Toggle the signal plotter.
        activateOrHide( mPlotterDialog );
    }
    else
    {
        // Called from the context menu, show the dialog.
        activateOrHide( mPlotterDialog, true );
    }
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
        connect( mStatistics, SIGNAL( monthStatisticsChanged() ),
                 mStatisticsDialog, SLOT( updateMonths() ) );
        connect( mStatistics, SIGNAL( yearStatisticsChanged() ),
                 mStatisticsDialog, SLOT( updateYears() ) );
        connect( mStatistics, SIGNAL( currentEntryChanged() ),
                 mStatisticsDialog, SLOT( updateCurrentEntry() ) );
        connect( mStatisticsDialog, SIGNAL( clearDailyStatisticsClicked() ),
                 mStatistics, SLOT( clearDayStatistics() ) );
        connect( mStatisticsDialog, SIGNAL( clearMonthlyStatisticsClicked() ),
                 mStatistics, SLOT( clearMonthStatistics() ) );
        connect( mStatisticsDialog, SIGNAL( clearYearlyStatisticsClicked() ),
                 mStatistics, SLOT( clearYearStatistics() ) );

        /* We need to show the dialog before we update the stats.  That way
         * the viewport doesn't get messed up when we scroll to the most recent
         * item. */
        mStatisticsDialog->show();

        mStatisticsDialog->updateDays();
        mStatisticsDialog->updateMonths();
        mStatisticsDialog->updateYears();
    }
    else
        mStatisticsDialog->show();
}

void Interface::resetData( int state )
{
    // For PPP interfaces we will reset all data to zero when the
    // interface gets disconnected. If the driver also resets its data
    // (like PPP seems to do) we will start from zero for every new
    // connection.
    if ( mType == PPP &&
         ( state == NOT_AVAILABLE ||
           state == NOT_EXISTING ) )
    {
        mData.prevTxBytes = mData.txBytes = 0;
        mData.prevRxBytes = mData.rxBytes = 0;
        mData.prevTxPackets = mData.txPackets = 0;
        mData.prevRxPackets = mData.rxPackets = 0;
    }
}

void Interface::updateDetails()
{
    InterfaceData &data = getData();

    mUptime += mGeneralData.pollInterval;
    QString uptime;
    if ( data.available )
    {
        time_t upsecs = mUptime;
        time_t updays = upsecs / 86400;

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
        double outgoingBytes = mData.outgoingBytes / 1024.0 / (double) mGeneralData.pollInterval;
        double incomingBytes = mData.incomingBytes / 1024.0 / (double) mGeneralData.pollInterval;
        mPlotterDialog->updatePlotter( incomingBytes, outgoingBytes );
    }
}

void Interface::startStatistics()
{
    mStatistics = new InterfaceStatistics( this );
    connect( &mMonitor, SIGNAL( incomingData( unsigned long ) ),
             mStatistics, SLOT( addIncomingData( unsigned long ) ) );
    connect( &mMonitor, SIGNAL( outgoingData( unsigned long ) ),
             mStatistics, SLOT( addOutgoingData( unsigned long ) ) );
    if ( mStatusDialog != 0 )
    {
        connect( mStatistics, SIGNAL( currentEntryChanged() ),
                 mStatusDialog, SLOT( statisticsChanged() ) );
        mStatusDialog->statisticsChanged();
    }

    mStatistics->loadStatistics();
}

void Interface::stopStatistics()
{
    if ( mStatisticsDialog != 0 )
    {
        // this will close an open statistics dialog
        delete mStatisticsDialog;
        mStatisticsDialog = 0;
    }

    mStatistics->saveStatistics();

    delete mStatistics;
    mStatistics = 0;
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
