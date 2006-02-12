/* This file is part of KNemo
   Copyright (C) 2004, 2005 Percy Leonhardt <percy@eris23.de>

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

#ifndef INTERFACEICON_H
#define INTERFACEICON_H

#include <qobject.h>
#include <qstring.h>

class Interface;
class InterfaceTray;

/**
 * This is the logical representation of the systemtray icon. It handles
 * creation and deletion of the real icon, setting the tooltip, setting
 * the correct icon image and displaying of the settings dialog.
 *
 * @short Logical representation of the systemtray icon
 * @author Percy Leonhardt <percy@eris23.de>
 */

class InterfaceIcon : public QObject
{
    Q_OBJECT
public:
    /**
     * Default Constructor
     */
    InterfaceIcon( Interface* interface );

    /**
     * Default Destructor
     */
    virtual ~InterfaceIcon();

    /*
     * Change the tooltip according to the alias of the interface
     */
    void updateToolTip();

    /*
     * Fill the context menu with entries if the user configured
     * start and stop command
     */
    void updateMenu();

signals:
    void statisticsSelected();

public slots:
    /*
     * Changes the icon image displayed in the tray
     */
    void updateStatus( int status );

    /*
     * Creates or deletes the tray icon
     */
    void updateTrayStatus( int previousState );

private slots:
    /*
     * Called when the user selects 'Configure KNemo' from the context menu
     */
    void showConfigDialog();

    /*
     * Called when the user setup custom commands and selects one
     * in the context menu
     */
    void menuActivated( int id );

private:
    // the interface this icon belongs to
    Interface* mInterface;
    // the real tray icon
    InterfaceTray* mTray;

    static const QString ICON_DISCONNECTED;
    static const QString ICON_CONNECTED;
    static const QString ICON_INCOMING;
    static const QString ICON_OUTGOING;
    static const QString ICON_TRAFFIC;
    static const QString SUFFIX_PPP;
    static const QString SUFFIX_LAN;
    static const QString SUFFIX_WLAN;
};

#endif // INTERFACEICON_H
