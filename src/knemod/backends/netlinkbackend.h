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

#ifndef NETLINKBACKEND_H
#define NETLINKBACKEND_H

#include "backendbase.h"
#include <netlink/route/link.h>

#ifdef HAVE_LIBIW
#include "netlinkbackend_wireless.h"
#endif

/**
 * This uses libnl and libiw to get information.
 * It then triggers the interface monitor to look for changes
 * in the state of the interface.
 *
 * @short Update the information of the interfaces via netlink
 * @author John Stamp <jstamp@users.sourceforge.net>
 */

class NetlinkBackend : public BackendBase
{
    Q_OBJECT
public:
    NetlinkBackend();
    virtual ~NetlinkBackend();

    static BackendBase* createInstance();

    virtual void update();
    virtual QStringList ifaceList();
    virtual QString defaultRouteIface( int afInet );

private:
    void updateIfaceData( const QString& ifName, BackendData* data );
    void updateAddresses( BackendData* data );
    nl_sock * rtsock;
    nl_cache *addrCache, *linkCache, *routeCache;
#ifdef HAVE_LIBIW
    NetlinkBackend_Wireless wireless;
#endif
};

#endif // NETLINKBACKEND_H
