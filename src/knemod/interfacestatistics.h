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

#include <QDate>
#include <QList>

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
     * Fill the statistics with a current entry
     */
    void initStatistics();
    /**
     * Check if the current day is in the day statistics. If found set
     * mCurrentDay to the found entry else create a new one.
     */
    void updateCurrentDay( const QDate & );
    /**
     * Check if the current week is in the week statistics. If found set
     * mCurrentWeek to the found entry else create a new one.
     */
    StatisticEntry * genNewWeek( const QDate & );
    StatisticEntry * genNewMonth( const QDate &, QDate = QDate() );
    StatisticEntry * genNewYear( const QDate & );
    void updateCurrentWeek( const QDate & );
    /**
     * Check if the current month is in the month statistics. If found set
     * mCurrentMonth to the found entry else create a new one.
     */
    void updateCurrentMonth( const QDate & );
    /**
     * Check if the current year is in the year statistics. If found set
     * mCurrentYear to the found entry else create a new one.
     */
    void updateCurrentYear( const QDate & );
    QDate setRebuildDate( QList<StatisticEntry *>& statistics, const QDate &recalcDate, int group );
    QDate getNextMonthStart( QDate );
    void rebuildStats( const QDate &recalcDate, int group );
    StatisticEntry* appendStats( QList<StatisticEntry *>& statistics, StatisticEntry* entry );
    void saveBillingStart();
    void loadStatsGroup( const KCalendarSystem * cal, const QDomElement& root, int group, QList<StatisticEntry *>& statistics );
    void buildStatsGroup( QDomDocument& doc, int group, const QList<StatisticEntry *>& statistics );

    QTimer* mSaveTimer;
    Interface* mInterface;
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
