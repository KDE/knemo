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

StatisticsModel::StatisticsModel( enum GroupType t, QString group, QString elem ) :
    QStandardItemModel(),
    mType( t ),
    mGroup( group ),
    mElem( elem )
{
    QStringList headerList;
    headerList << i18n( "Date" ) << i18n( "Sent" ) << i18n( "Received" ) << i18n( "Total" );
    setHorizontalHeaderLabels( headerList );
}

StatisticsModel::~StatisticsModel()
{
}

void StatisticsModel::appendStats( const QDate& date, int days, quint64 rx, quint64 tx )
{
    QStandardItem * dateItem = new QStandardItem();
    QStandardItem * txItem = new QStandardItem();
    QStandardItem * rxItem = new QStandardItem();
    QStandardItem * totalItem = new QStandardItem();

    txItem->setData( KIO::convertSize( tx ), Qt::DisplayRole );
    txItem->setData( tx, DataRole );

    rxItem->setData( KIO::convertSize( rx ), Qt::DisplayRole );
    rxItem->setData( rx, DataRole );

    totalItem->setData( KIO::convertSize( rx + tx ), Qt::DisplayRole );
    totalItem->setData( rx + tx, DataRole );

    QString dateStr;
    switch ( mType )
    {
        case Month:
            // Format for simple period
            // Starts on the first of the month, lasts exactly one month
            if ( mCalendar->day( date ) == 1 &&
                 days == mCalendar->daysInMonth( date ) )
                dateStr = QString( "%1 %2" )
                            .arg( mCalendar->monthName( date, KCalendarSystem::ShortName ) )
                            .arg( mCalendar->year( date ) );
            // Format for complex period
            else
            {
                QDate endDate = date.addDays( days - 1 );
                dateStr = QString( "%1 %2 - %4 %5 %6" )
                            .arg( mCalendar->day( date ) )
                            .arg( mCalendar->monthName( date, KCalendarSystem::ShortName ) )
                            .arg( mCalendar->day( endDate ) )
                            .arg( mCalendar->monthName( endDate, KCalendarSystem::ShortName ) )
                            .arg( mCalendar->year( endDate ) );
            }
            break;
        case Year:
            dateStr = QString::number( mCalendar->year( date ) );
            break;
        default:
            dateStr = mCalendar->formatDate( date, KLocale::ShortDate );
    }

    dateItem->setData( days, SpanRole );
    dateItem->setData( QDateTime( date ), DataRole );
    dateItem->setData( dateStr, Qt::DisplayRole );

    QList<QStandardItem*> entry;
    entry << dateItem << txItem << rxItem << totalItem;
    appendRow( entry );
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
    if ( row < 0 )
        row = rowCount() - 1;

    if ( rowCount() && rowCount() > row )
        return item( row, Date )->data( DataRole ).toDateTime().date();
    else
        return QDate();
}

int StatisticsModel::days( int row ) const
{
    if ( row < 0 )
        row = rowCount() - 1;

    if ( rowCount() && rowCount() > row )
        return item( row, Date )->data( SpanRole ).toInt();
    else
        return 0;
}

quint64 StatisticsModel::rxBytes( int row ) const
{
    if ( row < 0 )
        row = rowCount() - 1;

    if ( rowCount() && rowCount() > row )
        return item( row, RxBytes )->data( DataRole ).toULongLong();
    else
        return 0;
}

quint64 StatisticsModel::txBytes( int row ) const
{
    if ( row < 0 )
        row = rowCount() - 1;

    if ( rowCount() && rowCount() > row )
        return item( row, TxBytes )->data( DataRole ).toULongLong();
    else
        return 0;
}

quint64 StatisticsModel::totalBytes( int row ) const
{
    if ( row < 0 )
        row = rowCount() - 1;

    if ( rowCount() && rowCount() > row )
        return item( row, TotalBytes )->data( DataRole ).toULongLong();
    else
        return 0;
}

QString StatisticsModel::txText( int row ) const
{
    if ( row < 0 )
        row = rowCount() - 1;

    if ( rowCount() && rowCount() > row )
        return item( row, TxBytes )->data( Qt::DisplayRole ).toString();
    else
        return QString();
}

QString StatisticsModel::rxText( int row ) const
{
    if ( row < 0 )
        row = rowCount() - 1;

    if ( rowCount() && rowCount() > row )
        return item( row, RxBytes )->data( Qt::DisplayRole ).toString();
    else
        return QString();
}

QString StatisticsModel::totalText( int row ) const
{
    if ( row < 0 )
        row = rowCount() - 1;

    if ( rowCount() && rowCount() > row )
        return item( row, TotalBytes )->data( Qt::DisplayRole ).toString();
    else
        return QString();
}

void StatisticsModel::addTotalBytes( quint64 bytes )
{
    quint64 totalBytes = item( rowCount() - 1, TotalBytes )->data().toULongLong() + bytes;
    item( rowCount() - 1, TotalBytes )->setData( KIO::convertSize( totalBytes ), Qt::DisplayRole );
    item( rowCount() - 1, TotalBytes )->setData( totalBytes );
}

void StatisticsModel::addRxBytes( quint64 bytes )
{
    quint64 rxBytes = item( rowCount() - 1, RxBytes )->data().toULongLong() + bytes;
    item( rowCount() - 1, RxBytes )->setData( KIO::convertSize( rxBytes ), Qt::DisplayRole );
    item( rowCount() - 1, RxBytes )->setData( rxBytes );
    addTotalBytes( bytes );
}

void StatisticsModel::addTxBytes( quint64 bytes )
{
    quint64 txBytes = item( rowCount() - 1, TxBytes )->data().toULongLong() + bytes;
    item( rowCount() - 1, TxBytes )->setData( KIO::convertSize( txBytes ), Qt::DisplayRole );
    item( rowCount() - 1, TxBytes )->setData( txBytes );
    addTotalBytes( bytes );
}
