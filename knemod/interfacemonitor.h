/* This file is part of KNemo
   Copyright (C) 2004, 2006 Percy Leonhardt <percy@eris23.de>

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

#ifndef INTERFACEMONITOR_H
#define INTERFACEMONITOR_H

#include <qobject.h>

#include "data.h"

class Interface;

/**
 * This class monitors the interface for possible state changes and
 * for incoming and outgong traffic. If the state changed or traffic
 * was transmitted it sends an according signal.
 *
 * @short Monitor changes of the interface
 * @author Percy Leonhardt <percy@eris23.de>
 */

class InterfaceMonitor : public QObject
{
    Q_OBJECT
public:
    /**
     * Default Constructor
     */
    InterfaceMonitor(QObject* parent = 0L, const char* name = 0L);

    /**
     * Default Destructor
     */
    virtual ~InterfaceMonitor();

    /**
     * Tell the monitor to check the status of the interface
     */
    void checkStatus( Interface* interface );

signals:
    // the interface is now connected
    void available( int );
    // the interface is now disconnected
    void notAvailable( int );
    // the interface no longer exists
    void notExisting( int );
    // there was incoming and/or outgoing traffic
    void statusChanged( int );
    // the amount of incoming traffic (for statistics)
    void incomingData( unsigned long );
    // the amount of outgoing traffic (for statistics)
    void outgoingData( unsigned long );
};

#endif // INTERFACEMONITOR_H
