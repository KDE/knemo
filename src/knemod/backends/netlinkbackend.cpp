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

#include <net/if.h>

#include <KLocale>
#include <kio/global.h>

#include "config-knemo.h"
#include "utils.h"
#include "netlinkbackend.h"

NetlinkBackend::NetlinkBackend()
    : rtsock( NULL ),
      addrCache( NULL ),
      linkCache( NULL ),
      routeCache( NULL ),
      iwfd( -1 )
{
    rtsock = nl_handle_alloc();
    int c = nl_connect(rtsock, NETLINK_ROUTE);
    if ( c >= 0 )
    {
        addrCache = rtnl_addr_alloc_cache( rtsock );
        linkCache = rtnl_link_alloc_cache( rtsock );
        routeCache = rtnl_route_alloc_cache( rtsock );
    }
#ifdef HAVE_LIBIW
    iwfd = iw_sockets_open();
#endif
}

NetlinkBackend::~NetlinkBackend()
{
    nl_cache_free( addrCache );
    nl_cache_free( linkCache );
    nl_cache_free( routeCache );
    nl_close( rtsock );
    nl_handle_destroy( rtsock );
#ifdef HAVE_LIBIW
    if ( iwfd > 0 )
        close( iwfd );
#endif
}

QStringList NetlinkBackend::getIfaceList()
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
        updateInterfaceData( key, interface );

#ifdef HAVE_LIBIW
        if ( iwfd > 0 )
        {
            struct wireless_config wc;
            if ( iw_get_basic_config( iwfd, key.toLatin1(), &wc ) >= 0 )
            {
                interface->isWireless = true;
                updateWirelessData( iwfd, key, interface );
            }
        }
#endif
    }
    emit updateComplete();
}

QString NetlinkBackend::getDefaultRouteIface( int afInet )
{
    return getDefaultRoute( afInet, NULL, routeCache );
}

BackendBase* NetlinkBackend::createInstance()
{
    return new NetlinkBackend();
}

void NetlinkBackend::updateAddresses( struct rtnl_link* link, BackendData *data )
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

        // I don't think a link-local address should count as "connected" to
        // a network.  Yell if I'm wrong.
        if ( rtnl_link_get_flags( link ) & IFF_RUNNING &&
             addrVal.scope != RT_SCOPE_LINK &&
             addrVal.scope != RT_SCOPE_NOWHERE )
        {
            data->status |= KNemoIface::Connected;
        }

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
            strFlags += i18n( " homeaddress" );
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

void NetlinkBackend::updateInterfaceData( const QString& ifName, BackendData* data )
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
        data->status = KNemoIface::Available;
        if ( flags & IFF_UP )
            data->status |= KNemoIface::Up;

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

        updateAddresses( link, data );

        rtnl_link_put( link );
    }
    else
        data->status = KNemoIface::Unavailable;
}

#ifdef HAVE_LIBIW
void NetlinkBackend::updateWirelessData( int fd, const QString& ifName, BackendData* data )
{
    if ( fd > 0 )
    {
    // The following code was taken from iwconfig.c and iwlib.c.
        struct iwreq wrq;
        char buffer[ 128 ];
        struct iw_range range;
        bool has_range = ( iw_get_range_info( fd, ifName.toLatin1(), &range ) >= 0 );

        struct wireless_info info;
        if ( iw_get_stats( fd, ifName.toLatin1(), &(info.stats), 0, 0 ) >= 0 )
        {
            if ( has_range )
            {
                if ( range.max_qual.qual > 0 )
                    data->linkQuality = QString( "%1%" ).arg( 100 * info.stats.qual.qual / range.max_qual.qual );
                else
                    data->linkQuality = "0";
            }
            else
                data->linkQuality = QString::number( info.stats.qual.qual );
        }

        if ( iw_get_ext( fd, ifName.toLatin1(), SIOCGIWFREQ, &wrq ) >= 0 )
        {
            int channel = -1;
            double freq = iw_freq2float( &( wrq.u.freq ) );
            if( has_range )
            {
                if ( freq < 1e3 )
                {
                    channel = iw_channel_to_freq( static_cast<int>(freq), &freq, &range );
                }
                else
                {
                    channel = iw_freq_to_channel( freq, &range );
                }
                iw_print_freq_value( buffer, sizeof( buffer ), freq );
                data->frequency = buffer;
                data->channel = QString::number( channel );
            }
        }

        char essid[IW_ESSID_MAX_SIZE + 1];
        memset( essid, 0, IW_ESSID_MAX_SIZE + 1 );
        wrq.u.essid.pointer = static_cast<caddr_t>(essid);
        wrq.u.essid.length = IW_ESSID_MAX_SIZE + 1;
        wrq.u.essid.flags = 0;
        if ( iw_get_ext( fd, ifName.toLatin1(), SIOCGIWESSID, &wrq ) >= 0 )
        {
            if ( wrq.u.data.flags > 0 )
            {
                data->essid = essid;
            }
            else
            {
                data->essid = "any";
            }
        }

        if ( iw_get_ext( fd, ifName.toLatin1(), SIOCGIWAP, &wrq ) >= 0 )
        {
            char ap_addr[128];
            iw_ether_ntop( reinterpret_cast<const ether_addr *>(wrq.u.ap_addr.sa_data), ap_addr );
            data->accessPoint = ap_addr;
        }
        else
            data->accessPoint.clear();

        memset( essid, 0, IW_ESSID_MAX_SIZE + 1 );
        wrq.u.essid.pointer = static_cast<caddr_t>(essid);
        wrq.u.essid.length = IW_ESSID_MAX_SIZE + 1;
        wrq.u.essid.flags = 0;
        if ( iw_get_ext( fd, ifName.toLatin1(), SIOCGIWNICKN, &wrq ) >= 0 )
        {
            if ( wrq.u.data.length > 1 )
            {
                data->nickName = essid;
            }
            else
            {
                data->nickName = QString::null;
            }
        }

        if ( iw_get_ext( fd, ifName.toLatin1(), SIOCGIWRATE, &wrq ) >= 0 )
        {
            iwparam bitrate;
            memcpy (&(bitrate), &(wrq.u.bitrate), sizeof (iwparam));
            iw_print_bitrate( buffer, sizeof( buffer ), wrq.u.bitrate.value );
            data->bitRate = buffer;
        }

        if ( iw_get_ext( fd, ifName.toLatin1(), SIOCGIWMODE, &wrq ) >= 0 )
        {
            int mode = wrq.u.mode;
            if ( mode < IW_NUM_OPER_MODE && mode >= 0 )
            {
                data->mode = iw_operation_mode[mode];
            }
            else
            {
                data->mode = QString::null;
            }
        }

        if ( data->accessPoint != data->prevAccessPoint )
        {
            /* Reset encryption status for new access point */
            data->isEncrypted = false;
            data->prevAccessPoint = data->accessPoint;
        }
        if ( has_range )
            updateWirelessEncData( fd, ifName, range, data );
    }
}

void NetlinkBackend::updateWirelessEncData( int fd, const QString& ifName,
                                        const iw_range& range, BackendData* data )
{
    /* We only use left-over wireless scans to prevent doing a new scan every
     * polling period.  If our current access point disappears from the results
     * then updateWirelessEncData will use the last encryption status until the
     * results are updated again. */
    struct iwreq wrq;
    unsigned char * buffer = NULL;
    unsigned char * newbuf;
    int buflen = IW_SCAN_MAX_DATA; /* Min for compat WE<17 */

    // The following code was taken from iwlist.c with some small changes
realloc:
    /* (Re)allocate the buffer - realloc(NULL, len) == malloc(len) */
    newbuf = reinterpret_cast<unsigned char *>(realloc( buffer, buflen ));
    if ( newbuf == NULL )
    {
        if ( buffer )
            free( buffer );
        return;
    }
    buffer = newbuf;

    /* Try to read the results */
    wrq.u.data.pointer = buffer;
    wrq.u.data.flags = 0;
    wrq.u.data.length = buflen;
    if ( iw_get_ext( fd, ifName.toLatin1(), SIOCGIWSCAN, &wrq ) < 0 )
    {
        /* Check if buffer was too small (WE-17 only) */
        if( (errno == E2BIG) && (range.we_version_compiled > 16) )
        {
            /* Some driver may return very large scan results, either
             * because there are many cells, or because they have many
             * large elements in cells (like IWEVCUSTOM). Most will
             * only need the regular sized buffer. We now use a dynamic
             * allocation of the buffer to satisfy everybody. Of course,
             * as we don't know in advance the size of the array, we try
             * various increasing sizes. Jean II */

            /* Check if the driver gave us any hints. */
            if( wrq.u.data.length > buflen )
                buflen = wrq.u.data.length;
            else
                buflen *= 2;

            /* Try again */
            goto realloc;
        }

        /* EAGAIN or bad error
         * Try again on next poll tick */
        free( buffer );
        return;
    }

    if ( wrq.u.data.length )
    {
        struct iw_event iwe;
        struct stream_descr stream;
        int ret;
        bool foundAP = false;

        iw_init_event_stream( &stream, reinterpret_cast<char *>(buffer), wrq.u.data.length );
        do
        {
            /* Extract the event and process it */
            ret = iw_extract_event_stream( &stream, &iwe, range.we_version_compiled );
            if (ret > 0 )
            {
                switch ( iwe.cmd )
                {
                    case SIOCGIWAP:
                        if ( data->accessPoint == iw_sawap_ntop( &iwe.u.ap_addr, reinterpret_cast<char *>(buffer) ) )
                            foundAP = true;
                        break;
                    case SIOCGIWENCODE:
                        if ( foundAP )
                        {
                            if ( iwe.u.data.flags & IW_ENCODE_DISABLED )
                                data->isEncrypted = false;
                            else
                                data->isEncrypted = true;
                            free( buffer );
                            return;
                        }
                        break;
                }
            }
        }
        while ( ret > 0 );
    }
    free( buffer );
}
#endif

