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

#include <QDebug>

#include <KConfigGroup>
#include <KGlobal>
#include <KCalendarSystem>

#include "interface.h"
#include "interfacestatistics.h"

static const char* const statistics_prefix = "/statistics_";

static const char* const doc_name     = "statistics";
static const char* const group_days   = "days";
static const char* const group_weeks  = "weeks";
static const char* const group_months = "months";
static const char* const group_years  = "years";

static const char* const elem_day   = "day";
static const char* const elem_week  = "week";
static const char* const elem_month = "month";
static const char* const elem_year  = "year";

static const char* const attrib_calendar = "calendar";
static const char* const attrib_span     = "span";
static const char* const attrib_rx       = "rxBytes";
static const char* const attrib_tx       = "txBytes";

// Needed to upgrade from earlier versions
static const char* attrib_day   = "day";
static const char* attrib_month = "month";
static const char* attrib_year  = "year";

static bool statisticsLessThan( const StatisticEntry *s1, const StatisticEntry *s2 )
{
    if ( s1->date < s2->date )
        return true;
    else
        return false;
}


InterfaceStatistics::InterfaceStatistics( Interface* interface )
    : QObject(),
      mInterface( interface ),
      mWarningDone( false ),
      mAllMonths( true ),
      mBillingStart( mInterface->getSettings().billingStart ),
      mCalendar( KCalendarSystem::create( mInterface->getSettings().calendar ) )
{
    loadStatistics();
    if ( mMonthStatistics.count() )
    {
        QDate nextStart = getNextMonthStart( mBillingStart );
        if ( mBillingStart.daysTo( nextStart ) != mMonthStatistics.at( mMonthStatistics.count() -1 )->span
             || mBillingStart != mMonthStatistics.at( mMonthStatistics.count() - 1 )->date )
            rebuildStats( QDate( mBillingStart ), Month );
    }
    initStatistics();
    mSaveTimer = new QTimer();
    connect( mSaveTimer, SIGNAL( timeout() ), this, SLOT( saveStatistics() ) );
}

InterfaceStatistics::~InterfaceStatistics()
{
    mSaveTimer->stop();
    delete mSaveTimer;

    mDayStatistics.clear();
    mWeekStatistics.clear();
    mMonthStatistics.clear();
    mYearStatistics.clear();

    if ( mInterface->getSettings().customBilling )
        saveBillingStart();
}

void InterfaceStatistics::loadStatsGroup( const KCalendarSystem * cal, const QDomElement& parentItem, int group, QList<StatisticEntry *>& statistics )
{
    QString groupName;

    switch ( group )
    {
        case Day:
            groupName = group_days;
            break;
        case Week:
            groupName = group_weeks;
            break;
        case Month:
            groupName = group_months;
            mAllMonths = true;
            break;
        case Year:
            groupName = group_years;
            break;
    }

    QDomNode n = parentItem.namedItem( groupName );
    if ( !n.isNull() )
    {
        QDomNode node = n.firstChild();
        while ( !node.isNull() )
        {
            QDomElement element = node.toElement();
            if ( !element.isNull() )
            {
                StatisticEntry* entry = new StatisticEntry();
                entry->rxBytes = (quint64) element.attribute( attrib_rx ).toDouble();
                entry->txBytes = (quint64) element.attribute( attrib_tx ).toDouble();

                // The following attributes are particular to each statistic category
                int day = 1;
                switch ( group )
                {
                    case Day:
                        cal->setYMD( entry->date,
                                      element.attribute( attrib_year ).toInt(),
                                      element.attribute( attrib_month ).toInt(),
                                      element.attribute( attrib_day ).toInt() );

                        entry->span = 1;
                        break;
                    case Week:
                        cal->setYMD( entry->date,
                                      element.attribute( attrib_year ).toInt(),
                                      element.attribute( attrib_month ).toInt(),
                                      element.attribute( attrib_day ).toInt() );

                        // If calendar has changed then this will be recalculated elsewhere
                        entry->span = element.attribute( attrib_span ).toInt();
                        break;
                    case Month:
                        if ( element.hasAttribute( attrib_day ) )
                            day = element.attribute( attrib_day ).toInt();

                        cal->setYMD( entry->date,
                                      element.attribute( attrib_year ).toInt(),
                                      element.attribute( attrib_month ).toInt(),
                                      day );

                        entry->span = element.attribute( attrib_span ).toInt();
                        // Old format had no span, so daysInMonth using gregorian
                        if ( entry->span == 0 )
                            entry->span = entry->date.daysInMonth();
                        if ( cal->day( entry->date ) != 1 ||
                             entry->span != cal->daysInMonth( entry->date ) )
                            mAllMonths = false;
                        break;
                    case Year:
                        cal->setYMD( entry->date,
                                      element.attribute( attrib_year ).toInt(),
                                      1,
                                      1 );

                        // If calendar has changed then this will be recalculated elsewhere
                        entry->span = element.attribute( attrib_span ).toInt();
                        break;
                }
                if ( entry->date.isValid() )
                    statistics.append( entry );
                else
                    delete entry;
            }
            node = node.nextSibling();
        }
        qSort( statistics.begin(), statistics.end(), statisticsLessThan );
    }
}

void InterfaceStatistics::loadStatistics()
{
    QDomDocument doc( doc_name );
    KUrl dir( mInterface->getGeneralData().statisticsDir );
    if ( !dir.isLocalFile() )
        return;
    QFile file( dir.path() + statistics_prefix + mInterface->getName() );

    if ( !file.open( QIODevice::ReadOnly ) )
        return;
    if ( !doc.setContent( &file ) )
    {
        file.close();
        return;
    }
    file.close();

    mDayStatistics.clear();
    mWeekStatistics.clear();
    mMonthStatistics.clear();
    mYearStatistics.clear();

    QDomElement root = doc.documentElement();

    // If unknown or empty calendar it will default to gregorian
    KCalendarSystem *inCal = KCalendarSystem::create( root.attribute( attrib_calendar ) );
    loadStatsGroup( inCal, root, Day, mDayStatistics );
    loadStatsGroup( inCal, root, Week, mWeekStatistics );
    loadStatsGroup( inCal, root, Month, mMonthStatistics );
    loadStatsGroup( inCal, root, Year, mYearStatistics );

    // Discrepency: rebuild week and year based on calendar type in settings
    if ( root.attribute( attrib_calendar ).isEmpty() ||
         inCal->calendarType() != mCalendar->calendarType() )
        rebuildStats( mDayStatistics.first()->date, Week | Year );
    if ( mAllMonths == false && mInterface->getSettings().customBilling == false )
        rebuildStats( mMonthStatistics.first()->date, Month );
}

void InterfaceStatistics::buildStatsGroup( QDomDocument& doc, int group, const QList<StatisticEntry *>& statistics )
{
    QString groupName;
    QString elementName;
    switch ( group )
    {
        case Day:
            groupName = group_days;
            elementName = elem_day;
            break;
        case Week:
            groupName = group_weeks;
            elementName = elem_week;
            break;
        case Month:
            groupName = group_months;
            elementName = elem_month;
            break;
        case Year:
            groupName = group_years;
            elementName = elem_year;
    }
    QDomElement elements = doc.createElement( groupName );
    foreach( StatisticEntry *entry, statistics )
    {
        QDomElement element = doc.createElement( elementName );
        element.setAttribute( attrib_day, mCalendar->day( entry->date ) );
        element.setAttribute( attrib_month, mCalendar->month( entry->date ) );
        element.setAttribute( attrib_year, mCalendar->year( entry->date ) );
        if ( group > Day )
            element.setAttribute( attrib_span, entry->span );
        element.setAttribute( attrib_rx, (double) entry->rxBytes );
        element.setAttribute( attrib_tx, (double) entry->txBytes );
        elements.appendChild( element );
    }
    QDomElement statElement = doc.elementsByTagName( doc_name ).at( 0 ).toElement();
    statElement.appendChild( elements );
}

void InterfaceStatistics::saveStatistics()
{
    QDomDocument doc( doc_name );
    QDomElement docElement = doc.createElement( doc_name );
    docElement.setAttribute( attrib_calendar, mCalendar->calendarType() );
    doc.appendChild( docElement );

    buildStatsGroup( doc, Day, mDayStatistics );
    buildStatsGroup( doc, Week, mWeekStatistics );
    buildStatsGroup( doc, Month, mMonthStatistics );
    buildStatsGroup( doc, Year, mYearStatistics );

    KUrl dir( mInterface->getGeneralData().statisticsDir );
    if ( !dir.isLocalFile() )
        return;
    QFile file( dir.path() + statistics_prefix + mInterface->getName() );
    if ( !file.open( QIODevice::WriteOnly ) )
        return;

    QTextStream stream( &file );
    stream << doc.toString();
    file.close();
}

void InterfaceStatistics::configChanged()
{
    // restart the timer with the new value
    mSaveTimer->stop();
    if ( mInterface->getGeneralData().saveInterval > 0 )
        mSaveTimer->setInterval( mInterface->getGeneralData().saveInterval * 1000 );

    mWarningDone = false;
    // force a new ref day for billing periods
    mBillingStart = mInterface->getSettings().billingStart;
    if ( mAllMonths == false && mInterface->getSettings().customBilling == false )
        mBillingStart = mMonthStatistics.first()->date.addDays( 1 - mCalendar->day( mMonthStatistics.first()->date ) );
    // rebuildStats modifies mBillingStart, so copy first.
    rebuildStats( QDate( mBillingStart ), Month );

    mAllMonths = true;
    foreach ( StatisticEntry *entry, mMonthStatistics )
    {
        if ( mCalendar->day( entry->date ) != 1 ||
             entry->span != mCalendar->daysInMonth( entry->date ) )
            mAllMonths = false;
    }
    emit monthStatisticsChanged( true );
}

void InterfaceStatistics::addIncomingData( unsigned long data )
{
    checkCurrentEntry();

    mCurrentDay->rxBytes += data;
    mCurrentWeek->rxBytes += data;
    mCurrentMonth->rxBytes += data;
    mCurrentYear->rxBytes += data;

    // Only warn once per interface per session
    if ( !mWarningDone && mInterface->getSettings().warnThreshold > 0.0 )
    {
        quint64 thresholdBytes = mInterface->getSettings().warnThreshold * 1073741824;
        if ( mInterface->getSettings().warnTotalTraffic )
        {
            if ( mCurrentMonth->rxBytes + mCurrentMonth->txBytes >= thresholdBytes )
            {
                mWarningDone = true;
                emit warnMonthlyTraffic( mCurrentMonth->rxBytes + mCurrentMonth->txBytes );
            }
        }
        else if ( mCurrentMonth->rxBytes >= thresholdBytes )
        {
            mWarningDone = true;
            emit warnMonthlyTraffic( mCurrentMonth->rxBytes );
        }
    }
    emit currentEntryChanged();
}

void InterfaceStatistics::addOutgoingData( unsigned long data )
{
    checkCurrentEntry();

    mCurrentDay->txBytes += data;
    mCurrentWeek->txBytes += data;
    mCurrentMonth->txBytes += data;
    mCurrentYear->txBytes += data;

    emit currentEntryChanged();
}

void InterfaceStatistics::clearStatistics()
{
    QDate currentDate = QDate::currentDate();

    mDayStatistics.clear();
    mWeekStatistics.clear();
    mMonthStatistics.clear();
    mYearStatistics.clear();
    updateCurrentDay( currentDate );
    updateCurrentWeek( currentDate );
    updateCurrentMonth( currentDate );
    updateCurrentYear( currentDate );
}

void InterfaceStatistics::checkCurrentEntry()
{
    QDate currentDate = QDate::currentDate();

    if ( mCurrentDay->date != currentDate )
    {
        updateCurrentDay( currentDate );

        if ( mCalendar->weekNumber( mCurrentWeek->date ) != mCalendar->weekNumber( currentDate ) ||
             mCurrentWeek->date.addDays( mCalendar->daysInWeek( mCurrentWeek->date ) ) <= currentDate )
            updateCurrentWeek( currentDate );

        if ( mCurrentMonth->date.addDays( mCurrentMonth->span ) <= currentDate )
            updateCurrentMonth( currentDate );

        if ( mCalendar->year( mCurrentYear->date ) != mCalendar->year( currentDate ) )
            updateCurrentYear( currentDate );
    }
}

void InterfaceStatistics::initStatistics()
{
    QDate currentDate = QDate::currentDate();

    updateCurrentDay( currentDate );
    updateCurrentWeek( currentDate );
    updateCurrentMonth( currentDate );
    updateCurrentYear( currentDate );

    emit currentEntryChanged();
}

void InterfaceStatistics::updateCurrentDay( const QDate &currentDate )
{
    foreach ( mCurrentDay, mDayStatistics )
    {
        if ( mCurrentDay->date == currentDate )
            return;
    }

    mCurrentDay = new StatisticEntry();
    mCurrentDay->date = currentDate;
    mDayStatistics.append( mCurrentDay );
    emit dayStatisticsChanged();
}

StatisticEntry * InterfaceStatistics::genNewWeek( const QDate &date )
{
    StatisticEntry *week = new StatisticEntry();
    int dow = mCalendar->dayOfWeek( date );
    // ISO8601: week always starts on a Monday
    week->date = date.addDays( 1 - dow );
    week->span = mCalendar->daysInWeek( week->date );
    return week;
}

QDate InterfaceStatistics::getNextMonthStart( QDate nextMonthStart )
{
    int length = mInterface->getSettings().billingMonths;
    for ( int i = 0; i < length; i++ )
    {
        QDate refDay;
        mCalendar->setYMD( refDay, mCalendar->year( nextMonthStart ), mCalendar->month( nextMonthStart ), 1 );
        refDay = refDay.addDays( mCalendar->daysInMonth( refDay ) );

        nextMonthStart = nextMonthStart.addDays( mCalendar->daysInMonth( nextMonthStart ) );

        // Ensure we don't get weird spans like jan31->mar2
        // Instead, days will drift to a value that all months can handle.
        // Test for problematic dates in config module!
        if ( mCalendar->day( mBillingStart ) > 1 )
        {
            while ( mCalendar->month( nextMonthStart ) != mCalendar->month( refDay ) )
                nextMonthStart = nextMonthStart.addDays( -1 );
        }
    }
    return nextMonthStart;
}

StatisticEntry * InterfaceStatistics::genNewMonth( const QDate &date, QDate recalcDate )
{
    StatisticEntry *month = new StatisticEntry();

    // Partial month.  Very little to do.
    if ( recalcDate.isValid() )
    {
        month->date = date;
        month->span = date.daysTo( recalcDate );
        return month;
    }

    // Given a calendar day and a billing period start date, find a
    // billing period that the day belongs in.
    month->date = mBillingStart;
    QDate nextMonthStart = month->date;
    while ( nextMonthStart <= date )
    {
        month->date = nextMonthStart;
        nextMonthStart = getNextMonthStart( nextMonthStart );
    }

    month->span = month->date.daysTo( nextMonthStart );
    mBillingStart = month->date;

    return month;
}

StatisticEntry * InterfaceStatistics::genNewYear( const QDate &date )
{
    StatisticEntry *year = new StatisticEntry();
    int doy = mCalendar->dayOfYear( date );
    year->date = date.addDays( 1 - doy );
    year->span = mCalendar->daysInYear( year->date );
    return year;
}

void InterfaceStatistics::updateCurrentWeek( const QDate &currentDate )
{
    if ( mWeekStatistics.count() )
    {
        mCurrentWeek = mWeekStatistics.last();
        if ( mCurrentWeek->date.addDays( mCurrentWeek->span ) > currentDate )
            return;
    }

    mCurrentWeek = genNewWeek( currentDate );

    mWeekStatistics.append( mCurrentWeek );
    emit weekStatisticsChanged();
}

void InterfaceStatistics::updateCurrentMonth( const QDate &currentDate )
{
    if ( mMonthStatistics.count() )
    {
        mCurrentMonth = mMonthStatistics.last();
        if ( mCurrentMonth->date.addDays( mCurrentMonth->span ) > currentDate )
            return;
    }

    mCurrentMonth = genNewMonth( currentDate );
    mMonthStatistics.append( mCurrentMonth );

    emit monthStatisticsChanged( false );
}

void InterfaceStatistics::saveBillingStart()
{
    mInterface->getSettings().billingStart = mBillingStart;
    KConfig *config = KGlobal::config().data();
    KConfigGroup interfaceGroup( config, "Interface_" + mInterface->getName() );
    interfaceGroup.writeEntry( "BillingStart", mBillingStart );
    config->sync();
}

void InterfaceStatistics::updateCurrentYear( const QDate &currentDate )
{
    if ( mYearStatistics.count() )
    {
        mCurrentYear = mYearStatistics.last();
        if ( mCurrentYear->date.addDays( mCurrentYear->span ) > currentDate )
            return;
    }

    mCurrentYear = genNewYear( currentDate );
    mYearStatistics.append( mCurrentYear );
    emit yearStatisticsChanged();
}

QDate InterfaceStatistics::setRebuildDate( QList<StatisticEntry *>& statistics, const QDate &recalcDate, int group )
{
    QDate returnDate = recalcDate;

    // Keep removing entries and rolling back returnDate while
    // entry's start date + span > returnDate
    for ( int i = statistics.size() - 1; i >= 0; --i )
    {
        if ( statistics.at( i )->date.addDays( statistics.at( i )->span ) > returnDate ||
             statistics.at( i )->span < 1 )
        {
            if ( returnDate > statistics.at( i )->date )
                returnDate = statistics.at( i )->date;
            statistics.removeAt( i );
        }
        else
            break;
    }

    // now take care of instances when we're going earlier than the first recorded stats.

    // force full rebuild on months
    if ( group & Month &&
         mAllMonths == false &&
         mInterface->getSettings().customBilling == false
       )
    {
        if ( statistics.size() > 0 )
        {
            returnDate = returnDate.addDays( 1 - mCalendar->day( statistics.at( 0 )->date ) );
            statistics.clear();
        }
        returnDate = returnDate.addDays( 1 - mCalendar->day( returnDate ) );
    }

    if ( group & Week )
    {
        returnDate = returnDate.addDays( 1 - mCalendar->dayOfWeek( returnDate ) );
        while ( returnDate > recalcDate )
            returnDate = returnDate.addDays( -mCalendar->daysInWeek( returnDate ) );
    }
    else if ( group & Year )
    {
        returnDate = returnDate.addDays( 1 - mCalendar->dayOfYear( returnDate ) );
        while ( returnDate > recalcDate )
            returnDate = returnDate.addDays( mCalendar->daysInYear( returnDate ) );
    }

    return returnDate;
}

void InterfaceStatistics::rebuildStats( const QDate &recalcDate, int group )
{
    bool partial = false;
    StatisticEntry* weekEntry = 0;
    StatisticEntry* monthEntry = 0;
    StatisticEntry* yearEntry = 0;
    QDate weekStart;
    QDate monthStart;
    QDate yearStart;
    QDate walkbackDate;

    QList<QDate> s;
    s.append( recalcDate );

    if ( group & Week )
    {
        weekStart = setRebuildDate( mWeekStatistics, recalcDate, Week );
        s.append( weekStart );
    }
    if ( group & Month )
    {
        monthStart = setRebuildDate( mMonthStatistics, recalcDate, Month );
        // force an old date
        mBillingStart = monthStart;
        if ( recalcDate > monthStart )
            partial = true;
        s.append( monthStart );
    }
    if ( group & Year )
    {
        yearStart = setRebuildDate( mYearStatistics, recalcDate, Year );
        s.append( yearStart );
    }

    // Now find how far back we'll need to go
    qSort( s );
    walkbackDate = s.first();

    // Big loop, but this way we iterate through mDayStatistics once
    // no matter how many categories we're rebuilding
    foreach( StatisticEntry* day, mDayStatistics )
    {
        if ( day->date < walkbackDate )
            continue;

        if ( group & Week && day->date >= weekStart )
        {
            if ( !weekEntry || mCalendar->weekNumber( weekEntry->date ) != mCalendar->weekNumber( day->date ) ||
                 weekEntry->date.addDays( mCalendar->daysInWeek( weekEntry->date ) ) <= day->date )
            {
                if ( weekEntry )
                    mWeekStatistics.append( weekEntry );
                weekEntry = genNewWeek( day->date );
            }
            weekEntry->rxBytes += day->rxBytes;
            weekEntry->txBytes += day->txBytes;
        }

        if ( group & Month && day->date >= monthStart )
        {
            if ( !monthEntry || day->date >= monthEntry->date.addDays( monthEntry->span ) )
            {
                if ( monthEntry )
                    mMonthStatistics.append( monthEntry );
                if ( partial )
                {
                    monthEntry = genNewMonth( monthStart, recalcDate );
                    // Partial month created; next period will begin on recalcDate
                    mBillingStart = recalcDate;
                    partial = false;
                }
                else
                    monthEntry = genNewMonth( day->date );
            }
            monthEntry->rxBytes += day->rxBytes;
            monthEntry->txBytes += day->txBytes;
        }
        if ( group & Year && day->date >= yearStart )
        {
            if ( !yearEntry || mCalendar->year( yearEntry->date ) != mCalendar->year( day->date ) )
            {
                if ( yearEntry )
                    mYearStatistics.append( yearEntry );
                yearEntry = genNewYear( day->date );
            }
            yearEntry->rxBytes += day->rxBytes;
            yearEntry->txBytes += day->txBytes;
        }
    }

    mCurrentWeek = appendStats( mWeekStatistics, weekEntry );
    mCurrentMonth = appendStats( mMonthStatistics, monthEntry );
    mCurrentYear = appendStats( mYearStatistics, yearEntry );
}

StatisticEntry * InterfaceStatistics::appendStats( QList<StatisticEntry *>& statistics, StatisticEntry *entry )
{
    if ( entry )
        statistics.append( entry );
    return statistics.last();
}
