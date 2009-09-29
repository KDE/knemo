/* This file is part of KNemo
   Copyright (C) 2006 Percy Leonhardt <percy@eris23.de>
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

#include "backendbase.h"
#include "utils.h"

BackendBase::BackendBase( QHash<QString, Interface *>& interfaces )
    : mInterfaces( interfaces )
{
}

BackendBase::~BackendBase()
{
}

void BackendBase::updateComplete()
{
    foreach ( QString key, mInterfaces.keys() )
    {
        mInterfaces.value( key )->activateMonitor();
    }
}

void BackendBase::incBytes( int type, unsigned long bytes, unsigned long &changed, unsigned long &prevDataBytes, quint64 &curDataBytes )
{
    // We count the traffic on ourself to avoid an overflow after
    // 4GB of traffic.
    if ( bytes < prevDataBytes )
    {
        // there was an overflow
        if ( type == Interface::ETHERNET )
        {
            // This makes data counting more accurate but will not work
            // for interfaces that reset the transfered data to zero
            // when deactivated like ppp does.
            // BSD uses uint32_t for byte counters, so no need to test
            // size
#ifdef __linux__
            if ( 4 == sizeof(long) )
#endif
                curDataBytes += 0xFFFFFFFF - prevDataBytes;
        }
        prevDataBytes = 0L;
    }
    if ( curDataBytes == 0L )
    {
        // on startup set to currently received bytes
        curDataBytes = bytes;
        // this is new: KNemo only counts the traffic transfered
        // while it is running. Important to not falsify statistics!
        prevDataBytes = bytes;
    }
    else
        // afterwards only add difference to previous number of bytes
        curDataBytes += bytes - prevDataBytes;

    changed = bytes - prevDataBytes;
    prevDataBytes = bytes;
}

