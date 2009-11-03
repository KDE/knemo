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
    /**
     * Default Constructor
     */
    InterfaceStatistics( Interface* interface );

    /**
     * Default Destructor
     */
    virtual ~InterfaceStatistics();

    /**
     * Called from Interface::configChanged() when the user
     * changed the settings.
     */
    void configChanged();

    const StatisticEntry* getCurrentDay() const { return mCurrentDay; }
    const StatisticEntry* getCurrentWeek() const { return mCurrentWeek; }
    const StatisticEntry* getCurrentMonth() const { return mCurrentMonth; }
    const StatisticEntry* getCurrentYear() const { return mCurrentYear; }
    const QList<StatisticEntry *>& getDayStatistics() const { return mDayStatistics; }
    const QList<StatisticEntry *>& getWeekStatistics() const { return mWeekStatistics; }
    const QList<StatisticEntry *>& getMonthStatistics() const { return mMonthStatistics; }
    const QList<StatisticEntry *>& getYearStatistics() const { return mYearStatistics; }

    enum GroupType
    {
        Day   = 1,
        Week  = 2,
        Month = 4,
        Year  = 8
    };

signals:
    /**
     * The current entry has changed. There is only one signal
     * for day, week, month and year because if the day changes,
     * week, month and year also change.
     */
    void currentEntryChanged();
    void warnMonthlyTraffic( quint64 );
    /**
     * The list has changed i.e. there is a new day entry or the list was cleared
     */
    void dayStatisticsChanged();
    /**
     * The list has changed i.e. there is a new week entry or the list was cleared
     */
    void weekStatisticsChanged();
    /**
     * The list has changed i.e. there is a new month entry or the list was cleared
     */
    void monthStatisticsChanged( bool );
    /**
     * The list has changed i.e. there is a new year entry or the list was cleared
     */
    void yearStatisticsChanged();

public slots:
    /**
     * Save the statistics to a xml file
     * (slot so it can be triggered by a timer signal)
     */
    void saveStatistics();
    /**
     * Add incoming data to the current day, week, month and year
     */
    void addIncomingData( unsigned long data );
    /**
     * Add outgoing data to the current day, week, month and year
     */
    void addOutgoingData( unsigned long data );
    /**
     * Clear all statistics
     */
    void clearStatistics();

private:
    /**
     * Make sure the current entry corresponds with the current date
     */
    void checkCurrentEntry();

    /**
     * Load the statistics from a xml file
     */
    void loadStatistics();

    /**
     * Take nodes from the xml file and convert them into StatisticEntries,
     * using the calendar system cal
     */
    void loadStatsGroup( const KCalendarSystem * cal, const QDomElement& root,
                         enum GroupType group, QList<StatisticEntry *>& statistics );

    /**
     * Fill the statistics with a current entry
     */
    void initStatistics();

    /**
     * Check if the current (day|week|month|year) is in the statistics. If
     * found set mCurrent(Day|Week|Month|Year) to the found entry, otherwise
     * create a new one.
     */
    void updateCurrentDay( const QDate & );
    void updateCurrentWeek( const QDate & );
    void updateCurrentMonth( const QDate & );
    void updateCurrentYear( const QDate & );

    /**
     * Given a date, generate a StatisticEntry that contains it
     */
    StatisticEntry * genNewWeek( const QDate & );
    StatisticEntry * genNewYear( const QDate & );

    /**
     * Given a date, generate a StatisticEntry that contains it
     *
     * If the second date is valid, then we create a partial month entry that
     * spans the two dates. This happens when someone splits a month while
     * creating a custom billing period.
     *
     * If the generated entry spans a period that does not have any daily
     * statistics, then we create the next latest one that does.  This prevents
     * orphan monthly entries when using custom billing periods.
     */
    StatisticEntry * genNewMonth( const QDate &, QDate = QDate() );

    /**
     * Sometimes we need to set a rebuild date earlier than requested, so
     * given a list of statistics and a recalc date, find an actual rebuild
     * date
     */
    QDate setRebuildDate( QList<StatisticEntry *>& statistics,
                          const QDate &recalcDate, enum GroupType group );

    /**
     * Given period's start date, return the next period's start date
     */
    QDate getNextMonthStart( const QDate& );

    /**
     * Return true if the entry spans a period that contains at least one day
     * in mDayStatistics, else false
     */
    bool checkValidSpan( const StatisticEntry& entry );

    /**
     * Rebuild staistics for the groups at least as far back as recalcDate
     */
    void rebuildStats( const QDate &recalcDate, int groups );

    /**
     * Append a StatisticEntry to the list
     */
    StatisticEntry* appendStats( QList<StatisticEntry *>& statistics, StatisticEntry* entry );

    /**
     * If we're using custom billing periods, save the current billing start
     * date in the config
     */
    void saveBillingStart();

    /**
     * Convert StatisticEntries to QDomElements so we can save the xml file
     */
    void buildStatsGroup( QDomDocument& doc, enum GroupType group,
                          const QList<StatisticEntry *>& statistics );

    QTimer* mSaveTimer;
    Interface* mInterface;
    bool mWarningDone;
    bool mAllMonths;
    QDate mBillingStart;
    const KCalendarSystem* mCalendar;
    StatisticEntry* mCurrentDay;
    StatisticEntry* mCurrentWeek;
    StatisticEntry* mCurrentMonth;
    StatisticEntry* mCurrentYear;
    QList<StatisticEntry *> mDayStatistics;
    QList<StatisticEntry *> mWeekStatistics;
    QList<StatisticEntry *> mMonthStatistics;
    QList<StatisticEntry *> mYearStatistics;
};

#endif // INTERFACESTATISTICS_H
