/* This file is part of KNemo
   Copyright (C) 2006 Percy Leonhardt <percy@eris23.de>
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

#ifndef BACKENDBASE_H
#define BACKENDBASE_H

#include <QHash>

#include "../../common/data.h"

/**
 * This is the baseclass for all backends. Every backend that
 * should be used for KNemo must inherit from this class.
 *
 * @short Baseclass for all backends
 * @author Percy Leonhardt <percy@eris23.de>
 */

class BackendBase : public QObject
{
    Q_OBJECT
public:
    BackendBase();
    virtual ~BackendBase();

    /**
     * Create an instance of this backend because KNemo
     * does not know about the different types of backends.
     */

    /**
     * This function is called from KNemo whenever the
     * backend shall update the information of the
     * interfaces in the QHash.
     */
    virtual void update() = 0;
    virtual QStringList ifaceList() = 0;
    virtual QString defaultRouteIface( int afInet ) = 0;
    const BackendData* addIface( const QString& iface );
    void removeIface( const QString& iface );
    void clearTraffic( const QString& iface );
    void updatePackets( const QString& iface );

signals:
    /**
     * Emit this signal when you have completed the
     * update. It will trigger the interfaces to check
     * if there state has changed.
     */
    void updateComplete();

protected:
    QHash<QString, BackendData *> mInterfaces;
    QString ip4DefGw;
    QString ip6DefGw;
    void incBytes( KNemoIface::Type type, unsigned long bytes,
                   unsigned long &changed,
                   unsigned long &prevDataBytes, quint64 &curDataBytes );
};

extern BackendBase *backend;

#endif // BACKENDBASE_H
