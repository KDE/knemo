/* This file is part of KNemo
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

#ifndef UTILS_H
#define UTILS_H

#include <algorithm>
#include <QList>
#include <QString>
#include "data.h"

using namespace std;

/* This is for clamping min/max values read from the settings file */
template <class T> inline T clamp(T x, T a, T b)
{
    return min(max(x,a),b);
}

/*
 * Finds the default gateway for AF_INET or AF_INET6
 * It fills defaultGateway with the address and returns the interface name
 * If one isn't found, both are empty.
 * The netlink backend uses data to nl_cache data
 */
QString getDefaultRoute( int afType, QString * defaultGateway = NULL, void * data = NULL );

QList<KNemoTheme> findThemes();

/*
 * Given a string and tray icon width, return a font size that fits the text
 * in the tray.
 */
QFont setIconFont( const QString& text, const QFont& font, int iconWidth );

double validatePoll( double val );

#endif
