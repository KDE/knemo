/* This file is part of KNemo
   Copyright (C) 2004, 2006 Percy Leonhardt <percy@eris23.de>

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

#ifndef INTERFACE_H
#define INTERFACE_H

#include <qobject.h>
#include <qstring.h>
#include <qdatetime.h>

#include "data.h"
#include "global.h"
#include "interfaceicon.h"
#include "interfacemonitor.h"

class QTimer;
class SignalPlotter;
class InterfaceStatistics;
class InterfaceStatusDialog;
class InterfaceStatisticsDialog;

/**
 * This class is the central place for all things that belong to an
 * interface. It stores some information and knows about the interface
 * data, icon, monitor and settings.
 *
 * @short Central class for every interface
 * @author Percy Leonhardt <percy@eris23.de>
 */
class Interface : public QObject
{
    Q_OBJECT
public:
    /**
     * Default Constructor
     */
    Interface(QString ifname, const PlotterSettings& plotterSettings );

    /**
     * Default Destructor
     */
    virtual ~Interface();

    void setType( int type )
    {
        mType = type;
    }

    int getType()
    {
        return mType;
    }

    void setState( int state )
    {
        mState = state;
    }

    int getState()
    {
        return mState;
    }

    QDateTime getStartTime() const
    {
        return mStartTime;
    }

    QString getName() const
    {
        return mName;
    }

    InterfaceData& getData()
    {
        return mData;
    }

    InterfaceSettings& getSettings()
    {
        return mSettings;
    }

    WirelessData& getWirelessData()
    {
        return mWirelessData;
    }

    InterfaceStatistics* getStatistics()
    {
        return mStatistics;
    }

    /**
     * Called from reparseConfiguration() when the user changed
     * the settings.
     */
    void configChanged();

    /**
     * Called from the interface updater class after new data from
     * 'ifconfig' has been read. This will trigger the monitor to
     * to look for changes in interface data or interface state.
     */
    void activateMonitor();

    enum InterfaceState
    {
        UNKNOWN_STATE = -1,
        NOT_EXISTING  = 0,
        NOT_AVAILABLE = 1,
        AVAILABLE     = 2,
        RX_TRAFFIC    = 4,
        TX_TRAFFIC    = 8
    };

    enum InterfaceType
    {
        UNKNOWN_TYPE,
        ETHERNET,
        PPP
    };

    enum IconSet
    {
        MONITOR = 0,
        MODEM,
        NETWORK,
        WIRELESS
    };

public slots:
    /*
     * Called when the user left-clicks on the tray icon
     * Toggles the status dialog by showing it on the first click and
     * hiding it on the second click.
     */
    void showStatusDialog();

    /*
     * Called when the user middle-clicks on the tray icon
     * Toggles the signal plotter that displays the incoming and
     * outgoing traffic.
     */
    void showSignalPlotter( bool wasMiddleButton );

    /*
     * Called when the user selects the appropriate entry in the context menu.
     */
    void showStatisticsDialog();

private slots:
    /**
     * Start the uptimer when the interface is connected
     */
    void setStartTime( int );

    /**
     * Update the signal plotter with new data
     */
    void updatePlotter();

    /**
     * Configure the signal plotter with user settings
     */
    void configurePlotter();

private:
    /**
     * Start the statistics and load previously saved ones
     */
    void startStatistics();

    /**
     * Store the statistics and stop collecting any further data
     */
    void stopStatistics();

    enum VisibleBeams
    {
        NONE = 0,
        INCOMING_TRAFFIC = 1,
        OUTGOING_TRAFFIC = 2,
        BOTH = 3
    };

    int mType;
    int mState;
    int mOutgoingPos;
    int mIncomingPos;
    QString mName;
    QTimer* mPlotterTimer;
    QDateTime mStartTime;
    InterfaceIcon mIcon;
    InterfaceData mData;
    InterfaceMonitor mMonitor;
    InterfaceSettings mSettings;
    InterfaceStatistics* mStatistics;
    WirelessData mWirelessData;
    InterfaceStatusDialog* mStatusDialog;
    InterfaceStatisticsDialog* mStatisticsDialog;
    SignalPlotter* mPlotter;
    VisibleBeams mVisibleBeams;
    const PlotterSettings& mPlotterSettings;
};

#endif // INTERFACE_H
