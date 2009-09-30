/* This file is part of KNemo
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

#ifndef BSDBACKEND_H
#define BSDBACKEND_H

#include <sys/socket.h>

#include "backendbase.h"

/**
 * This backend uses getifaddrs() and friends.
 * It then triggers the interface monitor to look for changes
 * in the state of the interface.
 *
 * @short Update the information of the interfaces using system calls
 * @author John Stamp <jstamp@users.sourceforge.net>
 */

class BSDBackend : public BackendBase
{
public:
    BSDBackend(QHash<QString, Interface *>& interfaces );
    virtual ~BSDBackend();

    static BackendBase* createInstance( QHash<QString, Interface *>& interfaces );

    virtual void update();
    virtual QString getDefaultRouteIface( int afInet );

private:
    QString formattedAddr( struct sockaddr * addr );
    QString getAddr( struct ifaddrs *ifa, AddrData& addrData );
    int getSubnet( struct ifaddrs *ifa );
    void updateWirelessData( int fd, const QString& ifName, WirelessData& data );
    void updateInterfaceData( const QString& ifName, InterfaceData& data );
    struct ifaddrs *ifaddr;
    QHash<QByteArray, QStringList>ipv6Hash;
    QStringList mProcIfInet6;
};

#endif // BSDBACKEND_H