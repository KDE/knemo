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

#include "config-knemo.h"
#include "knemodaemon.h"
#include <KAboutData>
#include <KDBusService>
#include <KLocalizedString>
#include <QApplication>
#include <QCommandLineParser>
#include <QSessionManager>

static const char * const description =
    I18N_NOOP( "The KDE Network Monitor" );

extern "C" int main(int argc, char *argv[] )
{

    KLocalizedString::setApplicationDomain("knemo");

    KAboutData aboutData( QLatin1String("knemo"), i18n( "KNemo" ), QLatin1String(KNEMO_VERSION),
                          i18n( description ),
                          KAboutLicense::GPL_V2,
                          QString(),
                          i18n( "Copyright (C) 2004, 2005, 2006 Percy Leonhardt\nCopyright (C) 2009, 2010, 2015 John Stamp\n\nDate picker taken from Akonadi Contact\nCopyright (C) 2004 Bram Schoenmakers\nCopyright (C) 2009 Tobias Koenig" ),
                          QLatin1String("http://extragear.kde.org/apps/knemo/"));

    aboutData.addAuthor( i18n( "John Stamp" ), i18n( "Current maintainer" ),
                    QLatin1String("jstamp@mehercule.net") );
    aboutData.addAuthor( i18n( "Percy Leonhardt" ), i18n( "Original Author" ),
                    QLatin1String("percy@eris23.de") );
    aboutData.addCredit( i18n( "Michael Olbrich" ), i18n( "Threshold support" ),
                    QLatin1String("michael.olbrich@gmx.net") );
    aboutData.addCredit( i18n( "Bram Schoenmakers" ), i18n( "Date picker" ),
                    QLatin1String("bramschoenmakers@kde.nl") );
    aboutData.addCredit( i18n( "Tobias Koenig" ), i18n( "Date picker" ),
                    QLatin1String("tokoe@kde.org") );
    QApplication app(argc, argv);

    QCoreApplication::setApplicationName(i18n("KNemo"));
    QCoreApplication::setOrganizationDomain(QLatin1String("kde.org"));

    KAboutData::setApplicationData(aboutData);
    auto disableSessionManagement = [] (QSessionManager &sm) {
        sm.setRestartHint(QSessionManager::RestartNever);
    };
    QObject::connect(&app, &QGuiApplication::commitDataRequest, disableSessionManagement);
    QObject::connect(&app, &QGuiApplication::saveStateRequest, disableSessionManagement);

    QCommandLineParser parser;
    parser.addVersionOption();
    parser.addHelpOption();
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    KDBusService service(KDBusService::Unique);

    KNemoDaemon knemo;

    return app.exec();
}
