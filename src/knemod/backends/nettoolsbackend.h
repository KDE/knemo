/* This file is part of KNemo
   Copyright (C) 2004 Percy Leonhardt <percy@eris23.de>
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

#ifndef NETTOOLSBACKEND_H
#define NETTOOLSBACKEND_H

#include "backendbase.h"
#include <QProcess>

class KProcess;

/**
 * The nettools backend runs 'ifconfig', 'iwconfig' and 'route'
 * and parses their output. It then triggers the interface
 * monitor to look for changes in the state of the interface.
 *
 * @short Update the information of the interfaces via nettools
 * @author Percy Leonhardt <percy@eris23.de>
 */

class NetToolsBackend : public QObject, BackendBase
{
    Q_OBJECT
public:
    NetToolsBackend(QHash<QString, Interface *>& interfaces );
    virtual ~NetToolsBackend();

    static BackendBase* createInstance( QHash<QString, Interface *>& interfaces );

    void update();
    QString getDefaultRouteIface();

private slots:
    void routeProcessExited( int exitCode, QProcess::ExitStatus status );
    void routeProcessStdout();
    void ifconfigProcessExited( int exitCode, QProcess::ExitStatus status );
    void ifconfigProcessStdout();
    void iwconfigProcessExited( int exitCode, QProcess::ExitStatus status );
    void iwconfigProcessStdout();

private:
    void parseRouteOutput();
    void parseIfconfigOutput();
    void updateInterfaceData( QString& config, InterfaceData& data, int type );
    void parseIwconfigOutput();
    void updateWirelessData( QString& ifaceName, QString& config, WirelessData& data );
    void parseWirelessEncryption( QString& config, WirelessData& data );

    QString mRouteStdout;
    QString mIfconfigStdout;
    QString mIwconfigStdout;
    KProcess* mRouteProcess;
    KProcess* mIfconfigProcess;
    KProcess* mIwconfigProcess;
};

#endif // NETTOOLSBACKEND_H
