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

class StatisticsModel : public QStandardItemModel
{
    Q_OBJECT
public:
    enum GroupType
    {
        Hour  = 1,
        Day   = 2,
        Week  = 4,
        Month = 8,
        Year  = 16
    };

    StatisticsModel( enum GroupType t, QString group, QString elem );
    virtual ~StatisticsModel();

    enum StatisticRoles
    {
        DataRole = Qt::UserRole + 1,
        SpanRole
    };

    /**
     * Clear rows but leave column headers intact
     */
    void clearRows() { removeRows( 0, rowCount() ); }

    enum GroupType type() const { return mType; }
    QString group() const { return mGroup; }
    QString elem() const { return mElem; }

    void appendStats( const QDateTime& date, int tSpan, quint64 rx = 0, quint64 tx = 0 );
    void appendStats( const QDate& date, int days, quint64 rx = 0, quint64 tx = 0 );
    void setCalendar( const KCalendarSystem * c ) { mCalendar = c; }

    QDate date( int row = -1 ) const;
    QDateTime dateTime( int row = -1 ) const;
    int days( int row = -1 ) const;
    quint64 rxBytes( int row = -1 ) const;
    quint64 txBytes( int row = -1 ) const;
    quint64 totalBytes( int row = -1 ) const;
    QString rxText( int row = -1 ) const;
    QString txText( int row = -1 ) const;
    QString totalText( int row = -1 ) const;

    // Always added to the current entry (last row)
    void addRxBytes( quint64 bytes );
    void addTxBytes( quint64 bytes );

private:
    // Always added to the current entry (last row)
    void addTotalBytes( quint64 bytes );

    enum StatisticColumns
    {
        Date = 0,
        TxBytes,
        RxBytes,
        TotalBytes
    };

    enum GroupType mType;
    QString mGroup;
    QString mElem;
    const KCalendarSystem * mCalendar;
};

#endif
