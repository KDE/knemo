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

#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <QDir>

#include <kio/global.h>

#include "config-knemo.h"
#include "sysbackend.h"

#ifndef HAVE_LIBIW
#include <net/if.h>
#endif

#define RTF_GATEWAY 0x0002
#define SYSPATH "/sys/class/net/"
#define PROCROUTE "/proc/net/route"

SysBackend::SysBackend( QHash<QString, Interface *>& interfaces )
    : BackendBase( interfaces )
{
}

SysBackend::~SysBackend()
{
}

BackendBase* SysBackend::createInstance( QHash<QString, Interface *>& interfaces )
{
    return new SysBackend( interfaces );
}

void SysBackend::update()
{
    QDir dir( SYSPATH );
    QStringList ifList = dir.entryList( QDir::Dirs );

    foreach ( QString key, mInterfaces.keys() )
    {
        Interface *interface = mInterfaces.value( key );
        if ( !ifList.contains( key ) )
        {
            // The interface does not exist. Meaning the driver
            // isn't loaded and/or the interface has not been created.
            interface->getData().existing = false;
            interface->getData().available = false;
        }
        else
        {
#ifdef HAVE_LIBIW
            int fd = iw_sockets_open();
            if ( fd > 0 )
            {
                struct wireless_config wc;
                if ( iw_get_basic_config( fd, key.toLatin1(), &wc ) >= 0 )
                    interface->getData().wirelessDevice = true;
            }
#endif

            unsigned int carrier = 0;
            if ( !readNumberFromFile( SYSPATH + key + "/carrier", carrier ) ||
                 carrier == 0 )
            {
                // The interface is there but not useable.
                interface->getData().existing = true;
                interface->getData().available = false;
            }
            else
            {
                // ...determine the type of the interface
                unsigned int type = 0;
                if ( readNumberFromFile( SYSPATH + key + "/type", type ) &&
                     type == 512 )
                {
                    interface->setType( Interface::PPP );
                }
                else
                {
                    interface->setType( Interface::ETHERNET );
                }

                // Update the interface.
                interface->getData().existing = true;
                interface->getData().available = true;
                updateInterfaceData( key, interface->getData(), interface->getType() );

#ifdef HAVE_LIBIW
                if ( interface->getData().wirelessDevice == true )
                {
                    updateWirelessData( fd, key, interface->getWirelessData() );
                }
            }
            if ( fd > 0 )
                close( fd );
#else
            }
#endif
        }
    }
    updateComplete();
}

bool SysBackend::readNumberFromFile( const QString& fileName, unsigned int& value )
{
    FILE* file = fopen( fileName.toLatin1(), "r" );
    if ( file != NULL )
    {
        if ( fscanf( file, "%ul", &value ) > 0 )
        {
            fclose( file );
            return true;
        }
        fclose( file );
    }

    return false;
}

bool SysBackend::readStringFromFile( const QString& fileName, QString& string )
{
    char buffer[64];
    FILE* file = fopen( fileName.toLatin1(), "r" );
    if ( file != NULL )
    {
        if ( fscanf( file, "%s", buffer ) > 0 )
        {
            fclose( file );
            string = buffer;
            return true;
        }
        fclose( file );
    }

    return false;
}

QString SysBackend::getDefaultRouteIface()
{
    QString iface;
    QFile proc( PROCROUTE );
    if ( proc.open( QIODevice::ReadOnly ) )
    {
        QString routeData( proc.readAll().data() );
        QStringList routes = routeData.split( "\n" );
        QStringListIterator it( routes );
        while ( it.hasNext() )
        {
            QStringList routeParameter = it.next().split( QRegExp("\\s"), QString::SkipEmptyParts );
            if ( routeParameter.count() < 2 )
                continue;
            if ( routeParameter[1] != "00000000" ) // no default route
                continue;
            if ( routeParameter[0] == "lo" )
                continue;
            iface = routeParameter[0]; // default route interface
            break;
        }
        proc.close();
    }
    return iface;
}


void SysBackend::updateInterfaceData( const QString& ifName, InterfaceData& data, int type )
{
    QString ifFolder = SYSPATH + ifName + "/";

    unsigned int rxPackets = 0;
    if ( readNumberFromFile( ifFolder + "statistics/rx_packets", rxPackets ) )
    {
        data.rxPackets = rxPackets;
    }

    unsigned int txPackets = 0;
    if ( readNumberFromFile( ifFolder + "statistics/tx_packets", txPackets ) )
    {
        data.txPackets = txPackets;
    }

    unsigned int rxBytes = 0;
    if ( readNumberFromFile( ifFolder + "statistics/rx_bytes", rxBytes ) )
    {
        // We count the traffic on ourself to avoid an overflow after
        // 4GB of traffic.
        if ( rxBytes < data.prevRxBytes )
        {
            // there was an overflow
            if ( type == Interface::ETHERNET )
            {
                // This makes data counting more accurate but will not work
                // for interfaces that reset the transfered data to zero
                // when deactivated like ppp does.
                data.rxBytes += 0xFFFFFFFF - data.prevRxBytes;
            }
            data.prevRxBytes = 0L;
        }
        if ( data.rxBytes == 0L )
        {
            // on startup set to currently received bytes
            data.rxBytes = rxBytes;
            // this is new: KNemo only counts the traffic transfered
            // while it is running. Important to not falsify statistics!
            data.prevRxBytes = rxBytes;
        }
        else
            // afterwards only add difference to previous number of bytes
            data.rxBytes += rxBytes - data.prevRxBytes;

        data.incomingBytes = rxBytes - data.prevRxBytes;
        data.prevRxBytes = rxBytes;
        data.rxString = KIO::convertSize( data.rxBytes );
    }

    unsigned int txBytes = 0;
    if ( readNumberFromFile( ifFolder + "statistics/tx_bytes", txBytes ) )
    {
        // We count the traffic on ourself to avoid an overflow after
        // 4GB of traffic.
        if ( txBytes < data.prevTxBytes )
        {
            // there was an overflow
            if ( type == Interface::ETHERNET )
            {
                // This makes data counting more accurate but will not work
                // for interfaces that reset the transfered data to zero
                // when deactivated like ppp does.
                data.txBytes += 0xFFFFFFFF - data.prevTxBytes;
            }
            data.prevTxBytes = 0L;
        }
        if ( data.txBytes == 0L )
        {
            // on startup set to currently received bytes
            data.txBytes = txBytes;
            // this is new: KNemo only counts the traffic transfered
            // while it is running. Important to not falsify statistics!
            data.prevTxBytes = txBytes;
        }
        else
            // afterwards only add difference to previous number of bytes
            data.txBytes += txBytes - data.prevTxBytes;

        data.outgoingBytes = txBytes - data.prevTxBytes;
        data.prevTxBytes = txBytes;
        data.txString = KIO::convertSize( data.txBytes );
    }

    if ( type == Interface::ETHERNET )
    {
        QString hwAddress;
        if ( readStringFromFile( ifFolder + "address", hwAddress ) )
        {
            data.hwAddress = hwAddress;
        }

        // for the default gateway we use the proc filesystem
        QFile routeFile( PROCROUTE );
        if ( routeFile.open( QIODevice::ReadOnly ) )
        {
            QString routeData( routeFile.readAll().data() );
            QStringList routeEntries = routeData.split( "\n" );
            QStringList::Iterator it;
            for ( it = routeEntries.begin(); it != routeEntries.end(); ++it )
            {
                QRegExp regExp( ".*\\s+[\\w\\d]{8}\\s+([\\w\\d]{8})\\s+(\\d{4})" );
                if (   ( regExp.indexIn( *it ) > -1 )
                    && ( regExp.cap( 2 ).toUInt() & RTF_GATEWAY ) )
                {
                    bool ok;
                    struct in_addr in;
                    in.s_addr = regExp.cap( 1 ).toULong( &ok, 16 );
                    data.defaultGateway = inet_ntoa( in );
                    break;
                }
            }
            routeFile.close();
        }

    }

    // use ioctls for the rest
    int fd;
    struct ifreq ifr;
    if ( ( fd = socket(AF_INET, SOCK_DGRAM, 0) ) > -1 )
    {
        strcpy( ifr.ifr_name, ifName.toLatin1() );
        ifr.ifr_addr.sa_family = AF_INET;
        if ( ioctl( fd, SIOCGIFADDR, &ifr ) > -1 )
        {
            data.ipAddress = inet_ntoa(((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr);
        }
        if ( ioctl( fd, SIOCGIFDSTADDR, &ifr) > -1 )
        {
            data.ptpAddress = inet_ntoa(((struct sockaddr_in*)&ifr.ifr_dstaddr)->sin_addr);
        }

        if ( ioctl( fd, SIOCGIFBRDADDR, &ifr ) > -1 )
        {
            data.broadcastAddress = inet_ntoa(((struct sockaddr_in*)&ifr.ifr_broadaddr)->sin_addr);
        }

        if ( ioctl( fd, SIOCGIFNETMASK, &ifr ) > -1 )
        {
            data.subnetMask = inet_ntoa(((struct sockaddr_in*)&ifr.ifr_netmask)->sin_addr);
        }
        close( fd );
    }
}

void SysBackend::updateWirelessData( int fd, const QString& ifName, WirelessData& data )
{
#ifdef HAVE_LIBIW
    if ( fd > 0 )
    {
    // The following code was taken from iwconfig.c and iwlib.c.
        struct iwreq wrq;
        char buffer[128];
        struct iw_range range;
        bool has_range = ( iw_get_range_info( fd, ifName.toLatin1(), &range ) >= 0 );

        struct wireless_info info;
        if ( iw_get_stats( fd, ifName.toLatin1(), &(info.stats), 0, 0 ) >= 0 )
        {
            if ( has_range )
            {
                if ( range.max_qual.qual > 0 )
                    data.linkQuality = QString( "%1%" ).arg( 100 * info.stats.qual.qual / range.max_qual.qual );
                else
                    data.linkQuality = "0";
            }
            else
                data.linkQuality = QString::number( info.stats.qual.qual );
        }

        if ( iw_get_ext( fd, ifName.toLatin1(), SIOCGIWFREQ, &wrq ) >= 0 )
        {
            int channel = -1;
            double freq = iw_freq2float( &( wrq.u.freq ) );
            if( has_range )
            {
                if ( freq < KILO )
                {
                    channel = iw_channel_to_freq( (int) freq, &freq, &range );
                }
                else
                {
                    channel = iw_freq_to_channel( freq, &range );
                }
                iw_print_freq_value( buffer, sizeof( buffer ), freq );
                data.frequency = buffer;
                data.channel = QString::number( channel );
            }
        }

        char essid[IW_ESSID_MAX_SIZE + 1];
        memset( essid, 0, IW_ESSID_MAX_SIZE + 1 );
        wrq.u.essid.pointer = (caddr_t) essid;
        wrq.u.essid.length = IW_ESSID_MAX_SIZE + 1;
        wrq.u.essid.flags = 0;
        if ( iw_get_ext( fd, ifName.toLatin1(), SIOCGIWESSID, &wrq ) >= 0 )
        {
            if ( wrq.u.data.flags > 0 )
            {
                data.essid = essid;
            }
            else
            {
                data.essid = "any";
            }
        }

        if ( iw_get_ext( fd, ifName.toLatin1(), SIOCGIWAP, &wrq ) >= 0 )
        {
            char ap_addr[128];
            iw_ether_ntop( (const ether_addr*) wrq.u.ap_addr.sa_data, ap_addr);
            data.accessPoint = ap_addr;
        }
        else
            data.accessPoint.clear();

        memset( essid, 0, IW_ESSID_MAX_SIZE + 1 );
        wrq.u.essid.pointer = (caddr_t) essid;
        wrq.u.essid.length = IW_ESSID_MAX_SIZE + 1;
        wrq.u.essid.flags = 0;
        if ( iw_get_ext( fd, ifName.toLatin1(), SIOCGIWNICKN, &wrq ) >= 0 )
        {
            if ( wrq.u.data.length > 1 )
            {
                data.nickName = essid;
            }
            else
            {
                data.nickName = QString::null;
            }
        }

        if ( iw_get_ext( fd, ifName.toLatin1(), SIOCGIWRATE, &wrq ) >= 0 )
        {
            iwparam bitrate;
            memcpy (&(bitrate), &(wrq.u.bitrate), sizeof (iwparam));
            iw_print_bitrate( buffer, sizeof( buffer ), wrq.u.bitrate.value );
            data.bitRate = buffer;
        }

        if ( iw_get_ext( fd, ifName.toLatin1(), SIOCGIWMODE, &wrq ) >= 0 )
        {
            int mode = wrq.u.mode;
            if ( mode < IW_NUM_OPER_MODE && mode >= 0 )
            {
                data.mode = iw_operation_mode[mode];
            }
            else
            {
                data.mode = QString::null;
            }
        }

        if ( data.accessPoint != data.prevAccessPoint )
        {
            /* Reset encryption status for new access point */
            data.encryption = false;
            data.prevAccessPoint = data.accessPoint;
        }
        if ( has_range )
            updateWirelessEncData( fd, ifName, range, data );
    }
#endif
}

#ifdef HAVE_LIBIW
void SysBackend::updateWirelessEncData( int fd, const QString& ifName,
                                        const iw_range& range, WirelessData& data )
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
    newbuf = (unsigned char *)realloc( buffer, buflen );
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

        iw_init_event_stream( &stream, (char *)buffer, wrq.u.data.length );
        do
        {
            /* Extract the event and process it */
            ret = iw_extract_event_stream( &stream, &iwe, range.we_version_compiled );
            if (ret > 0 )
            {
                switch ( iwe.cmd )
                {
                    case SIOCGIWAP:
                        if ( data.accessPoint == iw_saether_ntop( &iwe.u.ap_addr, (char *)buffer ) )
                            foundAP = true;
                        break;
                    case SIOCGIWENCODE:
                        if ( foundAP )
                        {
                            if ( iwe.u.data.flags & IW_ENCODE_DISABLED )
                                data.encryption = false;
                            else
                                data.encryption = true;
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
