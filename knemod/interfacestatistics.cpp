/* This file is part of KNemo
   Copyright (C) 2005 Percy Leonhardt <percy@eris23.de>

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

#include <qdatetime.h>

#include <kdebug.h>

#include "interfacestatistics.h"

InterfaceStatistics::InterfaceStatistics( QObject* parent, const char* name )
    : QObject( parent, name )
{
    mDayStatistics.setAutoDelete( true );
    mMonthStatistics.setAutoDelete( true );
    mYearStatistics.setAutoDelete( true );
    initStatistics();
}

InterfaceStatistics::~InterfaceStatistics()
{
    mDayStatistics.clear();
    mMonthStatistics.clear();
    mYearStatistics.clear();
}

void InterfaceStatistics::loadStatistics( /*QString& fileName*/ )
{
}

void InterfaceStatistics::saveStatistics( /*QString& fileName*/ )
{
}

void InterfaceStatistics::clearAll()
{
    mDayStatistics.clear();
    mMonthStatistics.clear();
    mYearStatistics.clear();

    initStatistics();
}

const StatisticEntry* InterfaceStatistics::getCurrentDay()
{
    return mCurrentDay;
}

const StatisticEntry* InterfaceStatistics::getCurrentMonth()
{
    return mCurrentMonth;
}

const StatisticEntry* InterfaceStatistics::getCurrentYear()
{
    return mCurrentYear;
}

const QPtrList<StatisticEntry>& InterfaceStatistics::getDayStatistics()
{
   return mDayStatistics;
}

const QPtrList<StatisticEntry>& InterfaceStatistics::getMonthStatistics()
{
    return mMonthStatistics;
}

const QPtrList<StatisticEntry>& InterfaceStatistics::getYearStatistics()
{
    return mYearStatistics;
}

void InterfaceStatistics::addIncomingData( unsigned long data )
{
    checkCurrentEntry();

    mCurrentDay->rxBytes += data;
    mCurrentMonth->rxBytes += data;
    mCurrentYear->rxBytes += data;

    emit currentEntryChanged();
}

void InterfaceStatistics::addOutgoingData( unsigned long data )
{
    checkCurrentEntry();

    mCurrentDay->txBytes += data;
    mCurrentMonth->txBytes += data;
    mCurrentYear->txBytes += data;

    emit currentEntryChanged();
}

void InterfaceStatistics::checkCurrentEntry()
{
    if ( mCurrentDay->day != QDate::currentDate().day() ||
         mCurrentDay->month != QDate::currentDate().month() ||
         mCurrentDay->year != QDate::currentDate().year() )
    {
        // current day has changed
        updateCurrentDay();

        if ( mCurrentDay->month != QDate::currentDate().month() ||
             mCurrentDay->year != QDate::currentDate().year() )
        {
            // current month has also changed
            updateCurrentMonth();
        }

        if ( mCurrentDay->year != QDate::currentDate().year() )
        {
            // current year has also changed
            updateCurrentYear();
        }
    }
}

void InterfaceStatistics::initStatistics()
{
    mCurrentDay = new StatisticEntry();
    mCurrentDay->day = QDate::currentDate().day();
    mCurrentDay->month = QDate::currentDate().month();
    mCurrentDay->year = QDate::currentDate().year();
    mDayStatistics.append( mCurrentDay );

    mCurrentMonth = new StatisticEntry();
    mCurrentMonth->day = 0;
    mCurrentMonth->month = QDate::currentDate().month();
    mCurrentMonth->year = QDate::currentDate().year();
    mMonthStatistics.append( mCurrentMonth );

    mCurrentYear = new StatisticEntry();
    mCurrentYear->day = 0;
    mCurrentYear->month = 0;
    mCurrentYear->year = QDate::currentDate().year();
    mYearStatistics.append( mCurrentYear );

    emit currentEntryChanged();
    emit dayStatisticsChanged();
    emit monthStatisticsChanged();
    emit yearStatisticsChanged();
}

void InterfaceStatistics::updateCurrentDay()
{
    StatisticEntry* iterator = mDayStatistics.first();
    while ( iterator )
    {
        if ( iterator->day == QDate::currentDate().day() &&
             iterator->month == QDate::currentDate().month() &&
             iterator->year == QDate::currentDate().year() )
        {
            // found current day in list
            mCurrentDay = iterator;
            return;
        }
        iterator = mDayStatistics.next();
    }

    // the current day is not in the list
    iterator = new StatisticEntry();
    iterator->day = QDate::currentDate().day();
    iterator->month = QDate::currentDate().month();
    iterator->year = QDate::currentDate().year();
    mDayStatistics.append( mCurrentDay ); // TODO: insert at correct position
    emit dayStatisticsChanged();
}

void InterfaceStatistics::updateCurrentMonth()
{
    StatisticEntry* iterator = mMonthStatistics.first();
    while ( iterator )
    {
        if ( iterator->month == QDate::currentDate().month() &&
             iterator->year == QDate::currentDate().year() )
        {
            // found current month in list
            mCurrentMonth = iterator;
            return;
        }
        iterator = mMonthStatistics.next();
    }

    // the current month is not in the list
    iterator = new StatisticEntry();
    iterator->day = 0;
    iterator->month = QDate::currentDate().month();
    iterator->year = QDate::currentDate().year();
    mMonthStatistics.append( mCurrentMonth ); // TODO: insert at correct position
    emit monthStatisticsChanged();
}

void InterfaceStatistics::updateCurrentYear()
{
    StatisticEntry* iterator = mYearStatistics.first();
    while ( iterator )
    {
        if ( iterator->year == QDate::currentDate().year() )
        {
            // found current year in list
            mCurrentYear = iterator;
            return;
        }
        iterator = mYearStatistics.next();
    }

    // the current year is not in the list
    iterator = new StatisticEntry();
    iterator->day = 0;
    iterator->month = 0;
    iterator->year = QDate::currentDate().year();
    mYearStatistics.append( mCurrentYear ); // TODO: insert at correct position
    emit yearStatisticsChanged();
}


