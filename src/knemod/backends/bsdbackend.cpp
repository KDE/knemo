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


#include <arpa/inet.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <net/ethernet.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>

#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/if_var.h>
#include <netinet/in_var.h>

#include <KLocale>
#include <kio/global.h>
#include <stdio.h>

#include "config-knemo.h"
#include "bsdbackend.h"
#include "utils.h"

BSDBackend::BSDBackend( QHash<QString, Interface *>& interfaces )
    : BackendBase( interfaces )
{
}

BSDBackend::~BSDBackend()
{
}

BackendBase* BSDBackend::createInstance( QHash<QString, Interface *>& interfaces )
{
    return new BSDBackend( interfaces );
}

void BSDBackend::update()
{
    getifaddrs( &ifaddr );

    QString ip4DefGw, ip6DefGw;
    getDefaultRoute( AF_INET, &ip4DefGw );
    getDefaultRoute( AF_INET6, &ip6DefGw );

    foreach ( QString key, mInterfaces.keys() )
    {
        Interface *interface = mInterfaces.value( key );
        interface->getData().existing = false;
        interface->getData().available = false;
        interface->getData().incomingBytes = 0;
        interface->getData().outgoingBytes = 0;
        interface->getData().addrData.clear();
        interface->getData().ip4DefaultGateway = ip4DefGw;
        interface->getData().ip6DefaultGateway = ip6DefGw;
        interface->getData().interfaceType = Interface::ETHERNET;

        updateInterfaceData( key, interface->getData() );
    }

    freeifaddrs( ifaddr );
    updateComplete();
}

QString BSDBackend::getDefaultRouteIface( int afInet )
{
    return getDefaultRoute( afInet );
}

QString BSDBackend::formattedAddr( struct sockaddr* addr )
{
    int type = addr->sa_family;
    char host[ NI_MAXHOST ];
    int error;
    if ( addr )
    {
        if ( addr->sa_family == AF_INET6 )
        {
            // This way we get the textual zone index on BSD
            sockaddr_in6 * sin = reinterpret_cast<sockaddr_in6*>(addr);
            if ( IN6_IS_ADDR_LINKLOCAL( &sin->sin6_addr ) &&
                 *reinterpret_cast<u_short *>(&sin->sin6_addr.s6_addr[2]) != 0)
            {
                u_short index = *reinterpret_cast<u_short *>(&sin->sin6_addr.s6_addr[2]);
                *reinterpret_cast<u_short *>(&sin->sin6_addr.s6_addr[2]) = 0;
                if ( sin->sin6_scope_id == 0 )
                    sin->sin6_scope_id = ntohs( index );
            }
        }

        socklen_t size;
        if ( type == AF_INET )
            size = sizeof(struct sockaddr_in);
        else
            size = sizeof(struct sockaddr_in6);
        error = getnameinfo( addr, size, host, sizeof( host ), NULL, 0, NI_NUMERICHOST );
        if ( 0 == error )
        {
            QString fHost( host );
            int index = fHost.indexOf( '%' );
            if ( index > -1 )
                fHost = fHost.left( index );
            return fHost;
        }
    }
    return QString();
}

QString BSDBackend::getAddr( struct ifaddrs *ifa, AddrData& addrData )
{
    QString strFlags;
    addrData.afType = ifa->ifa_addr->sa_family;
    addrData.hasPeer = false;

    QString address = formattedAddr( ifa->ifa_addr );

    if ( ifa->ifa_flags & IFF_POINTOPOINT && ifa->ifa_dstaddr )
    {
        addrData.hasPeer = true;
        addrData.broadcastAddress = formattedAddr( ifa->ifa_dstaddr );
    }
    else if ( ifa->ifa_flags & IFF_BROADCAST && ifa->ifa_broadaddr )
        addrData.broadcastAddress = formattedAddr( ifa->ifa_broadaddr );

    if ( addrData.afType == AF_INET )
    {
        struct sockaddr_in * sin = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
        if ( reinterpret_cast<char*>(&(sin->sin_addr.s_addr))[0] == 0x7f )
            addrData.scope = RT_SCOPE_HOST;
        else
            addrData.scope = RT_SCOPE_UNIVERSE;
    }
    else if ( addrData.afType == AF_INET6 )
    {
        struct in6_ifaddr *ifa6 = reinterpret_cast<struct in6_ifaddr *>(ifa);
        if ( ifa6->ia6_flags & IN6_IFF_ANYCAST )
            strFlags += i18n( " anycast" );
        if ( ifa6->ia6_flags & IN6_IFF_TENTATIVE )
            strFlags += i18n( " tentative" );
        if ( ifa6->ia6_flags & IN6_IFF_DUPLICATED )
            strFlags += i18n( " duplicate" );
        if ( ifa6->ia6_flags & IN6_IFF_DETACHED )
            strFlags += i18n( " detached" );
        if ( ifa6->ia6_flags & IN6_IFF_DEPRECATED )
            strFlags += i18n( " deprecated" );
        if ( ifa6->ia6_flags & IN6_IFF_NODAD )
            strFlags += i18n( " nodad" );
        if ( ifa6->ia6_flags & IN6_IFF_AUTOCONF )
            strFlags += i18n( " autoconf" );
        if ( ifa6->ia6_flags & IN6_IFF_TEMPORARY )
            strFlags += i18n( " tempory" );
        addrData.ipv6Flags = strFlags;

        // Is this right?
        if ( IN6_IS_ADDR_LOOPBACK( &ifa6->ia_addr.sin6_addr ) )
            addrData.scope = RT_SCOPE_HOST;
        else if ( IN6_IS_ADDR_LINKLOCAL( &ifa6->ia_addr.sin6_addr ) )
            addrData.scope = RT_SCOPE_LINK;
        else if ( IN6_IS_ADDR_SITELOCAL( &ifa6->ia_addr.sin6_addr ) )
            addrData.scope = RT_SCOPE_SITE;
        else
            addrData.scope = RT_SCOPE_UNIVERSE;
    }

    if ( addrData.hasPeer && !addrData.broadcastAddress.isEmpty() )
        addrData.broadcastAddress = addrData.broadcastAddress + "/" + QString::number( getSubnet( ifa ) );
    else if ( !addrData.hasPeer && !address.isEmpty() )
        address = address + "/" + QString::number( getSubnet( ifa ) );

    return address;
}

int BSDBackend::getSubnet( struct ifaddrs * ifa )
{
    unsigned int len = 0;
    unsigned int maxlen = 0;
    uint8_t *ptr = NULL;

    if ( ifa->ifa_netmask )
    {
        if ( AF_INET == ifa->ifa_addr->sa_family )
        {
            struct in_addr netmask = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_netmask)->sin_addr;
            ptr = reinterpret_cast<uint8_t*>(&netmask);
            maxlen = 32;
        }
        else if ( AF_INET6 == ifa->ifa_addr->sa_family )
        {
            struct in6_addr netmask = reinterpret_cast<struct sockaddr_in6*>(ifa->ifa_netmask)->sin6_addr;
            ptr = reinterpret_cast<uint8_t*>(&netmask);
            maxlen = 128;
        }

        while ( (0xff == *ptr) )
        {
            len += 8;
            ptr++;
        }
        if ( len < maxlen )
        {
            uint8_t val = *ptr;
            while ( val )
            {
                len++;
                val <<= 1;
            }
        }
    }
    return len;
}

void BSDBackend::updateInterfaceData( const QString& ifName, InterfaceData& data )
{
    struct ifaddrs *ifa;
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        QString ifaddrName( ifa->ifa_name );
        int index = ifaddrName.indexOf( ':' );
        if ( index > -1 )
            ifaddrName = ifaddrName.left( index );

        if ( ifName != ifaddrName )
            continue;

        // stats
        if ( ifa->ifa_data )
        {
            unsigned long rx_bytes = 0, tx_bytes = 0;
            struct if_data * stats = static_cast<if_data *>(ifa->ifa_data);
            data.rxPackets = stats->ifi_ipackets;
            data.txPackets = stats->ifi_opackets;
            rx_bytes = stats->ifi_ibytes;
            tx_bytes = stats->ifi_obytes;
            if ( IFT_PPP == stats->ifi_type )
                data.interfaceType == Interface::PPP;
            incBytes( data.interfaceType, rx_bytes, data.incomingBytes, data.prevRxBytes, data.rxBytes );
            data.rxString = KIO::convertSize( data.rxBytes );

            incBytes( data.interfaceType, tx_bytes, data.outgoingBytes, data.prevTxBytes, data.txBytes );
            data.txString = KIO::convertSize( data.txBytes );
        }

        data.existing = true;

        // mac address, interface type
        if ( ifa->ifa_addr )
        {
            if ( ifa->ifa_addr->sa_family == AF_LINK )
            {
                // Mac address
                unsigned char macBuffer[6];
                char macstr[19];
                memset( macBuffer, 0, sizeof(macBuffer) );
                memset( macstr, 0, sizeof(macstr) );
                const struct sockaddr_dl* sdl = reinterpret_cast<struct sockaddr_dl*>(ifa->ifa_addr);
                if ( sdl->sdl_type == IFT_ETHER && sdl->sdl_alen == ETHER_ADDR_LEN )
                     memcpy(macBuffer, LLADDR(sdl), 6);
                snprintf( macstr, sizeof(macstr), "%02x:%02x:%02x:%02x:%02x:%02x",
                          macBuffer[0], macBuffer[1], macBuffer[2],
                          macBuffer[3], macBuffer[4], macBuffer[5] );
                data.hwAddress = macstr;
            }
            else if ( ifa->ifa_addr->sa_family == AF_INET ||
                      ifa->ifa_addr->sa_family == AF_INET6 )
            {
                QString addrKey;
                AddrData addrVal;

                if ( ifa->ifa_flags && IFF_UP )
                    data.available = true;

                addrKey = getAddr( ifa, addrVal );

                if ( !addrKey.isEmpty() )
                    data.addrData.insert( addrKey, addrVal );
            }
        }
    }
}

void BSDBackend::updateWirelessData( int fd, const QString& ifName, WirelessData& data )
{
    // TODO
}
