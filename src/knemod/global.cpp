/* This file is part of KNemo
   Copyright (C) 2010 John Stamp <jstamp@users.sourceforge.net>

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

#include <QString>
#include <KGlobal>
#include <QDebug>
#include <KLocalizedString>
#include <kio/global.h>
#include "global.h"

QString formattedRate( quint64 data, bool useBits )
{
    if ( !useBits )
        return KIO::convertSize( data ) + i18n( "/s" );

    QString fmtString;
    double bits = data;
    // bit/s typically uses SI notation
    int units = 0;
    while ( bits >= 1000.0 && units < 3 )
    {
        bits /= 1000.0;
        units++;
    }
    int precision = 0;
    if ( units )
        precision = 1;
    QLocale locale;
    QString formattedNum = locale.toString( bits, 'g', precision );
    switch (units)
    {
        case 0:
            fmtString = i18n( "%1 bit/s", formattedNum );
            break;
        case 1:
            fmtString = i18n( "%1 kbit/s", formattedNum );
            break;
        case 2:
            fmtString = i18n( "%1 Mbit/s", formattedNum );
            break;
        case 3:
            fmtString = i18n( "%1 Gbit/s", formattedNum );
            break;
    }
    return fmtString;
}
