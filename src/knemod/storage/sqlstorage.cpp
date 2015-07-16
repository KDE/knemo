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

#include <KLocalizedString>
#include <KMessageBox>

static const QString time_format( QLatin1String("hh:mm:ss") );

static const int current_db_version = 2;

SqlStorage::SqlStorage( QString ifaceName )
    : mValidDbVer( true )
    , mIfaceName( ifaceName )
{
    QUrl dir( generalSettings->statisticsDir );
    mDbPath = QString::fromLatin1( "%1%2%3.db" ).arg( dir.path() ).arg( statistics_prefix ).arg( mIfaceName );
    QStringList drivers = QSqlDatabase::drivers();
    if ( drivers.contains( QLatin1String("QSQLITE") ) )
        db = QSqlDatabase::addDatabase( QLatin1String("QSQLITE"), mIfaceName );
    mTypeMap.insert( KNemoStats::AllTraffic, QLatin1String("") );
    mTypeMap.insert( KNemoStats::OffpeakTraffic, QLatin1String("_offpeak") );

    if ( dbExists() && open() )
    {
        // KNemo 0.7.4 didn't create tables on a new db.  This lets us fix it
        // without forcing the user to intervene.
        if ( db.tables().isEmpty() )
            createDb();
        else
            migrateDb();
    }
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
    QString qryStr = QLatin1String("CREATE TABLE IF NOT EXISTS general (id INTEGER PRIMARY KEY, version INTEGER,") +
                     QLatin1String(" last_saved BIGINT, calendar TEXT, next_hour_id INTEGER );");
    qry.exec( qryStr );

    qryStr = QLatin1String("CREATE TABLE IF NOT EXISTS stats_rules (id INTEGER PRIMARY KEY, start_date DATETIME,") +
             QLatin1String(" period_units INTEGER, period_count INTEGER );");
    qry.exec( qryStr );

    qryStr = QLatin1String("CREATE TABLE IF NOT EXISTS stats_rules_offpeak (id INTEGER PRIMARY KEY,") +
             QLatin1String(" offpeak_start_time TEXT, offpeak_end_time TEXT,") +
             QLatin1String(" weekend_is_offpeak BOOLEAN, weekend_start_time TEXT, weekend_end_time TEXT,") +
             QLatin1String(" weekend_start_day INTEGER, weekend_end_day INTEGER );");
    qry.exec( qryStr );

    for ( int i = KNemoStats::Hour; i <= KNemoStats::HourArchive; ++i )
    {
        foreach ( KNemoStats::TrafficType j, mTypeMap.keys() )
        {
            QString dateTimeStr;
            qryStr = QLatin1String("CREATE TABLE IF NOT EXISTS %1s%2 (id INTEGER PRIMARY KEY,%3") +
                     QLatin1String(" rx BIGINT, tx BIGINT );");
            if ( j == KNemoStats::AllTraffic )
            {
                dateTimeStr = QLatin1String(" datetime DATETIME,");
                if ( i == KNemoStats::BillPeriod  )
                    dateTimeStr += QLatin1String(" days INTEGER,");
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
            searchCol = QLatin1String("datetime");
            startVal = startDateTime.toString( Qt::ISODate );
            endVal = nextStartDateTime.toString( Qt::ISODate );
        }
        else
        {
            searchCol = QLatin1String("id");
            startVal = QString::number( hourArchive->id( 0 ) );
            endVal = QString::number( hourArchive->id()+1 );
        }

        QString qryStr = QLatin1String("SELECT * FROM %1s%2 WHERE %3 >= '%4'");
        if ( nextStartDate.isValid() )
        {
            qryStr += QLatin1String(" AND datetime < '%5'");
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
        qryStr += QLatin1String(" ORDER BY id;");
        qry.exec( qryStr );
        int cId = qry.record().indexOf( QLatin1String("id") );
        int cRx = qry.record().indexOf( QLatin1String("rx") );
        int cTx = qry.record().indexOf( QLatin1String("tx") );
        int cDt = 0;
        if ( trafficType == KNemoStats::AllTraffic )
            cDt = qry.record().indexOf( QLatin1String("datetime") );

        while ( qry.next() )
        {
            int id = qry.value( cId ).toInt();
            if ( trafficType == KNemoStats::AllTraffic )
            {
                hourArchive->createEntry( QDateTime::fromString( qry.value( cDt ).toString(), Qt::ISODate ), id );
            }
            hourArchive->setTraffic( hourArchive->indexOfId( id ), qry.value( cRx ).toULongLong(), qry.value( cTx ).toULongLong(), trafficType );
            hourArchive->addTrafficType( trafficType, hourArchive->indexOfId( id ) );
        }
    }

    ok = QSqlDatabase::database( mIfaceName ).commit();
    db.close();
    return ok;
}

bool SqlStorage::migrateDb()
{
    bool ok = false;
    if ( !open() )
        return ok;

    QSqlDatabase::database( mIfaceName ).transaction();
    QSqlQuery qry( db );
    qry.exec( QLatin1String("SELECT * FROM general;") );
    if ( qry.next() )
    {
        int dbVersion = qry.value( qry.record().indexOf( QLatin1String("version") ) ).toInt();
        if ( dbVersion > current_db_version )
        {
            mValidDbVer = false;
            QSqlDatabase::database( mIfaceName ).commit();
            db.close();
            KMessageBox::error( NULL, i18n( "The statistics database for interface \"%1\" is incompatible with this version of KNemo.\n\nPlease upgrade to a more recent KNemo release.", mIfaceName ) );
            return false;
        }
        if ( dbVersion < 2 )
        {
            int lastSaved = qry.value( qry.record().indexOf( QLatin1String("last_saved") ) ).toInt();
            int nextHourId = qry.value( qry.record().indexOf( QLatin1String("next_hour_id") ) ).toInt();
            QString qryStr = QLatin1String("REPLACE INTO general (id, version, last_saved, calendar, next_hour_id )") +
                             QLatin1String(" VALUES (?, ?, ?, ?, ? );");
            qry.prepare( qryStr );
            qry.addBindValue( 1 );
            qry.addBindValue( current_db_version );
            qry.addBindValue( lastSaved );
            // FIXME
            //qry.addBindValue( QVariant( KCalendarSystem::calendarSystem( calendarType ) ).toString() );
            qry.addBindValue( nextHourId );
            qry.exec();
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

    KLocale::CalendarSystem calSystem = KLocale::QDateCalendar;
    qry.exec( QLatin1String("SELECT * FROM general;") );
    if ( qry.next() )
    {
        int cLastSaved = qry.record().indexOf( QLatin1String("last_saved") );
        int cCalendarSystem = qry.record().indexOf( QLatin1String("calendar") );
        int cNextHourId = qry.record().indexOf( QLatin1String("next_hour_id") );
        sd->lastSaved = qry.value( cLastSaved ).toUInt();
        calSystem = static_cast<KLocale::CalendarSystem>(qry.value( cCalendarSystem ).toInt());
        sd->nextHourId = qry.value( cNextHourId ).toInt();
    }
    sd->calendar = KCalendarSystem::create( calSystem );

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
                qry.exec( QString::fromLatin1( "SELECT * FROM %1s%2 ORDER BY id;" )
                        .arg( periods.at( s->periodType() ) )
                        .arg( mTypeMap.value( trafficType ) ) );
                int cId = qry.record().indexOf( QLatin1String("id") );
                int cRx = qry.record().indexOf( QLatin1String("rx") );
                int cTx = qry.record().indexOf( QLatin1String("tx") );
                int cDt = 0;
                int cDays = 0;
                if ( trafficType == KNemoStats::AllTraffic )
                {
                    cDt = qry.record().indexOf( QLatin1String("datetime") );
                    if ( s->periodType() == KNemoStats::BillPeriod )
                    {
                        cDays = qry.record().indexOf( QLatin1String("days") );
                    }
                }

                while ( qry.next() )
                {
                    int id = qry.value( cId ).toInt();
                    if ( trafficType == KNemoStats::AllTraffic )
                    {
                        int days = -1;
                        if ( s->periodType() == KNemoStats::BillPeriod )
                        {
                            days = qry.value( cDays ).toInt();
                        }
                        s->createEntry( QDateTime::fromString( qry.value( cDt ).toString(), Qt::ISODate ), id, days );
                    }
                    s->setTraffic( id, qry.value( cRx ).toULongLong(), qry.value( cTx ).toULongLong(), trafficType );
                    s->addTrafficType( trafficType, id );
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
        qry.exec( QLatin1String("SELECT * FROM stats_rules ORDER BY id;") );
        int cDt = qry.record().indexOf( QLatin1String("start_date") );
        int cType = qry.record().indexOf( QLatin1String("period_units") );
        int cUnits = qry.record().indexOf( QLatin1String("period_count") );
        while ( qry.next() )
        {
            StatsRule entry;
            entry.startDate = QDate::fromString( qry.value( cDt ).toString(), Qt::ISODate );
            entry.periodUnits = qry.value( cType ).toInt();
            entry.periodCount = qry.value( cUnits ).toInt();
            *rules << entry;
        }

        qry.exec( QLatin1String("SELECT * FROM stats_rules_offpeak ORDER BY id;") );
        int cId = qry.record().indexOf( QLatin1String("id") );
        int cOpStartTime = qry.record().indexOf( QLatin1String("offpeak_start_time") );
        int cOpEndTime = qry.record().indexOf( QLatin1String("offpeak_end_time") );
        int cWeekendIsOffpeak = qry.record().indexOf( QLatin1String("weekend_is_offpeak") );
        int cWStartTime = qry.record().indexOf( QLatin1String("weekend_start_time") );
        int cWEndTime = qry.record().indexOf( QLatin1String("weekend_end_time") );
        int cWStartDay = qry.record().indexOf( QLatin1String("weekend_start_day") );
        int cWEndDay = qry.record().indexOf( QLatin1String("weekend_end_day") );
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
    qry.exec( QLatin1String("VACUUM;") );
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
        qry.exec( QLatin1String("VACUUM;") );
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
            qry.exec( QString::fromLatin1( "DELETE FROM %1s%2;" ).arg( period ).arg( mTypeMap.value( i ) ) );
        }
    }
    save( sd );
    ok = QSqlDatabase::database( mIfaceName ).commit();
    qry.exec( QLatin1String("VACUUM;") );

    db.close();
    return ok;
}

bool SqlStorage::open()
{
    if ( !mValidDbVer )
        return false;

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
    QString qryStr = QLatin1String("REPLACE INTO general (id, version, last_saved, calendar, next_hour_id )") +
                     QLatin1String(" VALUES (?, ?, ?, ?, ? );");
    qry.prepare( qryStr );
    qry.addBindValue( 1 );
    qry.addBindValue( current_db_version );
    qry.addBindValue( QDateTime::currentDateTime().toTime_t() );
    qry.addBindValue( QVariant( sd->calendar->calendarSystem() ).toString() );
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
                    qryStr = QString::fromLatin1( "DELETE FROM %1s%2 WHERE id >= '%3';" )
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
                    dateTimeStr = QLatin1String(" datetime,");
                    dateTimeStr2 = QLatin1String(" ?,");
                    if ( s->periodType() == KNemoStats::BillPeriod )
                    {
                        dateTimeStr += QLatin1String(" days,");
                        dateTimeStr2 += QLatin1String(" ?,");
                    }
                }
                qryStr = QLatin1String("REPLACE INTO %1s%2 (id,%3 rx, tx )") +
                             QLatin1String(" VALUES (?,%4 ?, ? );");
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
        qryStr = QString::fromLatin1( "DELETE FROM stats_rules WHERE id >= '%1';" ).arg( rules->count() );
        qry.exec( qryStr );
        qryStr = QString::fromLatin1( "DELETE FROM stats_rules_offpeak WHERE id >= '%1';" ).arg( rules->count() );
        qry.exec( qryStr );

        if ( rules->count() )
        {
            qryStr = QLatin1String("REPLACE INTO stats_rules (id, start_date, period_units, period_count )") +
                     QLatin1String(" VALUES( ?, ?, ?, ? );");
            qry.prepare( qryStr );

            for ( int i = 0; i < rules->count(); ++i )
            {
                qry.addBindValue( i );
                qry.addBindValue( rules->at(i).startDate.toString( Qt::ISODate ) );
                qry.addBindValue( rules->at(i).periodUnits );
                qry.addBindValue( rules->at(i).periodCount );
                qry.exec();
            }

            qryStr = QLatin1String("REPLACE INTO stats_rules_offpeak (id,") +
                     QLatin1String(" offpeak_start_time, offpeak_end_time, weekend_is_offpeak,") +
                     QLatin1String(" weekend_start_time, weekend_end_time, weekend_start_day, weekend_end_day )") +
                     QLatin1String(" VALUES( ?, ?, ?, ?, ?, ?, ?, ? );");
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
