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

#include <QMenu>
#include <KApplication>

#include "interfaceplotterdialog.h"
#include "utils.h"
#include "signalplotter.h"
#include "plotterconfigdialog.h"

static const char plot_pixel[] = "Pixel";
static const char plot_distance[] = "Distance";
static const char plot_fontSize[] = "FontSize";
static const char plot_minimumValue[] = "MinimumValue";
static const char plot_maximumValue[] = "MaximumValue";
static const char plot_labels[] = "Labels";
static const char plot_bottomBar[] = "BottomBar";
static const char plot_verticalLines[] = "VerticalLines";
static const char plot_horizontalLines[] = "HorizontalLines";
static const char plot_showIncoming[] = "ShowIncoming";
static const char plot_showOutgoing[] = "ShowOutgoing";
static const char plot_automaticDetection[] = "AutomaticDetection";
static const char plot_verticalLinesScroll[] = "VerticalLinesScroll";
static const char plot_colorVLines[] = "ColorVLines";
static const char plot_colorHLines[] = "ColorHLines";
static const char plot_colorIncoming[] = "ColorIncoming";
static const char plot_colorOutgoing[] = "ColorOutgoing";
static const char plot_colorBackground[] = "ColorBackground";
static const char plot_opacity[] = "Opacity";

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


InterfacePlotterDialog::InterfacePlotterDialog( QString name )
    : KDialog(),
      mConfig( KGlobal::config() ),
      mConfigDlg( 0 ),
      mSetPos( true ),
      mWasShown( false ),
      mName( name ),
      mOutgoingPos( 0 ),
      mIncomingPos( 0 ),
      mVisibleBeams( NONE )
{
    setCaption( mName + " " + i18n( "Traffic" ) );
    setButtons( Close );
    setContextMenuPolicy( Qt::DefaultContextMenu );

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
    KConfigGroup interfaceGroup( config, confg_interface + mName );
    if ( interfaceGroup.hasKey( conf_plotterPos ) )
    {
        QPoint p = interfaceGroup.readEntry( conf_plotterPos, QPoint() );
        // See comment in event()
        mSetPos = false;
        move( p );
    }
    if ( interfaceGroup.hasKey( conf_plotterSize ) )
    {
        QSize s = interfaceGroup.readEntry( conf_plotterSize, QSize() );
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
    loadConfig();
}

InterfacePlotterDialog::~InterfacePlotterDialog()
{
    KConfig *config = mConfig.data();
    KConfigGroup interfaceGroup( config, confg_interface + mName );
    interfaceGroup.writeEntry( conf_plotterSize, size() );

    // If the dialog was never shown, then the position
    // will be wrong
    if ( mWasShown )
        interfaceGroup.writeEntry( conf_plotterPos, pos() );

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
        case QEvent::MouseButtonPress:
            {
                QMouseEvent *m = static_cast<QMouseEvent *>(e);
                if ( m->button() == Qt::RightButton )
                {
                    showContextMenu( m->pos() );
                    return true;
                }
            }
        default:
            ;;
    }

    return KDialog::event( e );
}

void InterfacePlotterDialog::showContextMenu( const QPoint &pos )
{
    QMenu pm;
    QAction *action = 0;

    action = pm.addAction( i18n( "&Properties" ) );
    action->setData( 1 );
    action = pm.exec( mapToGlobal(pos) );

    if ( action )
    {
        switch ( action->data().toInt() )
        {
            case 1:
                configPlotter();
                break;
        }
    }
}

void InterfacePlotterDialog::configPlotter()
{
    if ( mConfigDlg )
        return;

    mConfigDlg = new PlotterConfigDialog( this, mName, &mSettings );
    connect( mConfigDlg, SIGNAL( finished() ), this, SLOT( configFinished() ) );
    connect( mConfigDlg, SIGNAL( saved() ), this, SLOT( saveConfig() ) );
    mConfigDlg->show();
}

void InterfacePlotterDialog::configFinished()
{
    mConfigDlg->delayedDestruct();
    mConfigDlg = 0;
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

void InterfacePlotterDialog::loadConfig()
{
    KSharedConfigPtr config = KGlobal::config();
    // Set the plotter widgets
    QString group = confg_plotter + mName;
    // Plotter
    PlotterSettings s;
    KConfigGroup plotterGroup( config, group );
    mSettings.pixel = clamp<int>(plotterGroup.readEntry( plot_pixel, s.pixel ), 1, 50 );
    mSettings.distance = clamp<int>(plotterGroup.readEntry( plot_distance, s.distance ), 10, 120 );
    mSettings.fontSize = clamp<int>(plotterGroup.readEntry( plot_fontSize, s.fontSize ), 5, 24 );
    mSettings.minimumValue = clamp<int>(plotterGroup.readEntry( plot_minimumValue, s.minimumValue ), 0, 49999 );
    mSettings.maximumValue = clamp<int>(plotterGroup.readEntry( plot_maximumValue, s.maximumValue ), 0, 50000 );
    mSettings.labels = plotterGroup.readEntry( plot_labels, s.labels );
    mSettings.bottomBar = plotterGroup.readEntry( plot_bottomBar, s.bottomBar );
    mSettings.showIncoming = plotterGroup.readEntry( plot_showIncoming, s.showIncoming );
    mSettings.showOutgoing = plotterGroup.readEntry( plot_showOutgoing, s.showOutgoing );
    mSettings.verticalLines = plotterGroup.readEntry( plot_verticalLines, s.verticalLines );
    mSettings.horizontalLines = plotterGroup.readEntry( plot_horizontalLines, s.horizontalLines );
    mSettings.automaticDetection = plotterGroup.readEntry( plot_automaticDetection, s.automaticDetection );
    mSettings.verticalLinesScroll = plotterGroup.readEntry( plot_verticalLinesScroll, s.verticalLinesScroll );
    mSettings.colorVLines = plotterGroup.readEntry( plot_colorVLines, s.colorVLines );
    mSettings.colorHLines = plotterGroup.readEntry( plot_colorHLines, s.colorHLines );
    mSettings.colorIncoming = plotterGroup.readEntry( plot_colorIncoming, s.colorIncoming );
    mSettings.colorOutgoing = plotterGroup.readEntry( plot_colorOutgoing, s.colorOutgoing );
    mSettings.colorBackground = plotterGroup.readEntry( plot_colorBackground, s.colorBackground );
    mSettings.opacity = clamp<int>(plotterGroup.readEntry( plot_opacity, s.opacity ), 0, 100 );
    configChanged();
}

void InterfacePlotterDialog::saveConfig()
{
    KSharedConfigPtr config = KGlobal::config();
    // Set the plotter widgets
    QString group = confg_plotter + mName;
    // Plotter
    KConfigGroup plotterGroup( config, group );
    plotterGroup.writeEntry( plot_pixel, mSettings.pixel );
    plotterGroup.writeEntry( plot_distance, mSettings.distance );
    plotterGroup.writeEntry( plot_fontSize, mSettings.fontSize );
    plotterGroup.writeEntry( plot_minimumValue, mSettings.minimumValue );
    plotterGroup.writeEntry( plot_maximumValue, mSettings.maximumValue );
    plotterGroup.writeEntry( plot_labels, mSettings.labels );
    plotterGroup.writeEntry( plot_bottomBar, mSettings.bottomBar );
    plotterGroup.writeEntry( plot_verticalLines, mSettings.verticalLines );
    plotterGroup.writeEntry( plot_horizontalLines, mSettings.horizontalLines );
    plotterGroup.writeEntry( plot_showIncoming, mSettings.showIncoming );
    plotterGroup.writeEntry( plot_showOutgoing, mSettings.showOutgoing );
    plotterGroup.writeEntry( plot_automaticDetection, mSettings.automaticDetection );
    plotterGroup.writeEntry( plot_verticalLinesScroll, mSettings.verticalLinesScroll );
    plotterGroup.writeEntry( plot_colorVLines, mSettings.colorVLines );
    plotterGroup.writeEntry( plot_colorHLines, mSettings.colorHLines );
    plotterGroup.writeEntry( plot_colorIncoming, mSettings.colorIncoming );
    plotterGroup.writeEntry( plot_colorOutgoing, mSettings.colorOutgoing );
    plotterGroup.writeEntry( plot_colorBackground, mSettings.colorBackground );
    plotterGroup.writeEntry( plot_opacity, mSettings.opacity );
    config->sync();
    configChanged();
}

void InterfacePlotterDialog::configChanged()
{
    QFont font = mPlotter->axisFont();
    font.setPointSize( mSettings.fontSize );
    QFontMetrics fm( font );
    int axisTextWidth = fm.width(i18nc("Largest axis title", "99999 XXXX"));
    mPlotter->setMaxAxisTextWidth( axisTextWidth );
    mPlotter->setAxisFont( font );
    if ( !mSettings.automaticDetection )
    {
        mPlotter->setMinValue( mSettings.minimumValue );
        mPlotter->setMaxValue( mSettings.maximumValue );
    }
    mPlotter->setHorizontalScale( mSettings.pixel );
    mPlotter->setVerticalLinesDistance( mSettings.distance );
    mPlotter->setShowAxis( mSettings.labels );
    mPlotter->setShowVerticalLines( mSettings.verticalLines );
    mPlotter->setShowHorizontalLines( mSettings.horizontalLines );
    mPlotter->setUseAutoRange( mSettings.automaticDetection );
    mPlotter->setVerticalLinesScroll( mSettings.verticalLinesScroll );
    mPlotter->setVerticalLinesColor( mSettings.colorVLines );
    mPlotter->setHorizontalLinesColor( mSettings.colorHLines );
    mPlotter->setBackgroundColor( mSettings.colorBackground );
    mPlotter->setFillOpacity( mSettings.opacity * 255 / 100.0 + 0.5 );

    // add or remove beams according to user settings
    VisibleBeams nextVisibleBeams = NONE;
    if ( mSettings.showOutgoing )
        nextVisibleBeams = (VisibleBeams) ( nextVisibleBeams | OUTGOING_TRAFFIC );
    if ( mSettings.showIncoming )
        nextVisibleBeams = (VisibleBeams) ( nextVisibleBeams | INCOMING_TRAFFIC );

    mSentLabel->setLabel( i18n( "%1 Sent Data", mName ), mSettings.colorOutgoing, mIndicatorSymbol);
    mReceivedLabel->setLabel( i18n( "%1 Received Data", mName ), mSettings.colorIncoming, mIndicatorSymbol);

    switch( mVisibleBeams )
    {
    case NONE:
        if ( nextVisibleBeams == BOTH )
        {
            mOutgoingPos = 0;
            mPlotter->addBeam( mSettings.colorOutgoing );

            mIncomingPos = 1;
            mPlotter->addBeam( mSettings.colorIncoming );
        }
        else if ( nextVisibleBeams == OUTGOING_TRAFFIC )
        {
            mOutgoingPos = 0;
            mPlotter->addBeam( mSettings.colorOutgoing );
        }
        else if ( nextVisibleBeams == INCOMING_TRAFFIC )
        {
            mIncomingPos = 0;
            mPlotter->addBeam( mSettings.colorIncoming );
        }
        break;
    case INCOMING_TRAFFIC:
        if ( nextVisibleBeams == BOTH )
        {
            mOutgoingPos = 1;
            mPlotter->addBeam( mSettings.colorOutgoing );
            mPlotter->setBeamColor( mIncomingPos, mSettings.colorIncoming );
        }
        else if ( nextVisibleBeams == OUTGOING_TRAFFIC )
        {
            mPlotter->removeBeam( mIncomingPos );
            mOutgoingPos = 0;
            mPlotter->addBeam( mSettings.colorOutgoing );
        }
        else if ( nextVisibleBeams == INCOMING_TRAFFIC )
        {
            mPlotter->setBeamColor( mIncomingPos, mSettings.colorIncoming );
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
            mPlotter->addBeam( mSettings.colorIncoming );
            mPlotter->setBeamColor( mOutgoingPos, mSettings.colorOutgoing );
        }
        else if ( nextVisibleBeams == INCOMING_TRAFFIC )
        {
            mPlotter->removeBeam( mOutgoingPos );
            mIncomingPos = 0;
            mPlotter->addBeam( mSettings.colorIncoming );
        }
        else if ( nextVisibleBeams == OUTGOING_TRAFFIC )
        {
            mPlotter->setBeamColor( mOutgoingPos, mSettings.colorOutgoing );
        }
        else if ( nextVisibleBeams == NONE )
        {
            mPlotter->removeBeam( mOutgoingPos );
        }
        break;
    case BOTH:
        if ( nextVisibleBeams == BOTH )
        {
            mPlotter->setBeamColor( mIncomingPos, mSettings.colorIncoming );
            mPlotter->setBeamColor( mOutgoingPos, mSettings.colorOutgoing );
        }
        else if ( nextVisibleBeams == OUTGOING_TRAFFIC )
        {
            mOutgoingPos = 0;
            mPlotter->removeBeam( mIncomingPos );
            mPlotter->setBeamColor( mOutgoingPos, mSettings.colorOutgoing );
        }
        else if ( nextVisibleBeams == INCOMING_TRAFFIC )
        {
            mIncomingPos = 0;
            mPlotter->removeBeam( mOutgoingPos );
            mPlotter->setBeamColor( mIncomingPos, mSettings.colorIncoming );
        }
        else if ( nextVisibleBeams == NONE )
        {
            mPlotter->removeBeam( 0 );
            mPlotter->removeBeam( 0 );
        }
        break;
    }

    if ( mSettings.bottomBar )
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
