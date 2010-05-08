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

#ifndef STATS_VNSTAT
#define STATS_VNSTAT

#include "externalstats.h"
#include <time.h>
#include <QDateTime>

class StatsVnstat : public ExternalStats
{
    Q_OBJECT
    public:
        StatsVnstat( Interface * interface, QObject * parent = 0 );
        virtual ~StatsVnstat();
        void importIfaceStats();

        quint64 addBytes( quint64 localBytes, quint64 externalBytes );

        StatsPair addLagged( uint lastSaved, StatisticsModel * days );

    private:
        void parseOutput( const QString &output );
        void getBtime();

        quint64 mVnstatRx;
        quint64 mVnstatTx;
        time_t mSysBtime;
        time_t mVnstatBtime;
        uint mVnstatUpdated;
};

#endif
