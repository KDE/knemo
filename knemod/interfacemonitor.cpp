/* This file is part of KNemo
   Copyright (C) 2004 Percy Leonhardt <percy@eris23.de>

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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <kdebug.h>

#include "interface.h"
#include "interfacemonitor.h"

InterfaceMonitor::InterfaceMonitor( QObject* parent, const char* name )
    : QObject( parent, name )
{
}

InterfaceMonitor::~InterfaceMonitor()
{
}

void InterfaceMonitor::checkStatus( Interface* interface )
{
    int currentState;
    int previousState = interface->getState();
    const InterfaceData& data = interface->getData();
    int trafficThreshold = interface->getSettings().trafficThreshold;

    if ( !data.existing )
        // the interface does not exist
        currentState = Interface::NOT_EXISTING;
    else if ( !data.available )
        // the interface exists but is not connected
        currentState = Interface::NOT_AVAILABLE;
    else
    {
        // the interface is connected, look for traffic
        currentState = Interface::AVAILABLE;
        if ( ( data.rxPackets - mData.rxPackets ) > (unsigned int) trafficThreshold )
            currentState |= Interface::RX_TRAFFIC;
        if ( ( data.txPackets - mData.txPackets ) > (unsigned int) trafficThreshold )
            currentState |= Interface::TX_TRAFFIC;
    }

    mData = data; // backup current data

    if ( ( previousState == Interface::NOT_EXISTING ||
           previousState == Interface::NOT_AVAILABLE ||
           previousState == Interface::UNKNOWN_STATE ) &&
         currentState & Interface::AVAILABLE )
    {
        emit available( previousState );
    }
    else if ( ( previousState == Interface::NOT_EXISTING ||
                previousState & Interface::AVAILABLE ||
                previousState == Interface::UNKNOWN_STATE ) &&
              currentState == Interface::NOT_AVAILABLE )
    {
        emit notAvailable( previousState );
    }
    else if ( ( previousState == Interface::NOT_AVAILABLE ||
                previousState & Interface::AVAILABLE ||
                previousState == Interface::UNKNOWN_STATE ) &&
              currentState == Interface::NOT_EXISTING )
    {
        emit notExisting( previousState );
    }

    // make sure the icon fits the current state
    if ( previousState != currentState )
    {
        emit statusChanged( currentState );
        interface->setState( currentState );
    }
}

#include "interfacemonitor.moc"
