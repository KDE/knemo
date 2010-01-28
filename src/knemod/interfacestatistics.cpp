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

#include <QDomNode>
#include <QFile>
#include <QTimer>

#include <KCalendarSystem>
#include <KConfigGroup>
#include <KGlobal>

#include <unistd.h>

#include "interface.h"
#include "interfacestatistics.h"

static const char statistics_prefix[] = "/statistics_";

static const char doc_name[]     = "statistics";
static const char group_hours[]  = "hours";
static const char group_days[]   = "days";
static const char group_weeks[]  = "weeks";
static const char group_months[] = "months";
static const char group_years[]  = "years";

static const char elem_hour[]  = "hour";
static const char elem_day[]   = "day";
static const char elem_week[]  = "week";
static const char elem_month[] = "month";
static const char elem_year[]  = "year";

static const char attrib_calendar[] = "calendar";
static const char attrib_version[]  = "version";
static const char attrib_span[]     = "span";
static const char attrib_rx[]       = "rxBytes";
static const char attrib_tx[]       = "txBytes";

static const char attrib_hour[]  = "hour";
static const char attrib_day[]   = "day";
static const char attrib_month[] = "month";
static const char attrib_year[]  = "year";


InterfaceStatistics::InterfaceStatistics( Interface* interface )
    : QObject(),
      mSaveTimer( new QTimer() ),
      mInterface( interface ),
      mAllMonths( true )
{
    StatisticsModel * s = new StatisticsModel( StatisticsModel::Hour, group_hours, elem_hour );
    mModels.insert( StatisticsModel::Hour, s );
    s = new StatisticsModel( StatisticsModel::Day, group_days, elem_day );
    mModels.insert( StatisticsModel::Day, s );
    s = new StatisticsModel( StatisticsModel::Week, group_weeks, elem_week );
    mModels.insert( StatisticsModel::Week, s );
    s = new StatisticsModel( StatisticsModel::Month, group_months, elem_month );
    mModels.insert( StatisticsModel::Month, s );
    s = new StatisticsModel( StatisticsModel::Year, group_years, elem_year );
    mModels.insert( StatisticsModel::Year, s );

    connect( mSaveTimer, SIGNAL( timeout() ), this, SLOT( saveStatistics() ) );

    loadConfig();
    loadStatistics();
}

InterfaceStatistics::~InterfaceStatistics()
{
    mSaveTimer->stop();
    delete mSaveTimer;

    saveStatistics();

    if ( mInterface->getSettings().customBilling )
    {
        mInterface->getSettings().billingStart = mBillingStart;
        KConfig *config = KGlobal::config().data();
        KConfigGroup interfaceGroup( config, confg_interface + mInterface->getName() );
        interfaceGroup.writeEntry( conf_billingStart, mBillingStart );
        config->sync();
    }
}

void InterfaceStatistics::clearStatistics()
{
    foreach( StatisticsModel * s, mModels )
        s->clearRows();
    emit currentEntryChanged();
}

void InterfaceStatistics::loadConfig()
{
    mCalendar = KCalendarSystem::create( mInterface->getSettings().calendar );
    // TODO: Test for a KDE release that contains SVN commit 1013534
    KGlobal::locale()->setCalendar( mCalendar->calendarType() );
    foreach( StatisticsModel * s, mModels )
        s->setCalendar( mCalendar );

    mWarningDone = false;
    mBillingStart = mInterface->getSettings().billingStart;

    if ( mInterface->getGeneralData().saveInterval > 0 )
    {
        mSaveTimer->setInterval( mInterface->getGeneralData().saveInterval * 1000 );
        mSaveTimer->start();
    }
}

void InterfaceStatistics::configChanged()
{
    mSaveTimer->stop();
    QString oldType = mCalendar->calendarType();
    loadConfig();
    checkRebuild( oldType );
}

void InterfaceStatistics::loadStatsGroup( const KCalendarSystem * cal, const QDomElement& parentItem,
                                          StatisticsModel* statistics )
{
    QDomNode n = parentItem.namedItem( statistics->group() );
    if ( !n.isNull() )
    {
        QDomNode node = n.firstChild();
        while ( !node.isNull() )
        {
            QDomElement element = node.toElement();
            if ( !element.isNull() )
            {
                // The following attributes are particular to each statistic category
                QDate date;
                QTime time;

                int year = element.attribute( attrib_year ).toInt();
                int month = element.attribute( attrib_month, "1" ).toInt();
                int day = element.attribute( attrib_day, "1" ).toInt();
                cal->setDate( date, year, month, day );

                if ( date.isValid() )
                {
                    int days = element.attribute( attrib_span ).toInt();
                    switch ( statistics->type() )
                    {
                        case StatisticsModel::Hour:
                            time = QTime( element.attribute( attrib_hour ).toInt(), 0 );
                            break;
                        case StatisticsModel::Month:
                            // Old format had no span, so daysInMonth using gregorian
                            if ( days == 0 )
                                days = date.daysInMonth();
                            if ( cal->day( date ) != 1 ||
                                 days != cal->daysInMonth( date ) )
                                mAllMonths = false;
                            break;
                        case StatisticsModel::Year:
                            // Old format had no span, so daysInYear using gregorian
                            if ( days == 0 )
                                days = date.daysInYear();
                            break;
                        case StatisticsModel::Day:
                            days = 1;
                        default:
                            ;;
                    }

                    statistics->appendStats( QDateTime( date, time ), days,
                            element.attribute( attrib_rx ).toULongLong(),
                            element.attribute( attrib_tx ).toULongLong() );
                }
            }
            node = node.nextSibling();
        }
    }
}

void InterfaceStatistics::saveStatsGroup( QDomDocument& doc, const StatisticsModel* statistics )
{
    QString groupName;
    QString elementName;
    QDomElement elements = doc.createElement( statistics->group() );
    for ( int i = 0; i < statistics->rowCount(); ++i )
    {
        QDomElement element = doc.createElement( statistics->elem() );
        QDate date = statistics->date( i );
        element.setAttribute( attrib_day, mCalendar->day( date ) );
        element.setAttribute( attrib_month, mCalendar->month( date ) );
        element.setAttribute( attrib_year, mCalendar->year( date ) );
        if ( statistics->type() == StatisticsModel::Hour )
            element.setAttribute( attrib_hour, statistics->dateTime( i ).time().hour() );
        else if ( statistics->type() > StatisticsModel::Day )
            element.setAttribute( attrib_span, statistics->days( i ) );
        element.setAttribute( attrib_rx, statistics->rxBytes( i ) );
        element.setAttribute( attrib_tx, statistics->txBytes( i ) );
        elements.appendChild( element );
    }
    QDomElement statElement = doc.elementsByTagName( doc_name ).at( 0 ).toElement();
    statElement.appendChild( elements );
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

    QDomElement root = doc.documentElement();

    // If unknown or empty calendar it will default to gregorian
    KCalendarSystem *inCal = KCalendarSystem::create( root.attribute( attrib_calendar ) );
    foreach( StatisticsModel * s, mModels )
        loadStatsGroup( inCal, root, s );

    checkRebuild( inCal->calendarType() );
}

void InterfaceStatistics::saveStatistics()
{
    QDomDocument doc( doc_name );
    QDomElement docElement = doc.createElement( doc_name );
    docElement.setAttribute( attrib_calendar, mCalendar->calendarType() );
    docElement.setAttribute( attrib_version, "1.2" );
    doc.appendChild( docElement );

    foreach( StatisticsModel * s, mModels )
        saveStatsGroup( doc, s );

    KUrl dir( mInterface->getGeneralData().statisticsDir );
    if ( !dir.isLocalFile() )
        return;

    QFile file( dir.path() + statistics_prefix + mInterface->getName() );
    if ( file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
    {
        file.write( doc.toByteArray() );
        fsync( file.handle() );
        file.close();
    }
}

/**
 * Given a model with statistics of a certain unit (year, month, week, etc.)
 * and a requested rebuild date, how far back to we actually need to go
 * to accuratly rebuild statistics from the daily stats.
 */
QDate InterfaceStatistics::setRebuildDate( StatisticsModel* statistics,
                                           const QDate &recalcDate )
{
    QDate returnDate = recalcDate;
    if ( statistics->type() <= StatisticsModel::Day )
        return returnDate;

    // Keep removing entries and rolling back returnDate while
    // entry's start date + days > returnDate
    for ( int i = statistics->rowCount() - 1; i >= 0; --i )
    {
        int days = statistics->days( i );
        QDate date = statistics->date( i ).addDays( days );
        if ( date > mModels.value( StatisticsModel::Day )->date( 0 ) &&
             ( date > returnDate || days < 1 )
           )
        {
            if ( returnDate > statistics->date( i ) )
                returnDate = statistics->date( i );
            statistics->removeRow( i );
        }
        else
            break;
    }

    // now take care of instances when we're going earlier than the first recorded stats.
    if ( statistics->type() == StatisticsModel::Week )
    {
        returnDate = returnDate.addDays( 1 - mCalendar->dayOfWeek( returnDate ) );
        while ( returnDate > recalcDate )
            returnDate = returnDate.addDays( -mCalendar->daysInWeek( returnDate ) );
    }
    else if ( statistics->type() == StatisticsModel::Year )
    {
        returnDate = returnDate.addDays( 1 - mCalendar->dayOfYear( returnDate ) );
        while ( returnDate > recalcDate )
            returnDate = returnDate.addDays( mCalendar->daysInYear( returnDate ) );
    }

    return returnDate;
}

void InterfaceStatistics::amendStats( int i, StatisticsModel* model )
{
    StatisticsModel *days = mModels.value( StatisticsModel::Day );
    model->addRxBytes( days->rxBytes( i ) );
    model->addTxBytes( days->txBytes( i ) );
}

void InterfaceStatistics::doRebuild( const QDate &date, int groups )
{
    QDate recalcDate( date );
    bool partial = false;
    QDate weekStart;
    QDate monthStart;
    QDate yearStart;
    QDate walkbackDate;

    QList<QDate> s;
    s.append( recalcDate );

    if ( groups & StatisticsModel::Week )
    {
        weekStart = setRebuildDate( mModels.value( StatisticsModel::Week), recalcDate );
        s.append( weekStart );
    }
    if ( groups & StatisticsModel::Month )
    {
        monthStart = setRebuildDate( mModels.value( StatisticsModel::Month), recalcDate );
        // force an old date
        mBillingStart = monthStart;
        if ( recalcDate > monthStart )
            partial = true;
        s.append( monthStart );
    }
    if ( groups & StatisticsModel::Year )
    {
        yearStart = setRebuildDate( mModels.value( StatisticsModel::Year ), recalcDate );
        s.append( yearStart );
    }

    // Now find how far back we'll need to go
    qSort( s );
    walkbackDate = s.first();

    StatisticsModel *days = mModels.value( StatisticsModel::Day );
    for ( int i = 0; i < days->rowCount(); ++i )
    {
        QDate day = days->date( i );
        if ( day < walkbackDate )
            continue;

        if ( groups & StatisticsModel::Week && day >= weekStart )
        {
            genNewWeek( day );
            amendStats( i, mModels.value( StatisticsModel::Week ) );
        }

        if ( groups & StatisticsModel::Month && day >= monthStart )
        {
            // Not very pretty
            StatisticsModel *month = mModels.value( StatisticsModel::Month );
            if ( !partial )
            {
                genNewMonth( day );
                amendStats( i, month );
            }
            else
            {
                genNewMonth( monthStart, recalcDate );
                amendStats( i, month );
                // Partial month created; next period will begin on recalcDate
                mBillingStart = recalcDate;
                partial = false;
            }
        }
        if ( groups & StatisticsModel::Year && day >= yearStart )
        {
            genNewYear( day );
            amendStats( i, mModels.value( StatisticsModel::Year ) );
        }
    }
}

void InterfaceStatistics::checkRebuild( QString oldType )
{
    if ( oldType != mCalendar->calendarType() )
    {
        KUrl dir( mInterface->getGeneralData().statisticsDir );
        if ( dir.isLocalFile() )
        {
            QFile file( dir.path() + statistics_prefix + mInterface->getName() );
            file.copy( dir.path() + statistics_prefix + mInterface->getName() +
                   QString( "_%1.bak" ).arg( QDateTime::currentDateTime().toString( "yyyy-MM-dd-hhmmss" ) ) );
        }
        int modThese = StatisticsModel::Week | StatisticsModel::Year;
        QDate date = mModels.value( StatisticsModel::Day )->date( 0 );
        doRebuild( date, modThese );
    }

    StatisticsModel *month = mModels.value( StatisticsModel::Month );
    if ( mInterface->getSettings().customBilling == false )
    {
        if ( oldType != mCalendar->calendarType() || mAllMonths == false )
        {
            mBillingStart = month->date( 0 ).addDays( 1 - mCalendar->day( month->date( 0 ) ) );
        }
    }

    QDate nextStart = nextMonthDate( mBillingStart );
    if ( mBillingStart.daysTo( nextStart ) != month->days()
         || mBillingStart != month->date() )
        doRebuild( mBillingStart, StatisticsModel::Month );

    mAllMonths = true;
    for ( int i = 0; i < month->rowCount(); ++i )
    {
        QDate date = month->date( i );
        if ( mCalendar->day( date ) != 1 ||
            month->days( i ) != mCalendar->daysInMonth( date ) )
        {
            mAllMonths = false;
        }
    }

    emit currentEntryChanged();
}

/**
 * Find the next billing period's date.
 * This is complicated by a few things:
 *   1. it calculates the date according to whatever calendar type the
 *      the statistics are using
 *   2. the period can span one or more localized months
 *   3. termination dates can be awkward (e.g. if a period starts on
 *      Dec 30 and spans 2 months, which day should it end on?)
 **/
QDate InterfaceStatistics::nextMonthDate( const QDate &date )
{
    QDate nextDate( date );
    int length = mInterface->getSettings().billingMonths;
    for ( int i = 0; i < length; i++ )
    {
        QDate refDay;
        mCalendar->setDate( refDay, mCalendar->year( nextDate ), mCalendar->month( nextDate ), 1 );
        refDay = refDay.addDays( mCalendar->daysInMonth( refDay ) );

        nextDate = nextDate.addDays( mCalendar->daysInMonth( nextDate ) );

        // Ensure we don't get weird spans like Jan 31 -> Mar 2
        // Instead, days will drift to a value that all months can handle.
        // Test for problematic dates in config module!
        if ( mCalendar->day( mBillingStart ) > 1 )
        {
            while ( mCalendar->month( nextDate ) != mCalendar->month( refDay ) )
                nextDate = nextDate.addDays( -1 );
        }
    }
    return nextDate;
}

/**
 * Return true if at least one daily statistic entry is in a span of days
 **/
bool InterfaceStatistics::daysInSpan( const QDate& date, int days )
{
    StatisticsModel *model = mModels.value( StatisticsModel::Day );
    if ( !model->rowCount() )
        return false;

    QDate endDate = date.addDays( days );
    for ( int i = model->rowCount() - 1; i >= 0; --i )
    {
        // No others will be valid after this; stop early
        if ( model->date( i ) < date )
            return false;
        if ( model->date( i ) < endDate && model->date( i ) >= date )
            return true;
    }
    return false;
}

void InterfaceStatistics::genNewHour( const QDateTime &dateTime )
{
    StatisticsModel* hours = mModels.value( StatisticsModel::Hour );

    // Only 24 hours
    while ( hours->rowCount() )
    {
        if ( hours->dateTime( 0 ) <= dateTime.addDays( -1 ) )
            hours->removeRow( 0 );
        else
            break;
    }

    if ( hours->dateTime() == dateTime )
        return;

    hours->appendStats( dateTime, 0 );
}

void InterfaceStatistics::genNewDay( const QDate &date )
{
    StatisticsModel * model = mModels.value( StatisticsModel::Day );
    if ( model->rowCount() &&
         model->date().addDays( model->days() ) > date )
            return;

    mModels.value( StatisticsModel::Day )->appendStats( date, 1 );
}

void InterfaceStatistics::genNewWeek( const QDate &date )
{
    StatisticsModel * model = mModels.value( StatisticsModel::Week );
    if ( model->rowCount() &&
         model->date().addDays( model->days() ) > date )
            return;

    int dow = mCalendar->dayOfWeek( date );
    // ISO8601: week always starts on a Monday
    QDate newDate = date.addDays( 1 - dow );
    model->appendStats( newDate, mCalendar->daysInWeek( newDate ) );
}

void InterfaceStatistics::genNewMonth( const QDate &date, QDate endDate )
{
    StatisticsModel * model = mModels.value( StatisticsModel::Month );
    if ( model->rowCount() &&
         model->date().addDays( model->days() ) > date )
            return;

    int days;
    // Partial month.  Very little to do.
    if ( endDate.isValid() )
    {
        days = date.daysTo( endDate );
        if ( daysInSpan( date, date.daysTo( endDate ) ) )
        {
            mModels.value( StatisticsModel::Month )->appendStats( date, days );
            return;
        }
        else
        {
            // partial month contains no daily stats, so advance start date
            // and get a new period below
            mBillingStart = date.addDays( days );
        }
    }

    // Given a calendar day and a billing period start date, find a
    // billing period that the day belongs in.
    QDate newDate;
    QDate nextMonthStart = mBillingStart;
    do
    {
        newDate = nextMonthStart;
        nextMonthStart = nextMonthDate( newDate );
        days = newDate.daysTo( nextMonthStart );
    } while ( nextMonthStart <= date || !daysInSpan( newDate, days ) );

    mBillingStart = newDate;
    mModels.value( StatisticsModel::Month )->appendStats( newDate, days );
}

void InterfaceStatistics::genNewYear( const QDate &date )
{
    StatisticsModel * model = mModels.value( StatisticsModel::Year );
    if ( model->rowCount() &&
         model->date().addDays( model->days() ) > date )
            return;

    int doy = mCalendar->dayOfYear( date );
    QDate newDate = date.addDays( 1 - doy );
    mModels.value( StatisticsModel::Year )->appendStats( newDate, mCalendar->daysInYear( newDate ) );
}

void InterfaceStatistics::checkValidEntry()
{
    QDateTime curDateTime = QDateTime::currentDateTime();
    QDate curDate = curDateTime.date();
    StatisticsModel *days = mModels.value( StatisticsModel::Day );

    StatisticsModel *hours = mModels.value( StatisticsModel::Hour );
    if ( !hours->rowCount() || hours->dateTime().addSecs( 3600 ) < curDateTime )
        genNewHour( QDateTime( curDate, QTime( curDateTime.time().hour(), 0 ) ) );

    if ( !days->rowCount() || days->date() < curDate )
    {
        genNewDay( curDate );
        genNewWeek( curDate );
        genNewMonth( curDate );
        genNewYear( curDate );
    }
}

void InterfaceStatistics::checkTrafficLimit()
{
    // Only warn once per interface per session
    if ( !mWarningDone && mInterface->getSettings().warnThreshold > 0.0 )
    {
        quint64 thresholdBytes = mInterface->getSettings().warnThreshold * 1073741824;
        StatisticsModel *month = mModels.value( StatisticsModel::Month );
        if ( mInterface->getSettings().warnTotalTraffic )
        {
            if ( month->totalBytes() >= thresholdBytes )
            {
                mWarningDone = true;
                emit warnMonthlyTraffic( month->totalBytes() );
            }
        }
        else if ( month->rxBytes() >= thresholdBytes )
        {
            mWarningDone = true;
            emit warnMonthlyTraffic( month->rxBytes() );
        }
    }
}

void InterfaceStatistics::addRxBytes( unsigned long bytes )
{
    if ( bytes == 0 )
        return;

    checkValidEntry();

    foreach( StatisticsModel * s, mModels )
        s->addRxBytes( bytes );

    checkTrafficLimit();
    emit currentEntryChanged();
}

void InterfaceStatistics::addTxBytes( unsigned long bytes )
{
    if ( bytes == 0 )
        return;

    checkValidEntry();

    foreach( StatisticsModel * s, mModels )
        s->addTxBytes( bytes );

    checkTrafficLimit();
    emit currentEntryChanged();
}
