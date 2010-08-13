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

#ifndef STATISTICSMODEL_H
#define STATISTICSMODEL_H

#include <QDate>
#include <QStandardItemModel>
#include <KCalendarSystem>
#include "data.h"

class StatisticsModel : public QStandardItemModel
{
    Q_OBJECT
public:
    StatisticsModel( enum KNemoStats::PeriodUnits t, QObject *parent = 0 );
    virtual ~StatisticsModel();

    enum StatisticRoles
    {
        IdRole = Qt::UserRole + 1,
        SpanRole,
        DataRole
    };

    /**
     * Clear rows but leave column headers intact
     */
    void clearRows() { removeRows( 0, rowCount() ); }
    void updateDateText( int row );

    enum KNemoStats::PeriodUnits periodType() const { return mPeriodType; }

    void setCalendar( const KCalendarSystem * c ) { mCalendar = c; }

    int createEntry();
    int id( int row = -1 ) const;
    void setId( int id, int row = -1 );
    QDate date( int row = -1 ) const;
    QDateTime dateTime( int row = -1 ) const;
    void setDateTime( QDateTime );
    int days( int row = -1 ) const;
    void setDays( int days );
    int indexOfId( int row ) const;

    quint64 rxBytes( int row = -1 ) const;
    quint64 txBytes( int row = -1 ) const;
    quint64 totalBytes( int row = -1 ) const;
    void setTraffic( int i, quint64 rx, quint64 tx );

    QString rxText( int row = -1 ) const;
    QString txText( int row = -1 ) const;
    QString totalText( int row = -1 ) const;

    // Always added to the current entry (last row)
    void addRxBytes( quint64 bytes, int row = -1 );
    void addTxBytes( quint64 bytes, int row = -1 );

private:
    enum StatsColumn
    {
        Date = 0,
        TxBytes,
        RxBytes,
        TotalBytes
    };

    void addBytes( enum StatsColumn column, quint64 bytes, int row = -1 );
    quint64 bytes( enum StatsColumn column, int row ) const;
    QString text( enum StatsColumn column, int row ) const;
    void updateText( QStandardItem * i );

    enum KNemoStats::PeriodUnits mPeriodType;
    const KCalendarSystem * mCalendar;
};

#endif
