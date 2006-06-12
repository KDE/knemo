/* This file is part of KNemo
   Copyright (C) 2004, 2005 Percy Leonhardt <percy@eris23.de>

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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <qwidget.h>

#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kbugreport.h>
#include <kaboutdata.h>
#include <kpopupmenu.h>
#include <kiconloader.h>
#include <kapplication.h>
#include <kaboutapplication.h>
#include <kactioncollection.h>

#include "interfacetray.h"

static const char description[] =
   I18N_NOOP( "KNemo - the KDE Network Monitor" );

static const char version[] = "0.4.0";

InterfaceTray::InterfaceTray( const QString& ifname,
                              QWidget* parent, const char* name )
    : KSystemTray( parent, name )
{
    actionCollection()->clear(); // remove the quit entry

    KPopupMenu* popup = contextMenu();
    int id = popup->idAt( 0 );
    popup->changeTitle( id, SmallIcon( "knemo" ),
                        "KNemo - " + ifname );
    popup->insertItem( SmallIcon( "knemo" ),
                       i18n( "&About KNemo" ), this,
                       SLOT( showAboutDialog() ) );
    popup->insertItem( i18n( "&Report Bug..." ), this,
                       SLOT( showReportBugDialog() ) );
    popup->insertSeparator();
    popup->insertItem( SmallIcon( "configure" ),
                       i18n( "&Configure KNemo..." ), this,
                       SIGNAL( configSelected() ) );
    popup->insertItem( SmallIcon( "ksysguard" ),
                       i18n( "&Open Traffic Plotter" ), this,
                       SLOT( showGraph() ) );
}

InterfaceTray::~InterfaceTray()
{
}

void InterfaceTray::mousePressEvent( QMouseEvent* e )
{
    if ( !rect().contains( e->pos() ) )
        return;

    switch( e->button() )
    {
    case LeftButton:
        emit leftClicked();
        break;
    case MidButton:
        emit graphSelected( true );
        break;
    case RightButton:
        KSystemTray::mousePressEvent( e );
        break;
    default:
        break;
    }
}

void InterfaceTray::showAboutDialog()
{
    KAboutData data ( "knemo", I18N_NOOP( "KNemo" ), version,
                      description, KAboutData::License_GPL,
                      "(c) 2004, 2005, 2006 Percy Leonhardt\n\nSignal plotter taken from KSysGuard\n(c) 1999 - 2002, Chris Schlaeger",
		      0,
		      "http://extragear.kde.org/apps/knemo/"
                      );
    data.addAuthor( "Percy Leonhardt", I18N_NOOP( "Author" ),
                    "percy@eris23.de" );
    data.addCredit( "Michael Olbrich", I18N_NOOP( "Threshold support" ),
                    "michael.olbrich@gmx.net" );
    data.addAuthor( "Bernd Zimmer", I18N_NOOP( "German translation" ),
                    "berndzimmer@gmx.de" );
    data.addAuthor( "Raul Moratalla", I18N_NOOP( "Spanish translation" ),
                    "raul.moratalla@ono.com" );
    data.addAuthor( "Pedro Jurado Maqueda", I18N_NOOP( "Spanish translation" ),
                    "melenas@kdehispano.org" );
    data.addAuthor( "Malin Malinov", I18N_NOOP( "Bulgarian translation" ),
                    "lgmim@club-35.com" );
    data.addAuthor( "Samuele Kaplun", I18N_NOOP( "Italian translation" ),
                    "kaplun@aliceposta.it" );
    data.addAuthor( "Klara Cihlarova", I18N_NOOP( "Czech translation" ),
                    "cihlarov@suse.cz" );
    data.addAuthor( "Julien Morot", I18N_NOOP( "French translation" ),
                    "julien@momonux.org" );
    data.addAuthor( "Rogerio Pereira", I18N_NOOP( "Brazilian Portuguese translation" ),
                    "rogerio.araujo@gmail.com" );
    data.addAuthor( "Rinse de Vries", I18N_NOOP( "Dutch translation" ),
                    "rinsedevries@kde.nl" );
    data.addAuthor( "Alexander Shiyan", I18N_NOOP( "Russian translation" ),
                    "shc@milas.spb.ru" );
    data.addAuthor( "Charles Barcza", I18N_NOOP( "Hungarian translation" ),
                    "kbarcza@blackpanther.hu" );
    data.addCredit( "Chris Schlaeger", I18N_NOOP( "Signal plotter" ),
                    "cs@kde.org" );

    KAboutApplication about( &data );
    about.setProgramLogo( DesktopIcon( "knemo" ) );
    about.exec();
}

void InterfaceTray::showReportBugDialog()
{
    KAboutData data ( "knemo", I18N_NOOP( "KNemo" ), version );
    KBugReport bugReport( 0, true, &data );
    bugReport.exec();
}

void InterfaceTray::showGraph()
{
    emit graphSelected( false );
}

#include "interfacetray.moc"
