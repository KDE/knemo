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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <qdom.h>
#include <qfile.h>
#include <qtimer.h>
#include <qstring.h>
#include <qdatetime.h>

#include <kdebug.h>

#include "interface.h"
#include "interfacestatistics.h"

InterfaceStatistics::InterfaceStatistics( Interface* interface )
    : QObject(),
      mInterface( interface )
{
    mDayStatistics.setAutoDelete( true );
    mMonthStatistics.setAutoDelete( true );
    mYearStatistics.setAutoDelete( true );
    initStatistics();

    mSaveTimer = new QTimer();
    connect( mSaveTimer, SIGNAL( timeout() ), this, SLOT( saveStatistics() ) );
    mSaveTimer->start( mInterface->getGeneralData().saveInterval * 1000 );
}

InterfaceStatistics::~InterfaceStatistics()
{
    mSaveTimer->stop();
    delete mSaveTimer;

    mDayStatistics.clear();
    mMonthStatistics.clear();
    mYearStatistics.clear();
}

void InterfaceStatistics::loadStatistics()
{
    QDomDocument doc( "statistics" );
    QString dir = mInterface->getGeneralData().statisticsDir;
    QFile file( dir + "/statistics_" + mInterface->getName() );

    if ( !file.open( IO_ReadOnly ) )
        return;
    if ( !doc.setContent( &file ) )
    {
        file.close();
        return;
    }
    file.close();

    mDayStatistics.clear();
    mMonthStatistics.clear();
    mYearStatistics.clear();

    QDomElement root = doc.documentElement();
    QDomNode n = root.namedItem( "days" );
    if ( !n.isNull() )
    {
        QDomNode dayNode = n.firstChild();
        while ( !dayNode.isNull() )
        {
            QDomElement day = dayNode.toElement();
            if ( !day.isNull() )
            {
                StatisticEntry* entry = new StatisticEntry();
                entry->day = day.attribute( "day" ).toInt();
                entry->month = day.attribute( "month" ).toInt();
                entry->year = day.attribute( "year" ).toInt();
                entry->rxBytes = (Q_UINT64) day.attribute( "rxBytes" ).toDouble();
                entry->txBytes = (Q_UINT64) day.attribute( "txBytes" ).toDouble();
                mDayStatistics.append( entry );
            }
            dayNode = dayNode.nextSibling();
        }
        mDayStatistics.sort();
    }

    n = root.namedItem( "months" );
    if ( !n.isNull() )
    {
        QDomNode monthNode = n.firstChild();
        while ( !monthNode.isNull() )
        {
            QDomElement month = monthNode.toElement();
            if ( !month.isNull() )
            {
                StatisticEntry* entry = new StatisticEntry();
                entry->day = 0;
                entry->month = month.attribute( "month" ).toInt();
                entry->year = month.attribute( "year" ).toInt();
                entry->rxBytes = (Q_UINT64) month.attribute( "rxBytes" ).toDouble();
                entry->txBytes = (Q_UINT64) month.attribute( "txBytes" ).toDouble();
                mMonthStatistics.append( entry );
            }
            monthNode = monthNode.nextSibling();
        }
        mMonthStatistics.sort();
    }

    n = root.namedItem( "years" );
    if ( !n.isNull() )
    {
        QDomNode yearNode = n.firstChild();
        while ( !yearNode.isNull() )
        {
            QDomElement year = yearNode.toElement();
            if ( !year.isNull() )
            {
                StatisticEntry* entry = new StatisticEntry();
                entry->day = 0;
                entry->month = 0;
                entry->year = year.attribute( "year" ).toInt();
                entry->rxBytes = (Q_UINT64) year.attribute( "rxBytes" ).toDouble();
                entry->txBytes = (Q_UINT64) year.attribute( "txBytes" ).toDouble();
                mYearStatistics.append( entry );
            }
            yearNode = yearNode.nextSibling();
        }
        mYearStatistics.sort();
    }
    initStatistics();
}

void InterfaceStatistics::saveStatistics()
{
    QDomDocument doc( "statistics" );
    QDomElement root = doc.createElement( "statistics" );
    doc.appendChild( root );

    QDomElement days = doc.createElement( "days" );
    StatisticEntry* iterator = mDayStatistics.first();
    while ( iterator )
    {
        QDomElement day = doc.createElement( "day" );
        day.setAttribute( "day", iterator->day );
        day.setAttribute( "month", iterator->month );
        day.setAttribute( "year", iterator->year );
        day.setAttribute( "rxBytes", (double) iterator->rxBytes );
        day.setAttribute( "txBytes", (double) iterator->txBytes );
        days.appendChild( day );
        iterator = mDayStatistics.next();
    }
    root.appendChild( days );

    QDomElement months = doc.createElement( "months" );
    iterator = mMonthStatistics.first();
    while ( iterator )
    {
        QDomElement month = doc.createElement( "month" );
        month.setAttribute( "month", iterator->month );
        month.setAttribute( "year", iterator->year );
        month.setAttribute( "rxBytes", (double) iterator->rxBytes );
        month.setAttribute( "txBytes", (double) iterator->txBytes );
        months.appendChild( month );
        iterator = mMonthStatistics.next();
    }
    root.appendChild( months );

    QDomElement years = doc.createElement( "years" );
    iterator = mYearStatistics.first();
    while ( iterator )
    {
        QDomElement year = doc.createElement( "year" );
        year.setAttribute( "year", iterator->year );
        year.setAttribute( "rxBytes", (double) iterator->rxBytes );
        year.setAttribute( "txBytes", (double) iterator->txBytes );
        years.appendChild( year );
        iterator = mYearStatistics.next();
    }
    root.appendChild( years );

    QString dir = mInterface->getGeneralData().statisticsDir;
    QFile file( dir + "/statistics_" + mInterface->getName() );
    if ( !file.open( IO_WriteOnly ) )
        return;

    QTextStream stream( &file );
    stream << doc.toString();
    file.close();
}

void InterfaceStatistics::configChanged()
{
    // restart the timer with the new value
    mSaveTimer->changeInterval( mInterface->getGeneralData().saveInterval * 1000 );
}

const StatisticEntry* InterfaceStatistics::getCurrentDay() const
{
    return mCurrentDay;
}

const StatisticEntry* InterfaceStatistics::getCurrentMonth() const
{
    return mCurrentMonth;
}

const StatisticEntry* InterfaceStatistics::getCurrentYear() const
{
    return mCurrentYear;
}

const StatisticsPtrList<StatisticEntry>& InterfaceStatistics::getDayStatistics() const
{
   return mDayStatistics;
}

const StatisticsPtrList<StatisticEntry>& InterfaceStatistics::getMonthStatistics() const
{
    return mMonthStatistics;
}

const StatisticsPtrList<StatisticEntry>& InterfaceStatistics::getYearStatistics() const
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

void InterfaceStatistics::clearDayStatistics()
{
    mDayStatistics.clear();
    updateCurrentDay();
}

void InterfaceStatistics::clearMonthStatistics()
{
    mMonthStatistics.clear();
    updateCurrentMonth();
}

void InterfaceStatistics::clearYearStatistics()
{
    mYearStatistics.clear();
    updateCurrentYear();
}

void InterfaceStatistics::checkCurrentEntry()
{
    if ( mCurrentDay->day != QDate::currentDate().day() ||
         mCurrentDay->month != QDate::currentDate().month() ||
         mCurrentDay->year != QDate::currentDate().year() )
    {
        // current day has changed
        updateCurrentDay();

        if ( mCurrentMonth->month != QDate::currentDate().month() ||
             mCurrentMonth->year != QDate::currentDate().year() )
        {
            // current month has also changed
            updateCurrentMonth();
        }

        if ( mCurrentYear->year != QDate::currentDate().year() )
        {
            // current year has also changed
            updateCurrentYear();
        }
    }
}

void InterfaceStatistics::initStatistics()
{
    updateCurrentDay();
    updateCurrentMonth();
    updateCurrentYear();

    emit currentEntryChanged();
}

void InterfaceStatistics::updateCurrentDay()
{
    mCurrentDay = mDayStatistics.first();
    while ( mCurrentDay )
    {
        if ( mCurrentDay->day == QDate::currentDate().day() &&
             mCurrentDay->month == QDate::currentDate().month() &&
             mCurrentDay->year == QDate::currentDate().year() )
        {
            // found current day in list
            return;
        }
        mCurrentDay = mDayStatistics.next();
    }

    // the current day is not in the list
    mCurrentDay = new StatisticEntry();
    mCurrentDay->day = QDate::currentDate().day();
    mCurrentDay->month = QDate::currentDate().month();
    mCurrentDay->year = QDate::currentDate().year();
    mCurrentDay->rxBytes = 0;
    mCurrentDay->txBytes = 0;
    mDayStatistics.append( mCurrentDay ); // TODO: insert at correct position
    emit dayStatisticsChanged();
}

void InterfaceStatistics::updateCurrentMonth()
{
    mCurrentMonth = mMonthStatistics.first();
    while ( mCurrentMonth )
    {
        if ( mCurrentMonth->month == QDate::currentDate().month() &&
             mCurrentMonth->year == QDate::currentDate().year() )
        {
            // found current month in list
            return;
        }
        mCurrentMonth = mMonthStatistics.next();
    }

    // the current month is not in the list
    mCurrentMonth = new StatisticEntry();
    mCurrentMonth->day = 0;
    mCurrentMonth->month = QDate::currentDate().month();
    mCurrentMonth->year = QDate::currentDate().year();
    mCurrentMonth->rxBytes = 0;
    mCurrentMonth->txBytes = 0;
    mMonthStatistics.append( mCurrentMonth ); // TODO: insert at correct position
    emit monthStatisticsChanged();
}

void InterfaceStatistics::updateCurrentYear()
{
    mCurrentYear = mYearStatistics.first();
    while ( mCurrentYear )
    {
        if ( mCurrentYear->year == QDate::currentDate().year() )
        {
            // found current year in list
            return;
        }
        mCurrentYear = mYearStatistics.next();
    }

    // the current year is not in the list
    mCurrentYear = new StatisticEntry();
    mCurrentYear->day = 0;
    mCurrentYear->month = 0;
    mCurrentYear->year = QDate::currentDate().year();
    mCurrentYear->rxBytes = 0;
    mCurrentYear->txBytes = 0;
    mYearStatistics.append( mCurrentYear ); // TODO: insert at correct position
    emit yearStatisticsChanged();
}

