/* This file is part of KNemo
   Copyright (C) 2009 John Stamp <jstamp@users.sourceforge.net>

   Portions adapted from ifieee80211.c:

     Copyright 2001 The Aerospace Corporation.  All rights reserved.

     Redistribution and use in source and binary forms, with or without
     modification, are permitted provided that the following conditions
     are met:
     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
     3. The name of The Aerospace Corporation may not be used to endorse or
        promote products derived from this software.

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
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_media.h>
#include <netinet/in.h>
#define __KAME_NETINET_IN_H_INCLUDED_
#define _WANT_IFADDR
#include <netinet6/in6.h>

#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/if_var.h>
#include <netinet/in_var.h>

#include <KLocale>
#include <kio/global.h>
#include <stdio.h>
#include <unistd.h>

#ifdef __GLIBC__
#include <netinet/ether.h>
#endif

#include "config-knemo.h"
#include "bsdbackend.h"
#include "utils.h"

BSDBackend::BSDBackend()
{
    m_fd = socket( AF_INET, SOCK_DGRAM, 0 );
}

BSDBackend::~BSDBackend()
{
    if ( m_fd >= 0 )
        close( m_fd );
}

BackendBase* BSDBackend::createInstance()
{
    return new BSDBackend();
}

QStringList BSDBackend::ifaceList()
{
    QStringList ifaces;
    struct ifaddrs *ifaddr;
    struct ifaddrs *ifa;
    getifaddrs( &ifaddr );
    for ( ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next )
    {
        ifaces << ifa->ifa_name;
    }
    freeifaddrs( ifaddr );
    return ifaces;
}

void BSDBackend::update()
{
    struct ifaddrs *ifap;
    getifaddrs( &ifap );

    QString ip4DefGw, ip6DefGw;
    getDefaultRoute( AF_INET, &ip4DefGw );
    getDefaultRoute( AF_INET6, &ip6DefGw );

    foreach ( QString key, mInterfaces.keys() )
    {
        BackendData *interface = mInterfaces.value( key );
        interface->status = KNemoIface::UnknownState;
        interface->incomingBytes = 0;
        interface->outgoingBytes = 0;
        interface->prevRxPackets = interface->rxPackets;
        interface->prevTxPackets = interface->txPackets;
        interface->rxPackets = 0;
        interface->txPackets = 0;
        interface->addrData.clear();
        interface->ip4DefaultGateway = ip4DefGw;
        interface->ip6DefaultGateway = ip6DefGw;
        interface->interfaceType = KNemoIface::Ethernet;

        updateIfaceData( ifap, key, interface );

        if ( m_fd >= 0 )
        {
            char essidData[ 32 ];
            int len;
            if ( get80211id( key, -1, essidData, sizeof( essidData ), &len, 0 ) >= 0 )
            {
                interface->isWireless = true;
                updateWirelessData( key, interface );
            }
        }
    }

    freeifaddrs( ifap );
    updateComplete();
}

QString BSDBackend::defaultRouteIface( int afInet )
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
        u_int32_t flags6 = 0;
        const struct sockaddr_in6 * sin6 = NULL;
        struct in6_ifreq ifr6;
        int sock6;

        sin6 = (const struct sockaddr_in6 *)ifa->ifa_addr;
        if (sin6 != NULL) {
            strncpy(ifr6.ifr_name, ifa->ifa_name, sizeof(ifr6.ifr_name));
            ifr6.ifr_addr = *sin6;
            if ((sock6 = socket(AF_INET6, SOCK_DGRAM, 0)) >= 0) {
                if (ioctl(sock6, SIOCGIFAFLAG_IN6, &ifr6) == 0) {
                    flags6 = ifr6.ifr_ifru.ifru_flags6;
                }
            }
        }
        if ( flags6 & IN6_IFF_ANYCAST )
            strFlags += i18n( " anycast" );
        if ( flags6 & IN6_IFF_TENTATIVE )
            strFlags += i18n( " tentative" );
        if ( flags6 & IN6_IFF_DUPLICATED )
            strFlags += i18n( " duplicate" );
        if ( flags6 & IN6_IFF_DETACHED )
            strFlags += i18n( " detached" );
        if ( flags6 & IN6_IFF_DEPRECATED )
            strFlags += i18n( " deprecated" );
        if ( flags6 & IN6_IFF_NODAD )
            strFlags += i18n( " nodad" );
        if ( flags6 & IN6_IFF_AUTOCONF )
            strFlags += i18n( " autoconf" );
        if ( flags6 & IN6_IFF_TEMPORARY )
            strFlags += i18n( " tempory" );
        addrData.ipv6Flags = strFlags;

        // Is this right?
        if ( IN6_IS_ADDR_LOOPBACK( &((const struct sockaddr_in6 *)(&ifa->ifa_addr))->sin6_addr ) )
            addrData.scope = RT_SCOPE_HOST;
        else if ( IN6_IS_ADDR_LINKLOCAL( &((const struct sockaddr_in6 *)(&ifa->ifa_addr))->sin6_addr ) )
            addrData.scope = RT_SCOPE_LINK;
        else if ( IN6_IS_ADDR_SITELOCAL( &((const struct sockaddr_in6 *)(&ifa->ifa_addr))->sin6_addr ) )
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

void BSDBackend::updateIfaceData( struct ifaddrs * ifap, const QString& ifName, BackendData* data )
{
    struct ifaddrs *ifa;
    unsigned long rx_bytes = 0, tx_bytes = 0;
    data->prevStatus = data->status;
    for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next)
    {
        QString ifaddrName( ifa->ifa_name );
        if ( ifName != ifaddrName )
            continue;

        data->status = KNemoIface::Available;

        if ( ifa->ifa_flags & IFF_UP )
            data->status |= KNemoIface::Up;

        if ( ifa->ifa_flags & IFF_POINTOPOINT )
            data->interfaceType = KNemoIface::PPP;


        if ( ifa->ifa_addr )
        {
            if ( ifa->ifa_addr->sa_family == AF_LINK )
            {
                const struct sockaddr_dl* sdl = reinterpret_cast<struct sockaddr_dl*>(ifa->ifa_addr);
                if (sdl != NULL && sdl->sdl_alen > 0)
                {
                    if ( ( sdl->sdl_type == IFT_ETHER ||
                           sdl->sdl_type == IFT_L2VLAN ||
                           sdl->sdl_type == IFT_BRIDGE )
                         && sdl->sdl_alen == ETHER_ADDR_LEN )
                        data->hwAddress = ether_ntoa((struct ether_addr *)LLADDR(sdl));
                }
                if ( ifa->ifa_data )
                {
                    struct if_data * stats = static_cast<if_data *>(ifa->ifa_data);
                    data->rxPackets += stats->ifi_ipackets;
                    data->txPackets += stats->ifi_opackets;
                    rx_bytes += stats->ifi_ibytes;
                    tx_bytes += stats->ifi_obytes;
                }
            }
            // inet address
            else if ( ifa->ifa_addr->sa_family == AF_INET ||
                      ifa->ifa_addr->sa_family == AF_INET6 )
            {
                QString addrKey;
                AddrData addrVal;

                addrKey = getAddr( ifa, addrVal );

                // Check here too for non-ethernet interfaces
                struct ifmediareq ifmr;
                memset( &ifmr, 0, sizeof( ifmr ) );
                strncpy( ifmr.ifm_name, ifName.toLatin1(), sizeof( ifmr.ifm_name ) );
                if ( ioctl( m_fd, SIOCGIFMEDIA, &ifmr ) >= 0 &&
                     ifmr.ifm_status & IFM_AVALID &&
                     ifmr.ifm_status & IFM_ACTIVE &&
                     addrVal.scope != RT_SCOPE_LINK &&
                     addrVal.scope != RT_SCOPE_NOWHERE )
                {
                    data->status |= KNemoIface::Connected;
                }

                if ( !addrKey.isEmpty() )
                {
                    data->addrData.insert( addrKey, addrVal );

                    if ( data->interfaceType == KNemoIface::PPP &&
                         data->status & KNemoIface::Up )
                    {
                        data->status |= KNemoIface::Connected;
                    }
                }
            }
        }

    }

    // Traffic stats. No check needed: if there were no stats,
    // values are and always were 0
    incBytes( data->interfaceType, rx_bytes, data->incomingBytes, data->prevRxBytes, data->rxBytes );
    data->rxString = KIO::convertSize( data->rxBytes );
    incBytes( data->interfaceType, tx_bytes, data->outgoingBytes, data->prevTxBytes, data->txBytes );
    data->txString = KIO::convertSize( data->txBytes );

    if ( data->status < KNemoIface::Available )
        data->status = KNemoIface::Unavailable;
}

void BSDBackend::updateWirelessData( const QString& ifName, BackendData* data )
{
    /* TODO?
       data->bitRate;
       data->nickName;
    */

    struct ieee80211_channel curchan;
    if ( get80211( ifName, IEEE80211_IOC_CURCHAN, &curchan, sizeof( curchan ) ) >= 0 )
    {
        if ( curchan.ic_freq >= 1000 )
        {
            qreal trFreq;
            trFreq = curchan.ic_freq/1000.0;
            data->frequency = i18n( "%1 GHz", trFreq );
        }
        else
            data->frequency = i18n( "%1 MHz", curchan.ic_freq );
        data->channel = QString::number( curchan.ic_ieee );
    }

    int val;
    char essiddat[ 32 ];
    data->essid.clear();
    if ( get80211val( ifName, IEEE80211_IOC_NUMSSIDS, &val ) >= 0 )
    {
        if ( val > 0 )
        {
            for ( int i = 0; i < val; i++ )
            {
                int len;
                if ( get80211id( ifName, i, essiddat, sizeof( essiddat ), &len, 0 ) >= 0 && len > 0 )
                {
                    data->essid = QByteArray( essiddat, len );
                    break;
                }
            }
        }
    }

    if ( get80211( ifName, IEEE80211_IOC_BSSID, essiddat, IEEE80211_ADDR_LEN ) >= 0 )
        data->accessPoint = ether_ntoa( reinterpret_cast<struct ether_addr *>(essiddat) );

    if ( data->accessPoint != data->prevAccessPoint )
    {
        /* Reset encryption status for new access point */
        data->isEncrypted = false;
        data->prevAccessPoint = data->accessPoint;
    }

    if ( get80211val( ifName, IEEE80211_IOC_PRIVACY, &val ) >= 0 )
    {
        if ( val > 0 )
            data->isEncrypted = true;
    }

    enum ieee80211_opmode opmode = get80211opmode( ifName );
    switch ( opmode )
    {
        case IEEE80211_M_AHDEMO:
            data->mode = i18n( "Ad-Hoc Demo" );
            break;
        case IEEE80211_M_IBSS:
            data->mode = i18n( "Ad-Hoc" );
            break;
        case IEEE80211_M_HOSTAP:
            data->mode = i18n( "Host AP" );
            break;
        case IEEE80211_M_MONITOR:
            data->mode = i18n( "Monitor" );
            break;
        case IEEE80211_M_MBSS:
            data->mode = i18n( "Mesh" );
            break;
        case IEEE80211_M_STA:
            data->mode = i18n( "Managed" );
	    break;
	default:
	    break;
    }

    int len;

    union
    {
        struct ieee80211req_sta_req req;
        uint8_t buf[ 24 * 1024 ];
    } u;

    /* broadcast address =>'s get all stations */
    memset( u.req.is_u.macaddr, 0xff, IEEE80211_ADDR_LEN );
    if ( opmode == IEEE80211_M_STA )
    {
        /*
         * Get information about the associated AP.
         */
        get80211( ifName, IEEE80211_IOC_BSSID, u.req.is_u.macaddr, IEEE80211_ADDR_LEN );
    }

    if ( get80211len( ifName, IEEE80211_IOC_STA_INFO, &u, sizeof( u ), &len ) >= 0 &&
         (size_t)len >= sizeof( struct ieee80211req_sta_info )
       )
    {
        /*
         * See http://www.ces.clemson.edu/linux/nm-ipw2200.shtml
         */
        int perfect = 20;
        int worst = 85;
        int rssi = u.req.info->isi_rssi;
        int qual = ( 100 * (perfect - worst) * (perfect - worst) - (perfect - rssi) *
                     (15 * (perfect - worst) + 62 * (perfect - rssi) ) ) /
                   ( (perfect - worst) * (perfect - worst) );
        data->linkQuality = QString( "%1%" ).arg( qual );

        int maxRate = -1;
        for ( int i = 0; i < u.req.info->isi_nrates; i++)
        {
            int rate = u.req.info->isi_rates[ i ] & IEEE80211_RATE_VAL;
            if ( rate > maxRate )
                maxRate = rate;
        }
        if ( maxRate >= 0 )
            data->bitRate = i18n( "%1 Mbps", maxRate / 2 );
    }
}

int BSDBackend::get80211( const QString &ifName, int type, void *data, int len )
{
    struct ieee80211req ireq;

    memset( &ireq, 0, sizeof( ireq ) );
    strncpy( ireq.i_name, ifName.toLatin1(), sizeof( ireq.i_name ) );
    ireq.i_type = type;
    ireq.i_data = data;
    ireq.i_len = len;
    return ioctl( m_fd, SIOCG80211, &ireq );
}

int BSDBackend::get80211len( const QString &ifName, int type, void *data, int len, int *plen)
{
    struct ieee80211req ireq;

    memset( &ireq, 0, sizeof( ireq ) );
    strncpy( ireq.i_name, ifName.toLatin1(), sizeof( ireq.i_name ) );
    ireq.i_type = type;
    ireq.i_len = len;
    if ( ireq.i_len == len )
    {
        ireq.i_data = data;
        if ( ioctl( m_fd, SIOCG80211, &ireq ) < 0 )
            return -1;
        *plen = ireq.i_len;
    }
    return 0;
}

int BSDBackend::get80211id( const QString &ifName, int ix, void *data, size_t len, int *plen, int mesh )
{
    struct ieee80211req ireq;

    memset( &ireq, 0, sizeof( ireq ) );
    strncpy( ireq.i_name, ifName.toLatin1(), sizeof( ireq.i_name ) );
    ireq.i_type = mesh ? IEEE80211_IOC_MESH_ID : IEEE80211_IOC_SSID;
    ireq.i_val = ix;
    ireq.i_data = data;
    ireq.i_len = len;

    if ( ioctl( m_fd, SIOCG80211, &ireq ) < 0 )
        return -1;

    *plen = ireq.i_len;
    return 0;
}

int BSDBackend::get80211val( const QString &ifName, int type, int *val )
{
    struct ieee80211req ireq;

    memset( &ireq, 0, sizeof( ireq ) );
    strncpy( ireq.i_name, ifName.toLatin1(), sizeof( ireq.i_name ) );
    ireq.i_type = type;

    if ( ioctl( m_fd, SIOCG80211, &ireq ) < 0 )
        return -1;

    *val = ireq.i_val;
    return 0;
}

enum ieee80211_opmode BSDBackend::get80211opmode( const QString &ifName )
{
    struct ifmediareq ifmr;

    memset( &ifmr, 0, sizeof( ifmr ) );
    strncpy( ifmr.ifm_name, ifName.toLatin1(), sizeof( ifmr.ifm_name ) );

    if ( ioctl( m_fd, SIOCGIFMEDIA, &ifmr ) >= 0 )
    {
        if ( ifmr.ifm_current & IFM_IEEE80211_ADHOC )
        {
            if ( ifmr.ifm_current & IFM_FLAG0 )
                return IEEE80211_M_AHDEMO;
            else
                return IEEE80211_M_IBSS;
        }
        if ( ifmr.ifm_current & IFM_IEEE80211_HOSTAP )
            return IEEE80211_M_HOSTAP;
        if ( ifmr.ifm_current & IFM_IEEE80211_MONITOR )
            return IEEE80211_M_MONITOR;
        if ( ifmr.ifm_current & IFM_IEEE80211_MBSS )
            return IEEE80211_M_MBSS;
    }
    return IEEE80211_M_STA;
}



#include "bsdbackend.moc"
