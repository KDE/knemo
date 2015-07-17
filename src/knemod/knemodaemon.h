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

#ifndef KNEMODAEMON_H
#define KNEMODAEMON_H

#include <QHash>
#include <QApplication>

class QTimer;
class Interface;
class BackendBase;

/**
 * This class is the main entry point of KNemo. It reads the configuration,
 * creates the logical interfaces and starts an interface updater. It also
 * takes care of configuration changes by the user.
 *
 * @short KNemos main entry point
 * @author Percy Leonhardt <percy@eris23.de>
 */
//class KNemoDaemon : public KDEDModule
class KNemoDaemon : public QObject
{
    Q_OBJECT
    Q_CLASSINFO( "D-Bus Interface", "org.kde.knemo" )
public:
    /**
     * Default Constructor
     */
    KNemoDaemon();

    /**
     * Default Destructor
     */
    virtual ~KNemoDaemon();

    // tell the control center module which interface the user selected
    static QString sSelectedInterface;


public Q_SLOTS:
    /*
     * Called from the control center module when the user changed
     * the settings. It updates the internal list of interfaces
     * that should be monitored.
     */
    Q_SCRIPTABLE void reparseConfiguration();

    /* When the user selects 'Configure KNemo...' from the context
     * menus this functions gets called from the control center
     * module. This way the module knows for which interface the
     * user opened the dialog and can preselect the appropriate
     * interface in the list.
     */
    Q_SCRIPTABLE QString getSelectedInterface();

private:
    /*
     * Read the configuration on startup
     */
    void readConfig();

private Q_SLOTS:
    /**
     * trigger the backend to update the interface informations
     */
    void updateInterfaces();

    void togglePlotters();

private:
    bool mHaveInterfaces;

    // every time this timer expires we will
    // gather new informations from the backend
    QTimer* mPollTimer;
    // the name of backend we currently use
    QString mBackendName;
    // a list of all interfaces the user wants to monitor
    QHash<QString, Interface *> mInterfaceHash;
};

#endif // KNEMODAEMON_H
