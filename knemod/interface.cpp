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
#include <qpixmap.h>

#include <kwin.h>
#include <kdebug.h>
#include <klocale.h>
#include <kwinmodule.h>
#include <kiconloader.h>

#include "interface.h"
#include "signalplotter.h"
#include "interfacestatistics.h"
#include "interfacestatusdialog.h"
#include "interfacestatisticsdialog.h"

Interface::Interface( QString ifname,
                      const GeneralData& generalData,
                      const PlotterSettings& plotterSettings )
    : QObject(),
      mType( UNKNOWN_TYPE ),
      mState( UNKNOWN_STATE ),
      mOutgoingPos( 0 ),
      mIncomingPos( 0 ),
      mName( ifname ),
      mPlotterTimer( 0 ),
      mIcon( this ),
      mStatistics( 0 ),
      mStatusDialog( 0 ),
      mStatisticsDialog(  0 ),
      mPlotter( 0 ),
      mVisibleBeams( NONE ),
      mGeneralData( generalData ),
      mPlotterSettings( plotterSettings )
{
    connect( &mMonitor, SIGNAL( statusChanged( int ) ),
             &mIcon, SLOT( updateStatus( int ) ) );
    connect( &mMonitor, SIGNAL( available( int ) ),
             &mIcon, SLOT( updateTrayStatus( int ) ) );
    connect( &mMonitor, SIGNAL( notAvailable( int ) ),
             &mIcon, SLOT( updateTrayStatus( int ) ) );
    connect( &mMonitor, SIGNAL( notExisting( int ) ),
             &mIcon, SLOT( updateTrayStatus( int ) ) );
    connect( &mMonitor, SIGNAL( available( int ) ),
             this, SLOT( setStartTime( int ) ) );
    connect( &mIcon, SIGNAL( statisticsSelected() ),
             this, SLOT( showStatisticsDialog() ) );
}

Interface::~Interface()
{
    if ( mStatusDialog != 0 )
    {
        delete mStatusDialog;
    }
    if ( mPlotter != 0 )
    {
        delete mPlotter;
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
    mIcon.updateToolTip();
    mIcon.updateMenu();

    if ( mPlotter != 0L )
    {
        configurePlotter();
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
    mStartTime.setDate( QDate::currentDate() );
    mStartTime.setTime( QTime::currentTime() );
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
    // No plotter, create it.
    if ( mPlotter == 0L )
    {
        mPlotter = new SignalPlotter( 0L, mName.local8Bit() );
        mPlotter->setIcon( SmallIcon( "knemo" ) );
        mPlotter->setCaption( mName + " " + i18n( "Traffic" ) );
        mPlotter->setTitle( mName );
        configurePlotter();
        activateOrHide( mPlotter, true );

        mPlotterTimer = new QTimer();
        connect( mPlotterTimer, SIGNAL( timeout() ),
                 this, SLOT( updatePlotter() ) );
        mPlotterTimer->start( 1000 );
    }
    else
    {
        if ( wasMiddleButton )
        {
            // Toggle the signal plotter.
            activateOrHide( mPlotter );
        }
        else
        {
            // Called from the context menu, show the dialog.
            activateOrHide( mPlotter, true );
        }
    }
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

        mStatisticsDialog->updateDays();
        mStatisticsDialog->updateMonths();
        mStatisticsDialog->updateYears();
    }
    mStatisticsDialog->show();
}

void Interface::updatePlotter()
{
    if ( mPlotter )
    {
        double outgoingBytes = mData.outgoingBytes / 1024.0 / (double) mGeneralData.pollInterval;
        double incomingBytes = mData.incomingBytes / 1024.0 / (double) mGeneralData.pollInterval;

        QValueList<double> trafficList;
        switch ( mVisibleBeams )
        {
        case BOTH:
            if ( mIncomingPos == 1 )
            {
                trafficList.append( outgoingBytes );
                trafficList.append( incomingBytes );
            }
            else
            {
                trafficList.append( incomingBytes );
                trafficList.append( outgoingBytes );
            }
            mPlotter->addSample( trafficList );
            break;
        case INCOMING_TRAFFIC:
            trafficList.append( incomingBytes );
            mPlotter->addSample( trafficList );
            break;
        case OUTGOING_TRAFFIC:
            trafficList.append( outgoingBytes );
            mPlotter->addSample( trafficList );
            break;
        case NONE:
            break;
        }
    }
}

void Interface::configurePlotter()
{
    mPlotter->setFontSize( mPlotterSettings.fontSize );
    if ( !mPlotterSettings.automaticDetection )
    {
        mPlotter->setMinValue( mPlotterSettings.minimumValue );
        mPlotter->setMaxValue( mPlotterSettings.maximumValue );
    }
    mPlotter->setHorizontalScale( mPlotterSettings.pixel );
    mPlotter->setHorizontalLinesCount( mPlotterSettings.count );
    mPlotter->setVerticalLinesDistance( mPlotterSettings.distance );
    mPlotter->setShowLabels( mPlotterSettings.labels );
    mPlotter->setShowTopBar( mPlotterSettings.topBar );
    mPlotter->setShowVerticalLines( mPlotterSettings.verticalLines );
    mPlotter->setShowHorizontalLines( mPlotterSettings.horizontalLines );
    mPlotter->setUseAutoRange( mPlotterSettings.automaticDetection );
    mPlotter->setVerticalLinesScroll( mPlotterSettings.verticalLinesScroll );
    mPlotter->setVerticalLinesColor( mPlotterSettings.colorVLines );
    mPlotter->setHorizontalLinesColor( mPlotterSettings.colorHLines );
    mPlotter->setBackgroundColor( mPlotterSettings.colorBackground );

    // add or remove beams according to user settings
    VisibleBeams nextVisibleBeams = NONE;
    if ( mPlotterSettings.showOutgoing )
        nextVisibleBeams = (VisibleBeams) ( nextVisibleBeams | OUTGOING_TRAFFIC );
    if ( mPlotterSettings.showIncoming )
        nextVisibleBeams = (VisibleBeams) ( nextVisibleBeams | INCOMING_TRAFFIC );

    QValueList<QColor>& colors = mPlotter->beamColors();
    switch( mVisibleBeams )
    {
    case NONE:
        if ( nextVisibleBeams == BOTH )
        {
            mOutgoingPos = 0;
            mPlotter->addBeam( mPlotterSettings.colorOutgoing );
            mIncomingPos = 1;
            mPlotter->addBeam( mPlotterSettings.colorIncoming );
        }
        else if ( nextVisibleBeams == OUTGOING_TRAFFIC )
        {
            mOutgoingPos = 0;
            mPlotter->addBeam( mPlotterSettings.colorOutgoing );
        }
        else if ( nextVisibleBeams == INCOMING_TRAFFIC )
        {
            mIncomingPos = 0;
            mPlotter->addBeam( mPlotterSettings.colorIncoming );
        }
        break;
    case INCOMING_TRAFFIC:
        if ( nextVisibleBeams == BOTH )
        {
            mOutgoingPos = 1;
            mPlotter->addBeam( mPlotterSettings.colorOutgoing );
        }
        else if ( nextVisibleBeams == OUTGOING_TRAFFIC )
        {
            mPlotter->removeBeam( mIncomingPos );
            mOutgoingPos = 0;
            mPlotter->addBeam( mPlotterSettings.colorOutgoing );
        }
        else if ( nextVisibleBeams == INCOMING_TRAFFIC )
        {
            colors[mIncomingPos] = ( mPlotterSettings.colorIncoming );
        }
        else if ( nextVisibleBeams == NONE )
        {
            mPlotter->removeBeam( mIncomingPos );
        }
        break;
    case OUTGOING_TRAFFIC:
        if ( nextVisibleBeams == BOTH )
        {
            mIncomingPos = 1;
            mPlotter->addBeam( mPlotterSettings.colorIncoming );
        }
        else if ( nextVisibleBeams == INCOMING_TRAFFIC )
        {
            mPlotter->removeBeam( mOutgoingPos );
            mIncomingPos = 0;
            mPlotter->addBeam( mPlotterSettings.colorIncoming );
        }
        else if ( nextVisibleBeams == OUTGOING_TRAFFIC )
        {
            colors[mOutgoingPos] = ( mPlotterSettings.colorOutgoing );
        }
        else if ( nextVisibleBeams == NONE )
        {
            mPlotter->removeBeam( mOutgoingPos );
        }
        break;
    case BOTH:
        if ( nextVisibleBeams == BOTH )
        {
            colors[mIncomingPos] = ( mPlotterSettings.colorIncoming );
            colors[mOutgoingPos] = ( mPlotterSettings.colorOutgoing );
        }
        else if ( nextVisibleBeams == OUTGOING_TRAFFIC )
        {
            mOutgoingPos = 0;
            mPlotter->removeBeam( mIncomingPos );
        }
        else if ( nextVisibleBeams == INCOMING_TRAFFIC )
        {
            mIncomingPos = 0;
            mPlotter->removeBeam( mOutgoingPos );
        }
        else if ( nextVisibleBeams == NONE )
        {
            mPlotter->removeBeam( 0 );
            mPlotter->removeBeam( 0 );
        }
        break;
    }
    mVisibleBeams = nextVisibleBeams;
    mPlotter->repaint();
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

    KWin::WindowInfo info1 = KWin::windowInfo( widget->winId(), NET::XAWMState | NET::WMState );
    // mapped = visible (but possibly obscured)
    bool mapped = (info1.mappingState() == NET::Visible) && !info1.isMinimized();
    //    - not mapped -> show, raise, focus
    //    - mapped
    //        - obscured -> raise, focus
    //        - not obscured -> hide
    if( !mapped )
    {
        KWin::setOnDesktop( widget->winId(), KWin::currentDesktop() );
        widget->show();
        widget->raise();
	KWin::activateWindow( widget->winId() );
    }
    else
    {
        KWinModule module;
        for( QValueList< WId >::ConstIterator it = module.stackingOrder().fromLast();
             it != module.stackingOrder().end() && (*it) != widget->winId();
             --it )
        {
            KWin::WindowInfo info2 = KWin::windowInfo( *it, (unsigned long)
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
            KWin::activateWindow( widget->winId());
            return;
        }
        if ( !onlyActivate )
        {
            widget->hide();
        }
    }
}

#include "interface.moc"
