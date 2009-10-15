/* This file is part of KNemo
   Copyright (C) 2004, 2006 Percy Leonhardt <percy@eris23.de>
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

#ifndef INTERFACESTATUSDIALOG_H
#define INTERFACESTATUSDIALOG_H

#include <KDialog>

#include "ui_interfacestatusdlg.h"

class Interface;

/**
 * This class serves as the main window for KNemo.  It handles the
 * menus, toolbars, and status bars.
 *
 * @short Main window class
 * @author Percy Leonhardt <percy@eris23.de>
 */

class InterfaceStatusDialog : public KDialog
{
    Q_OBJECT
public:
    /**
     * Default Constructor
     */
    InterfaceStatusDialog( Interface* interface,
                           QWidget* parent = 0L );

    /**
     * Default Destructor
     */
    virtual ~InterfaceStatusDialog();

    /**
     * Enable or disable the statistics group when the user turned statistics on or off.
     */
    void setStatisticsGroupEnabled( bool enabled );

public slots:
    /**
     * Enable the network groups when the interface is connected
     */
    void enableNetworkGroups();
    /**
     * Disable the network groups when the interface is not connected
     */
    void disableNetworkGroups();
    /**
     * Update the statistics tab when data changed
     */
    void statisticsChanged();
    void updateDialog();

protected:
    bool event( QEvent *e );

private:
    Ui::InterfaceStatusDlg ui;
    bool mSetPos;

    KSharedConfigPtr mConfig;
    Interface* mInterface;
};

#endif // INTERFACESTATUSDIALOG_H
