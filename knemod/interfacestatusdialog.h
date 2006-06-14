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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef INTERFACESTATUSDIALOG_H
#define INTERFACESTATUSDIALOG_H

#include <qwidget.h>

#include "interfacestatusdlg.h"

class QTimer;
class Interface;

/**
 * This class serves as the main window for KNemo.  It handles the
 * menus, toolbars, and status bars.
 *
 * @short Main window class
 * @author Percy Leonhardt <percy@eris23.de>
 */

class InterfaceStatusDialog : public InterfaceStatusDlg
{
    Q_OBJECT
public:
    /**
     * Default Constructor
     */
    InterfaceStatusDialog( Interface* interface,
                           QWidget* parent = 0L, const char* name = 0L );

    /**
     * Default Destructor
     */
    virtual ~InterfaceStatusDialog();

    /**
     * Implemented to store position.
     */
    void hide();
    void show();

    /**
     * Show the statistics tab when the user turned on statistics
     */
    void showStatisticsTab();
    /**
     * Hide the statistics tab when the user turned off statistics
     */
    void hideStatisticsTab();

public slots:
    /**
     * Enable the network tabs when the interface is connected
     */
    void enableNetworkTabs( int );
    /**
     * Disable the network tabs when the interface is not connected
     */
    void disableNetworkTabs( int );
    /**
     * Update the statistics tab when data changed
     */
    void statisticsChanged();

private slots:
    void updateDialog();

private:
    QPoint mPos;
    bool mPosInitialized;

    QTimer* mTimer;
    Interface* mInterface;

    // The position of the statistics tab depends on the wireless state
    int statisticsTabPos;
    // We remember the statistics tab to add it at a later time
    QWidget* mStatisticsTab;
};

#endif // INTERFACESTATUSDIALOG_H
