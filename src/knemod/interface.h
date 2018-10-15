/* This file is part of KNemo
   Copyright (C) 2004, 2006 Percy Leonhardt <percy@eris23.de>
   Copyright (C) 2009, 2010 John Stamp <jstamp@users.sourceforge.net>

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

#ifndef INTERFACE_H
#define INTERFACE_H

#include <time.h>
#include "data.h"

class InterfacePlotterDialog;
class InterfaceStatistics;
class InterfaceStatusDialog;
class InterfaceStatisticsDialog;
class TrayIcon;

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
    Interface(const QString& ifname,
              const BackendData * const );

    /**
     * Default Destructor
     */
    virtual ~Interface();

    int ifaceState()
    {
        return mIfaceState;
    }

    int previousIfaceState()
    {
        return mPreviousIfaceState;
    }

    QString uptimeString()
    {
        return mUptimeString;
    }

    const QString& ifaceName() const
    {
        return mIfaceName;
    }

    const BackendData* backendData() const
    {
        return mBackendData;
    }

    InterfaceSettings& settings()
    {
        return mSettings;
    }

    InterfaceStatistics* ifaceStatistics()
    {
        return mIfaceStatistics;
    }

    unsigned long rxRate()
    {
        return mRxRate;
    }

    unsigned long txRate()
    {
        return mTxRate;
    }

    QString rxRateStr()
    {
        return mRxRateStr;
    }

    QString txRateStr()
    {
        return mTxRateStr;
    }

    bool plotterVisible();

    /**
     * Called from reparseConfiguration() when the user changed
     * the settings.
     */
    void configChanged();

public Q_SLOTS:
    /**
     * Called when the backend emits the updateComplete signal.
     * This looks for changes in interface data or state.
     */
    void processUpdate();

    /*
     * Called when the user left-clicks on the tray icon
     * Toggles the status dialog by showing it on the first click and
     * hiding it on the second click.
     */
    void showStatusDialog( bool onlyActivate = true );

    /*
     * Called when the user middle-clicks on the tray icon
     * Toggles the signal plotter that displays the incoming and
     * outgoing traffic.
     */
    void showSignalPlotter( bool onlyActivate = true );

    void toggleSignalPlotter( bool show );

    /*
     * Called when the user selects the appropriate entry in the context menu.
     */
    void showStatisticsDialog();

private Q_SLOTS:
    /**
     * Emit a notification when traffic exceeds a threshold
     */
    void warnTraffic( QString text, quint64 threshold, quint64 current );

private:
    /**
     * Start the statistics and load previously saved ones
     */
    void startStatistics();

    /**
     * Store the statistics and stop collecting any further data
     */
    void stopStatistics();

    /**
     * The following function is taken from ksystemtray.cpp for
     * correct show, raise, focus and hide of status dialog and
     * signal plotter.
     */
    void activateOrHide( QWidget* widget, bool onlyActivate = false );

    void updateTime();

    void resetUptime();

    int mIfaceState;
    int mPreviousIfaceState;
    QString mIfaceName;
    qreal mRealSec;
    time_t mUptime;
    QString mUptimeString;
    unsigned long mRxRate;
    unsigned long mTxRate;
    QString mRxRateStr;
    QString mTxRateStr;
    TrayIcon *mIcon;
    InterfaceSettings mSettings;
    InterfaceStatistics* mIfaceStatistics;
    InterfaceStatusDialog* mStatusDialog;
    InterfaceStatisticsDialog* mStatisticsDialog;
    InterfacePlotterDialog* mPlotterDialog;
    const BackendData* mBackendData;
};

#endif // INTERFACE_H
