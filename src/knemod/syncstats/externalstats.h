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

#ifndef EXTERNALSTATS_H
#define EXTERNALSTATS_H

#include <QObject>

struct StatsPair
{
    StatsPair()
        : rxBytes( 0 ),
          txBytes( 0 )
    {
    }
    quint64 rxBytes;
    quint64 txBytes;
};

class Interface;
class StatisticsModel;
class KCalendarSystem;

class ExternalStats : public QObject
{
    Q_OBJECT
    public:
        ExternalStats( Interface * interface, KCalendarSystem * calendar, QObject * parent = 0 );
        virtual ~ExternalStats();

        /* Import hour/day statistics into StatisticsModels */
        virtual void importIfaceStats() = 0;

        /* This returns the bytes since the last time statistics were recorded
         * by the external tool.
         */
        virtual StatsPair addLagged( uint lastUpdated, StatisticsModel * days ) = 0;

        /* The imported statistics */
        const StatisticsModel * days() const { return mExternalDays; }
        const StatisticsModel * hours() const { return mExternalHours; }

        /* Given KNemo's byte record and what the external tool reports for the
         * same entry, return the difference.  We do it this way because vnstat
         * reports KiB, and I want to account for rounding errors in a
         * reasonable way.
         */
        virtual quint64 addBytes( quint64 localBytes, quint64 externalBytes ) = 0;

    protected:
        Interface * mInterface;
        StatisticsModel * mExternalDays;
        StatisticsModel * mExternalHours;
};

#endif
