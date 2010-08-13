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
        TrafficTypeRole,
        DataRole
    };

    /**
     * Clear rows
     */
    void clearRows() { removeRows( 0, rowCount() ); }

    /**
     * Update the text in the date cell.  Handy after a rebuild or if a fancy
     * short date changes.
     */
    void updateDateText( int row );

    /**
     * Return the type of statistics that this model is tracking
     */
    enum KNemoStats::PeriodUnits periodType() const { return mPeriodType; }

    /**
     * Set the calendar that the model will use.  This must be done before any
     * entries are added.
     */
    void setCalendar( const KCalendarSystem * calendar ) { mCalendar = calendar; }

    /**
     * Creates a stats entry for the model.  If id < 0 it will create an id
     * that matches rowCount().  If days > 0 it will set a period of length
     * 'days'.  The latter is relevant only for custom billing periods.
     */

    int createEntry( const QDateTime &startDateTime, int id = -1, int days = -1 );

    /**
     * Return the id of a row.  If row < 0 it will return the id of the last
     * entry.  An invalid row will return -1.
     */
    int id( int row = -1 ) const;

    /**
     * Set the id of a row.  If row < 0 it will set the id of the last entry.
     */
    void setId( int id, int row = -1 );

    /**
     * Return the QDate of a row.  If row < 0 it will return the date of the
     * last entry.  An invalid row will return an invalid QDate.
     */
    QDate date( int row = -1 ) const;

    /**
     * Return the QDateTime of a row.  If row < 0 it will return QDateTime of
     * the last entry.  An invalid row will return an invalid QDateTime.
     */
    QDateTime dateTime( int row = -1 ) const;

    /**
     * Return the number of days that an entry spans.  If row < 0 it will
     * return the day span of the last entry.  An invalid row will return 0.
     */
    int days( int row = -1 ) const;

    /**
     * Return a list of all traffic types tracked by this entry. If row < 0 it
     * will return the types of the last entry.
     */
    QList<KNemoStats::TrafficType> trafficTypes( int row = -1 ) const;

    /**
     * Clears the list of traffic types being tracked by the entry in the row.
     * If row < 0 it will clear the last entry.
     */
    void resetTrafficTypes( int row = -1 );

    /**
     * Set a traffic type as tracked by this entry.  If row < 0 it will add the
     * type to the list in the last entry.
     */
    void addTrafficType( int trafficType, int row = -1 );

    /**
     * Return the index of the given id.  An invalid id will return -1.
     */
    int indexOfId( int row ) const;

    /**
     * These return the rx, tx, total bytes for the entry in row 'row'.  If row
     * < 0 it will return the bytes of the last entry.
     */
    quint64 rxBytes( int row = -1, KNemoStats::TrafficType trafficType = KNemoStats::AllTraffic ) const;
    quint64 txBytes( int row = -1, KNemoStats::TrafficType traffictype = KNemoStats::AllTraffic ) const;
    quint64 totalBytes( int row = -1, KNemoStats::TrafficType trafficType = KNemoStats::AllTraffic ) const;

    /**
     * Set the traffic for given row and traffic type
     */
    void setTraffic( int row, quint64 rx, quint64 tx, KNemoStats::TrafficType trafficType = KNemoStats::AllTraffic );

    /**
     * Get the formatted text of the traffic levels.  If row < 0 it will return
     * the traffic of the last entry.
     */
    QString rxText( int row = -1 ) const;
    QString txText( int row = -1 ) const;
    QString totalText( int row = -1 ) const;

    /**
     * These add traffic for the given type and row.  If row < 0 it will add
     * traffic to the last entry.
     */
    void addRxBytes( quint64 bytes, KNemoStats::TrafficType trafficType = KNemoStats::AllTraffic, int row = -1 );
    void addTxBytes( quint64 bytes, KNemoStats::TrafficType trafficType = KNemoStats::AllTraffic, int row = -1 );

private:
    enum StatsColumn
    {
        Date = 0,
        TxBytes,
        RxBytes,
        TotalBytes
    };

    void addBytes( enum StatsColumn column, KNemoStats::TrafficType trafficType, quint64 bytes, int row = -1 );
    quint64 bytes( enum StatsColumn column, int role, int row ) const;
    QString text( enum StatsColumn column, int row ) const;
    void updateText( QStandardItem * i );

    enum KNemoStats::PeriodUnits mPeriodType;
    const KCalendarSystem * mCalendar;
};

#endif
