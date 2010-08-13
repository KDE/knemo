/* This file is part of KNemo
   Copyright (C) 2004, 2006 Percy Leonhardt <percy@eris23.de>
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

#include <stdio.h>
#include <stdlib.h>

#include "knemodaemon.h"
#include <KCmdLineArgs>
#include <KUniqueApplication>

extern "C" int main(int argc, char *argv[] )
{

    KNemoDaemon::createAboutData();
    KCmdLineArgs::init( argc, argv, KNemoDaemon::aboutData() );
    KUniqueApplication::addCmdLineOptions();

    if ( !KUniqueApplication::start() )
    {
        fprintf( stderr, "KNemo is already running!\n" );
        exit( 0 );
    }
    KUniqueApplication app;
    app.disableSessionManagement();
    app.setQuitOnLastWindowClosed( false );

    KNemoDaemon knemo;

    int ret = app.exec();
    KNemoDaemon::destroyAboutData();
    return ret;
}
