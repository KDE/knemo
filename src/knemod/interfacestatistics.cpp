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
      mEntryTimer( new QTimer() )
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

    foreach ( StatisticsModel *s, mModels )
    {
        mStorageData.saveFromId.insert( s->periodType(), 0 );
    }

    connect( mSaveTimer, SIGNAL( timeout() ), this, SLOT( saveStatistics() ) );
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
    mEntryTimer->stop();
    delete mSaveTimer;
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

void InterfaceStatistics::genNewHour( const QDateTime &dateTime )
{
    StatisticsModel* hours = mModels.value( KNemoStats::Hour );

    // Only 24 hours
    while ( hours->rowCount() )
    {
        if ( hours->dateTime( 0 ) <= dateTime.addDays( -1 ) )
        {
            hours->removeRow( 0 );
            mStorageData.saveFromId.insert( KNemoStats::Hour, 0 );
        }
        else
            break;
    }

    if ( hours->dateTime() == dateTime )
        return;

    hours->createEntry();
    hours->setDateTime( dateTime );

    if ( mInterface->getSettings().warnType == NotifyHour ||
         mInterface->getSettings().warnType == NotifyRoll24Hour )
        mWarningDone = false;
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
            if ( mInterface->getSettings().warnType == NotifyDay ||
                 mInterface->getSettings().warnType == NotifyRoll7Day ||
                 mInterface->getSettings().warnType == NotifyRoll30Day )
                mWarningDone = false;
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
            if ( mInterface->getSettings().warnType == NotifyMonth )
                mWarningDone = false;
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
}


// END STATS ENTRY GENERATORS

void InterfaceStatistics::configChanged()
{
    mSaveTimer->stop();

    mWarningDone = false;

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

    checkValidEntry();
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

        hours->addRxBytes( syncHours->rxBytes( i ) );
        hours->addTxBytes( syncHours->txBytes( i ) );
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
            if ( s->periodType() == KNemoStats::Hour )
               continue;

            s->addRxBytes( v->addBytes( s->rxBytes(), syncDays->rxBytes( i ) ) );
            s->addTxBytes( v->addBytes( s->txBytes(), syncDays->txBytes( i ) ) );
            for ( int j = hours->rowCount() - 1; j >= 0; --j )
            {
                if ( hours->date( j ) < syncDate )
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
                s->addRxBytes( lag.rxBytes );
                s->addTxBytes( lag.txBytes );
            }
        }
    }
    delete v;
}


/**************************************
 * Rebuilding Statistics              *
 **************************************/

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

        if ( nextPeriodStart >= startDate )
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
    dest->addRxBytes( source->rxBytes( i ) );
    dest->addTxBytes( source->txBytes( i ) );
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

        if ( !rulesMatch )
        {
            if ( !recalcPos.isValid() )
                recalcPos = newRules[i].startDate;
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
            }
            // We're out of new rules but there's more old ones
            // so rebuild from next old rule's date using final new rule.
            else if ( mStatsRules.count() > j + 1 )
            {

                if ( !recalcPos.isValid() )
                    recalcPos = mStatsRules[j+1].startDate;
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



/******************************
 * Public Interface           *
 ******************************/
void InterfaceStatistics::clearStatistics()
{
    foreach( StatisticsModel * s, mModels )
        s->clearRows();
    foreach ( StatisticsModel *s, mModels )
    {
        mStorageData.saveFromId.insert( s->periodType(), 0 );
    }
    sql->clearStats( &mStorageData );
    checkValidEntry();
    emit currentEntryChanged();
}

void InterfaceStatistics::checkThreshold( quint64 currentBytes )
{
    int warnMult = pow( 1024, mInterface->getSettings().warnUnits );
    quint64 thresholdBytes = mInterface->getSettings().warnThreshold * warnMult;
    if ( currentBytes > thresholdBytes )
    {
        mWarningDone = true;
        emit warnTraffic( thresholdBytes, currentBytes );
    }
}

void InterfaceStatistics::rollingUnit( const StatisticsModel* model, int count )
{
    quint64 total = 0;
    int lowerLimit = model->rowCount() - count;
    if ( lowerLimit < 0 )
       lowerLimit = 0;

    for ( int i = model->rowCount() - 1; i >= lowerLimit; --i )
    {
       if ( mInterface->getSettings().warnTotalTraffic )
            total += model->totalBytes( i );
        else
            total += model->rxBytes( i );
    }
    checkThreshold( total );
}

void InterfaceStatistics::checkTrafficLimit()
{
    int wtype = mInterface->getSettings().warnType;

    if ( !mWarningDone && mInterface->getSettings().warnThreshold > 0.0 )
    {
        switch ( wtype )
        {
            case NotifyHour:
                rollingUnit( mModels.value( KNemoStats::Hour ), 1 );
                break;
            case NotifyDay:
                rollingUnit( mModels.value( KNemoStats::Day ), 1 );
                break;
            case NotifyMonth:
                rollingUnit( mModels.value( KNemoStats::Month ), 1 );
                break;
            case NotifyRoll24Hour:
                rollingUnit( mModels.value( KNemoStats::Hour ), 24 );
                break;
            case NotifyRoll7Day:
                rollingUnit( mModels.value( KNemoStats::Day ), 7 );
                break;
            case NotifyRoll30Day:
                rollingUnit( mModels.value( KNemoStats::Day ), 30 );
                break;
        }
    }
}

void InterfaceStatistics::addRxBytes( unsigned long bytes )
{
    if ( bytes == 0 )
        return;

    foreach( StatisticsModel * s, mModels )
    {
        s->addRxBytes( bytes );
    }

    checkTrafficLimit();
    emit currentEntryChanged();
}

void InterfaceStatistics::addTxBytes( unsigned long bytes )
{
    if ( bytes == 0 )
        return;

    foreach( StatisticsModel * s, mModels )
    {
        s->addTxBytes( bytes );
    }

    checkTrafficLimit();
    emit currentEntryChanged();
}

#include "interfacestatistics.moc"
