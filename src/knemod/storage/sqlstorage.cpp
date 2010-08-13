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
                     " last_saved BIGINT, calendar TEXT );";
    qry.exec( qryStr );

    qryStr = "CREATE TABLE IF NOT EXISTS stats_rules (id INTEGER PRIMARY KEY, start_date DATETIME,"
             " period_units INTEGER, period_count INTEGER );";
    qry.exec( qryStr );

    for ( int i = KNemoStats::Hour; i <= KNemoStats::Year; ++i )
    {
        QString daysStr;
        qryStr = "CREATE TABLE IF NOT EXISTS %1s (id INTEGER PRIMARY KEY, datetime DATETIME,%2"
                 " rx BIGINT, tx BIGINT );";
        if ( i == KNemoStats::BillPeriod  )
            daysStr += " days INTEGER,";
        qryStr = qryStr.arg( periods.at( i ) ).arg( daysStr );
        qry.exec( qryStr );
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
        sd->lastSaved = qry.value( cLastSaved ).toUInt();
        cal = qry.value( cCalendar ).toString();
    }
    sd->calendar = KCalendarSystem::create( cal );

    if ( models )
    {
        foreach( StatisticsModel * s, *models )
            s->setCalendar( sd->calendar );

        foreach ( StatisticsModel * s, *models )
        {
            qry.exec( QString( "SELECT * FROM %1s ORDER BY id;" ).arg( periods.at( s->periodType() ) ) );
            int cId = qry.record().indexOf( "id" );
            int cDt = qry.record().indexOf( "datetime" );
            int cRx = qry.record().indexOf( "rx" );
            int cTx = qry.record().indexOf( "tx" );
            int cDays = 0;
            if ( s->periodType() == KNemoStats::BillPeriod )
            {
                cDays = qry.record().indexOf( "days" );
            }

            while ( qry.next() )
            {
                int id = qry.value( cId ).toInt();
                s->createEntry();
                s->setId( id );
                if ( s->periodType() == KNemoStats::BillPeriod )
                {
                    s->setDays( qry.value( cDays ).toInt() );
                }
                s->setDateTime( QDateTime::fromString( qry.value( cDt ).toString(), Qt::ISODate ) );
                s->setTraffic( id, qry.value( cRx ).toULongLong(), qry.value( cTx ).toULongLong() );
            }
            if ( s->rowCount() )
            {
                sd->saveFromId.insert( s->periodType(), s->id() );
            }
        }
    }

    if ( rules )
    {
        qry.exec( "SELECT * FROM stats_rules;" );
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
        qry.exec( QString( "DELETE FROM %1s;" ).arg( period ) );
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
    QString qryStr = "REPLACE INTO general (id, version, last_saved, calendar )"
                     " VALUES (?, ?, ?, ? );";
    qry.prepare( qryStr );
    qry.addBindValue( 1 );
    qry.addBindValue( 1 );
    qry.addBindValue( QDateTime::currentDateTime().toTime_t() );
    qry.addBindValue( sd->calendar->calendarType() );
    qry.exec();

    if ( models )
    {
        foreach ( StatisticsModel * s, *models )
        {
            if ( s->periodType() == KNemoStats::Hour ||
                 ( fullSave && !( ( ( s->periodType() == KNemoStats::Day ) ) ) )
               )
            {
                int deleteFrom = sd->saveFromId.value( s->periodType() );
                qryStr = QString( "DELETE FROM %1s WHERE id >= '%2';" )
                            .arg( periods.at( s->periodType() ) )
                            .arg( deleteFrom );
                qry.exec( qryStr );
            }

            if ( !s->rowCount() )
            {
                continue;
            }

            QString daysStr1;
            QString daysStr2;
            if ( s->periodType() == KNemoStats::BillPeriod )
            {
                daysStr1 += " days,";
                daysStr2 += " ?,";
            }
            qryStr = "REPLACE INTO %1s (id, datetime,%2 rx, tx )"
                         " VALUES (?, ?,%3 ?, ? );";
            qryStr = qryStr
                        .arg( periods.at( s->periodType() ) )
                        .arg( daysStr1 )
                        .arg( daysStr2 );
            qry.prepare( qryStr );

            int j = sd->saveFromId.value( s->periodType() );
            for ( j = s->indexOfId( j ); j < s->rowCount(); ++j )
            {
                qry.addBindValue( s->id( j ) );
                qry.addBindValue( s->dateTime( j ).toString( Qt::ISODate ) );
                if ( s->periodType() == KNemoStats::BillPeriod )
                {
                    qry.addBindValue( s->days( j ) );
                }
                qry.addBindValue( s->rxBytes( j ) );
                qry.addBindValue( s->txBytes( j ) );
                qry.exec();
            }
            if ( s->rowCount() )
            {
                sd->saveFromId.insert( s->periodType(), s->id() );
            }
        }
    }

    if ( fullSave && rules )
    {
        qryStr = QString( "DELETE FROM stats_rules WHERE id >= '%1';" ).arg( rules->count() );
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
        }
    }
}
