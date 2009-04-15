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

#include <QDate>
#include <QDomNode>
#include <QFile>
#include <QTextStream>
#include <QTimer>

#include "interface.h"
#include "interfacestatistics.h"


static bool statisticsLessThan( const StatisticEntry *s1, const StatisticEntry *s2 )
{
    if ( s1->year < s2->year )
        return true;
    else if ( s1->year > s2->year )
        return false;

    if ( s1->month < s2->month )
        return true;
    else if ( s1->month > s2->month )
        return false;

    return ( s1->day < s2->day );
}


InterfaceStatistics::InterfaceStatistics( Interface* interface )
    : QObject(),
      mInterface( interface )
{
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
    KUrl dir( mInterface->getGeneralData().statisticsDir );
    if ( !dir.isLocalFile() )
        return;
    QFile file( dir.path() + "/statistics_" + mInterface->getName() );

    if ( !file.open( QIODevice::ReadOnly ) )
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
                entry->rxBytes = (quint64) day.attribute( "rxBytes" ).toDouble();
                entry->txBytes = (quint64) day.attribute( "txBytes" ).toDouble();
                mDayStatistics.append( entry );
            }
            dayNode = dayNode.nextSibling();
        }
        qSort( mDayStatistics.begin(), mDayStatistics.end(), statisticsLessThan );
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
                entry->rxBytes = (quint64) month.attribute( "rxBytes" ).toDouble();
                entry->txBytes = (quint64) month.attribute( "txBytes" ).toDouble();
                mMonthStatistics.append( entry );
            }
            monthNode = monthNode.nextSibling();
        }
        qSort( mMonthStatistics.begin(), mMonthStatistics.end(), statisticsLessThan );
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
                entry->rxBytes = (quint64) year.attribute( "rxBytes" ).toDouble();
                entry->txBytes = (quint64) year.attribute( "txBytes" ).toDouble();
                mYearStatistics.append( entry );
            }
            yearNode = yearNode.nextSibling();
        }
        qSort( mYearStatistics.begin(), mYearStatistics.end(), statisticsLessThan );
    }
    initStatistics();
}

void InterfaceStatistics::saveStatistics()
{
    QDomDocument doc( "statistics" );
    QDomElement root = doc.createElement( "statistics" );
    doc.appendChild( root );

    QDomElement days = doc.createElement( "days" );
    foreach( StatisticEntry *entry, mDayStatistics )
    {
        QDomElement day = doc.createElement( "day" );
        day.setAttribute( "day", entry->day );
        day.setAttribute( "month", entry->month );
        day.setAttribute( "year", entry->year );
        day.setAttribute( "rxBytes", (double) entry->rxBytes );
        day.setAttribute( "txBytes", (double) entry->txBytes );
        days.appendChild( day );
    }
    root.appendChild( days );

    QDomElement months = doc.createElement( "months" );
    foreach ( StatisticEntry *entry, mMonthStatistics )
    {
        QDomElement month = doc.createElement( "month" );
        month.setAttribute( "month", entry->month );
        month.setAttribute( "year", entry->year );
        month.setAttribute( "rxBytes", (double) entry->rxBytes );
        month.setAttribute( "txBytes", (double) entry->txBytes );
        months.appendChild( month );
    }
    root.appendChild( months );

    QDomElement years = doc.createElement( "years" );
    foreach ( StatisticEntry *entry, mYearStatistics )
    {
        QDomElement year = doc.createElement( "year" );
        year.setAttribute( "year", entry->year );
        year.setAttribute( "rxBytes", (double) entry->rxBytes );
        year.setAttribute( "txBytes", (double) entry->txBytes );
        years.appendChild( year );
    }
    root.appendChild( years );

    KUrl dir( mInterface->getGeneralData().statisticsDir );
    if ( !dir.isLocalFile() )
        return;
    QFile file( dir.path() + "/statistics_" + mInterface->getName() );
    if ( !file.open( QIODevice::WriteOnly ) )
        return;

    QTextStream stream( &file );
    stream << doc.toString();
    file.close();
}

void InterfaceStatistics::configChanged()
{
    // restart the timer with the new value
    mSaveTimer->setInterval( mInterface->getGeneralData().saveInterval * 1000 );
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

const QList<StatisticEntry *>& InterfaceStatistics::getDayStatistics() const
{
   return mDayStatistics;
}

const QList<StatisticEntry *>& InterfaceStatistics::getMonthStatistics() const
{
    return mMonthStatistics;
}

const QList<StatisticEntry *>& InterfaceStatistics::getYearStatistics() const
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
    foreach ( mCurrentDay, mDayStatistics )
    {
        if ( mCurrentDay->day == QDate::currentDate().day() &&
             mCurrentDay->month == QDate::currentDate().month() &&
             mCurrentDay->year == QDate::currentDate().year() )
        {
            // found current day in list
            return;
        }
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
    foreach ( mCurrentMonth, mMonthStatistics )
    {
        if ( mCurrentMonth->month == QDate::currentDate().month() &&
             mCurrentMonth->year == QDate::currentDate().year() )
        {
            // found current month in list
            return;
        }
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
    foreach ( mCurrentYear, mYearStatistics )
    {
        if ( mCurrentYear->year == QDate::currentDate().year() )
        {
            // found current year in list
            return;
        }
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

