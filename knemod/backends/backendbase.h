/* This file is part of KNemo
   Copyright (C) 2006 Percy Leonhardt <percy@eris23.de>

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

#include <qdict.h>
#include <qstring.h>

#include <klocale.h>

#include "data.h"
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
    BackendBase( QDict<Interface>& interfaces );
    virtual ~BackendBase();

    /**
     * Create an instance of this backend because KNemo
     * does not know about the different types of backends.
     */

    /**
     * This function is called from KNemo whenever the
     * backend shall update the information of the
     * interfaces in the QDict.
     */
    virtual void update() = 0;

protected:
    /**
     * Call this function when you have completed the
     * update. It will trigger the interfaces to check
     * if there state has changed.
     */
    void updateComplete();

    const QDict<Interface>& mInterfaces;
};

#endif // BACKENDBASE_H
