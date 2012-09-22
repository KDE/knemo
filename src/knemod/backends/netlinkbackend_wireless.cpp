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

#include "netlinkbackend_wireless.h"

#include <iwlib.h>
#include <QString>

void updateWirelessEncData( int fd, const QString& ifName,
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

void updateWirelessData( int fd, const QString& ifName, BackendData* data )
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

NetlinkBackend_Wireless::NetlinkBackend_Wireless()
    : iwfd( -1 )
{
}

void NetlinkBackend_Wireless::openSocket()
{
    iwfd = iw_sockets_open();
}

void NetlinkBackend_Wireless::closeSocket()
{
    if ( iwfd > 0 )
        close( iwfd );
}

void NetlinkBackend_Wireless::update(QString key, BackendData *interface )
{
    if ( iwfd > 0 )
    {
        struct wireless_config wc;
        if ( iw_get_basic_config( iwfd, key.toLatin1(), &wc ) >= 0 )
        {
            interface->isWireless = true;
            updateWirelessData( iwfd, key, interface );
        }
    }
}
