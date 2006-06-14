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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef INTERFACETRAY_H
#define INTERFACETRAY_H

#include <ksystemtray.h>

class QWidget;

/**
 * This class is the graphical representation of and interface
 * in the system tray. It has a customized context menu and will
 * emit a signal when the user left clicks the icon.
 *
 * @short Graphical representation of the tray icon
 * @author Percy Leonhardt <percy@eris23.de>
 */

class InterfaceTray : public KSystemTray
{
    Q_OBJECT
public:
    /**
     * Default Constructor
     */
    InterfaceTray( const QString& ifname,
                   QWidget* parent = 0L, const char* name = 0L );

    /**
     * Default Destructor
     */
    virtual ~InterfaceTray();

signals:
    void leftClicked();
    void graphSelected( bool );
    void configSelected();
    void startCommandSelected();
    void stopCommandSelected();

protected:
    void mousePressEvent( QMouseEvent* e );

protected slots:
    /**
     * Will display the about dialog if the user selected
     * the corresponding entry in the context menu.
     */
    void showAboutDialog();

    /**
     * Will display the report bug dialog that allows the user
     * to send a bug report by mail.
     */
    void showReportBugDialog();

    /**
     * Opens the traffic plotter or brings it to the front if it
     * is hidden.
     */
    void showGraph();
};

#endif // INTERFACETRAY_H
