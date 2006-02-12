/* This file is part of KNemo
   Copyright (C) 2005, 2006 Percy Leonhardt <percy@eris23.de>

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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef INTERFACESTATISTICS_H
#define INTERFACESTATISTICS_H

#include <qobject.h>
#include <qptrlist.h>

#include "global.h"

template<class type>
class StatisticsPtrList : public QPtrList<type>
{
protected:
    virtual int compareItems ( QPtrCollection::Item item1, QPtrCollection::Item item2 )
    {
        StatisticEntry* entry1 = static_cast<StatisticEntry*>( item1 );
        StatisticEntry* entry2 = static_cast<StatisticEntry*>( item2 );

        if ( entry1->year > entry2->year )
        {
            return 1;
        }
        else if ( entry2->year > entry1->year )
        {
            return -1;
        } // ...here we know that years are the same...
        else if ( entry1->month > entry2->month )
        {
            return 1;
        }
        else if ( entry2->month > entry1->month )
        {
            return -1;
        } // ...here we know that months are the same...
        else if ( entry1->day > entry2->day )
        {
            return 1;
        }
        else if ( entry2->day > entry1->day )
        {
            return -1;
        } // ...here we know that dates are equal.

        return 0;
    };
};

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
    InterfaceStatistics( QObject* parent = 0L, const char* name = 0L );

    /**
     * Default Destructor
     */
    virtual ~InterfaceStatistics();

    /**
     * Load the statistics from a xml file
     */
    void loadStatistics( QString& fileName );
    /**
     * Save the statistics to a xml file
     */
    void saveStatistics( QString& fileName );

    const StatisticEntry* getCurrentDay() const;
    const StatisticEntry* getCurrentMonth() const;
    const StatisticEntry* getCurrentYear() const;
    const StatisticsPtrList<StatisticEntry>& getDayStatistics() const;
    const StatisticsPtrList<StatisticEntry>& getMonthStatistics() const;
    const StatisticsPtrList<StatisticEntry>& getYearStatistics() const;

signals:
    /**
     * The current entry has changed. There is only one signal
     * for day, month and year because if the day changes,
     * month and year also change.
     */
    void currentEntryChanged();
    /**
     * The list has changed i.e. there is a new day entry or the list was cleared
     */
    void dayStatisticsChanged();
    /**
     * The list has changed i.e. there is a new month entry or the list was cleared
     */
    void monthStatisticsChanged();
    /**
     * The list has changed i.e. there is a new year entry or the list was cleared
     */
    void yearStatisticsChanged();

public slots:
    /**
     * Add incoming data to the current day, month and year
     */
    void addIncomingData( unsigned long data );
    /**
     * Add outgoing data to the current day, month and year
     */
    void addOutgoingData( unsigned long data );
    /**
     * Clear all entries of the day statistics
     */
    void clearDayStatistics();
    /**
     * Clear all entries of the month statistics
     */
    void clearMonthStatistics();
    /**
     * Clear all entries of the year statistics
     */
    void clearYearStatistics();

private:
    /**
     * Make sure the current entry corresponds with the current date
     */
    void checkCurrentEntry();
    /**
     * Fill the statistics with a current entry
     */
    void initStatistics();
    /**
     * Check if the current day is in the day statistics. If found set
     * mCurrentDay to the found entry else create a new one.
     */
    void updateCurrentDay();
    /**
     * Check if the current month is in the month statistics. If found set
     * mCurrentMonth to the found entry else create a new one.
     */
    void updateCurrentMonth();
    /**
     * Check if the current year is in the year statistics. If found set
     * mCurrentYear to the found entry else create a new one.
     */
    void updateCurrentYear();


    StatisticEntry* mCurrentDay;
    StatisticEntry* mCurrentMonth;
    StatisticEntry* mCurrentYear;
    StatisticsPtrList<StatisticEntry> mDayStatistics;
    StatisticsPtrList<StatisticEntry> mMonthStatistics;
    StatisticsPtrList<StatisticEntry> mYearStatistics;
};

#endif // INTERFACESTATISTICS_H
