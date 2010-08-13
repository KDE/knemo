/* This file is part of KNemo
   Copyright (C) 2010 John Stamp <jstamp@users.sourceforge.net>

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

#include "global.h"
#include "statisticsmodel.h"
#include "sqlstorage.h"
#include "commonstorage.h"

#include <QFile>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>

static const QString time_format( "hh:mm:ss" );

SqlStorage::SqlStorage( QString ifaceName )
    : mIfaceName( ifaceName )
{
    KUrl dir( generalSettings->statisticsDir );
    mDbPath = QString( "%1%2%3.db" ).arg( dir.path() ).arg( statistics_prefix ).arg( mIfaceName );
    QStringList drivers = QSqlDatabase::drivers();
    if ( drivers.contains( "QSQLITE" ) )
        db = QSqlDatabase::addDatabase( "QSQLITE", mIfaceName );
    mTypeMap.insert( KNemoStats::AllTraffic, "" );
    mTypeMap.insert( KNemoStats::OffpeakTraffic, "_offpeak" );

}

SqlStorage::~SqlStorage()
{
    db.close();
}

bool SqlStorage::dbExists()
{
    QFile dbfile( mDbPath );
    return dbfile.exists();
}

bool SqlStorage::createDb()
{
    bool ok = false;

    if ( !open() )
        return ok;

    QSqlDatabase::database( mIfaceName ).transaction();
    QSqlQuery qry( db );
    QString qryStr = "CREATE TABLE IF NOT EXISTS general (id INTEGER PRIMARY KEY, version INTEGER,"
                     " last_saved BIGINT, calendar TEXT, next_hour_id INTEGER );";
    qry.exec( qryStr );

    qryStr = "CREATE TABLE IF NOT EXISTS stats_rules (id INTEGER PRIMARY KEY, start_date DATETIME,"
             " period_units INTEGER, period_count INTEGER );";
    qry.exec( qryStr );

    qryStr = "CREATE TABLE IF NOT EXISTS stats_rules_offpeak (id INTEGER PRIMARY KEY,"
             " offpeak_start_time TEXT, offpeak_end_time TEXT,"
             " weekend_is_offpeak BOOLEAN, weekend_start_time TEXT, weekend_end_time TEXT,"
             " weekend_start_day INTEGER, weekend_end_day INTEGER );";
    qry.exec( qryStr );

    for ( int i = KNemoStats::Hour; i <= KNemoStats::HourArchive; ++i )
    {
        foreach ( KNemoStats::TrafficType j, mTypeMap.keys() )
        {
            QString dateTimeStr;
            qryStr = "CREATE TABLE IF NOT EXISTS %1s%2 (id INTEGER PRIMARY KEY,%3"
                     " rx BIGINT, tx BIGINT );";
            if ( j == KNemoStats::AllTraffic )
            {
                dateTimeStr = " datetime DATETIME,";
                if ( i == KNemoStats::BillPeriod  )
                    dateTimeStr += " days INTEGER,";
            }
            qryStr = qryStr.arg( periods.at( i ) ).arg( mTypeMap.value( j ) ).arg( dateTimeStr );
            qry.exec( qryStr );
        }
    }

    ok = QSqlDatabase::database( mIfaceName ).commit();
    db.close();
    return ok;
}

bool SqlStorage::loadHourArchives( StatisticsModel *hourArchive, const QDate &startDate, const QDate &nextStartDate )
{
    bool ok = false;
    if ( !open() )
        return ok;

    QDateTime startDateTime = QDateTime( startDate, QTime() );
    QDateTime nextStartDateTime = QDateTime( nextStartDate, QTime() );

    QSqlDatabase::database( mIfaceName ).transaction();
    QSqlQuery qry( db );

    QString searchCol;
    QString startVal;
    QString endVal;
    foreach ( KNemoStats::TrafficType trafficType, mTypeMap.keys() )
    {
        if ( trafficType == KNemoStats::AllTraffic )
        {
            searchCol = "datetime";
            startVal = startDateTime.toString( Qt::ISODate );
            endVal = nextStartDateTime.toString( Qt::ISODate );
        }
        else
        {
            searchCol = "id";
            startVal = QString::number( hourArchive->id( 0 ) );
            endVal = QString::number( hourArchive->id()+1 );
        }

        QString qryStr = "SELECT * FROM %1s%2 WHERE %3 >= '%4'";
        if ( nextStartDate.isValid() )
        {
            qryStr += " AND datetime < '%5'";
            qryStr = qryStr.arg( periods.at( KNemoStats::HourArchive ) )
                           .arg( mTypeMap.value( trafficType ) )
                           .arg( searchCol )
                           .arg( startVal )
                           .arg( endVal );
        }
        else
        {
            qryStr = qryStr.arg( periods.at( KNemoStats::HourArchive ) )
                           .arg( mTypeMap.value( trafficType ) )
                           .arg( searchCol )
                           .arg( startVal );
        }
        qryStr += " ORDER BY id;";
        qry.exec( qryStr );
        int cId = qry.record().indexOf( "id" );
        int cRx = qry.record().indexOf( "rx" );
        int cTx = qry.record().indexOf( "tx" );
        int cDt = 0;
        if ( trafficType == KNemoStats::AllTraffic )
            cDt = qry.record().indexOf( "datetime" );

        while ( qry.next() )
        {
            int id = qry.value( cId ).toInt();
            if ( trafficType == KNemoStats::AllTraffic )
            {
                hourArchive->createEntry();
                hourArchive->setId( id );
                hourArchive->setDateTime( QDateTime::fromString( qry.value( cDt ).toString(), Qt::ISODate ) );
            }
            hourArchive->setTraffic( hourArchive->indexOfId( id ), qry.value( cRx ).toULongLong(), qry.value( cTx ).toULongLong(), trafficType );
            hourArchive->addTrafficType( trafficType );
        }
    }

    ok = QSqlDatabase::database( mIfaceName ).commit();
    db.close();
    return ok;
}

bool SqlStorage::loadStats( StorageData *sd, QHash<int, StatisticsModel*> *models, QList<StatsRule> *rules )
{
    bool ok = false;
    if ( !open() )
        return ok;

    QSqlDatabase::database( mIfaceName ).transaction();
    QSqlQuery qry( db );
    QDateTime curDateTime = QDateTime::currentDateTime();

    QString cal;
    qry.exec( "SELECT * FROM general;" );
    if ( qry.next() )
    {
        //int cVersion = qry.record().indexOf("version");
        int cLastSaved = qry.record().indexOf( "last_saved" );
        int cCalendar = qry.record().indexOf( "calendar" );
        int cNextHourId = qry.record().indexOf( "next_hour_id" );
        sd->lastSaved = qry.value( cLastSaved ).toUInt();
        cal = qry.value( cCalendar ).toString();
        sd->nextHourId = qry.value( cNextHourId ).toInt();
    }
    sd->calendar = KCalendarSystem::create( cal );

    if ( models )
    {
        foreach( StatisticsModel * s, *models )
            s->setCalendar( sd->calendar );

        foreach ( StatisticsModel * s, *models )
        {
            if ( s->periodType() == KNemoStats::HourArchive )
                continue;
            foreach ( KNemoStats::TrafficType trafficType, mTypeMap.keys() )
            {
                qry.exec( QString( "SELECT * FROM %1s%2 ORDER BY id;" )
                        .arg( periods.at( s->periodType() ) )
                        .arg( mTypeMap.value( trafficType ) ) );
                int cId = qry.record().indexOf( "id" );
                int cRx = qry.record().indexOf( "rx" );
                int cTx = qry.record().indexOf( "tx" );
                int cDt = 0;
                int cDays = 0;
                if ( trafficType == KNemoStats::AllTraffic )
                {
                    cDt = qry.record().indexOf( "datetime" );
                    if ( s->periodType() == KNemoStats::BillPeriod )
                    {
                        cDays = qry.record().indexOf( "days" );
                    }
                }

                while ( qry.next() )
                {
                    int id = qry.value( cId ).toInt();
                    if ( trafficType == KNemoStats::AllTraffic )
                    {
                        s->createEntry();
                        s->setId( id );
                        if ( s->periodType() == KNemoStats::BillPeriod )
                        {
                            s->setDays( qry.value( cDays ).toInt() );
                        }
                        s->setDateTime( QDateTime::fromString( qry.value( cDt ).toString(), Qt::ISODate ) );
                    }
                    s->setTraffic( id, qry.value( cRx ).toULongLong(), qry.value( cTx ).toULongLong(), trafficType );
                    s->addTrafficType( trafficType );
                }
                if ( trafficType == KNemoStats::AllTraffic && s->rowCount() )
                {
                    sd->saveFromId.insert( s->periodType(), s->id() );
                }
            }
        }
    }

    if ( rules )
    {
        qry.exec( "SELECT * FROM stats_rules ORDER BY id;" );
        int cDt = qry.record().indexOf( "start_date" );
        int cType = qry.record().indexOf( "period_units" );
        int cUnits = qry.record().indexOf( "period_count" );
        while ( qry.next() )
        {
            StatsRule entry;
            entry.startDate = QDate::fromString( qry.value( cDt ).toString(), Qt::ISODate );
            entry.periodUnits = qry.value( cType ).toInt();
            entry.periodCount = qry.value( cUnits ).toInt();
            *rules << entry;
        }

        qry.exec( "SELECT * FROM stats_rules_offpeak ORDER BY id;" );
        int cId = qry.record().indexOf( "id" );
        int cOpStartTime = qry.record().indexOf( "offpeak_start_time" );
        int cOpEndTime = qry.record().indexOf( "offpeak_end_time" );
        int cWeekendIsOffpeak = qry.record().indexOf( "weekend_is_offpeak" );
        int cWStartTime = qry.record().indexOf( "weekend_start_time" );
        int cWEndTime = qry.record().indexOf( "weekend_end_time" );
        int cWStartDay = qry.record().indexOf( "weekend_start_day" );
        int cWEndDay = qry.record().indexOf( "weekend_end_day" );
        while ( qry.next() )
        {
            int id = qry.value( cId ).toInt();
            if ( id < rules->count() )
            {
                (*rules)[ id ].logOffpeak = true;
                (*rules)[ id ].offpeakStartTime = QTime::fromString( qry.value( cOpStartTime ).toString(), time_format );
                (*rules)[ id ].offpeakEndTime = QTime::fromString( qry.value( cOpEndTime ).toString(), time_format );
                (*rules)[ id ].weekendIsOffpeak = qry.value( cWeekendIsOffpeak ).toBool();
                (*rules)[ id ].weekendDayStart = qry.value( cWStartDay ).toInt();
                (*rules)[ id ].weekendDayEnd = qry.value( cWEndDay ).toInt();
                (*rules)[ id ].weekendTimeStart = QTime::fromString( qry.value( cWStartTime ).toString(), time_format );
                (*rules)[ id ].weekendTimeEnd = QTime::fromString( qry.value( cWEndTime ).toString(), time_format );
            }
        }
    }
    ok = QSqlDatabase::database( mIfaceName ).commit();
    qry.exec( "VACUUM;" );
    db.close();
    return ok;
}

bool SqlStorage::saveStats( StorageData *sd, QHash<int, StatisticsModel*> *models, QList<StatsRule> *rules, bool fullSave )
{
    bool ok = false;
    if ( !open() )
        return ok;

    QSqlDatabase::database( mIfaceName ).transaction();

    save( sd, models, rules, fullSave );

    ok = QSqlDatabase::database( mIfaceName ).commit();
    if ( fullSave )
    {
        QSqlQuery qry( db );
        qry.exec( "VACUUM;" );
    }
    db.close();
    return ok;
}

bool SqlStorage::clearStats( StorageData *sd )
{
    bool ok = false;
    if ( !open() )
        return ok;

    QSqlDatabase::database( mIfaceName ).transaction();
    QSqlQuery qry( db );
    foreach ( QString period, periods )
    {
        foreach ( KNemoStats::TrafficType i, mTypeMap.keys() )
        {
            if ( i == KNemoStats::AllTraffic &&
                 ( period == periods.at( KNemoStats::Hour ) || period == periods.at( KNemoStats::HourArchive ) ) )
                continue;
            qry.exec( QString( "DELETE FROM %1s%2;" ).arg( period ).arg( mTypeMap.value( i ) ) );
        }
    }
    save( sd );
    ok = QSqlDatabase::database( mIfaceName ).commit();
    qry.exec( "VACUUM;" );

    db.close();
    return ok;
}

bool SqlStorage::open()
{
    if ( !db.isValid() )
        return false;

    if ( db.isOpen() )
        return true;

    db.setDatabaseName( mDbPath );
    return db.open();
}

void SqlStorage::save( StorageData *sd, QHash<int, StatisticsModel*> *models, QList<StatsRule> *rules, bool fullSave )
{
    QSqlQuery qry( db );
    QString qryStr = "REPLACE INTO general (id, version, last_saved, calendar, next_hour_id )"
                     " VALUES (?, ?, ?, ?, ? );";
    qry.prepare( qryStr );
    qry.addBindValue( 1 );
    qry.addBindValue( 1 );
    qry.addBindValue( QDateTime::currentDateTime().toTime_t() );
    qry.addBindValue( sd->calendar->calendarType() );
    qry.addBindValue( sd->nextHourId );
    qry.exec();

    if ( models )
    {
        foreach ( StatisticsModel * s, *models )
        {
            foreach ( KNemoStats::TrafficType trafficType, mTypeMap.keys() )
            {
                /* Always do deletes for:
                     hour*
                   Even on a full save, don't delete anything from:
                     days
                     hour_archives
                     hour_archives_* if the model is empty
                 */
                if ( s->periodType() == KNemoStats::Hour ||
                     ( fullSave && !( ( trafficType == KNemoStats::AllTraffic && ( s->periodType() == KNemoStats::Day || s->periodType() == KNemoStats::HourArchive ) ) ||
                                      ( s->periodType() == KNemoStats::HourArchive && !s->rowCount() )
                                    )
                     )
                   )
                {
                    int deleteFrom = sd->saveFromId.value( s->periodType() );
                    qryStr = QString( "DELETE FROM %1s%2 WHERE id >= '%3';" )
                                .arg( periods.at( s->periodType() ) )
                                .arg( mTypeMap.value( trafficType ) )
                                .arg( deleteFrom );
                    qry.exec( qryStr );
                }

                if ( !s->rowCount() )
                {
                    continue;
                }

                QString dateTimeStr;
                QString dateTimeStr2;
                if ( trafficType == KNemoStats::AllTraffic )
                {
                    dateTimeStr = " datetime,";
                    dateTimeStr2 = " ?,";
                    if ( s->periodType() == KNemoStats::BillPeriod )
                    {
                        dateTimeStr += " days,";
                        dateTimeStr2 += " ?,";
                    }
                }
                qryStr = "REPLACE INTO %1s%2 (id,%3 rx, tx )"
                             " VALUES (?,%4 ?, ? );";
                qryStr = qryStr
                            .arg( periods.at( s->periodType() ) )
                            .arg( mTypeMap.value( trafficType ) )
                            .arg( dateTimeStr )
                            .arg( dateTimeStr2 );
                qry.prepare( qryStr );

                int j = sd->saveFromId.value( s->periodType() );
                for ( j = s->indexOfId( j ); j < s->rowCount(); ++j )
                {
                    if ( s->trafficTypes( j ).contains( trafficType ) )
                    {
                        qry.addBindValue( s->id( j ) );
                        if ( trafficType == KNemoStats::AllTraffic )
                        {
                            qry.addBindValue( s->dateTime( j ).toString( Qt::ISODate ) );
                            if ( s->periodType() == KNemoStats::BillPeriod )
                            {
                                qry.addBindValue( s->days( j ) );
                            }
                        }
                        qry.addBindValue( s->rxBytes( j, trafficType ) );
                        qry.addBindValue( s->txBytes( j, trafficType ) );
                        qry.exec();
                    }
                }
            }
            if ( s->rowCount() )
            {
                sd->saveFromId.insert( s->periodType(), s->id() );
                if ( s->periodType() == KNemoStats::HourArchive )
                    s->clearRows();
            }
        }
    }

    if ( fullSave && rules )
    {
        qryStr = QString( "DELETE FROM stats_rules WHERE id >= '%1';" ).arg( rules->count() );
        qry.exec( qryStr );
        qryStr = QString( "DELETE FROM stats_rules_offpeak WHERE id >= '%1';" ).arg( rules->count() );
        qry.exec( qryStr );

        if ( rules->count() )
        {
            qryStr = "REPLACE INTO stats_rules (id, start_date, period_units, period_count )"
                     " VALUES( ?, ?, ?, ? );";
            qry.prepare( qryStr );

            for ( int i = 0; i < rules->count(); ++i )
            {
                qry.addBindValue( i );
                qry.addBindValue( rules->at(i).startDate.toString( Qt::ISODate ) );
                qry.addBindValue( rules->at(i).periodUnits );
                qry.addBindValue( rules->at(i).periodCount );
                qry.exec();
            }

            qryStr = "REPLACE INTO stats_rules_offpeak (id,"
                     " offpeak_start_time, offpeak_end_time, weekend_is_offpeak,"
                     " weekend_start_time, weekend_end_time, weekend_start_day, weekend_end_day )"
                     " VALUES( ?, ?, ?, ?, ?, ?, ?, ? );";
            qry.prepare( qryStr );

            for ( int i = 0; i < rules->count(); ++i )
            {
                QVariant startTime = QVariant::String;
                QVariant stopTime = QVariant::String;
                QVariant weekendIsOffpeak = QVariant::Bool;
                QVariant wStartTime = QVariant::String;
                QVariant wEndTime = QVariant::String;
                QVariant wStartDay = QVariant::Int;
                QVariant wEndDay = QVariant::Int;

                if ( !rules->at(i).logOffpeak )
                    continue;

                startTime = rules->at(i).offpeakStartTime.toString( time_format );
                stopTime = rules->at(i).offpeakEndTime.toString( time_format );
                weekendIsOffpeak = rules->at(i).weekendIsOffpeak;

                if ( rules->at(i).weekendIsOffpeak )
                {
                    wStartTime = rules->at(i).weekendTimeStart.toString( time_format );
                    wEndTime = rules->at(i).weekendTimeEnd.toString( time_format );
                    wStartDay = rules->at(i).weekendDayStart;
                    wEndDay = rules->at(i).weekendDayEnd;
                }

                qry.addBindValue( i );
                qry.addBindValue( startTime );
                qry.addBindValue( stopTime );
                qry.addBindValue( weekendIsOffpeak );
                qry.addBindValue( wStartTime );
                qry.addBindValue( wEndTime );
                qry.addBindValue( wStartDay );
                qry.addBindValue( wEndDay );
                qry.exec();
            }
        }
    }
}
