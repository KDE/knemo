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

#ifndef DAEMONREGISTRY_H
#define DAEMONREGISTRY_H

#include "sysbackend.h"
#include "nettoolsbackend.h"

/**
 * This registry tells KNemo what backends are available
 * and how they can be created. It is only used by the daemon
 * to create the selected backend. Two registries were
 * necessary to avoid linking the KCM module against all backends.
 *
 * @short Registry for all backends
 * @author Percy Leonhardt <percy@eris23.de>
 */

struct DaemonRegistryEntry
{
    QString name;
    BackendBase* (*function) ( QHash<QString, Interface *>& );
};

DaemonRegistryEntry DaemonRegistry[] =
{
    { "Sys", SysBackend::createInstance },
    { "Nettools", NetToolsBackend::createInstance },
    { QString::null, 0 }
};

#endif // DAEMONREGISTRY_H
