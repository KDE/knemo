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

#ifndef KCMREGISTRY_H
#define KCMREGISTRY_H

#include <qstring.h>

#include <klocale.h>

/**
 * This registry tells the KCM module what backends are available
 * and how they can be created. It also offers a short description
 * for every backend that is used in the configuration dialog of KNemo.
 * It should describe how a backend gathers its information.
 *
 * @short Registry for all backends
 * @author Percy Leonhardt <percy@eris23.de>
 */

struct KCMRegistryEntry
{
    QString name;
    QString description;
};

KCMRegistryEntry KCMRegistry[] =
{
    { "Nettools",
      i18n( "Uses the tools from the nettool packge like ifconfig, "    \
            "iwconfig and route to read the necessary information "     \
            "from the ouput of these commands.\n"                       \
            "This backend works rather stable but causes a relativly "  \
            "high CPU load." ) },
    { "Sys",
      i18n( "Uses the sys filesystem available in 2.6 kernels and "     \
            "direct system calls to the Linux kernel.\n"                \
            "This backend is rather new, so expect minor problems. "    \
            "As an advantage this backend should reduce the CPU load "  \
            "and should not access the harddisc while gathering "       \
            "information." ) },
    { QString::null, QString::null }
};

#endif // KCMREGISTRY_H
