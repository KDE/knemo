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

#include <QTimer>

#include <KCalendarSystem>
#include <KConfigGroup>
#include <KGlobal>

#include <cmath>
#include <unistd.h>

#include "global.h"
#include "interface.h"
#include "interfacestatistics.h"
#include "statisticsmodel.h"
#include "syncstats/statsfactory.h"
#include "storage/sqlstorage.h"
#include "storage/xmlstorage.h"

static bool statsLessThan( const StatsRule& s1, const StatsRule& s2 )
{
    if ( s1.startDate < s2.startDate )
        return true;
    else
        return false;
}

InterfaceStatistics::InterfaceStatistics( Interface* interface )
    : QObject(),
      mInterface( interface ),
      mSaveTimer( new QTimer() ),
      mWarnTimer( new QTimer() ),
      mEntryTimer( new QTimer() ),
      mTrafficChanged( false )
{
    StatisticsModel * s = new StatisticsModel( KNemoStats::Hour, this );
    mModels.insert( KNemoStats::Hour, s );
    s = new StatisticsModel( KNemoStats::Day, this );
    mModels.insert( KNemoStats::Day, s );
    s = new StatisticsModel( KNemoStats::Week, this );
    mModels.insert( KNemoStats::Week, s );
    s = new StatisticsModel( KNemoStats::Month, this );
    mModels.insert( KNemoStats::Month, s );
    s = new StatisticsModel( KNemoStats::Year, this );
    mModels.insert( KNemoStats::Year, s );
    s = new StatisticsModel( KNemoStats::BillPeriod, this );
    mModels.insert( KNemoStats::BillPeriod, s );
    s = new StatisticsModel( KNemoStats::HourArchive, this );
    mModels.insert( KNemoStats::HourArchive, s );

    foreach ( StatisticsModel *s, mModels )
    {
        mStorageData.saveFromId.insert( s->periodType(), 0 );
    }

    connect( mSaveTimer, SIGNAL( timeout() ), this, SLOT( saveStatistics() ) );
    connect( mWarnTimer, SIGNAL( timeout() ), this, SLOT( checkWarnings() ) );
    connect( mEntryTimer, SIGNAL( timeout() ), this, SLOT( checkValidEntry() ) );

    KUrl dir( generalSettings->statisticsDir );
    sql = new SqlStorage( mInterface->getName() );
    loadStats();
    syncWithExternal( mStorageData.lastSaved );
    configChanged();
}

InterfaceStatistics::~InterfaceStatistics()
{
    mSaveTimer->stop();
    mWarnTimer->stop();
    mEntryTimer->stop();
    delete mSaveTimer;
    delete mWarnTimer;
    delete mEntryTimer;

    saveStatistics();
    delete sql;
    QSqlDatabase::removeDatabase( mInterface->getName() );
}

void InterfaceStatistics::saveStatistics( bool fullSave )
{
    sql->saveStats( &mStorageData, &mModels, &mStatsRules, fullSave );
}

bool InterfaceStatistics::loadStats()
{
    KUrl dir( generalSettings->statisticsDir );
    if ( !dir.isLocalFile() )
        return 0;

    bool loaded = false;

    if ( sql->dbExists() )
    {
        loaded = sql->loadStats( &mStorageData, &mModels, &mStatsRules );
        qSort( mStatsRules.begin(), mStatsRules.end(), statsLessThan );
    }
    else
    {
        XmlStorage xml;
        loaded = xml.loadStats( mInterface->getName(), &mStorageData, &mModels );
        sql->createDb();
        if ( loaded )
        {
            hoursToArchive( QDateTime::currentDateTime() );
            checkRebuild( mStorageData.calendar->calendarType(), true );
        }
    }

    if ( !mStorageData.calendar )
    {
        mStorageData.calendar = KCalendarSystem::create( mInterface->getSettings().calendar );
        foreach( StatisticsModel * s, mModels )
        {
            s->setCalendar( mStorageData.calendar );
        }
    }
    return loaded;
}

/**********************************
 * Stats Entry Generators         *
 **********************************/

void InterfaceStatistics::resetWarnings( int modelType )
{
    for ( int i = 0; i < mInterface->getSettings().warnRules.count(); ++i )
    {
        if ( modelType == mInterface->getSettings().warnRules[i].periodUnits )
            mInterface->getSettings().warnRules[i].warnDone = false;
    }
}

void InterfaceStatistics::hoursToArchive( const QDateTime &dateTime )
{
    bool removedRow = false;
    StatisticsModel* hours = mModels.value( KNemoStats::Hour );
    StatisticsModel* hourArchives = mModels.value( KNemoStats::HourArchive );

    // Only 24 hours
    while ( hours->rowCount() )
    {
        if ( hours->dateTime( 0 ) <= dateTime.addDays( -1 ) )
        {
            QList<QStandardItem*> row = hours->takeRow( 0 );
            hourArchives->appendRow( row );
            hourArchives->setId( mStorageData.nextHourId );
            mStorageData.nextHourId++;
            removedRow = true;
        }
        else
            break;
    }
    if ( removedRow )
    {
        for ( int i = 0; i < hours->rowCount(); ++i )
        {
            hours->setId( i, i );
        }
        mStorageData.saveFromId.insert( hours->periodType(), 0 );
        mStorageData.saveFromId.insert( hourArchives->periodType(), hourArchives->id( 0 ) );
    }
}

void InterfaceStatistics::genNewHour( const QDateTime &dateTime )
{
    hoursToArchive( dateTime );

    StatisticsModel* hours = mModels.value( KNemoStats::Hour );

    if ( hours->dateTime() == dateTime )
        return;

    int ruleIndex = ruleForDate( dateTime.date() );
    if ( ruleIndex >= 0  && isOffpeak( mStatsRules[ruleIndex], dateTime ) )
    {
        hours->addTrafficType( KNemoStats::OffpeakTraffic );
    }

    hours->createEntry();
    hours->setDateTime( dateTime );

    resetWarnings( hours->periodType() );
}

bool InterfaceStatistics::genNewCalendarType( const QDate &date, const KNemoStats::PeriodUnits stype )
{
    if ( stype < KNemoStats::Day || stype > KNemoStats::Year )
        return false;

    StatisticsModel * model = mModels.value( stype );
    if ( model->rowCount() &&
         model->date().addDays( model->days() ) > date )
            return false;

    QDate newDate;
    int dayOf;
    int weekStartDay = mStorageData.calendar->weekStartDay();

    switch ( stype )
    {
        case KNemoStats::Day:
            newDate = date;
            break;
        case KNemoStats::Week:
            dayOf = mStorageData.calendar->dayOfWeek( date );
            if ( dayOf >= weekStartDay )
                newDate = date.addDays( weekStartDay - dayOf );
            else
                newDate = date.addDays( weekStartDay - mStorageData.calendar->daysInWeek( date ) - dayOf );
            break;
        case KNemoStats::Month:
            dayOf = mStorageData.calendar->day( date );
            newDate = date.addDays( 1 - dayOf );
            break;
        case KNemoStats::Year:
            dayOf = mStorageData.calendar->dayOfYear( date );
            newDate = date.addDays( 1 - dayOf );
            break;
        default:
            return false;
    }

    mModels.value( stype )->createEntry();
    mModels.value( stype )->setDateTime( QDateTime( newDate, QTime() ) );
    resetWarnings( stype );

    return true;
}

// Return true if at least one daily statistic entry is in a span of days
bool InterfaceStatistics::daysInSpan( const QDate& date, int days )
{
    StatisticsModel *model = mModels.value( KNemoStats::Day );
    if ( !model->rowCount() )
        return false;

    QDate nextRuleStart = date.addDays( days );
    for ( int i = model->rowCount() - 1; i >= 0; --i )
    {
        // No others will be valid after this; stop early
        if ( model->date( i ) < date )
            return false;
        if ( model->date( i ) < nextRuleStart && model->date( i ) >= date )
            return true;
    }
    return false;
}

QDate InterfaceStatistics::nextBillPeriodStart( const StatsRule &rules, const QDate &date )
{
    QDate nextDate;
    QDate refDay;
    int modelType = rules.periodUnits;
    switch ( modelType )
    {
        case KNemoStats::Day:
            nextDate = date.addDays( rules.periodCount );
            break;
        case KNemoStats::Week:
            nextDate = date;
            for ( int i = 0; i < rules.periodCount; ++i )
                nextDate = nextDate.addDays( mStorageData.calendar->daysInWeek( nextDate ) );
            break;
        default:// KNemoStats::Month:
            nextDate = mStorageData.calendar->addMonths( date, rules.periodCount );
            // Example: one month starting Jan 31 = Jan 31 -> Mar 1
            // This seems the most common way to handle late start dates
            if ( mStorageData.calendar->day( nextDate ) < mStorageData.calendar->day( date ) )
                nextDate = nextDate.addDays( 1 );
            break;
    }
    return nextDate;
}

void InterfaceStatistics::genNewBillPeriod( const QDate &date )
{
    int ruleIndex = ruleForDate( date );
    if ( ruleIndex < 0 )
        return;

    StatisticsModel * billing = mModels.value( KNemoStats::BillPeriod );
    QDate nextRuleStart;

    if ( mStatsRules.count() > ruleIndex+1 )
        nextRuleStart = mStatsRules.at( ruleIndex+1 ).startDate;

    // Harder to find whether we should skip generating a new billing entry
    if ( nextRuleStart.isValid() && billing->rowCount() && billing->date().addDays( billing->days() ) >= nextRuleStart )
        return;

    int days;

    // Given a calendar day and a billing period start date, find a
    // billing period that the day belongs in.
    QDate newDate;
    QDate nextStartDate;
    if ( !billing->rowCount() ||
         billing->date() < mStatsRules.at( ruleIndex ).startDate )
    {
        nextStartDate = mStatsRules.at( ruleIndex ).startDate;
    }
    else
    {
        nextStartDate = billing->date();
    }

    do
    {
        newDate = nextStartDate;
        nextStartDate = nextBillPeriodStart( mStatsRules.at( ruleIndex ), newDate );
        days = newDate.daysTo( nextStartDate );
    } while ( nextStartDate <= date || !daysInSpan( newDate, days ) );

    // Truncate a billing period if necessary
    if ( nextRuleStart.isValid() && nextRuleStart < nextStartDate )
        days = newDate.daysTo( nextRuleStart );

    if ( billing->rowCount() && newDate == billing->date() )
        return;

    billing->createEntry();
    billing->setDays( days );
    billing->setDateTime( QDateTime( newDate, QTime() ) );

    resetWarnings( KNemoStats::BillPeriod );
}


// END STATS ENTRY GENERATORS

void InterfaceStatistics::configChanged()
{
    mSaveTimer->stop();
    mWarnTimer->stop();

    QString origCalendarType;
    if ( mStorageData.calendar )
       origCalendarType = mStorageData.calendar->calendarType();

    if ( mInterface->getSettings().calendar != origCalendarType )
    {
        mStorageData.calendar = KCalendarSystem::create( mInterface->getSettings().calendar );

        foreach( StatisticsModel * s, mModels )
        {
            s->setCalendar( mStorageData.calendar );
        }
        if ( !origCalendarType.isEmpty() )
        {
            StatisticsModel *hours = mModels.value( KNemoStats::Hour );
            StatisticsModel *days = mModels.value( KNemoStats::Day );
            for ( int i = 0; i < days->rowCount(); ++i )
            {
                days->updateDateText( i );
            }
            for ( int i = 0; i < hours->rowCount(); ++i )
            {
                hours->updateDateText( i );
            }
        }
    }

    checkRebuild( origCalendarType );

    if ( generalSettings->saveInterval > 0 )
    {
        mSaveTimer->setInterval( generalSettings->saveInterval * 1000 );
        mSaveTimer->start();
    }

    foreach ( StatisticsModel * s, mModels )
    {
        resetWarnings( s->periodType() );
    }

    checkValidEntry();

    mWarnTimer->setInterval( 2000 );
    mWarnTimer->start();
}

int InterfaceStatistics::ruleForDate( const QDate &date )
{
    for( int i = mStatsRules.count() - 1; i >= 0; --i )
    {
        if ( date >= mStatsRules[i].startDate )
            return i;
    }
    return -1;
}

void InterfaceStatistics::syncWithExternal( uint updated )
{
    ExternalStats *v = StatsFactory::stats( mInterface, mStorageData.calendar );
    if ( !v )
        return;

    v->importIfaceStats();
    const StatisticsModel *syncDays = v->days();
    const StatisticsModel *syncHours = v->hours();
    StatisticsModel *days = mModels.value( KNemoStats::Day );
    StatisticsModel *hours = mModels.value( KNemoStats::Hour );
    QDateTime curDateTime = QDateTime( QDate::currentDate(), QTime( QDateTime::currentDateTime().time().hour(), 0 ) );

    for ( int i = 0; i < syncHours->rowCount(); ++i )
    {
        QDateTime syncDateTime = syncHours->dateTime( i );

        if ( hours->rowCount() && hours->dateTime() > syncDateTime )
            continue;
        if ( !hours->rowCount() || hours->dateTime() < syncDateTime )
            genNewHour( syncDateTime );

        foreach ( KNemoStats::TrafficType t, hours->trafficTypes() )
        {
            hours->addRxBytes( syncHours->rxBytes( i ), t );
            hours->addTxBytes( syncHours->txBytes( i ), t );
        }
    }

    for ( int i = 0; i < syncDays->rowCount(); ++i )
    {
        QDate syncDate = syncDays->date( i );

        if ( days->rowCount() && days->date() > syncDate )
            continue;
        if ( !days->rowCount() || days->date() < syncDate )
        {
            genNewCalendarType( syncDate, KNemoStats::Day );
            genNewCalendarType( syncDate, KNemoStats::Week );
            genNewCalendarType( syncDate, KNemoStats::Month );
            genNewCalendarType( syncDate, KNemoStats::Year );
            genNewBillPeriod( syncDate );
        }

        foreach( StatisticsModel * s, mModels )
        {
            if ( s->periodType() == KNemoStats::Hour || s->periodType() == KNemoStats::HourArchive )
               continue;

            s->addRxBytes( v->addBytes( s->rxBytes(), syncDays->rxBytes( i ) ) );
            s->addTxBytes( v->addBytes( s->txBytes(), syncDays->txBytes( i ) ) );
            for ( int j = hours->rowCount() - 1; j >= 0; --j )
            {
                if ( hours->date( j ) == syncDate )
                {
                    foreach( KNemoStats::TrafficType t, hours->trafficTypes( j ) )
                    {
                        if ( t == KNemoStats::AllTraffic )
                            continue;
                        s->addRxBytes( hours->rxBytes( j, t ), t );
                        s->addTxBytes( hours->txBytes( j, t ), t );
                    }
                }
                else if ( hours->date( j ) < syncDate )
                    break;
            }
        }
    }

    StatsPair lag = v->addLagged( updated, days );
    if ( lag.rxBytes > 0 || lag.txBytes > 0 )
    {
        if ( lag.rxBytes || lag.txBytes )
        {
            genNewHour( curDateTime );
            genNewCalendarType( curDateTime.date(), KNemoStats::Day );
            genNewCalendarType( curDateTime.date(), KNemoStats::Week );
            genNewCalendarType( curDateTime.date(), KNemoStats::Month );
            genNewCalendarType( curDateTime.date(), KNemoStats::Year );
            genNewBillPeriod( curDateTime.date() );

            foreach ( StatisticsModel * s, mModels )
            {
                if ( s->periodType() == KNemoStats::HourArchive )
                    continue;
                foreach ( KNemoStats::TrafficType t, hours->trafficTypes() )
                {
                    s->addRxBytes( lag.rxBytes, t );
                    s->addTxBytes( lag.txBytes, t );
                }
            }
        }
    }
    delete v;
}

bool InterfaceStatistics::isOffpeak( const StatsRule &rules, const QDateTime &curDT )
{
    if ( !rules.logOffpeak )
        return false;

    bool isOffpeak = false;
    QTime curHour = QTime( curDT.time().hour(), 0 );

    // This block just tests weekly hours
    if ( rules.offpeakStartTime < rules.offpeakEndTime &&
         curHour >= rules.offpeakStartTime && curHour < rules.offpeakEndTime )
    {
        isOffpeak = true;
    }
    else if ( rules.offpeakStartTime > rules.offpeakEndTime &&
         ( curHour >= rules.offpeakStartTime || curHour < rules.offpeakEndTime ) )
    {
        isOffpeak = true;
    }

    if ( rules.weekendIsOffpeak )
    {
        int dow = mStorageData.calendar->dayOfWeek( curDT.date() );
        if ( rules.weekendDayStart <= rules.weekendDayEnd && rules.weekendDayStart <= dow )
        {
            QDateTime dayBegin = curDT.addDays( dow - rules.weekendDayStart );
            dayBegin = QDateTime( dayBegin.date(), rules.weekendTimeStart );
            QDateTime end = curDT.addDays( rules.weekendDayEnd - dow );
            end = QDateTime( end.date(), rules.weekendTimeEnd );

            if ( dayBegin <= curDT && curDT < end )
                isOffpeak = true;
        }
        else if ( rules.weekendDayStart > rules.weekendDayEnd &&
                ( dow >= rules.weekendDayStart || dow <= rules.weekendDayEnd ) )
        {
            QDateTime dayBegin = curDT.addDays( rules.weekendDayStart - dow );
            dayBegin = QDateTime( dayBegin.date(), rules.weekendTimeStart );
            QDateTime end = curDT.addDays( mStorageData.calendar->daysInWeek( curDT.date() ) - dow + rules.weekendDayEnd );
            end = QDateTime( end.date(), rules.weekendTimeEnd );

            if ( dayBegin <= curDT && curDT < end )
                isOffpeak = true;
        }
    }

    return isOffpeak;
}


/**************************************
 * Rebuilding Statistics              *
 **************************************/

int InterfaceStatistics::rebuildHours( StatisticsModel *s, const StatsRule &rules, const QDate &start, const QDate &nextRuleStart )
{
    if ( !s->rowCount() )
        return 0;

    int i = s->rowCount();
    while ( i > 0 && s->date( i - 1 ) >= start )
    {
        i--;
        if ( nextRuleStart.isValid() && s->date( i ) >= nextRuleStart )
            continue;

        s->resetTrafficTypes( i );
        if ( isOffpeak( rules, s->dateTime( i ) ) )
        {
            s->setTraffic( i, s->rxBytes( i ), s->txBytes( i ), KNemoStats::OffpeakTraffic );
            s->addTrafficType( KNemoStats::OffpeakTraffic, i );
        }
    }
    if ( mStorageData.saveFromId.value( s->periodType() ) > s->id( i ) )
    {
        mStorageData.saveFromId.insert( s->periodType(), s->id( i ) );
    }

    return i;
}

int InterfaceStatistics::rebuildDay( int dayIndex, int hourIndex, StatisticsModel *hours )
{
    QMap<KNemoStats::TrafficType, QPair<quint64, quint64> > dayTraffic;
    StatisticsModel *days = mModels.value( KNemoStats::Day );
    while ( hourIndex >= 0 && hours->date( hourIndex ) > days->date( dayIndex ).addDays( 1 ) )
    {
        --hourIndex;
    }
    while ( hourIndex >= 0 && hours->date( hourIndex ) == days->date( dayIndex ) )
    {
        foreach ( KNemoStats::TrafficType t, hours->trafficTypes( hourIndex ) )
        {
            if ( t == KNemoStats::AllTraffic )
                continue;
            quint64 rx = hours->rxBytes( hourIndex, t ) + dayTraffic.value( t ).first;
            quint64 tx = hours->txBytes( hourIndex, t ) + dayTraffic.value( t ).second;
            dayTraffic.insert( t, QPair<quint64, quint64>( rx, tx ) );
        }
        --hourIndex;
    }
    foreach (KNemoStats::TrafficType t, dayTraffic.keys() )
    {
        days->setTraffic( dayIndex, dayTraffic.value( t ).first, dayTraffic.value( t ).second, t );
        days->addTrafficType( t, dayIndex );
    }
    return hourIndex;
}

// A rebuild of hours and days never changes the number of entries
// We just change what bytes count as off-peak
void InterfaceStatistics::rebuildBaseUnits( const StatsRule &rules, const QDate &start, const QDate &nextRuleStart )
{
    int hIndex = 0;
    int haIndex = 0;

    StatisticsModel *hours = mModels.value( KNemoStats::Hour );
    StatisticsModel *hourArchives = mModels.value( KNemoStats::HourArchive );
    StatisticsModel *days = mModels.value( KNemoStats::Day );
    sql->loadHourArchives( hourArchives, start, nextRuleStart );
    if ( hourArchives->rowCount() )
        mStorageData.saveFromId.insert( hourArchives->periodType(), hourArchives->id( 0 ) );

    rebuildHours( hourArchives, rules, start, nextRuleStart );
    rebuildHours( hours, rules, start, nextRuleStart );

    if ( hours->rowCount() )
        hIndex = hours->rowCount() - 1;
    if ( hourArchives->rowCount() )
        haIndex = hourArchives->rowCount() - 1;

    if ( !days->rowCount() )
        return;

    int dayIndex = days->rowCount();
    while ( dayIndex > 0 && days->date( dayIndex - 1 ) >= start )
    {
        dayIndex--;
        if ( nextRuleStart.isValid() && days->date( dayIndex ) >= nextRuleStart )
            continue;

        days->resetTrafficTypes( dayIndex );
        if ( rules.logOffpeak )
        {
            haIndex = rebuildDay( dayIndex, haIndex, hourArchives );
            hIndex = rebuildDay( dayIndex, hIndex, hours );
        }
    }
    if ( mStorageData.saveFromId.value( days->periodType() ) > days->id( dayIndex ) )
    {
        mStorageData.saveFromId.insert( days->periodType(), days->id( dayIndex ) );
    }
}

/**
 * Given a model with statistics of a certain unit (year, month, week, etc.)
 * and a requested rebuild date, how far back to we actually need to go
 * to accuratly rebuild statistics from the daily stats.
 */
QDate InterfaceStatistics::prepareRebuild( StatisticsModel* statistics, const QDate &startDate )
{
    QDate newStartDate = startDate;
    if ( statistics->periodType() <= KNemoStats::Day ||
         statistics->periodType() > KNemoStats::Year )
        return newStartDate;

    for ( int i = 0; i < statistics->rowCount(); ++i )
    {
        int days = statistics->days( i );
        QDate nextPeriodStart = statistics->date( i ).addDays( days );

        if ( statistics->periodType() == KNemoStats::BillPeriod )
        {
            // Have to check if a billing period's truncation changed
            int ruleIndex = ruleForDate( statistics->date( i ) );
            if ( ruleIndex < 0 )
                break;
            QDate nextFullPeriodStart = nextBillPeriodStart( mStatsRules.at( ruleIndex ), statistics->date( i ) );
            if ( nextFullPeriodStart > nextPeriodStart )
            {
                if ( mStatsRules.count() == ruleIndex+1 ||
                     mStatsRules.at( ruleIndex + 1 ).startDate != nextPeriodStart )
                {
                    // Truncation changed
                    // This will make sure the entry gets rebuilt
                    nextPeriodStart = nextFullPeriodStart;
                }
            }
        }

        if ( nextPeriodStart > startDate )
        {
            if ( statistics->date( i ) < startDate )
            {
                newStartDate = statistics->date( i );
            }
            if ( statistics->rowCount() && mStorageData.saveFromId.value( statistics->periodType() ) > statistics->id( i ) )
            {
                mStorageData.saveFromId.insert( statistics->periodType(), statistics->id( i ) );
            }
            statistics->removeRows( i, statistics->rowCount() - i );
            break;
        }
    }

    return newStartDate;
}

void InterfaceStatistics::amendStats( int i, const StatisticsModel *source, StatisticsModel* dest )
{
    foreach ( KNemoStats::TrafficType t, source->trafficTypes( i ) )
    {
        dest->addRxBytes( source->rxBytes( i, t ), t );
        dest->addTxBytes( source->txBytes( i, t ), t );
        dest->addTrafficType( t );
    }
}

void InterfaceStatistics::rebuildCalendarPeriods( const QDate &requestedStart, bool weekOnly )
{
    QDate weekStart;
    QDate monthStart;
    QDate walkbackDate;

    QList<QDate> s;

    weekStart = prepareRebuild( mModels.value( KNemoStats::Week), requestedStart );
    s.append( weekStart );
    if ( !weekOnly )
    {
        monthStart = prepareRebuild( mModels.value( KNemoStats::Month), requestedStart );
        s.append( monthStart );
    }

    // Now find how far back we'll need to go
    qSort( s );
    walkbackDate = s.first();

    StatisticsModel *days = mModels.value( KNemoStats::Day );
    for ( int i = 0; i < days->rowCount(); ++i )
    {
        QDate day = days->date( i );
        if ( day < walkbackDate )
            continue;

        if ( day >= weekStart )
        {
            genNewCalendarType( day, KNemoStats::Week );
            amendStats( i, mModels.value( KNemoStats::Day ), mModels.value( KNemoStats::Week ) );
        }

        if ( !weekOnly && day >= monthStart )
        {
            genNewCalendarType( day, KNemoStats::Month );
            amendStats( i, mModels.value( KNemoStats::Day ), mModels.value( KNemoStats::Month ) );
        }
    }

    if ( weekOnly )
        return;

    // Build years from months...save time
    QDate yearStart = prepareRebuild( mModels.value( KNemoStats::Year ), requestedStart );
    StatisticsModel *months = mModels.value( KNemoStats::Month );
    for ( int i = 0; i < months->rowCount(); ++i )
    {
        QDate day = months->date( i );
        if ( day < yearStart )
            continue;
        genNewCalendarType( day, KNemoStats::Year );
        amendStats( i, mModels.value( KNemoStats::Month ), mModels.value( KNemoStats::Year ) );
    }
}

void InterfaceStatistics::rebuildBillPeriods( const QDate &requestedStart )
{
    QDate walkbackDate;
    StatisticsModel *days = mModels.value( KNemoStats::Day );
    StatisticsModel *billPeriods = mModels.value( KNemoStats::BillPeriod );

    if ( billPeriods->rowCount() )
        walkbackDate = prepareRebuild( mModels.value( KNemoStats::BillPeriod), requestedStart );
    else
        walkbackDate = days->date( 0 );

    for ( int i = 0; i < days->rowCount(); ++i )
    {
        QDate day = days->date( i );

        if ( day >= walkbackDate )
        {
            genNewBillPeriod( day );
            amendStats( i, mModels.value( KNemoStats::Day ), mModels.value( KNemoStats::BillPeriod ) );
        }
    }
}

void InterfaceStatistics::prependStatsRule( QList<StatsRule> &rules )
{
    qSort( rules.begin(), rules.end(), statsLessThan );
    StatisticsModel * days = mModels.value( KNemoStats::Day );
    if ( rules.count() == 0 ||
         ( days->rowCount() > 0 && rules[0].startDate > days->date( 0 ) )
       )
    {
        QDate date;
        if ( days->rowCount() )
            date = days->date( 0 );
        else
            date = QDate::currentDate();
        StatsRule s;
        s.startDate = date.addDays( 1 - mStorageData.calendar->day( date ) );
        rules.prepend( s );
    }
}

void InterfaceStatistics::checkRebuild( const QString &oldCalendar, bool force )
{
    QList<StatsRule> newRules = mInterface->getSettings().statsRules;
    bool forceWeek = false;

    if ( oldCalendar != mInterface->getSettings().calendar )
    {
        StatisticsModel *hours = mModels.value( KNemoStats::Hour );
        StatisticsModel *days = mModels.value( KNemoStats::Day );
        for ( int i = 0; i < days->rowCount(); ++i )
        {
            days->updateDateText( i );
        }
        for ( int i = 0; i < hours->rowCount(); ++i )
        {
            hours->updateDateText( i );
        }
        force = true;
    }
    if ( mModels.value( KNemoStats::Week )->rowCount() )
    {
        QDate testDate = mModels.value( KNemoStats::Week )->date( 0 );
        if ( mStorageData.calendar->dayOfWeek( testDate ) != mStorageData.calendar->weekStartDay() )
            forceWeek = true;
    }

    bool doBilling = newRules.count();
    int oldRuleCount = mStatsRules.count();
    QDate bpBeginDate;
    if ( !doBilling )
    {
        mModels.value( KNemoStats::BillPeriod )->clearRows();
        mStorageData.saveFromId.insert( KNemoStats::BillPeriod, 0 );
    }

    // This is just a dummy for calculation
    prependStatsRule( newRules );
    prependStatsRule( mStatsRules );

    if ( !oldRuleCount && mStatsRules[0] == newRules[0] && newRules.count() > mStatsRules.count() )
        bpBeginDate = mModels.value( KNemoStats::Day )->date( 0 );

    int j = 0;
    QDate recalcPos;
    for ( int i = 0; i < newRules.count(); ++i )
    {

        bool rulesMatch = ( newRules[i] == mStatsRules[j] );

        QDate nextRuleStart;
        if ( !rulesMatch )
        {
            if ( newRules.count() > i + 1 )
                nextRuleStart = newRules[i+1].startDate;
            if ( !recalcPos.isValid() )
                recalcPos = newRules[i].startDate;
            rebuildBaseUnits( newRules[i], newRules[i].startDate, nextRuleStart );
        }
        else
        {
            // rules match, now scan forward to see if we need to extend new rule
            if ( newRules.count() > i + 1 )
            {
                int first = -1;
                int k;
                // Here we want to skip over any intermediary old rules that will
                // get taken care of when we recalculate this section.
                for ( k = 0; k < mStatsRules.count(); ++k )
                {
                    if ( mStatsRules[k].startDate > newRules[i].startDate &&
                         mStatsRules[k].startDate < newRules[i+1].startDate )
                    {
                        if ( first < 0 )
                            first = k;
                        j = k;
                        if ( !recalcPos.isValid() )
                            recalcPos = mStatsRules[j].startDate;
                    }
                }
                if ( first >= 0 )
                {
                    rebuildBaseUnits( newRules[i], mStatsRules[first].startDate, newRules[i+1].startDate );
                }
            }
            // We're out of new rules but there's more old ones
            // so rebuild from next old rule's date using final new rule.
            else if ( mStatsRules.count() > j + 1 )
            {

                if ( !recalcPos.isValid() )
                    recalcPos = mStatsRules[j+1].startDate;
                rebuildBaseUnits( newRules[i], recalcPos, nextRuleStart );
            }
            if ( mStatsRules.count() > j + 1 )
                ++j;
        }
    }

    /*
    now do the rest
    */
    mStatsRules = newRules;
    if ( force )
        recalcPos = mModels.value( KNemoStats::Day )->date( 0 );

    if ( recalcPos.isValid() )
    {
        rebuildCalendarPeriods( recalcPos );

        if ( doBilling )
        {
            rebuildBillPeriods( recalcPos );
        }
    }
    else if ( forceWeek )
    {
        rebuildCalendarPeriods( mModels.value( KNemoStats::Day )->date( 0 ), true );
    }

    mStatsRules = mInterface->getSettings().statsRules;
    if ( mStatsRules.count() )
        prependStatsRule( mStatsRules );

    if ( recalcPos.isValid() )
    {
        saveStatistics( true );
    }

    mTrafficChanged = true;
    emit currentEntryChanged();
}

// END REBUILDING STATISTICS


void InterfaceStatistics::checkValidEntry()
{
    mEntryTimer->stop();
    QDateTime curDateTime = QDateTime::currentDateTime();
    QDate curDate = curDateTime.date();
    StatisticsModel *days = mModels.value( KNemoStats::Day );

    StatisticsModel *hours = mModels.value( KNemoStats::Hour );
    if ( !hours->rowCount() || hours->dateTime().addSecs( 3600 ) <= curDateTime )
    {
        genNewHour( QDateTime( curDate, QTime( curDateTime.time().hour(), 0 ) ) );
    }

    if ( !days->rowCount() || days->date() < curDate )
    {
        genNewCalendarType( curDate, KNemoStats::Day );
        genNewCalendarType( curDate, KNemoStats::Week );
        genNewCalendarType( curDate, KNemoStats::Month );
        genNewCalendarType( curDate, KNemoStats::Year );
        genNewBillPeriod( curDate );

        // The fancy short date may need updating
        for ( int i = 0; i < hours->rowCount(); ++i )
            hours->updateDateText( i );
    }

    QDateTime ndt = curDateTime.addSecs( 3600 - curDateTime.time().minute()*60 - curDateTime.time().second() );

    int secs = curDateTime.secsTo( ndt );
    mEntryTimer->setInterval( secs * 1000 );
    mEntryTimer->start();
}

void InterfaceStatistics::checkWarnings()
{
    if ( !mTrafficChanged )
        return;
    mTrafficChanged = false;

    QList<WarnRule> warn = mInterface->getSettings().warnRules;
    for ( int wi=0; wi < warn.count(); ++wi )
    {
        if ( warn[wi].warnDone || !warn[wi].threshold > 0.0 )
            continue;

        quint64 total = 0;
        StatisticsModel *model = 0;

        model = mModels.value( warn[wi].periodUnits );
        if ( !model )
            return;

        int lowerIndex = model->rowCount() - warn[wi].periodCount;

        for ( int i = model->rowCount() - 1; i >= 0; --i )
        {
            if ( i >= lowerIndex )
            {
                switch ( warn[wi].trafficDirection )
                {
                    case KNemoStats::TrafficIn:
                        if ( warn[wi].trafficType == KNemoStats::PeakOffpeak )
                            total += model->rxBytes( i );
                        else if ( warn[wi].trafficType == KNemoStats::Offpeak )
                            total += model->rxBytes( i, KNemoStats::OffpeakTraffic );
                        else
                            total += model->rxBytes( i ) - model->rxBytes( i, KNemoStats::OffpeakTraffic );
                        break;
                    case KNemoStats::TrafficOut:
                        if ( warn[wi].trafficType == KNemoStats::PeakOffpeak )
                            total += model->txBytes( i );
                        else if ( warn[wi].trafficType == KNemoStats::Offpeak )
                            total += model->txBytes( i, KNemoStats::OffpeakTraffic );
                        else
                            total += model->txBytes( i ) - model->txBytes( i, KNemoStats::OffpeakTraffic );
                        break;
                    default:
                        if ( warn[wi].trafficType == KNemoStats::PeakOffpeak )
                            total += model->totalBytes( i );
                        else if ( warn[wi].trafficType == KNemoStats::Offpeak )
                            total += model->totalBytes( i, KNemoStats::OffpeakTraffic );
                        else
                            total += model->totalBytes( i ) - model->totalBytes( i, KNemoStats::OffpeakTraffic );
                }
            }
            else
                break;
        }

        int warnMult = pow( 1024, warn[wi].trafficUnits );
        quint64 thresholdBytes = warn[wi].threshold * warnMult;
        if ( total > thresholdBytes )
        {
            emit warnTraffic( warn[wi].customText, thresholdBytes, total );
            mInterface->getSettings().warnRules[wi].warnDone = true;
        }
    }
}



/******************************
 * Public Interface           *
 ******************************/
void InterfaceStatistics::clearStatistics()
{
    foreach( StatisticsModel * s, mModels )
        s->clearRows();
    mStorageData.nextHourId = 0;
    foreach ( StatisticsModel *s, mModels )
    {
        mStorageData.saveFromId.insert( s->periodType(), 0 );
    }
    sql->clearStats( &mStorageData );
    checkValidEntry();
    mTrafficChanged = true;
    emit currentEntryChanged();
}

void InterfaceStatistics::addRxBytes( unsigned long bytes )
{
    if ( bytes == 0 )
        return;

    foreach( StatisticsModel * s, mModels )
    {
        if ( s->periodType() == KNemoStats::HourArchive )
            continue;
        foreach ( KNemoStats::TrafficType t, mModels.value( KNemoStats::Hour )->trafficTypes() )
        {
            s->addRxBytes( bytes, t );
        }
    }

    mTrafficChanged = true;
    emit currentEntryChanged();
}

void InterfaceStatistics::addTxBytes( unsigned long bytes )
{
    if ( bytes == 0 )
        return;

    foreach( StatisticsModel * s, mModels )
    {
        if ( s->periodType() == KNemoStats::HourArchive )
            continue;
        foreach ( KNemoStats::TrafficType t, mModels.value( KNemoStats::Hour )->trafficTypes() )
        {
            s->addTxBytes( bytes, t );
        }
    }

    mTrafficChanged = true;
    emit currentEntryChanged();
}

#include "interfacestatistics.moc"
