/* This file is part of KNemo
   Copyright (C) 2004 Percy Leonhardt <percy@eris23.de>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef DATA_H
#define DATA_H

#include <qpair.h>
#include <qcolor.h>
#include <qstring.h>
#include <qvaluevector.h>

#include <klocale.h>
#include <kio/global.h>

/**
 * This file contains data structures used to store information about
 * an interface, the wireless data and the user settings for an interface.
 *
 * @short Store interface information
 * @author Percy Leonhardt <percy@eris23.de>
 */

struct GeneralData
{
    int toolTipContent;
};

struct InterfaceData
{
    InterfaceData::InterfaceData()
      : existing( false ),
        available( false ),
        wirelessDevice( false ),
        prevRxBytes( 0L ),
        prevTxBytes( 0L ),
        incomingBytes( 0L ),
        outgoingBytes( 0L ),
        rxBytes( 0L ),
        txBytes( 0L )
    {}

    bool existing;
    bool available;
    bool wirelessDevice;
    unsigned long rxPackets;
    unsigned long txPackets;
    unsigned long prevRxBytes;
    unsigned long prevTxBytes;
    unsigned long incomingBytes;
    unsigned long outgoingBytes;
    QString ipAddress;
    QString subnetMask;
    QString hwAddress;
    QString ptpAddress;
    QString broadcastAddress;
    QString defaultGateway;
    QString rxString;
    QString txString;
    KIO::filesize_t rxBytes;
    KIO::filesize_t txBytes;
};

struct WirelessData
{
    QString essid;
    QString mode;
    QString frequency;
    QString channel;
    QString bitRate;
    QString signal;
    QString noise;
    QString linkQuality;
};

struct InterfaceCommand
{
    int id;
    bool runAsRoot;
    QString command;
    QString menuText;
};

struct InterfaceSettings
{
    InterfaceSettings::InterfaceSettings()
      : iconSet( 0 ),
        numCommands( 0 ),
        toolTipContent( 2 ),
        trafficThreshold( 0 ),
        hideWhenNotExisting( false ),
        hideWhenNotAvailable( false ),
        customCommands( false )
    {}

    int iconSet;
    int numCommands;
    int toolTipContent;
    int trafficThreshold;
    bool hideWhenNotExisting;
    bool hideWhenNotAvailable;
    bool customCommands;
    QString alias;
    QValueVector<InterfaceCommand> commands;
};

struct PlotterSettings
{
    int pixel;
    int count;
    int distance;
    int fontSize;
    int minimumValue;
    int maximumValue;
    bool labels;
    bool topBar;
    bool showIncoming;
    bool showOutgoing;
    bool verticalLines;
    bool horizontalLines;
    bool automaticDetection;
    bool verticalLinesScroll;
    QColor colorVLines;
    QColor colorHLines;
    QColor colorIncoming;
    QColor colorOutgoing;
    QColor colorBackground;
};

enum ToolTipEnums
{
    INTERFACE = 1,
    ALIAS = 2,
    STATUS = 4,
    UPTIME = 8,
    IP_ADDRESS = 16,
    SUBNET_MASK = 32,
    HW_ADDRESS = 64,
    PTP_ADDRESS = 128,
    RX_PACKETS = 256,
    TX_PACKETS = 512,
    RX_BYTES = 1024,
    TX_BYTES = 2048,
    ESSID = 4096,
    MODE = 8192,
    FREQUENCY = 16384,
    BIT_RATE = 32768,
    SIGNAL_NOISE = 65536,
    LINK_QUALITY = 131072,
    BCAST_ADDRESS = 262144,
    GATEWAY = 524288,
    DOWNLOAD_SPEED = 1048576,
    UPLOAD_SPEED = 2097152
};

extern QPair<QString, int> ToolTips[];

#endif // DATA_H
