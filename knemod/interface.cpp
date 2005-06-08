/* This file is part of KNemo
   Copyright (C) 2004 Percy Leonhardt <percy@eris23.de>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <qtimer.h>
#include <qpixmap.h>

#include <kdebug.h>
#include <kiconloader.h>

#include "interface.h"
#include "signalplotter.h"
#include "interfacestatusdialog.h"

Interface::Interface( QString ifname, const PlotterSettings& plotterSettings )
    : QObject(),
      mType( UNKNOWN_TYPE ),
      mState( UNKNOWN_STATE ),
      mOutgoingPos( 0 ),
      mIncomingPos( 0 ),
      mName( ifname ),
      mPlotterTimer( 0L ),
      mIcon( this ),
      mDialog( 0L ),
      mPlotter( 0L ),
      mVisibleBeams( NONE ),
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
    connect( &mMonitor, SIGNAL( notAvailable( int ) ),
             this, SLOT( resetDataCounter( int ) ) );
    connect( &mMonitor, SIGNAL( notExisting( int ) ),
             this, SLOT( resetDataCounter( int ) ) );
}

Interface::~Interface()
{
    if ( mDialog != 0L )
        delete mDialog;
    if ( mPlotter != 0L )
        delete mPlotter;
    if ( mPlotterTimer != 0L )
    {
        mPlotterTimer->stop();
        delete mPlotterTimer;
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
        configurePlotter();
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

void Interface::resetDataCounter( int )
{
    mData.rxBytes = 0;
    mData.txBytes = 0;
    mData.prevRxBytes = 0;
    mData.prevTxBytes = 0;
}

void Interface::showStatusDialog()
{
    /* Toggle the status dialog.
     * First click will show the status dialog, second will hide it.
     */
    if ( mDialog == 0L )
    {
        mDialog = new InterfaceStatusDialog( this );
        connect( &mMonitor, SIGNAL( available( int ) ),
                 mDialog, SLOT( enableNetworkTabs( int ) ) );
        connect( &mMonitor, SIGNAL( notAvailable( int ) ),
                 mDialog, SLOT( disableNetworkTabs( int ) ) );
        connect( &mMonitor, SIGNAL( notExisting( int ) ),
                 mDialog, SLOT( disableNetworkTabs( int ) ) );
        mDialog->show();
    }
    else
    {
        // Toggle the signal plotter.
        if ( mDialog->isHidden() )
            mDialog->show();
        else
        {
            if ( mDialog->isActiveWindow() )
                mDialog->hide();
            else
            {
                mDialog->raise();
                mDialog->setActiveWindow();
            }
        }
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
        mPlotter->show();

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
            if ( mPlotter->isHidden() )
                mPlotter->show();
            else
            {
                if ( mPlotter->isActiveWindow() )
                    mPlotter->hide();
                else
                {
                    mPlotter->raise();
                    mPlotter->setActiveWindow();
                }
            }
        }
        else
        {
            // Called from the context menu, show the dialog.
            if ( mPlotter->isHidden() )
                mPlotter->show();
            else
            {
                mPlotter->raise();
                mPlotter->setActiveWindow();
            }
        }
    }
}

void Interface::updatePlotter()
{
    if ( mPlotter )
    {
        QValueList<double> trafficList;
        switch ( mVisibleBeams )
        {
        case BOTH:
            if ( mIncomingPos == 1 )
            {
                trafficList.append( (double) mData.outgoingBytes / 1024 );
                trafficList.append( (double) mData.incomingBytes / 1024 );
            }
            else
            {
                trafficList.append( (double) mData.incomingBytes / 1024 );
                trafficList.append( (double) mData.outgoingBytes / 1024 );
            }
            mPlotter->addSample( trafficList );
            break;
        case INCOMING_TRAFFIC:
            trafficList.append( (double) mData.incomingBytes / 1024 );
            mPlotter->addSample( trafficList );
            break;
        case OUTGOING_TRAFFIC:
            trafficList.append( (double) mData.outgoingBytes / 1024 );
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

#include "interface.moc"
