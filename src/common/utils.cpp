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

   Portions taken from FreeSWITCH
       Copyright (c) 2007-2008, Thomas BERNARD <miniupnp@free.fr>

       Permission to use, copy, modify, and/or distribute this software for any
       purpose with or without fee is hereby granted, provided that the above
       copyright notice and this permission notice appear in all copies.
*/

#include <KStandardDirs>

QStringList findIconSets()
{
    KStandardDirs iconDirs;
    iconDirs.addResourceType("knemo_pics", "data", "knemo/pics");
    QStringList iconlist = iconDirs.findAllResources( "knemo_pics", "*.png" );

    QStringList iconSets;
    foreach ( QString iconName, iconlist )
    {
        QRegExp rx( "pics\\/(.+)_(connected|disconnected|incoming|outgoing|traffic)\\.png" );
        if ( rx.indexIn( iconName ) > -1 )
            if ( !iconSets.contains( rx.cap( 1 ) ) )
                iconSets << rx.cap( 1 );
    }
    return iconSets;
}
