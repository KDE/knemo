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

#include "statisticsmodel.h"
#include "global.h"
#include <QStringList>
#include <KLocale>
#include <kio/global.h>
#include <QLocale>

StatisticsModel::StatisticsModel( enum KNemoStats::PeriodUnits t, QObject *parent ) :
    QStandardItemModel( parent ),
    mPeriodType( t ),
    mCalendar( 0 )
{
    QStringList headerList;
    headerList << i18n( "Date" ) << i18n( "Sent" ) << i18n( "Received" ) << i18n( "Total" );
    setHorizontalHeaderLabels( headerList );
    setSortRole( DataRole );
}

StatisticsModel::~StatisticsModel()
{
}

void StatisticsModel::addBytes( enum StatsColumn column, KNemoStats::TrafficType trafficType, quint64 bytes, int row )
{
    if ( !bytes || !rowCount() )
        return;
    if ( row < 0 )
        row = rowCount() - 1;

    int role = DataRole + trafficType;

    quint64 b = item( row, column )->data( role ).toULongLong() + bytes;
    item( row, column )->setData( b, role );
    if ( trafficType == KNemoStats::AllTraffic )
        updateText( item( row, column ) );
}

quint64 StatisticsModel::bytes( enum StatsColumn column, int role, int row ) const
{
    if ( row < 0 )
        row = rowCount() - 1;

    if ( rowCount() && rowCount() > row )
        return item( row, column )->data( role ).toULongLong();
    else
        return 0;
}

QString StatisticsModel::text( StatsColumn column, int row ) const
{
    if ( row < 0 )
        row = rowCount() - 1;

    if ( rowCount() && rowCount() > row )
        return item( row, column )->data( Qt::DisplayRole ).toString();
    else
        return QString();
}

void StatisticsModel::updateText( QStandardItem * i )
{
    quint64 all = i->data( DataRole ).toULongLong();
    i->setData( KIO::convertSize( all ), Qt::DisplayRole );
}

int StatisticsModel::createEntry( const QDateTime &dateTime, int entryId, int days )
{
    QList<QStandardItem*> entry;
    QStandardItem * dateItem = new QStandardItem();
    QStandardItem * txItem = new QStandardItem();
    QStandardItem * rxItem = new QStandardItem();
    QStandardItem * totalItem = new QStandardItem();

    dateItem->setData( dateTime, DataRole );
    if ( entryId < 0 )
    {
        entryId = rowCount();
    }
    dateItem->setData( entryId, IdRole );
    if ( days > 0 )
    {
        dateItem->setData( days, SpanRole );
    }
    entry << dateItem << txItem << rxItem << totalItem;
    appendRow( entry );
    setTraffic( rowCount() - 1, 0, 0 );
    updateDateText( rowCount() - 1 );
    return entryId;
}

void StatisticsModel::updateDateText( int row )
{
    if ( row < 0 || !rowCount() || !mCalendar )
        return;

    QLocale locale;
    QString dateStr;
    QDateTime dt = dateTime( row );
    int dy = days( row );
    switch ( mPeriodType )
    {
        case KNemoStats::Hour:
            dateStr = locale.toString( dt.time(), QLocale::ShortFormat );
            if ( dt.date() == QDate::currentDate() )
            {
                dateStr += " " + i18n("Today");
            } else {
                dateStr += " " + i18n("Yesterday");
            }
            break;
        case KNemoStats::Month:
            dateStr = QString( "%1 %2" )
                        .arg( mCalendar->monthName( dt.date(), KCalendarSystem::ShortName ) )
                        .arg( mCalendar->year( dt.date() ) );
            break;
        case KNemoStats::Year:
            dateStr = QString::number( mCalendar->year( dt.date() ) );
            break;
        case KNemoStats::BillPeriod:
            // Format for simple period
            // Starts on the first of the month, lasts exactly one month
            if ( mCalendar->day( dt.date() ) == 1 &&
                 dy == mCalendar->daysInMonth( dt.date() ) )
                dateStr = QString( "%1 %2" )
                            .arg( mCalendar->monthName( dt.date(), KCalendarSystem::ShortName ) )
                            .arg( mCalendar->year( dt.date() ) );
            // Format for complex period
            else
            {
                QDate endDate = dt.date().addDays( dy - 1 );
                dateStr = QString( "%1 %2 - %4 %5 %6" )
                            .arg( mCalendar->day( dt.date() ) )
                            .arg( mCalendar->monthName( dt.date(), KCalendarSystem::ShortName ) )
                            .arg( mCalendar->day( endDate ) )
                            .arg( mCalendar->monthName( endDate, KCalendarSystem::ShortName ) )
                            .arg( mCalendar->year( endDate ) );
            }
            break;
        default:
            dateStr = mCalendar->formatDate( dt.date(), KLocale::ShortDate );
    }
    item( row, Date )->setData( dateStr, Qt::DisplayRole );
}

void StatisticsModel::setId( int id, int row )
{
    if ( !rowCount() )
        return;
    if ( row < 0 )
        row = rowCount() - 1;

    item( row, Date )->setData(id, IdRole );
}

int StatisticsModel::id( int row ) const
{
    if ( row < 0 )
        row = rowCount() - 1;

    if ( !rowCount() || rowCount() <= row )
        return -1;

    return item( row, Date )->data( IdRole ).toInt();
}

QDateTime StatisticsModel::dateTime( int row ) const
{
    if ( row < 0 )
        row = rowCount() - 1;

    if ( rowCount() && rowCount() > row )
        return item( row, Date )->data( DataRole ).toDateTime();
    else
        return QDateTime();
}

QDate StatisticsModel::date( int row ) const
{
    return dateTime( row ).date();
}

int StatisticsModel::days( int row ) const
{
    if ( row < 0 )
        row = rowCount() - 1;

    if ( rowCount() && rowCount() > row )
    {
        if ( mPeriodType != KNemoStats::BillPeriod )
        {
            QDate dateTime = item( row, Date )->data( DataRole ).toDateTime().date();
            switch ( mPeriodType )
            {
                case KNemoStats::Day:
                    return 1;
                    break;
                case KNemoStats::Week:
                    return mCalendar->daysInWeek( dateTime );
                    break;
                case KNemoStats::Month:
                    return mCalendar->daysInMonth( dateTime );
                    break;
                case KNemoStats::Year:
                    return mCalendar->daysInYear( dateTime );
                    break;
                default:
                    return 0;
            }
        }
        else
        {
            return item( row, Date )->data( SpanRole ).toInt();
        }
    }
    else
        return 0;
}

void StatisticsModel::addTrafficType( int trafficType, int row )
{
    if ( row < 0 )
        row = rowCount() - 1;
    if ( rowCount() && rowCount() > row )
    {
        int types = item( row, Date )->data( TrafficTypeRole ).toInt();
        item( row, Date )->setData( types | trafficType, TrafficTypeRole );
    }
}

void StatisticsModel::resetTrafficTypes( int row )
{
    if ( row < 0 )
        row = rowCount() - 1;
    if ( rowCount() && rowCount() > row )
        item( row, Date )->setData( KNemoStats::AllTraffic, TrafficTypeRole );
}

QList<KNemoStats::TrafficType> StatisticsModel::trafficTypes( int row ) const
{
    if ( row < 0 )
        row = rowCount() - 1;
    QList<KNemoStats::TrafficType> typeList;
    typeList << KNemoStats::AllTraffic;
    if ( rowCount() && rowCount() > row )
    {
       int types = item( row, Date )->data( TrafficTypeRole ).toInt();
       if ( types & KNemoStats::OffpeakTraffic )
           typeList << KNemoStats::OffpeakTraffic;
    }
    return typeList;
}

quint64 StatisticsModel::rxBytes( int row, KNemoStats::TrafficType trafficType ) const
{
    return bytes( RxBytes, DataRole+trafficType, row );
}

quint64 StatisticsModel::txBytes( int row, KNemoStats::TrafficType trafficType ) const
{
    return bytes( TxBytes, DataRole+trafficType, row );
}

quint64 StatisticsModel::totalBytes( int row, KNemoStats::TrafficType trafficType ) const
{
    return bytes( TotalBytes, DataRole+trafficType, row );
}

QString StatisticsModel::txText( int row ) const
{
    return text( TxBytes, row );
}

QString StatisticsModel::rxText( int row ) const
{
    return text( RxBytes, row );
}

QString StatisticsModel::totalText( int row ) const
{
    return text( TotalBytes, row );
}

void StatisticsModel::addRxBytes( quint64 bytes, KNemoStats::TrafficType trafficType, int row )
{
    addBytes( RxBytes, trafficType, bytes, row );
    addBytes( TotalBytes, trafficType, bytes, row );
}

void StatisticsModel::addTxBytes( quint64 bytes, KNemoStats::TrafficType trafficType, int row )
{
    addBytes( TxBytes, trafficType, bytes, row );
    addBytes( TotalBytes, trafficType, bytes, row );
}

int StatisticsModel::indexOfId( int id ) const
{
    int index = 0;
    while ( index < rowCount() )
    {
        if ( item( index, Date )->data( IdRole ).toInt() == id )
            return index;
        index++;
    }
    return -1;
}

void StatisticsModel::setTraffic( int row, quint64 rx, quint64 tx, KNemoStats::TrafficType trafficType )
{
    if ( row < 0 || row >= rowCount() )
        return;

    item( row, RxBytes )->setData( rx, DataRole + trafficType );
    item( row, TxBytes )->setData( tx, DataRole + trafficType );
    item( row, TotalBytes )->setData( rx+tx, DataRole + trafficType );
    if ( trafficType == KNemoStats::AllTraffic )
    {
        updateText( item( row, RxBytes ) );
        updateText( item( row, TxBytes ) );
        updateText( item( row, TotalBytes ) );
    }
}

#include "statisticsmodel.moc"
