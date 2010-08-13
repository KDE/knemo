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

#ifndef SQLSTORAGE_H
#define SQLSTORAGE_H

#include "storagedata.h"
#include <QSqlDatabase>

class SqlStorage
{
    public:
        SqlStorage( QString ifaceName );
        ~SqlStorage();
        bool dbExists();
        bool createDb();
        bool loadHourArchives( StatisticsModel *hourArchive, const QDate &startDate, const QDate &endDate );
        bool loadStats( StorageData *gd, QHash<int, StatisticsModel*> *models );
        bool saveStats( StorageData *gd, QHash<int, StatisticsModel*> *models );
        bool clearStats( StorageData *gd );

    private:
        bool open();
        void save( StorageData *gd, QHash<int, StatisticsModel*> *models = 0 );
        QString mDbPath;

        QSqlDatabase db;
        QString mIfaceName;
};

#endif
