/* This file is part of KNemo
   Copyright (C) 2010 John Stamp <jstamp@users.sourceforge.net>

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

#include "statisticsview.h"
#include "statisticsmodel.h"
#include <kio/global.h>

#include <QApplication>
#include <QDesktopWidget>
#include <QEvent>
#include <QHeaderView>
#include <QLabel>
#include <QMouseEvent>
#include <QStylePainter>
#include <QTimer>


class StatsTip : public QLabel
{
    public:
        StatsTip();
        void showText( const bool followMouse, const QString &, QWidget * );

    private:
        QTimer statTimer;
        void placeTip( const QPoint & );
        void timeout();

    protected:
        void paintEvent(QPaintEvent *e);
        void resizeEvent(QResizeEvent *e);
};

StatsTip::StatsTip() : QLabel( 0, Qt::ToolTip )
{
    statTimer.setSingleShot( true );
    connect ( &statTimer, SIGNAL( timeout() ), this, SLOT( hide() ) );

    setForegroundRole(QPalette::ToolTipText);
    setBackgroundRole(QPalette::ToolTipBase);
    setStyleSheet( qApp->styleSheet() );
    setPalette( qApp->palette() );
    ensurePolished();
    setMargin( 1 + style()->pixelMetric(QStyle::PM_ToolTipLabelFrameWidth, 0, this ) );
}

void StatsTip::showText( const bool followMouse, const QString & tip, QWidget * w )
{
    if ( tip != text() )
    {
        setText( tip );
        resize( sizeHint() );
    }
    show();
    if ( followMouse )
    {
        placeTip( QCursor::pos() + QPoint( 2, 16 ) );
    }
    else
    {
        QPoint center = w->rect().center();
        center.setY( w->rect().height() * 0.4 );
        QPoint half = QPoint( sizeHint().width()/2, sizeHint().height()/2 );
        placeTip( w->mapToGlobal( center - half ) );
    }
}

void StatsTip::placeTip( const QPoint & point )
{
    int screenNum;
    if ( QApplication::desktop()->isVirtualDesktop() )
        screenNum = QApplication::desktop()->screenNumber( point );
    else
        screenNum = QApplication::desktop()->screenNumber( static_cast<QWidget*>(parent()) );

    QRect screen = QApplication::desktop()->screenGeometry( screenNum );

    QPoint p = point;
    if ( point.x() + width() > screen.width() )
        p.setX( screen.width() - width() );
    if ( point.x() < 0 )
        p.setX( 0 );
    if ( point.y() + height() > screen.height() )
        p.setY( screen.height() - height() );
    if ( p.y() < 0 )
        p.setY( 0 );

    move( p );
    timeout();
}

void StatsTip::paintEvent( QPaintEvent *e )
{
    QStylePainter p( this );
    QStyleOptionFrame opt;
    opt.init( this );
    p.drawPrimitive( QStyle::PE_PanelTipLabel, opt );
    p.end();

    QLabel::paintEvent( e );
}

void StatsTip::resizeEvent( QResizeEvent *e )
{
    QStyleHintReturnMask frameMask;
    QStyleOption option;
    option.init( this );
    if ( style()->styleHint( QStyle::SH_ToolTip_Mask, &option, this, &frameMask ) )
        setMask( frameMask.region );

    QLabel::resizeEvent( e );
}

void StatsTip::timeout()
{
    statTimer.start( 2000 );
}

static StatsTip * statsTip = NULL;


StatisticsView::StatisticsView( QWidget * parent ) :
    QTableView( parent ),
    mFollow( false ),
    mOffpeak( false )
{
    if ( !statsTip )
        statsTip = new StatsTip();
}

StatisticsView::~StatisticsView()
{
}

void StatisticsView::setModel( QAbstractItemModel *m )
{
    QTableView::setModel( m );
    horizontalHeader()->setMovable( true );
    connect( selectionModel(), SIGNAL( selectionChanged ( const QItemSelection &, const QItemSelection & ) ),
             this, SLOT( updateSum() ) );
}

void StatisticsView::updateSum()
{
    quint64 total = 0;
     foreach( QModelIndex i, selectionModel()->selectedIndexes() )
        total += selectionModel()->model()->data( i, StatisticsModel::DataRole ).toULongLong();
    totalString = KIO::convertSize( total );

    quint64 offpeak = 0;
    quint64 peak = 0;
    if ( mOffpeak )
    {
        foreach( QModelIndex i, selectionModel()->selectedIndexes() )
            offpeak += selectionModel()->model()->data( i, StatisticsModel::DataRole + KNemoStats::OffpeakTraffic ).toULongLong();
        offpeakString = KIO::convertSize( offpeak );
        if ( total > offpeak )
            peak = total - offpeak;
        peakString = KIO::convertSize( peak );
    }
}

void StatisticsView::showSum( const QPoint &p )
{
    QString sumString;
    QString pStr = i18n( "Peak:" );
    QString opStr = i18n( "Off-Peak:" );
    QString tStr = i18n( "Total:" );
    if ( mOffpeak )
        sumString = "<table><tr><td>%1</td><td>%2</td></tr><tr><td>%3</td><td>%4</td></tr><tr><td><b>%5</b></td><td><b>%6</b></td></tr></table>";
    else
        sumString = "<table><tr><td>%1</td><td>%2</td></tr></table>";


    if ( selectionModel()->selectedIndexes().count() > 0 && selectionModel()->isSelected( indexAt( p ) ) )
    {
        if ( mOffpeak )
            statsTip->showText( mFollow, sumString.arg( pStr ).arg( peakString )
                    .arg( opStr ).arg( offpeakString )
                    .arg( tStr ).arg( totalString ), this );
        else
            statsTip->showText( mFollow, sumString.arg( tStr ).arg( totalString ), this );
    }
    else if ( indexAt( p ).isValid() )
    {
        quint64 totalBytes = model()->data( indexAt( p ), StatisticsModel::DataRole ).toULongLong();
        if ( mOffpeak )
        {
            quint64 offpeakBytes = model()->data( indexAt( p ), StatisticsModel::DataRole + KNemoStats::OffpeakTraffic ).toULongLong();
            quint64 peakBytes = 0;
            if ( totalBytes > offpeakBytes )
                peakBytes = totalBytes - offpeakBytes;
            statsTip->showText( mFollow, sumString.arg( pStr ).arg( KIO::convertSize( peakBytes ) )
                    .arg( opStr ).arg( KIO::convertSize( offpeakBytes ) )
                    .arg( tStr ).arg( KIO::convertSize( totalBytes ) ), this );
        }
        else
        {
            statsTip->showText( mFollow, sumString.arg( tStr ).arg( KIO::convertSize( totalBytes ) ), this );
        }
    }
    else
    {
        statsTip->hide();
    }
}

void StatisticsView::hideEvent( QHideEvent * e )
{
    statsTip->hide();
    QTableView::hideEvent( e );
}

void StatisticsView::mousePressEvent( QMouseEvent * e )
{
    mFollow = true;
    QTableView::mousePressEvent( e );
}

void StatisticsView::keyPressEvent( QKeyEvent * e )
{

    mFollow = false;
    QTableView::keyPressEvent( e );
}

bool StatisticsView::viewportEvent( QEvent * e )
{
    switch ( e->type() )
    {
        case QEvent::HoverEnter:
        case QEvent::HoverMove:
            mFollow = true;
            showSum( static_cast<QHoverEvent*>(e)->pos() );
            break;
        case QEvent::HoverLeave:
            statsTip->hide();
        default:
            ;;
    }
    return QTableView::viewportEvent( e );
}

#include "statisticsview.moc"
