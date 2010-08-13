/* This file is part of KNemo
   Copyright (C) 2005 Percy Leonhardt <percy@eris23.de>
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

#ifndef GLOBAL_H
#define GLOBAL_H

#include "data.h"

/**
 * This file contains data structures and enums used in the knemo daemon.
 *
 * @short Daemon wide structures and enums
 * @author Percy Leonhardt <percy@eris23.de>
 */

// application wide settings are stored here
extern GeneralSettings *generalSettings;

QString formattedRate( quint64 data, bool useBits );

#endif // GLOBAL_H
