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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef DATA_H
#define DATA_H

#include <qstring.h>
#include <qvaluevector.h>

/**
 * This file contains data structures used to store information about
 * an interface. It is shared between the daemon and the control center
 * module.
 *
 * @short Shared data structures
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
	bool activateStatistics;
    bool customCommands;
    QString alias;
    QValueVector<InterfaceCommand> commands;
};

enum ToolTipEnums
{
    INTERFACE        = 0x00000001,
    ALIAS            = 0x00000002,
    STATUS           = 0x00000004,
    UPTIME           = 0x00000008,
    IP_ADDRESS       = 0x00000010,
    SUBNET_MASK      = 0x00000020,
    HW_ADDRESS       = 0x00000040,
    PTP_ADDRESS      = 0x00000080,
    RX_PACKETS       = 0x00000100,
    TX_PACKETS       = 0x00000200,
    RX_BYTES         = 0x00000400,
    TX_BYTES         = 0x00000800,
    ESSID            = 0x00001000,
    MODE             = 0x00002000,
    FREQUENCY        = 0x00004000,
    BIT_RATE         = 0x00008000,
    SIGNAL_NOISE     = 0x00010000,
    LINK_QUALITY     = 0x00020000,
    BCAST_ADDRESS    = 0x00040000,
    GATEWAY          = 0x00080000,
    DOWNLOAD_SPEED   = 0x00100000,
    UPLOAD_SPEED     = 0x00200000
};

#endif // DATA_H
