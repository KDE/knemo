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

#ifndef REGISTRY_H
#define REGISTRY_H

#include <qstring.h>

#include <klocale.h>

#include "backendbase.h"
#include "nettoolsbackend.h"

/**
 * This registry tells KNemo what backends are available
 * and how they can be created. It also offers a short
 * description for every backend that is used in the
 * configuration dialog of KNemo. It should describe how
 * a backend gathers its information.
 *
 * @short Registry for all backends
 * @author Percy Leonhardt <percy@eris23.de>
 */

struct RegistryEntry
{
    QString name;
    QString description;
    BackendBase* (*function) ( QDict<Interface>& );

};

RegistryEntry Registry[] =
{
    { "Nettools", i18n( "None." ), NetToolsBackend::createInstance },
    { QString::null, QString::null, 0 }
};

#endif // REGISTRY_H
