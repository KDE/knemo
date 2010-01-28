/* This file is part of KNemo
   Copyright (C) 2005, 2006 Percy Leonhardt <percy@eris23.de>
   Copyright (C) 2009 John Stamp <jstamp@users.sourceforge.net>

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

#ifndef INTERFACESTATISTICS_H
#define INTERFACESTATISTICS_H

#include "global.h"
#include "statisticsmodel.h"

class QDomDocument;
class QDomElement;
class QTimer;
class KCalendarSystem;
class Interface;

/**
 * This class is able to collect transfered data for an interface,
 * store it in a file and deliver it on request.
 *
 * @short Statistics of transfered data for an interface
 * @author Percy Leonhardt <percy@eris23.de>
 */

class InterfaceStatistics : public QObject
{
    Q_OBJECT
public:
    InterfaceStatistics( Interface* interface );
    virtual ~InterfaceStatistics();

    void configChanged();
    StatisticsModel* getStatistics( enum StatisticsModel::GroupType t ) { return mModels.value( t ); }
    void addRxBytes( unsigned long bytes );
    void addTxBytes( unsigned long bytes );

signals:
    void currentEntryChanged();
    void warnMonthlyTraffic( quint64 );

public slots:
    void clearStatistics();

private slots:
    void saveStatistics();

private:
    void loadConfig();
    void loadStatistics();
    void loadStatsGroup( const KCalendarSystem * cal, const QDomElement& root,
                         StatisticsModel* statistics );

    void saveStatsGroup( QDomDocument& doc, const StatisticsModel* statistics );

    void checkRebuild( QString oldType );
    void doRebuild( const QDate &recalcDate, int groups );
    QDate setRebuildDate( StatisticsModel* statistics,
                          const QDate &recalcDate );
    void amendStats( int, StatisticsModel * );

    void checkValidEntry();
    void checkTrafficLimit();

    void genNewHour( const QDateTime &dateTime );
    void genNewDay( const QDate & );
    void genNewWeek( const QDate & );
    void genNewYear( const QDate & );
    void genNewMonth( const QDate &, QDate = QDate() );
    QDate nextMonthDate( const QDate& );
    bool daysInSpan( const QDate& entry, int span );

    QTimer* mSaveTimer;
    Interface* mInterface;
    bool mWarningDone;
    bool mAllMonths;
    QDate mBillingStart;
    const KCalendarSystem* mCalendar;
    QHash<int, StatisticsModel*> mModels;
};

#endif // INTERFACESTATISTICS_H
