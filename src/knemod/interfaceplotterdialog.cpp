/* This file is part of KNemo
   Copyright (C) 2004, 2006 Percy Leonhardt <percy@eris23.de>
   Copyright (C) 2009 John Stamp <jstamp@users.sourceforge.net>

   Portions adapted from FancyPlotter.cc in KSysGuard
   Copyright (c) 1999 - 2002 Chris Schlaeger <cs@kde.org>

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

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <KApplication>
#include <KLocale>

#include "interfaceplotterdialog.h"
#include "signalplotter.h"

class FancyPlotterLabel : public QWidget
{
public:
    FancyPlotterLabel(QWidget *parent) : QWidget(parent)
    {
        QBoxLayout *layout = new QHBoxLayout(this);
        label = new QLabel(this);
        layout->addWidget(label);
        value = new QLabel(this);
        layout->addWidget(value);
        layout->addStretch(1);
    }
    void setLabel(const QString &name, const QColor &color, const QChar &indicatorSymbol)
    {
        if ( kapp->layoutDirection() == Qt::RightToLeft )
	            label->setText(QString("<qt>: ") + name + " <font color=\"" + color.name() + "\">" + indicatorSymbol + "</font>");
        else
            label->setText(QString("<qt><font color=\"") + color.name() + "\">" + indicatorSymbol + "</font> " + name + " :");
    }
    QLabel *label;
    QLabel *value;
};


InterfacePlotterDialog::InterfacePlotterDialog( const PlotterSettings &plotterSettings, QString name )
    : KDialog(),
      mConfig( KGlobal::config() ),
      mSetPos( true ),
      mWasShown( false ),
      mPlotterSettings( plotterSettings ),
      mName( name ),
      mOutgoingPos( 0 ),
      mIncomingPos( 0 ),
      mVisibleBeams( NONE )
{
    setCaption( mName + " " + i18n( "Traffic" ) );
    setButtons( Close );

    mIndicatorSymbol = '#';
    QFontMetrics fm(font());
    if (fm.inFont(QChar(0x25CF)))
        mIndicatorSymbol = QChar(0x25CF);
    
    QBoxLayout *layout = new QVBoxLayout();
    mainWidget()->setLayout( layout );
    layout->setSpacing(0);
    mPlotter = new KSignalPlotter( this );
    mPlotter->setThinFrame( true );
    mPlotter->setShowAxis( true );
    int axisTextWidth = fontMetrics().width(i18nc("Largest axis title", "99999 XXXX"));
    mPlotter->setMaxAxisTextWidth( axisTextWidth );
    layout->addWidget(mPlotter);

    /* Create a set of labels underneath the graph. */
    QBoxLayout *outerLabelLayout = new QHBoxLayout;
    outerLabelLayout->setSpacing(0);
    layout->addLayout(outerLabelLayout);
 
    /* create a spacer to fill up the space up to the start of the graph */
    outerLabelLayout->addItem(new QSpacerItem(axisTextWidth, 0, QSizePolicy::Preferred));

    mLabelLayout = new QHBoxLayout;
    outerLabelLayout->addLayout(mLabelLayout);

    mReceivedLabel = new FancyPlotterLabel( this );
    mSentLabel = new FancyPlotterLabel( this );

    mLabelLayout->addWidget( mSentLabel );
    mLabelLayout->addWidget( mReceivedLabel );

    // Restore window size and position.
    KConfig *config = mConfig.data();
    KConfigGroup interfaceGroup( config, "Interface_" + mName );
    if ( interfaceGroup.hasKey( "PlotterPos" ) )
    {
        QPoint p = interfaceGroup.readEntry( "PlotterPos", QPoint() );
        // See comment in event()
        mSetPos = false;
        move( p );
    }
    if ( interfaceGroup.hasKey( "PlotterSize" ) )
    {
        QSize s = interfaceGroup.readEntry( "PlotterSize", QSize() );
        // A little hack so the plotter's data isn't chopped off the first time
        // the dialog appears
        mPlotter->resize( s );
        resize( s );
    }
    else
    {
        // HACK
        mPlotter->resize( 500, 350 );
        // Improve the chance that we have a decent sized dialog
        // the first time it's shown
        resize( 500, 350 );
    }
    configChanged();
}

InterfacePlotterDialog::~InterfacePlotterDialog()
{
    KConfig *config = mConfig.data();
    KConfigGroup interfaceGroup( config, "Interface_" + mName );
    interfaceGroup.writeEntry( "PlotterSize", size() );

    // If the dialog was never shown, then the position
    // will be wrong
    if ( mWasShown )
        interfaceGroup.writeEntry( "PlotterPos", pos() );

    config->sync();
}

bool InterfacePlotterDialog::event( QEvent *e )
{
    /* If we do not explicitly call size() and move() at least once then
     * hiding and showing the dialog will cause it to forget its previous
     * size and position. */
    switch ( e->type() )
    {
        case QEvent::Move:
            if ( mSetPos && !pos().isNull() )
            {
                mSetPos = false;
                move( pos() );
            }
            break;
        case QEvent::Show:
            mWasShown = true;
            break;
        default:
            ;;
    }

    return KDialog::event( e );
}

void InterfacePlotterDialog::updatePlotter( const double incomingBytes, const double outgoingBytes )
{
    QList<double> trafficList;
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

    if ( mPlotter->numBeams() > 0 )
    {
        for ( int beamId = 0; beamId < mPlotter->numBeams(); beamId++ )
        {
            QString lastValue = mPlotter->lastValueAsString(beamId);
            static_cast<FancyPlotterLabel *>((static_cast<QWidgetItem *>(mLabelLayout->itemAt(beamId)))->widget())->value->setText(lastValue);

        }
    }
}

void InterfacePlotterDialog::configChanged()
{
    QFont font = mPlotter->axisFont();
    font.setPointSize( mPlotterSettings.fontSize );
    QFontMetrics fm( font );
    int axisTextWidth = fm.width(i18nc("Largest axis title", "99999 XXXX"));
    mPlotter->setMaxAxisTextWidth( axisTextWidth );
    mPlotter->setAxisFont( font );
    if ( !mPlotterSettings.automaticDetection )
    {
        mPlotter->setMinValue( mPlotterSettings.minimumValue );
        mPlotter->setMaxValue( mPlotterSettings.maximumValue );
    }
    mPlotter->setHorizontalScale( mPlotterSettings.pixel );
    mPlotter->setVerticalLinesDistance( mPlotterSettings.distance );
    mPlotter->setShowAxis( mPlotterSettings.labels );
    mPlotter->setShowVerticalLines( mPlotterSettings.verticalLines );
    mPlotter->setShowHorizontalLines( mPlotterSettings.horizontalLines );
    mPlotter->setUseAutoRange( mPlotterSettings.automaticDetection );
    mPlotter->setVerticalLinesScroll( mPlotterSettings.verticalLinesScroll );
    mPlotter->setVerticalLinesColor( mPlotterSettings.colorVLines );
    mPlotter->setHorizontalLinesColor( mPlotterSettings.colorHLines );
    mPlotter->setBackgroundColor( mPlotterSettings.colorBackground );
    mPlotter->setFillOpacity( mPlotterSettings.opacity * 255 / 100.0 + 0.5 );

    // add or remove beams according to user settings
    VisibleBeams nextVisibleBeams = NONE;
    if ( mPlotterSettings.showOutgoing )
        nextVisibleBeams = (VisibleBeams) ( nextVisibleBeams | OUTGOING_TRAFFIC );
    if ( mPlotterSettings.showIncoming )
        nextVisibleBeams = (VisibleBeams) ( nextVisibleBeams | INCOMING_TRAFFIC );

    mSentLabel->setLabel( i18n( "%1 Sent Data", mName ), mPlotterSettings.colorOutgoing, mIndicatorSymbol);
    mReceivedLabel->setLabel( i18n( "%1 Received Data", mName ), mPlotterSettings.colorIncoming, mIndicatorSymbol);

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
            mPlotter->setBeamColor( mIncomingPos, mPlotterSettings.colorIncoming );
        }
        else if ( nextVisibleBeams == OUTGOING_TRAFFIC )
        {
            mPlotter->removeBeam( mIncomingPos );
            mOutgoingPos = 0;
            mPlotter->addBeam( mPlotterSettings.colorOutgoing );
        }
        else if ( nextVisibleBeams == INCOMING_TRAFFIC )
        {
            mPlotter->setBeamColor( mIncomingPos, mPlotterSettings.colorIncoming );
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
            mPlotter->setBeamColor( mOutgoingPos, mPlotterSettings.colorOutgoing );
        }
        else if ( nextVisibleBeams == INCOMING_TRAFFIC )
        {
            mPlotter->removeBeam( mOutgoingPos );
            mIncomingPos = 0;
            mPlotter->addBeam( mPlotterSettings.colorIncoming );
        }
        else if ( nextVisibleBeams == OUTGOING_TRAFFIC )
        {
            mPlotter->setBeamColor( mOutgoingPos, mPlotterSettings.colorOutgoing );
        }
        else if ( nextVisibleBeams == NONE )
        {
            mPlotter->removeBeam( mOutgoingPos );
        }
        break;
    case BOTH:
        if ( nextVisibleBeams == BOTH )
        {
            mPlotter->setBeamColor( mIncomingPos, mPlotterSettings.colorIncoming );
            mPlotter->setBeamColor( mOutgoingPos, mPlotterSettings.colorOutgoing );
        }
        else if ( nextVisibleBeams == OUTGOING_TRAFFIC )
        {
            mOutgoingPos = 0;
            mPlotter->removeBeam( mIncomingPos );
            mPlotter->setBeamColor( mOutgoingPos, mPlotterSettings.colorOutgoing );
        }
        else if ( nextVisibleBeams == INCOMING_TRAFFIC )
        {
            mIncomingPos = 0;
            mPlotter->removeBeam( mOutgoingPos );
            mPlotter->setBeamColor( mIncomingPos, mPlotterSettings.colorIncoming );
        }
        else if ( nextVisibleBeams == NONE )
        {
            mPlotter->removeBeam( 0 );
            mPlotter->removeBeam( 0 );
        }
        break;
    }

    if ( mPlotterSettings.bottomBar )
        switch ( nextVisibleBeams )
        {
            case BOTH:
                mSentLabel->show();
                mReceivedLabel->show();
                break;
            case OUTGOING_TRAFFIC:
                mSentLabel->show();
                mReceivedLabel->hide();
                break;
            case INCOMING_TRAFFIC:
                mSentLabel->hide();
                mReceivedLabel->show();
                break;
            case NONE:
                mSentLabel->hide();
                mReceivedLabel->hide();
                break;
        }
    else
    {
        mSentLabel->hide();
        mReceivedLabel->hide();
    }
    mVisibleBeams = nextVisibleBeams;
}

#include "interfaceplotterdialog.moc"
