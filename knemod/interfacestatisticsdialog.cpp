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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <qtable.h>
#include <qdatetime.h>
#include <qpushbutton.h>

#include <klocale.h>
#include <kio/global.h>
#include <kiconloader.h>
#include <kcalendarsystem.h>

#include "data.h"
#include "interface.h"
#include "interfacestatistics.h"
#include "interfacestatisticsdialog.h"

/*
 * I assume that the last entry in each table is also the current. This will fail
 * if we walk back in time, so better not play with the system date...
 */

InterfaceStatisticsDialog::InterfaceStatisticsDialog( Interface* interface, QWidget* parent, const char* name )
    : InterfaceStatisticsDlg( parent, name ),
      mInterface( interface )
{
    setIcon( SmallIcon( "knemo" ) );
    setCaption( interface->getName() + " " + i18n( "Statistics" ) );

    connect( buttonClearDaily, SIGNAL( clicked() ), SIGNAL( clearDailyStatisticsClicked() ) );
    connect( buttonClearMonthly, SIGNAL( clicked() ), SIGNAL( clearMonthlyStatisticsClicked() ) );
    connect( buttonClearYearly, SIGNAL( clicked() ), SIGNAL( clearYearlyStatisticsClicked() ) );
}

InterfaceStatisticsDialog::~InterfaceStatisticsDialog()
{
}

void InterfaceStatisticsDialog::updateDays()
{
    QPtrList<StatisticEntry> dayStatistics = mInterface->getStatistics()->getDayStatistics();
    StatisticEntry* iterator = dayStatistics.first();
    tableDaily->setNumRows( dayStatistics.count() );
    int row = 0;
    while ( iterator )
    {
        QDate date( iterator->year, iterator->month,  iterator->day );
        tableDaily->verticalHeader()->setLabel( row, KGlobal::locale()->formatDate( date, true ) );
        tableDaily->setText( row, 0, KIO::convertSize( iterator->txBytes ) );
        tableDaily->setText( row, 1, KIO::convertSize( iterator->rxBytes ) );
        tableDaily->setText( row, 2, KIO::convertSize( iterator->rxBytes + iterator->txBytes ) );
        row++;
        iterator = dayStatistics.next();
    }

    tableDaily->setCurrentCell( row - 1, 2 );
    tableDaily->ensureCellVisible( row - 1, 2 );
}

void InterfaceStatisticsDialog::updateMonths()
{
    QPtrList<StatisticEntry> monthStatistics = mInterface->getStatistics()->getMonthStatistics();
    StatisticEntry* iterator = monthStatistics.first();
    tableMonthly->setNumRows( monthStatistics.count() );
    int row = 0;
    while ( iterator )
    {
        const KCalendarSystem* calendar = KGlobal::locale()->calendar();
        QString monthName = calendar->monthName( iterator->month, iterator->year ) + " " + QString::number( iterator->year );
        tableMonthly->verticalHeader()->setLabel( row, monthName );
        tableMonthly->setText( row, 0, KIO::convertSize( iterator->txBytes ) );
        tableMonthly->setText( row, 1, KIO::convertSize( iterator->rxBytes ) );
        tableMonthly->setText( row, 2, KIO::convertSize( iterator->rxBytes + iterator->txBytes ) );
        row++;
        iterator = monthStatistics.next();
    }

    tableMonthly->setCurrentCell( row - 1, 2 );
    tableMonthly->ensureCellVisible( row - 1, 2 );
}

void InterfaceStatisticsDialog::updateYears()
{
    QPtrList<StatisticEntry> yearStatistics = mInterface->getStatistics()->getYearStatistics();
    StatisticEntry* iterator = yearStatistics.first();
    tableYearly->setNumRows( yearStatistics.count() );
    int row = 0;
    while ( iterator )
    {
        tableYearly->verticalHeader()->setLabel( row, QString::number( iterator->year ) );
        tableYearly->setText( row, 0, KIO::convertSize( iterator->txBytes ) );
        tableYearly->setText( row, 1, KIO::convertSize( iterator->rxBytes ) );
        tableYearly->setText( row, 2, KIO::convertSize( iterator->rxBytes + iterator->txBytes ) );
        row++;
        iterator = yearStatistics.next();
    }

    tableYearly->setCurrentCell( row - 1, 2 );
    tableYearly->ensureCellVisible( row - 1, 2 );
}

void InterfaceStatisticsDialog::updateCurrentEntry()
{
    int lastRow = tableDaily->numRows() - 1;
    const StatisticEntry* currentEntry = mInterface->getStatistics()->getCurrentDay();
    tableDaily->setText( lastRow, 0, KIO::convertSize( currentEntry->txBytes ) );
    tableDaily->setText( lastRow, 1, KIO::convertSize( currentEntry->rxBytes ) );
    tableDaily->setText( lastRow, 2, KIO::convertSize( currentEntry->rxBytes + currentEntry->txBytes ) );

    lastRow = tableMonthly->numRows() - 1;
    currentEntry = mInterface->getStatistics()->getCurrentMonth();
    tableMonthly->setText( lastRow, 0, KIO::convertSize( currentEntry->txBytes ) );
    tableMonthly->setText( lastRow, 1, KIO::convertSize( currentEntry->rxBytes ) );
    tableMonthly->setText( lastRow, 2, KIO::convertSize( currentEntry->rxBytes + currentEntry->txBytes ) );

    lastRow = tableYearly->numRows() - 1;
    currentEntry = mInterface->getStatistics()->getCurrentYear();
    tableYearly->setText( lastRow, 0, KIO::convertSize( currentEntry->txBytes ) );
    tableYearly->setText( lastRow, 1, KIO::convertSize( currentEntry->rxBytes ) );
    tableYearly->setText( lastRow, 2, KIO::convertSize( currentEntry->rxBytes + currentEntry->txBytes ) );
}

#include "interfacestatisticsdialog.moc"
