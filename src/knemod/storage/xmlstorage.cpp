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

#include "global.h"
#include "statisticsmodel.h"
#include "xmlstorage.h"
#include "commonstorage.h"

#include <QDomNode>
#include <QFile>
#include <KCalendarSystem>

// xml storage
static const char doc_name[]        = "statistics";
static const char attrib_calendar[] = "calendar";
static const char attrib_updated[]  = "lastUpdated";
static const char attrib_rx[]       = "rxBytes";
static const char attrib_tx[]       = "txBytes";


XmlStorage::XmlStorage()
{
}

void XmlStorage::loadGroup( StorageData *sd, const QDomElement& parentItem,
    StatisticsModel* statistics )
{
    QDomNode n = parentItem.namedItem( periods.at( statistics->periodType() ) + "s" );
    if ( !n.isNull() )
    {
        QDomNode node = n.firstChild();
        while ( !node.isNull() )
        {
            QDomElement element = node.toElement();
            if ( !element.isNull() )
            {
                QDate date;
                QTime time;

                int year = element.attribute( periods.at( KNemoStats::Year ) ).toInt();
                int month = element.attribute( periods.at( KNemoStats::Month ), "1" ).toInt();
                int day = element.attribute( periods.at( KNemoStats::Day ), "1" ).toInt();
                sd->calendar->setDate( date, year, month, day );

                if ( date.isValid() )
                {
                    switch ( statistics->periodType() )
                    {
                        case KNemoStats::Hour:
                            time = QTime( element.attribute( periods.at( KNemoStats::Hour ) ).toInt(), 0 );
                            break;
                        default:
                            ;;
                    }

                    int entryIndex = statistics->createEntry();
                    statistics->setDateTime( QDateTime( date, time ) );
                    statistics->setTraffic( entryIndex, element.attribute( attrib_rx ).toULongLong(), element.attribute( attrib_tx ).toULongLong() );
                }
            }
            node = node.nextSibling();
        }
        statistics->sort( 0 );
    }
}

bool XmlStorage::loadStats( QString name, StorageData *sd, QHash<int, StatisticsModel*> *models )
{
    KUrl dir( generalSettings->statisticsDir );
    QDomDocument doc( doc_name );
    QFile file( dir.path() + statistics_prefix + name );

    if ( !file.open( QIODevice::ReadOnly ) )
        return false;
    if ( !doc.setContent( &file ) )
    {
        file.close();
        return false;
    }
    file.close();

    QDomElement root = doc.documentElement();

    // If unknown or empty calendar it will default to gregorian
    sd->calendar = KCalendarSystem::create( root.attribute( attrib_calendar ) );
    foreach( StatisticsModel * s, *models )
    {
        s->setCalendar( sd->calendar );
        loadGroup( sd, root, s );
    }

    sd->lastSaved = root.attribute( attrib_updated ).toUInt();

    return true;
}


