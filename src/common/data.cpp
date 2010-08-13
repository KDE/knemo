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

#include "data.h"
#include <KCalendarSystem>

bool StatsRule::operator==( StatsRule &r )
{
    if ( startDate != r.startDate ||
         periodCount != r.periodCount ||
         periodUnits != r.periodUnits ||
         logOffpeak != r.logOffpeak )
    {
        return false;
    }
    else if ( logOffpeak &&
              ( offpeakStartTime != r.offpeakStartTime ||
                offpeakEndTime != r.offpeakEndTime ||
                weekendIsOffpeak != r.weekendIsOffpeak )
            )
    {
        return false;
    }
    else if ( weekendIsOffpeak &&
              ( weekendDayStart != r.weekendDayStart ||
                weekendDayEnd != r.weekendDayEnd ||
                weekendTimeStart != r.weekendTimeStart ||
                weekendTimeEnd != r.weekendTimeEnd )
            )
    {
        return false;
    }
    return true;
}

bool StatsRule::isValid( KCalendarSystem *cal )
{
    if ( !cal->isValid( startDate ) )
        return false;
    if ( logOffpeak && ( !offpeakStartTime.isValid() || !offpeakEndTime.isValid() ) )
        return false;
    if ( weekendIsOffpeak && ( !weekendTimeStart.isValid() || !weekendTimeEnd.isValid() ) )
        return false;
    if ( periodUnits < KNemoStats::Day || periodUnits > KNemoStats::Year )
        return false;
    return true;
}
