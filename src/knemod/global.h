/* This file is part of KNemo
   Copyright (C) 2005 Percy Leonhardt <percy@eris23.de>
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

#ifndef GLOBAL_H
#define GLOBAL_H

#include <QColor>
#include <QString>

#include <KUrl>

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
    KUrl statisticsDir;
};

struct InterfaceData
{
    InterfaceData()
      : existing( false ),
        available( false ),
        wirelessDevice( false ),
        prevRxPackets( 0L ),
        prevTxPackets( 0L ),
        rxPackets( 0L ),
        txPackets( 0L ),
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
    unsigned long prevRxPackets;
    unsigned long prevTxPackets;
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
    quint64 rxBytes;
    quint64 txBytes;
};

struct WirelessData
{
    QString essid;
    QString mode;
    QString frequency;
    QString channel;
    QString bitRate;
    QString linkQuality;
    QString accessPoint;
    QString prevAccessPoint;
    QString nickName;
    bool encryption;
};

struct PlotterSettings
{
    int pixel;
    int distance;
    int fontSize;
    int minimumValue;
    int maximumValue;
    bool labels;
    bool bottomBar;
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
    quint64 rxBytes;
    quint64 txBytes;
};

#endif // GLOBAL_H
