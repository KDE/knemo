/* This file is part of KNemo
   Copyright (C) 2006 Percy Leonhardt <percy@eris23.de>

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

#include <stdio.h>
#include <iwlib.h>
//#include <net/if.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <qmap.h>
#include <qdir.h>
#include <qregexp.h>
#include <qstringlist.h>

#include <kdebug.h>
#include <kprocess.h>
#include <kio/global.h>

#include "sysbackend.h"

#include "config.h"

#define SYSPATH "/sys/class/net/"
#define PROCROUTE "/proc/net/route"

SysBackend::SysBackend( QDict<Interface>& interfaces )
    : BackendBase( interfaces )
{
}

SysBackend::~SysBackend()
{
}

BackendBase* SysBackend::createInstance( QDict<Interface>& interfaces )
{
    return new SysBackend( interfaces );
}

void SysBackend::update()
{
    QDir dir( SYSPATH );
    QStringList ifList = dir.entryList( QDir::Dirs );

    QDictIterator<Interface> ifIt( mInterfaces );
    for ( ; ifIt.current(); ++ifIt )
    {
        QString key = ifIt.currentKey();
        Interface* interface = ifIt.current();

        if ( ifList.find( key ) == ifList.end() )
        {
            // The interface does not exist. Meaning the driver
            // isn't loaded and/or the interface has not been created.
            interface->getData().existing = false;
            interface->getData().available = false;
        }
        else
        {
            if ( QFile::exists( SYSPATH + key + "/wireless" ) )
            {
                interface->getData().wirelessDevice = true;
            }

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

                if ( interface->getData().wirelessDevice == true )
                {
                    updateWirelessData( key, interface->getWirelessData() );
                }
            }
        }
    }
    updateComplete();
}

bool SysBackend::readNumberFromFile( const QString& fileName, unsigned int& value )
{
    FILE* file = fopen( fileName.latin1(), "r" );
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
    FILE* file = fopen( fileName.latin1(), "r" );
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
    }

    // for the default gateway we use the proc filesystem
    QFile routeFile( PROCROUTE );
    if ( routeFile.open( IO_ReadOnly ) )
    {
        QString routeData( routeFile.readAll().data() );
        QRegExp regExp( ".*\\s+[\\w\\d]{8}\\s+([\\w\\d]{8})\\s+0+3" );
        if ( regExp.search( routeData ) > -1 )
        {
            bool ok;
            struct in_addr in;
            in.s_addr = regExp.cap( 1 ).toULong( &ok, 16 );
            data.defaultGateway = inet_ntoa( in );
        }
    }

    // use ioctls for the rest
    int fd;
    struct ifreq ifr;
    if ( ( fd = socket(AF_INET, SOCK_DGRAM, 0) ) > -1 )
    {
        strcpy( ifr.ifr_name, ifName.latin1() );
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
    }
}

void SysBackend::updateWirelessData( const QString& ifName, WirelessData& data )
{
    QString wirelessFolder = SYSPATH + ifName + "/wireless/";

    unsigned int link = 0;
    if ( readNumberFromFile( wirelessFolder + "link", link ) )
    {
        data.linkQuality = QString::number( link );
    }

    // The following code was taken from iwconfig.c and iwlib.c.
    int fd;
    if ( ( fd = iw_sockets_open() ) > 0 )
    {
        struct iwreq wrq;
        char buffer[128];
        if ( iw_get_ext( fd, ifName.latin1(), SIOCGIWFREQ, &wrq ) >= 0 )
        {
            int channel = -1;
            double freq = iw_freq2float( &( wrq.u.freq ) );
            struct iw_range range;
            if( iw_get_range_info( fd, ifName.latin1(), &range ) >= 0 )
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
        if ( iw_get_ext( fd, ifName.latin1(), SIOCGIWESSID, &wrq ) >= 0 )
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

        if ( iw_get_ext( fd, ifName.latin1(), SIOCGIWAP, &wrq ) >= 0 )
        {
            char ap_addr[128];
            iw_ether_ntop( (const ether_addr*) wrq.u.ap_addr.sa_data, ap_addr);
            data.accessPoint = ap_addr;
        }

        memset( essid, 0, IW_ESSID_MAX_SIZE + 1 );
        wrq.u.essid.pointer = (caddr_t) essid;
        wrq.u.essid.length = IW_ESSID_MAX_SIZE + 1;
        wrq.u.essid.flags = 0;
        if ( iw_get_ext( fd, ifName.latin1(), SIOCGIWNICKN, &wrq ) >= 0 )
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

        if ( iw_get_ext( fd, ifName.latin1(), SIOCGIWRATE, &wrq ) >= 0 )
        {
            iwparam bitrate;
            memcpy (&(bitrate), &(wrq.u.bitrate), sizeof (iwparam));
            iw_print_bitrate( buffer, sizeof( buffer ), wrq.u.bitrate.value );
            data.bitRate = buffer;
        }

        if ( iw_get_ext( fd, ifName.latin1(), SIOCGIWMODE, &wrq ) >= 0 )
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

        unsigned char key[IW_ENCODING_TOKEN_MAX];
        wrq.u.data.pointer = (caddr_t) key;
        wrq.u.data.length = IW_ENCODING_TOKEN_MAX;
        wrq.u.data.flags = 0;
        if ( iw_get_ext( fd, ifName.latin1(), SIOCGIWENCODE, &wrq ) >= 0 )
        {
            if ( ( wrq.u.data.flags & IW_ENCODE_DISABLED ) || ( wrq.u.data.length == 0 ) )
            {
                data.encryption = false;
            }
            else
            {
                data.encryption = true;
            }
        }
        else
        {
            data.encryption = false;
        }
        close( fd );
    }
}

