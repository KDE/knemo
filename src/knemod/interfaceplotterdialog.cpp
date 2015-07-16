/* This file is part of KNemo
   Copyright (C) 2004, 2006 Percy Leonhardt <percy@eris23.de>
   Copyright (C) 2009, 2010 John Stamp <jstamp@users.sourceforge.net>

   Portions adapted from FancyPlotter.cpp in KSysGuard
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
#include <QMouseEvent>
#include <QApplication>
#include <KConfigGroup>

#include "global.h"
#include "interfaceplotterdialog.h"
#include "utils.h"
#include <ksysguard/ksignalplotter.h>
#include "plotterconfigdialog.h"
#include <math.h>

static const char plot_pixel[] = "Pixel";
static const char plot_distance[] = "Distance";
static const char plot_fontSize[] = "FontSize";
static const char plot_minimumValue[] = "MinimumValue";
static const char plot_maximumValue[] = "MaximumValue";
static const char plot_labels[] = "Labels";
static const char plot_verticalLines[] = "VerticalLines";
static const char plot_horizontalLines[] = "HorizontalLines";
static const char plot_showIncoming[] = "ShowIncoming";
static const char plot_showOutgoing[] = "ShowOutgoing";
static const char plot_automaticDetection[] = "AutomaticDetection";
static const char plot_verticalLinesScroll[] = "VerticalLinesScroll";
static const char plot_colorIncoming[] = "ColorIncoming";
static const char plot_colorOutgoing[] = "ColorOutgoing";

class FancyPlotterLabel : public QLabel {
  public:
    FancyPlotterLabel(QWidget *parent) : QLabel(parent) {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        longHeadingWidth = 0;
        shortHeadingWidth = 0;
        textMargin = 0;
        setLayoutDirection(Qt::LeftToRight); //We do this because we organise the strings ourselves.. is this going to muck it up though for RTL languages?
    }
    ~FancyPlotterLabel() {
    }
    void setLabel(const QString &name, const QColor &color) {
        labelName = name;

        if(indicatorSymbol.isNull()) {
            if(fontMetrics().inFont(QChar(0x25CF)))
                indicatorSymbol = QChar(0x25CF);
            else
                indicatorSymbol = QLatin1Char('#');
        }
        changeLabel(color);

    }
    void setValueText(const QString &value) {
        //value can have multiple strings, separated with the 0x9c character
        valueText = value.split(QChar(0x9c));
        resizeEvent(NULL);
        update();
    }
    virtual void resizeEvent( QResizeEvent * ) {
        QFontMetrics fm = fontMetrics();

        if(valueText.isEmpty()) {
            if(longHeadingWidth < width())
                setText(longHeadingText);
            else
                setText(shortHeadingText);
            return;
        }
        QString value = valueText.first();

        int textWidth = fm.boundingRect(value).width();
        if(textWidth + longHeadingWidth < width())
            setBothText(longHeadingText, value);
        else if(textWidth + shortHeadingWidth < width())
            setBothText(shortHeadingText, value);
        else {
            int valueTextCount = valueText.count();
            int i;
            for(i = 1; i < valueTextCount; ++i) {
                textWidth = fm.boundingRect(valueText.at(i)).width();
                if(textWidth + shortHeadingWidth <= width()) {
                    break;
                }
            }
            if(i < valueTextCount)
                setBothText(shortHeadingText, valueText.at(i));
            else
                setText(noHeadingText + valueText.last()); //This just sets the color of the text
        }
    }
    void changeLabel(const QColor &_color) {
        color = _color;

        if ( qApp->layoutDirection() == Qt::RightToLeft )
            longHeadingText = QLatin1String(": ") + labelName + QLatin1String(" <font color=\"") + color.name() + QLatin1String("\">") + indicatorSymbol + QLatin1String("</font>");
        else
            longHeadingText = QLatin1String("<qt><font color=\"") + color.name() + QLatin1String("\">") + indicatorSymbol + QLatin1String("</font> ") + labelName + QLatin1String(" :");
        shortHeadingText = QLatin1String("<qt><font color=\"") + color.name() + QLatin1String("\">") + indicatorSymbol + QLatin1String("</font>");
        noHeadingText = QLatin1String("<qt><font color=\"") + color.name() + QLatin1String("\">");

        textMargin = fontMetrics().width(QLatin1Char('x')) + margin()*2 + frameWidth()*2;
        longHeadingWidth = fontMetrics().boundingRect(labelName + QLatin1String(" :") + indicatorSymbol + QLatin1String(" x")).width() + textMargin;
        shortHeadingWidth = fontMetrics().boundingRect(indicatorSymbol).width() + textMargin;
        setMinimumWidth(shortHeadingWidth);
        update();
    }
  private:
    void setBothText(const QString &heading, const QString & value) {
        if(QApplication::layoutDirection() == Qt::LeftToRight)
            setText(heading + QLatin1Char(' ') + value);
        else
            setText(QLatin1String("<qt>") + value + QLatin1Char(' ') + heading);
    }
    int textMargin;
    QString longHeadingText;
    QString shortHeadingText;
    QString noHeadingText;
    int longHeadingWidth;
    int shortHeadingWidth;
    QList<QString> valueText;
    QString labelName;
    QColor color;
    static QChar indicatorSymbol;
};

QChar FancyPlotterLabel::indicatorSymbol;


InterfacePlotterDialog::InterfacePlotterDialog( QString name )
    : QDialog(),
      mConfig( KSharedConfig::openConfig() ),
      mConfigDlg( 0 ),
      mLabelsWidget( NULL ),
      mSetPos( true ),
      mWasShown( false ),
      mUseBitrate( generalSettings->useBitrate ),
      mMultiplier( 1024 ),
      mOutgoingVisible( false ),
      mIncomingVisible( false ),
      mName( name )
{
    setWindowTitle( i18nc( "interface name", "%1 Traffic", mName ) );
    setContextMenuPolicy( Qt::DefaultContextMenu );

    mByteUnits << ki18n( "%1 B/s" ) << ki18n( "%1 KiB/s" ) << ki18n( "%1 MiB/s" ) << ki18n( "%1 GiB/s" );
    mBitUnits << ki18n( "%1 bit/s" ) << ki18n( "%1 kbit/s" ) << ki18n( "%1 Mbit/s" ) << ki18n( "%1 Gbit/s" );

    mIndicatorSymbol = QLatin1Char('#');
    QFontMetrics fm(font());
    if (fm.inFont(QChar(0x25CF)))
        mIndicatorSymbol = QChar(0x25CF);
    
    QBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    setLayout( layout );
    mPlotter = new KSignalPlotter( this );
    int axisTextWidth = fontMetrics().width(i18nc("Largest axis title", "99999 XXXX"));
    mPlotter->setMaxAxisTextWidth( axisTextWidth );
    mPlotter->setShowAxis( true );
    mPlotter->setUseAutoRange( true );
    layout->addWidget(mPlotter);

    /* Create a set of labels underneath the graph. */
    mLabelsWidget = new QWidget;
    layout->addWidget(mLabelsWidget);
    QBoxLayout *outerLabelLayout = new QHBoxLayout(mLabelsWidget);
    outerLabelLayout->setSpacing(0);
    outerLabelLayout->setContentsMargins(0,0,0,0);

    /* create a spacer to fill up the space up to the start of the graph */
    outerLabelLayout->addItem(new QSpacerItem(axisTextWidth + 10, 0, QSizePolicy::Preferred));

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
    connect( mPlotter, SIGNAL(axisScaleChanged()), this, SLOT(setPlotterUnits()) );
    loadConfig();
}

InterfacePlotterDialog::~InterfacePlotterDialog()
{
    if ( mWasShown )
    {
        // If the dialog was never shown, then the position
        // will be wrong
        KConfig *config = mConfig.data();
        KConfigGroup interfaceGroup( config, confg_interface + mName );
        interfaceGroup.writeEntry( conf_plotterSize, size() );
        interfaceGroup.writeEntry( conf_plotterPos, pos() );
        config->sync();
    }
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

    return QDialog::event( e );
}

void InterfacePlotterDialog::resizeEvent( QResizeEvent* )
{
    bool showLabels = true;;

    if( this->height() <= mLabelsWidget->sizeHint().height() + mPlotter->minimumHeight() )
        showLabels = false;
    mLabelsWidget->setVisible(showLabels);
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
    connect( mConfigDlg, SIGNAL( finished(int) ), this, SLOT( configFinished() ) );
    connect( mConfigDlg, SIGNAL( saved() ), this, SLOT( saveConfig() ) );
    mConfigDlg->show();
}

void InterfacePlotterDialog::configFinished()
{
    // FIXME
    mConfigDlg->close();
    mConfigDlg = 0;
}

void InterfacePlotterDialog::setPlotterUnits()
{
    // Prevent this being called recursively
    disconnect( mPlotter, SIGNAL(axisScaleChanged()), this, SLOT(setPlotterUnits()) );
    qreal value = mPlotter->currentMaximumRangeValue();
    int units = 0;

    if (value >= pow( mMultiplier, 3)*0.7) //If it's over 0.7GiB, then set the scale to gigabytes
    {
        units = 3;
    }
    else if (value > pow(mMultiplier,2))
    {
        units = 2;
    }
    else if (value > mMultiplier)
    {
        units = 1;
    }
    mPlotter->setScaleDownBy( pow(mMultiplier, units ) );
    if ( mUseBitrate )
        mPlotter->setUnit( mBitUnits[units] );
    else
        mPlotter->setUnit( mByteUnits[units] );
    // reconnect
    connect( mPlotter, SIGNAL(axisScaleChanged()), this, SLOT(setPlotterUnits()) );
}

void InterfacePlotterDialog::useBitrate( bool useBits )
{
    // Have to wipe the plotters if we change units
    if ( mUseBitrate != useBits )
    {
        int nb = mPlotter->numBeams();
        for ( int i = 0; i < nb; i++ )
        {
            mPlotter->removeBeam(0);
        }
        mOutgoingVisible = false;
        mIncomingVisible = false;
    }

    mUseBitrate = useBits;
    if ( mUseBitrate )
        mMultiplier = 1000;
    else
        mMultiplier = 1024;
    addBeams();
    for ( int beamId = 0; beamId < mPlotter->numBeams(); beamId++ )
    {
        QString lastValue = formattedRate( mPlotter->lastValue(beamId), mUseBitrate );
        static_cast<FancyPlotterLabel *>((static_cast<QWidgetItem *>(mLabelLayout->itemAt(beamId)))->widget())->setText(lastValue);
    }
    setPlotterUnits();
}

void InterfacePlotterDialog::updatePlotter( const double incomingBytes, const double outgoingBytes )
{
    QList<qreal> trafficList;
    if ( mOutgoingVisible )
       trafficList.append( outgoingBytes );
    if ( mIncomingVisible )
        trafficList.append( incomingBytes );
    mPlotter->addSample( trafficList );

    for ( int beamId = 0; beamId < mPlotter->numBeams(); beamId++ )
    {
        QString lastValue = formattedRate( mPlotter->lastValue(beamId), mUseBitrate );
        static_cast<FancyPlotterLabel *>((static_cast<QWidgetItem *>(mLabelLayout->itemAt(beamId)))->widget())->setValueText(lastValue);
    }
}

void InterfacePlotterDialog::loadConfig()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    // Set the plotter widgets
    QString group = confg_plotter + mName;
    // Plotter
    PlotterSettings s;
    KConfigGroup plotterGroup( config, group );
    mSettings.pixel = clamp<int>(plotterGroup.readEntry( plot_pixel, s.pixel ), 1, 50 );
    mSettings.distance = clamp<int>(plotterGroup.readEntry( plot_distance, s.distance ), 10, 120 );
    mSettings.fontSize = clamp<int>(plotterGroup.readEntry( plot_fontSize, s.fontSize ), 5, 24 );
    mSettings.minimumValue = clamp<double>(plotterGroup.readEntry( plot_minimumValue, s.minimumValue ), 0.0, pow(1024.0, 3) - 1 );
    mSettings.maximumValue = clamp<double>(plotterGroup.readEntry( plot_maximumValue, s.maximumValue ), 0.0, pow(1024.0, 3) );
    mSettings.labels = plotterGroup.readEntry( plot_labels, s.labels );
    mSettings.showIncoming = plotterGroup.readEntry( plot_showIncoming, s.showIncoming );
    mSettings.showOutgoing = plotterGroup.readEntry( plot_showOutgoing, s.showOutgoing );
    mSettings.verticalLines = plotterGroup.readEntry( plot_verticalLines, s.verticalLines );
    mSettings.horizontalLines = plotterGroup.readEntry( plot_horizontalLines, s.horizontalLines );
    mSettings.automaticDetection = plotterGroup.readEntry( plot_automaticDetection, s.automaticDetection );
    mSettings.verticalLinesScroll = plotterGroup.readEntry( plot_verticalLinesScroll, s.verticalLinesScroll );
    mSettings.colorIncoming = plotterGroup.readEntry( plot_colorIncoming, s.colorIncoming );
    mSettings.colorOutgoing = plotterGroup.readEntry( plot_colorOutgoing, s.colorOutgoing );
    configChanged();
}

void InterfacePlotterDialog::saveConfig()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
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
    plotterGroup.writeEntry( plot_verticalLines, mSettings.verticalLines );
    plotterGroup.writeEntry( plot_horizontalLines, mSettings.horizontalLines );
    plotterGroup.writeEntry( plot_showIncoming, mSettings.showIncoming );
    plotterGroup.writeEntry( plot_showOutgoing, mSettings.showOutgoing );
    plotterGroup.writeEntry( plot_automaticDetection, mSettings.automaticDetection );
    plotterGroup.writeEntry( plot_verticalLinesScroll, mSettings.verticalLinesScroll );
    plotterGroup.writeEntry( plot_colorIncoming, mSettings.colorIncoming );
    plotterGroup.writeEntry( plot_colorOutgoing, mSettings.colorOutgoing );
    config->sync();
    configChanged();
}

void InterfacePlotterDialog::configChanged()
{
    QFont pfont = mPlotter->font();
    pfont.setPointSize( mSettings.fontSize );
    QFontMetrics fm( pfont );
    int axisTextWidth = fm.width(i18nc("Largest axis title", "99999 XXXX"));
    mPlotter->setMaxAxisTextWidth( axisTextWidth );
    mPlotter->setFont( pfont );
    if ( !mSettings.automaticDetection )
    {
        mPlotter->setMinimumValue( mSettings.minimumValue * mMultiplier );
        mPlotter->setMaximumValue( mSettings.maximumValue * mMultiplier );
    }
    else
    {
        // Don't want the disabled settings to be used as hints
        mPlotter->setMinimumValue( 0 );
        mPlotter->setMaximumValue( 1 );
    }
    mPlotter->setHorizontalScale( mSettings.pixel );
    mPlotter->setVerticalLinesDistance( mSettings.distance );
    mPlotter->setShowAxis( mSettings.labels );
    mPlotter->setShowVerticalLines( mSettings.verticalLines );
    mPlotter->setShowHorizontalLines( mSettings.horizontalLines );
    mPlotter->setUseAutoRange( mSettings.automaticDetection );
    mPlotter->setVerticalLinesScroll( mSettings.verticalLinesScroll );

    mSentLabel->setLabel( i18nc( "network traffic", "Sending" ), mSettings.colorOutgoing);
    mReceivedLabel->setLabel( i18nc( "network traffic", "Receiving" ), mSettings.colorIncoming);

    addBeams();
}

void InterfacePlotterDialog::addBeams()
{
    if ( mSettings.showOutgoing )
    {
        if ( !mOutgoingVisible )
        {
            mPlotter->addBeam( mSettings.colorOutgoing );
            mSentLabel->show();
            mOutgoingVisible = true;
            if ( mIncomingVisible )
            {
                QList<int> newOrder;
                newOrder << 1 << 0;
                mPlotter->reorderBeams( newOrder );
            }
        }
    }
    else if ( mOutgoingVisible == true )
    {
        mPlotter->removeBeam( 0 );
        mSentLabel->hide();
        mOutgoingVisible = false;
    }

    if ( mSettings.showIncoming )
    {
        if ( !mIncomingVisible )
        {
            mPlotter->addBeam( mSettings.colorIncoming );
            mReceivedLabel->show();
            mIncomingVisible = true;
        }
    }
    else if ( mIncomingVisible == true )
    {
        mPlotter->removeBeam( mPlotter->numBeams() - 1 );
        mReceivedLabel->hide();
        mIncomingVisible = false;
    }
}

#include "moc_interfaceplotterdialog.cpp"
