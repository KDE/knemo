/* This file is part of KNemo
   Copyright (C) 2005, 2006 Percy Leonhardt <percy@eris23.de>
   Copyright (C) 2009, 2010 John Stamp <jstamp@users.sourceforge.net>

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

#include "storage/storagedata.h"

class QTimer;
class Interface;
class StatisticsModel;
class SqlStorage;

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

    /**
     * Called whenever the configuration changes.  This updates statistics
     * settings and checks whether any of the statistics need to be rebuilt.
     */
    void configChanged();

    /**
     * Return a pointer to the StatisticsModel tracking a period unit
     */
    StatisticsModel* getStatistics( enum KNemoStats::PeriodUnits t ) { return mModels.value( t ); }

    /**
     * Add received bytes to each of the models
     */
    void addRxBytes( unsigned long bytes );

    /** Add transmitted bytes to each of the models
     */
    void addTxBytes( unsigned long bytes );

    /**
     * Return a pointer to the active calendar
     */
    KCalendarSystem *calendar() { return mStorageData.calendar; }

Q_SIGNALS:
    /**
     * Emitted when an entry is updated (i.e. when new bytes are transmitted,
     * received, or the stats are cleared).
     */
    void currentEntryChanged();

    /**
     * Emitted when a traffic threshold has been reached
     */
    void warnTraffic( QString warnText, quint64 threshold, quint64 current );

public Q_SLOTS:
    void clearStatistics();
    void checkValidEntry();

private Q_SLOTS:
    void saveStatistics( bool fullSave = false );
    void checkWarnings();

private:
    bool loadStats();

    void resetWarnings( int periodUnits );
    void hoursToArchive( const QDateTime &dateTime );
    bool daysInSpan( const QDate& entry, int span );
    QDate nextBillPeriodStart( const StatsRule &rule, const QDate& );

    void genNewHour( const QDateTime &dateTime );
    bool genNewCalendarType( const QDate &, const enum KNemoStats::PeriodUnits );
    void genNewBillPeriod( const QDate & );

    int ruleForDate( const QDate &date );
    void syncWithExternal( uint updated );
    bool isOffpeak( const StatsRule & rule, const QDateTime &dt );
    QDate prepareRebuild( StatisticsModel* statistics, const QDate &recalcDate );
    void amendStats( int index, const StatisticsModel *source, StatisticsModel *dest );

    int rebuildHours( StatisticsModel *s, const StatsRule &rules, const QDate &start, const QDate &end );
    int rebuildDay( int dayIndex, int hourIndex, StatisticsModel *s );
    void rebuildBaseUnits( const StatsRule & rule, const QDate & start, const QDate & end );
    void rebuildCalendarPeriods( const QDate &requestedStart, bool weekOnly = false );
    void rebuildBillPeriods( const QDate &requestedStart );

    void prependStatsRule( QList<StatsRule> &rules );
    void checkRebuild( const KLocale::CalendarSystem oldCalendar, bool force = false );

    Interface* mInterface;
    QTimer* mSaveTimer;
    QTimer* mWarnTimer;
    QTimer* mEntryTimer;
    bool mTrafficChanged;
    //int mWeekStartDay;
    StorageData mStorageData;
    QHash<int, StatisticsModel*> mModels;
    QList<StatsRule> mStatsRules;
    SqlStorage *sql;
};

#endif // INTERFACESTATISTICS_H
