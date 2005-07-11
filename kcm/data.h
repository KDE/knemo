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

#include <qcolor.h>
#include <qstring.h>
#include <qvaluevector.h>

#include <klocale.h>
#include <kio/global.h>

/**
 * This file contains data structures used to store the settings for
 * an interface and the application.
 *
 * @short Store interface and application settings
 * @author Percy Leonhardt <percy@eris23.de>
 */

struct InterfaceCommand
{
    int id;
    bool runAsRoot;
    QString command;
    QString menuText;
};

struct InterfaceSettings
{
    InterfaceSettings()
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

#endif // DATA_H
