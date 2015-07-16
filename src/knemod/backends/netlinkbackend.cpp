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

#include <netlink/route/addr.h>
#include <netlink/route/rtnl.h>
#include <netlink/route/link.h>
#include <netlink/route/route.h>

#include <KLocalizedString>
#include <kio/global.h>

#include "config-knemo.h"
#include "utils.h"
#include "netlinkbackend.h"

#ifndef IFF_LOWER_UP
#define IFF_LOWER_UP   0x10000
#endif

NetlinkBackend::NetlinkBackend()
    : rtsock( NULL ),
      addrCache( NULL ),
      linkCache( NULL ),
      routeCache( NULL )
{
    rtsock = nl_socket_alloc();
    int c = nl_connect(rtsock, NETLINK_ROUTE);
    if ( c >= 0 )
    {
        rtnl_addr_alloc_cache( rtsock, &addrCache );
        rtnl_link_alloc_cache( rtsock, AF_UNSPEC, &linkCache );
        rtnl_route_alloc_cache( rtsock, AF_UNSPEC, 0, &routeCache );
    }
#ifdef HAVE_LIBIW
    wireless.openSocket();
#endif
}

NetlinkBackend::~NetlinkBackend()
{
    nl_cache_free( addrCache );
    nl_cache_free( linkCache );
    nl_cache_free( routeCache );
    nl_close( rtsock );
    nl_socket_free( rtsock );
#ifdef HAVE_LIBIW
    wireless.closeSocket();
#endif
}

QStringList NetlinkBackend::ifaceList()
{
    QStringList ifaces;
    struct rtnl_link * rtlink;
    for ( rtlink = reinterpret_cast<struct rtnl_link *>(nl_cache_get_first( linkCache ));
          rtlink != NULL;
          rtlink = reinterpret_cast<struct rtnl_link *>(nl_cache_get_next( reinterpret_cast<struct nl_object *>(rtlink) ))
        )
    {
        QString ifname( rtnl_link_get_name( rtlink ) );
        ifaces << ifname;
    }
    return ifaces;
}
void NetlinkBackend::update()
{
    nl_cache_refill( rtsock, addrCache );
    nl_cache_refill( rtsock, linkCache );
    nl_cache_refill( rtsock, routeCache );

    getDefaultRoute( AF_INET, &ip4DefGw, routeCache );
    getDefaultRoute( AF_INET6, &ip6DefGw, routeCache );

    foreach ( QString key, mInterfaces.keys() )
    {
        BackendData *interface = mInterfaces.value( key );
        updateIfaceData( key, interface );

#ifdef HAVE_LIBIW
        wireless.update( key, interface );
#endif
    }
    emit updateComplete();
}

QString NetlinkBackend::defaultRouteIface( int afInet )
{
    return getDefaultRoute( afInet, NULL, routeCache );
}

BackendBase* NetlinkBackend::createInstance()
{
    return new NetlinkBackend();
}

void NetlinkBackend::updateAddresses( BackendData *data )
{
    struct rtnl_addr * rtaddr;
    for ( rtaddr = reinterpret_cast<struct rtnl_addr *>(nl_cache_get_first( addrCache ));
          rtaddr != NULL;
          rtaddr = reinterpret_cast<struct rtnl_addr *>(nl_cache_get_next( reinterpret_cast<struct nl_object *>(rtaddr) ))
        )
    {
        if ( data->index != rtnl_addr_get_ifindex( rtaddr ) )
            continue;

        struct nl_addr * addr = rtnl_addr_get_local( rtaddr );
        char buf[ 128 ];
        QString addrKey;
        AddrData addrVal;

        addrVal.afType = rtnl_addr_get_family( rtaddr );
        addrVal.label = rtnl_addr_get_label( rtaddr );
        addrVal.scope = rtnl_addr_get_scope( rtaddr );

        nl_addr2str( addr, buf, sizeof( buf ) );
        addrKey = buf;

        QString strFlags;
        int flags = rtnl_addr_get_flags( rtaddr );
        if (flags & IFA_F_SECONDARY )
            strFlags += i18n( " secondary" );
        if ( flags & IFA_F_NODAD )
            strFlags += i18n( " nodad" );
        if ( flags & IFA_F_OPTIMISTIC )
            strFlags += i18n( " optimistic" );
        if ( flags & IFA_F_HOMEADDRESS )
            strFlags += i18nc( "mobile ipv6 home address flag", " homeaddress" );
        if ( flags & IFA_F_DEPRECATED )
            strFlags += i18n( " deprecated" );
        if ( flags & IFA_F_TENTATIVE )
            strFlags += i18n( " tentative" );
        if ( ! (flags & IFA_F_PERMANENT) )
            strFlags += i18n( " dynamic" );
        addrVal.ipv6Flags = strFlags;

        addrVal.hasPeer = false;
        addr = rtnl_addr_get_peer( rtaddr );
        if ( addr )
        {
            nl_addr2str( addr, buf, sizeof( buf ) );
            addrVal.hasPeer = true;
            addrVal.broadcastAddress = buf;
            int prefixlen = rtnl_addr_get_prefixlen( rtaddr );
            if ( prefixlen >= 0 && !addrVal.broadcastAddress.contains( '/' ) )
                addrVal.broadcastAddress = addrVal.broadcastAddress + "/" + QString::number( prefixlen );
        }
        else
        {
            addr = rtnl_addr_get_broadcast( rtaddr );
            if ( addr )
            {
                nl_addr2str( addr, buf, sizeof( buf ) );
                addrVal.broadcastAddress = buf;
            }
            int prefixlen = rtnl_addr_get_prefixlen( rtaddr );
            if ( prefixlen >= 0 && !addrKey.contains( '/' ) )
                addrKey = addrKey + "/" + QString::number( prefixlen );
        }

        data->addrData.insert( addrKey, addrVal );
    }
}

void NetlinkBackend::updateIfaceData( const QString& ifName, BackendData* data )
{
    if ( !linkCache || !addrCache )
        return;

    data->status = KNemoIface::UnknownState;
    data->incomingBytes = 0;
    data->outgoingBytes = 0;
    data->prevRxPackets = data->rxPackets;
    data->prevTxPackets = data->txPackets;
    data->addrData.clear();
    data->ip4DefaultGateway = ip4DefGw;
    data->ip6DefaultGateway = ip6DefGw;

    struct rtnl_link * link = rtnl_link_get_by_name( linkCache, ifName.toLocal8Bit().data() );
    if ( link )
    {
        data->index = rtnl_link_get_ifindex( link );
        unsigned long rx_bytes = 0, tx_bytes = 0;
        char mac[ 20 ];
        memset( mac, 0, sizeof( mac ) );

        unsigned int flags = rtnl_link_get_flags( link );
        if ( flags & IFF_POINTOPOINT )
            data->interfaceType = KNemoIface::PPP;
        else
            data->interfaceType = KNemoIface::Ethernet;

        // hw address
        struct nl_addr * addr = rtnl_link_get_addr( link );
        if ( addr && nl_addr_get_len( addr ) )
            nl_addr2str( addr, mac, sizeof( mac ) );
        data->hwAddress = mac;

        // traffic statistics
        data->rxPackets = rtnl_link_get_stat( link, RTNL_LINK_RX_PACKETS );
        data->txPackets = rtnl_link_get_stat( link, RTNL_LINK_TX_PACKETS );
        rx_bytes = rtnl_link_get_stat( link, RTNL_LINK_RX_BYTES );
        tx_bytes = rtnl_link_get_stat( link, RTNL_LINK_TX_BYTES );

        incBytes( data->interfaceType, rx_bytes, data->incomingBytes, data->prevRxBytes, data->rxBytes );
        incBytes( data->interfaceType, tx_bytes, data->outgoingBytes, data->prevTxBytes, data->txBytes );
        data->rxString = KIO::convertSize( data->rxBytes );
        data->txString = KIO::convertSize( data->txBytes );

        updateAddresses( data );

        data->status = KNemoIface::Available;
        if ( flags & IFF_UP )
        {
            data->status |= KNemoIface::Up;
            if ( rtnl_link_get_flags( link ) & IFF_LOWER_UP )
            {
                if ( !(flags & IFF_POINTOPOINT) || data->addrData.size() )
                    data->status |= KNemoIface::Connected;
            }
        }


        rtnl_link_put( link );
    }
    else
        data->status = KNemoIface::Unavailable;
}

#include "moc_netlinkbackend.cpp"
