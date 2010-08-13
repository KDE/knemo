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

// This will go away soon
static QString typeToElem( enum KNemoStats::PeriodUnits t )
{
    int i = 0;
    int v = t;
    while ( v > KNemoStats::Hour )
    {
        v = v >> 1;
        i++;
    }
    return periods[i];
}

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

    foreach ( QString period, periods )
    {
        qryStr = "CREATE TABLE IF NOT EXISTS %1s (id INTEGER PRIMARY KEY, datetime DATETIME,"
                 " rx BIGINT, tx BIGINT, days INTEGER );";

        qryStr = qryStr.arg( period );
        qry.exec( qryStr );
    }

    ok = QSqlDatabase::database( mIfaceName ).commit();
    db.close();
    return ok;
}

bool SqlStorage::loadStats( StorageData *sd, QHash<int, StatisticsModel*> *models )
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
            qry.exec( QString( "SELECT * FROM %1s ORDER BY id;" ).arg( typeToElem( s->periodType() ) ) );
            int cId = qry.record().indexOf( "id" );
            int cDt = qry.record().indexOf( "datetime" );
            int cRx = qry.record().indexOf( "rx" );
            int cTx = qry.record().indexOf( "tx" );
            int cDays = qry.record().indexOf( "days" );

            while ( qry.next() )
            {
                int id = qry.value( cId ).toInt();
                s->createEntry();
                s->setId( id );
                s->setDays( qry.value( cDays ).toInt() );
                s->setDateTime( QDateTime::fromString( qry.value( cDt ).toString(), Qt::ISODate ) );
                s->setTraffic( id, qry.value( cRx ).toULongLong(), qry.value( cTx ).toULongLong() );
            }
            if ( s->rowCount() )
                sd->saveFromId.insert( s->periodType(), s->id() );
        }
    }

    ok = QSqlDatabase::database( mIfaceName ).commit();
    db.close();
    return ok;
}

bool SqlStorage::saveStats( StorageData *sd, QHash<int, StatisticsModel*> *models )
{
    bool ok = false;
    if ( !open() )
        return ok;

    QSqlDatabase::database( mIfaceName ).transaction();

    save( sd, models );

    ok = QSqlDatabase::database( mIfaceName ).commit();
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
        qry.exec( QString( "DELETE FROM %1s;" ).arg( period ) );
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

void SqlStorage::save( StorageData *sd, QHash<int, StatisticsModel*> *models )
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
            qryStr = QString( "DELETE FROM %1s WHERE id >= '%2';" )
                        .arg( typeToElem( s->periodType() ) )
                        .arg( s->id() );
            qry.exec( qryStr );
            QString days1;
            QString days2;
            qryStr = "REPLACE INTO %1s (id, datetime, rx, tx, days )"
                         " VALUES (?, ?, ?, ?, ? );";
            qryStr = qryStr.arg( typeToElem( s->periodType() ) );
            qry.prepare( qryStr );

            for ( int i = sd->saveFromId.value( s->periodType() ); i < s->rowCount(); ++i )
            {
                qry.addBindValue( s->id( i ) );
                qry.addBindValue( s->dateTime( i ).toString( Qt::ISODate ) );
                qry.addBindValue( s->rxBytes( i ) );
                qry.addBindValue( s->txBytes( i ) );
                qry.addBindValue( s->days( i ) );
                qry.exec();
            }
            int sf = s->rowCount() - 1;
            if ( sf < 0 )
                sf = 0;
            sd->saveFromId.insert( s->periodType(), sf );
        }
    }
}
