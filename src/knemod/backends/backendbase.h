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

#include "interface.h"

/**
 * This is the baseclass for all backends. Every backend that
 * should be used for KNemo must inherit from this class.
 *
 * @short Baseclass for all backends
 * @author Percy Leonhardt <percy@eris23.de>
 */

class BackendBase
{
public:
    BackendBase( QHash<QString, Interface *>& interfaces );
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
    virtual QString getDefaultRouteIface( int afInet ) = 0;

protected:
    /**
     * Call this function when you have completed the
     * update. It will trigger the interfaces to check
     * if there state has changed.
     */
    virtual void updateComplete();

    const QHash<QString, Interface *>& mInterfaces;
    QString ip4DefGw;
    QString ip6DefGw;
    void incBytes( int type, unsigned long bytes, unsigned long &changed, unsigned long &prevDataBytes, quint64 &curDataBytes );
    virtual void updateInterfaceData( const QString&, InterfaceData& ) = 0;
};

#endif // BACKENDBASE_H
