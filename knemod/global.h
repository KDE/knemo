/* This file is part of KNemo
   Copyright (C) 2005 Percy Leonhardt <percy@eris23.de>

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

#ifndef GLOBAL_H
#define GLOBAL_H

#include <qpair.h>
#include <qcolor.h>
#include <qstring.h>

/**
 * This file contains data structures and enums used in the knemo daemon.
 *
 * @short Daemon wide structures and enums
 * @author Percy Leonhardt <percy@eris23.de>
 */

struct GeneralData
{
    int toolTipContent;
    int pollInterval;
    int saveInterval;
    QString statisticsDir;
};

struct InterfaceData
{
    InterfaceData()
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
    Q_UINT64 rxBytes;
    Q_UINT64 txBytes;
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

struct StatisticEntry
{
    int day;
    int month;
    int year;
    Q_UINT64 rxBytes;
    Q_UINT64 txBytes;
};

extern QPair<QString, int> ToolTips[];

#endif // GLOBAL_H
