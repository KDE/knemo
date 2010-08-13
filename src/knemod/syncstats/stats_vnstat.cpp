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

#include "stats_vnstat.h"
#include "statisticsmodel.h"
#include "interface.h"
#include "data.h"
#include <QFile>

#include <KProcess>

#ifndef __linux__
#include <sys/sysctl.h>
#endif

StatsVnstat::StatsVnstat( Interface * interface, KCalendarSystem * calendar, QObject * parent )
    : ExternalStats( interface, calendar, parent ),
      mVnstatRx( 0 ),
      mVnstatTx( 0 ),
      mSysBtime( 0 ),
      mVnstatBtime( 0 )
{
}

StatsVnstat::~StatsVnstat()
{
}

void StatsVnstat::importIfaceStats()
{
    mExternalHours->clear();
    mExternalDays->clear();
    KProcess vnstatProc;
    vnstatProc.setOutputChannelMode( KProcess::OnlyStdoutChannel );
    vnstatProc.setEnv( "LANG", "C" );
    vnstatProc.setEnv( "LC_ALL", "C" );
    vnstatProc << "vnstat" << "--dumpdb" << "-i" << mInterface->getName();
    vnstatProc.execute();

    parseOutput( vnstatProc.readAllStandardOutput() );
    getBtime();
}

void StatsVnstat::getBtime()
{
#ifdef __linux__
    QFile proc( "/proc/stat" );
    if ( proc.open( QIODevice::ReadOnly ) )
    {
        QString routeData( proc.readAll().data() );
        QStringList routes = routeData.split( "\n" );
        QStringListIterator it( routes );
        while ( it.hasNext() )
        {
            QStringList line = it.next().split( ' ' );
            if ( line.count() < 2 )
                continue;
            if ( line[0] == "btime" )
            {
                mSysBtime = line[1].toUInt();
                break;
            }
        }
        proc.close();
    }
#else
    struct timeval bootTime;
    size_t len = sizeof( bootTime );
    int mib[2] = { CTL_KERN, KERN_BOOTTIME };

    if ( sysctl( mib, 2, &bootTime, &len, NULL, 0 ) >= 0 )
        mSysBtime = bootTime.tv_sec;
#endif
}

StatsPair StatsVnstat::addLagged( uint lastUpdated, StatisticsModel * days )
{
    StatsPair lag;

    if ( mExternalDays->rowCount() < 1 )
        return lag;

    if ( abs(mSysBtime - mVnstatBtime) > 30 )
    {
        if ( mSysBtime > lastUpdated )
        {
            lag.rxBytes = mInterface->getData()->rxBytes;
            lag.txBytes = mInterface->getData()->txBytes;
        }
    }
    else if ( ( lastUpdated > mVnstatUpdated ) )
    {
        if ( days->rxBytes() > mExternalDays->rxBytes() )
        {
            quint64 loggedDiff = days->rxBytes() - mExternalDays->rxBytes();
            quint64 byteCounterDiff = mInterface->getData()->rxBytes - mVnstatRx;
            if ( byteCounterDiff > loggedDiff )
            {
                lag.rxBytes = byteCounterDiff - loggedDiff;
            }
        }
        if ( days->txBytes() > mExternalDays->txBytes() )
        {
            quint64 loggedDiff = days->txBytes() - mExternalDays->txBytes();
            quint64 byteCounterDiff = mInterface->getData()->txBytes - mVnstatTx;
            if ( byteCounterDiff > loggedDiff )
            {
                lag.txBytes = byteCounterDiff - loggedDiff;
            }
        }
    }

    return lag;
}

quint64 StatsVnstat::addBytes( quint64 rxBytes, quint64 syncBytes )
{
    quint64 appendBytes = 0;
    if ( rxBytes < syncBytes && syncBytes - rxBytes >= 512 )
        appendBytes = syncBytes - rxBytes;
    return appendBytes;
}

void StatsVnstat::parseOutput( const QString &output )
{
    QStringList lines = output.split( '\n' );
    foreach ( QString line, lines )
    {
        QStringList fields = line.split( ';' );
        if ( fields[0] == "updated" )
        {
            mVnstatUpdated = fields[1].toUInt();
        }
        else if ( fields[0] == "currx" )
        {
            mVnstatRx = fields[1].toULongLong();
        }
        else if ( fields[0] == "curtx" )
        {
            mVnstatTx = fields[1].toULongLong();
        }
        else if ( fields[0] == "btime" )
        {
            mVnstatBtime = fields[1].toUInt();
        }
        else if ( fields[0] == "d" )
        {
            quint64 rx = fields[3].toULongLong() * 1024 * 1024;
            quint64 tx = fields[4].toULongLong() * 1024 * 1024;
            rx += fields[5].toULongLong() * 1024;
            tx += fields[6].toULongLong() * 1024;
            if ( rx != 0 || tx != 0 )
            {
                int entryIndex = mExternalDays->createEntry();
                mExternalDays->setDateTime( QDateTime::fromTime_t(fields[2].toUInt()) );
                mExternalDays->setDays( 1 );
                mExternalDays->setTraffic( entryIndex, rx, tx );
            }
        }
        else if ( fields[0] == "h" )
        {
            quint64 rx = fields[3].toULongLong() * 1024;
            quint64 tx = fields[4].toULongLong() * 1024;
            QDateTime hour = QDateTime::fromTime_t(fields[2].toUInt());
            hour = QDateTime( hour.date(), QTime( hour.time().hour(), 0 ) );
            if ( rx != 0 || tx != 0 )
            {
                int entryIndex = mExternalHours->createEntry();
                mExternalHours->setDateTime( hour );
                mExternalHours->setTraffic( entryIndex, rx, tx );
            }
        }
    }
    mExternalHours->sort( 0 );
    mExternalDays->sort( 0 );
}

#include "stats_vnstat.moc"
