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

#include <cmath>
#include <unistd.h>

#include "global.h"
#include "interface.h"
#include "interfacestatistics.h"
#include "syncstats/statsfactory.h"

static const char statistics_prefix[] = "/statistics_";

static const char doc_name[]     = "statistics";

static const char attrib_calendar[] = "calendar";
static const char attrib_version[]  = "version";
static const char attrib_updated[]  = "lastUpdated";
static const char attrib_span[]     = "span";
static const char attrib_rx[]       = "rxBytes";
static const char attrib_tx[]       = "txBytes";

static const char* intervals[] = { "hour", "day", "week", "month", "year" };

InterfaceStatistics::InterfaceStatistics( Interface* interface )
    : QObject(),
      mSaveTimer( new QTimer() ),
      mInterface( interface ),
      mAllMonths( true )
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

QString InterfaceStatistics::typeToElem( enum KNemoStats::PeriodUnits t )
{
    int i = 0;
    int v = t;
    while ( v > KNemoStats::Hour )
    {
        v = v >> 1;
        i++;
    }
    return intervals[i];
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

    if ( generalSettings->saveInterval > 0 )
    {
        mSaveTimer->setInterval( generalSettings->saveInterval * 1000 );
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
    QDomNode n = parentItem.namedItem( typeToElem(statistics->periodType()) + "s" );
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

                int year = element.attribute( typeToElem(KNemoStats::Year) ).toInt();
                int month = element.attribute( typeToElem(KNemoStats::Month), "1" ).toInt();
                int day = element.attribute( typeToElem(KNemoStats::Day), "1" ).toInt();
                cal->setDate( date, year, month, day );

                if ( date.isValid() )
                {
                    int days = element.attribute( attrib_span ).toInt();
                    switch ( statistics->periodType() )
                    {
                        case KNemoStats::Hour:
                            time = QTime( element.attribute( typeToElem(KNemoStats::Hour) ).toInt(), 0 );
                            break;
                        case KNemoStats::Month:
                            // Old format had no span, so daysInMonth using gregorian
                            if ( days == 0 )
                                days = date.daysInMonth();
                            if ( cal->day( date ) != 1 ||
                                 days != cal->daysInMonth( date ) )
                                mAllMonths = false;
                            break;
                        case KNemoStats::Year:
                            // Old format had no span, so daysInYear using gregorian
                            if ( days == 0 )
                                days = date.daysInYear();
                            break;
                        case KNemoStats::Day:
                            days = 1;
                        default:
                            ;;
                    }
                    statistics->createEntry();
                    statistics->setDateTime( QDateTime( date, time ) );
                    statistics->setDays( days );
                    statistics->addRxBytes( element.attribute( attrib_rx ).toULongLong() );
                    statistics->addTxBytes( element.attribute( attrib_tx ).toULongLong() );
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
    QDomElement elements = doc.createElement( QString( typeToElem(statistics->periodType()) ) + "s" );
    for ( int i = 0; i < statistics->rowCount(); ++i )
    {
        QDomElement element = doc.createElement( typeToElem(statistics->periodType()) );
        QDate date = statistics->date( i );
        element.setAttribute( typeToElem(KNemoStats::Day), mCalendar->day( date ) );
        element.setAttribute( typeToElem(KNemoStats::Month), mCalendar->month( date ) );
        element.setAttribute( typeToElem(KNemoStats::Year), mCalendar->year( date ) );
        if ( statistics->periodType() == KNemoStats::Hour )
            element.setAttribute( typeToElem(KNemoStats::Hour), statistics->dateTime( i ).time().hour() );
        else if ( statistics->periodType() > KNemoStats::Day )
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
    KUrl dir( generalSettings->statisticsDir );
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

    uint updated = root.attribute( attrib_updated ).toUInt();

    // If unknown or empty calendar it will default to gregorian
    KCalendarSystem *inCal = KCalendarSystem::create( root.attribute( attrib_calendar ) );
    foreach( StatisticsModel * s, mModels )
        loadStatsGroup( inCal, root, s );

    checkRebuild( inCal->calendarType() );

    syncWithExternal( updated );
}

void InterfaceStatistics::syncWithExternal( uint updated )
{
    ExternalStats *v = StatsFactory::stats( mInterface );
    if ( !v )
        return;

    v->importIfaceStats();
    const StatisticsModel *syncDays = v->days();
    const StatisticsModel *syncHours = v->hours();
    StatisticsModel *days = mModels.value( KNemoStats::Day );
    StatisticsModel *hours = mModels.value( KNemoStats::Hour );
    QDateTime curDateTime = QDateTime( QDate::currentDate(), QTime( QDateTime::currentDateTime().time().hour(), 0 ) );

    for ( int i = 0; i < syncDays->rowCount(); ++i )
    {
        QDate syncDate = syncDays->date( i );

        if ( days->rowCount() && days->date() > syncDate )
            continue;
        if ( !days->rowCount() || days->date() < syncDate )
        {
            genNewDay( syncDate );
            genNewWeek( syncDate );
            genNewMonth( syncDate );
            genNewYear( syncDate );
        }

        foreach( StatisticsModel * s, mModels )
        {
            if ( s->periodType() == KNemoStats::Hour )
               continue;

            s->addRxBytes( v->addBytes( s->rxBytes(), syncDays->rxBytes( i ) ) );
            s->addTxBytes( v->addBytes( s->txBytes(), syncDays->txBytes( i ) ) );
        }
    }
    for ( int i = 0; i < syncHours->rowCount(); ++i )
    {
        QDateTime syncDateTime = syncHours->dateTime( i );

        if ( hours->rowCount() && hours->dateTime() > syncDateTime )
            continue;
        if ( !hours->rowCount() || hours->dateTime() < syncDateTime )
            genNewHour( syncDateTime );

        hours->addRxBytes( v->addBytes( hours->rxBytes(), syncHours->rxBytes( i ) ) );
        hours->addTxBytes( v->addBytes( hours->txBytes(), syncHours->txBytes( i ) ) );
    }
    StatsPair lag = v->addLagged( updated, days );
    if ( lag.rxBytes > 0 || lag.txBytes > 0 )
    {
        if ( lag.rxBytes || lag.txBytes )
        {
            genNewHour( curDateTime );
            genNewDay( curDateTime.date() );
            genNewWeek( curDateTime.date() );
            genNewMonth( curDateTime.date() );
            genNewYear( curDateTime.date() );

            foreach ( StatisticsModel * s, mModels )
            {
                s->addRxBytes( lag.rxBytes );
                s->addTxBytes( lag.txBytes );
            }
        }
    }
    delete v;
}

void InterfaceStatistics::saveStatistics()
{
    QDomDocument doc( doc_name );
    QDomElement docElement = doc.createElement( doc_name );
    docElement.setAttribute( attrib_calendar, mCalendar->calendarType() );
    docElement.setAttribute( attrib_version, "1.3" );
    docElement.setAttribute( attrib_updated, QDateTime::currentDateTime().toTime_t() );
    doc.appendChild( docElement );

    foreach( StatisticsModel * s, mModels )
        saveStatsGroup( doc, s );

    KUrl dir( generalSettings->statisticsDir );
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
    if ( statistics->periodType() <= KNemoStats::Day )
        return returnDate;

    // Keep removing entries and rolling back returnDate while
    // entry's start date + days > returnDate
    for ( int i = statistics->rowCount() - 1; i >= 0; --i )
    {
        int days = statistics->days( i );
        QDate date = statistics->date( i ).addDays( days );
        if ( date > mModels.value( KNemoStats::Day )->date( 0 ) &&
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
    if ( statistics->periodType() == KNemoStats::Week )
    {
        returnDate = returnDate.addDays( 1 - mCalendar->dayOfWeek( returnDate ) );
        while ( returnDate > recalcDate )
            returnDate = returnDate.addDays( -mCalendar->daysInWeek( returnDate ) );
    }
    else if ( statistics->periodType() == KNemoStats::Year )
    {
        returnDate = returnDate.addDays( 1 - mCalendar->dayOfYear( returnDate ) );
        while ( returnDate > recalcDate )
            returnDate = returnDate.addDays( -mCalendar->daysInYear( returnDate ) );
    }

    return returnDate;
}

void InterfaceStatistics::amendStats( int i, StatisticsModel* model )
{
    StatisticsModel *days = mModels.value( KNemoStats::Day );
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

    if ( groups & KNemoStats::Week )
    {
        weekStart = setRebuildDate( mModels.value( KNemoStats::Week), recalcDate );
        s.append( weekStart );
    }
    if ( groups & KNemoStats::Month )
    {
        monthStart = setRebuildDate( mModels.value( KNemoStats::Month), recalcDate );
        // force an old date
        mBillingStart = monthStart;
        if ( recalcDate > monthStart )
            partial = true;
        s.append( monthStart );
    }
    if ( groups & KNemoStats::Year )
    {
        yearStart = setRebuildDate( mModels.value( KNemoStats::Year ), recalcDate );
        s.append( yearStart );
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

        if ( groups & KNemoStats::Week && day >= weekStart )
        {
            genNewWeek( day );
            amendStats( i, mModels.value( KNemoStats::Week ) );
        }

        if ( groups & KNemoStats::Month && day >= monthStart )
        {
            // Not very pretty
            StatisticsModel *month = mModels.value( KNemoStats::Month );
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
        if ( groups & KNemoStats::Year && day >= yearStart )
        {
            genNewYear( day );
            amendStats( i, mModels.value( KNemoStats::Year ) );
        }
    }
}

void InterfaceStatistics::checkRebuild( QString oldType )
{
    if ( oldType != mCalendar->calendarType() )
    {
        KUrl dir( generalSettings->statisticsDir );
        if ( dir.isLocalFile() )
        {
            QFile file( dir.path() + statistics_prefix + mInterface->getName() );
            file.copy( dir.path() + statistics_prefix + mInterface->getName() +
                   QString( "_%1.bak" ).arg( QDateTime::currentDateTime().toString( "yyyy-MM-dd-hhmmss" ) ) );
        }
        int modThese = KNemoStats::Week | KNemoStats::Year;
        QDate date = mModels.value( KNemoStats::Day )->date( 0 );
        doRebuild( date, modThese );
    }

    StatisticsModel *month = mModels.value( KNemoStats::Month );
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
        doRebuild( mBillingStart, KNemoStats::Month );

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
    StatisticsModel *model = mModels.value( KNemoStats::Day );
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
    StatisticsModel* hours = mModels.value( KNemoStats::Hour );

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

    hours->createEntry();
    hours->setDateTime( dateTime );

    if ( mInterface->getSettings().warnType == NotifyHour ||
         mInterface->getSettings().warnType == NotifyRoll24Hour )
        mWarningDone = false;
}

void InterfaceStatistics::genNewDay( const QDate &date )
{
    StatisticsModel * model = mModels.value( KNemoStats::Day );
    if ( model->rowCount() &&
         model->date().addDays( model->days() ) > date )
            return;

    model->createEntry();
    model->setDateTime( QDateTime( date, QTime() ) );
    model->setDays( 1 );

    if ( mInterface->getSettings().warnType == NotifyDay ||
         mInterface->getSettings().warnType == NotifyRoll7Day ||
         mInterface->getSettings().warnType == NotifyRoll30Day )
        mWarningDone = false;
}

void InterfaceStatistics::genNewWeek( const QDate &date )
{
    StatisticsModel * model = mModels.value( KNemoStats::Week );
    if ( model->rowCount() &&
         model->date().addDays( model->days() ) > date )
            return;

    int dow = mCalendar->dayOfWeek( date );
    // ISO8601: week always starts on a Monday
    QDate newDate = date.addDays( 1 - dow );
    model->createEntry();
    model->setDateTime( QDateTime( date, QTime() ) );
    model->setDays( mCalendar->daysInWeek( date ) );
}

void InterfaceStatistics::genNewMonth( const QDate &date, QDate endDate )
{
    StatisticsModel * model = mModels.value( KNemoStats::Month );
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
            model->createEntry();
            model->setDateTime( QDateTime( date, QTime() ) );
            model->setDays( days );
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
    model->createEntry();
    model->setDateTime( QDateTime( newDate, QTime() ) );
    model->setDays( days );

    if ( mInterface->getSettings().warnType == NotifyMonth )
        mWarningDone = false;
}

void InterfaceStatistics::genNewYear( const QDate &date )
{
    StatisticsModel * model = mModels.value( KNemoStats::Year );
    if ( model->rowCount() &&
         model->date().addDays( model->days() ) > date )
            return;

    int doy = mCalendar->dayOfYear( date );
    QDate newDate = date.addDays( 1 - doy );
    model->createEntry();
    model->setDateTime( QDateTime( newDate, QTime() ) );
    model->setDays( mCalendar->daysInYear( newDate ) );
}

void InterfaceStatistics::checkValidEntry( QDateTime curDateTime )
{
    QDate curDate = curDateTime.date();
    StatisticsModel *days = mModels.value( KNemoStats::Day );

    StatisticsModel *hours = mModels.value( KNemoStats::Hour );
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

void InterfaceStatistics::oneUnit( const StatisticsModel* model )
{
    if ( mInterface->getSettings().warnTotalTraffic )
        checkThreshold( model->totalBytes() );
    else
        checkThreshold( model->rxBytes() );
}

void InterfaceStatistics::rollingUnit( const StatisticsModel* model, int days )
{
    quint64 total = 0;
    QDateTime lowerLimit = model->dateTime().addDays( -days );

    for ( int i = model->rowCount() - 1; i >= 0; --i )
    {
        if ( model->dateTime( i ) > lowerLimit )
        {
            if ( mInterface->getSettings().warnTotalTraffic )
                total += model->totalBytes( i );
            else
                total += model->rxBytes( i );
        }
        else
            break;
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
                oneUnit( mModels.value( KNemoStats::Hour ) );
                break;
            case NotifyDay:
                oneUnit( mModels.value( KNemoStats::Day ) );
                break;
            case NotifyMonth:
                oneUnit( mModels.value( KNemoStats::Month ) );
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

#include "interfacestatistics.moc"
