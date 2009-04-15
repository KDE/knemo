/* This file is part of KNemo
   Copyright (C) 2006 Percy Leonhardt <percy@eris23.de>
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

#include <QStandardItemModel>

#include <kio/global.h>
#include <KCalendarSystem>

#include "data.h"
#include "interface.h"
#include "interfacestatistics.h"
#include "interfacestatisticsdialog.h"

/*
 * I assume that the last entry in each table is also the current. This will fail
 * if we walk back in time, so better not play with the system date...
 */

InterfaceStatisticsDialog::InterfaceStatisticsDialog( Interface* interface, QWidget* parent )
    : KDialog( parent ),
      mSetPos( true ),
      mConfig( KGlobal::config() ),
      mInterface( interface )
{
    setCaption( interface->getName() + " " + i18n( "Statistics" ) );
    setButtons( Close );

    ui.setupUi( mainWidget() );

    dailyModel = new QStandardItemModel( this );
    monthlyModel = new QStandardItemModel( this );
    yearlyModel = new QStandardItemModel( this );

    ui.tableDaily->setModel( dailyModel );
    ui.tableMonthly->setModel( monthlyModel );
    ui.tableYearly->setModel( yearlyModel );

    QStringList headerList;
    headerList << i18n( "Sent" ) << i18n( "Received" ) << i18n( "Total" );
    dailyModel->setHorizontalHeaderLabels( headerList );
    monthlyModel->setHorizontalHeaderLabels( headerList );
    yearlyModel->setHorizontalHeaderLabels( headerList );

    connect( ui.buttonClearDaily, SIGNAL( clicked() ), SIGNAL( clearDailyStatisticsClicked() ) );
    connect( ui.buttonClearMonthly, SIGNAL( clicked() ), SIGNAL( clearMonthlyStatisticsClicked() ) );
    connect( ui.buttonClearYearly, SIGNAL( clicked() ), SIGNAL( clearYearlyStatisticsClicked() ) );

    // Restore window size and position.
    KConfig *config = mConfig.data();
    KConfigGroup interfaceGroup( config, "Interface_" + mInterface->getName() );
    if ( interfaceGroup.hasKey( "StatisticsPos" ) )
    {
        QPoint p = interfaceGroup.readEntry( "StatisticsPos", QPoint() );
        // See comment in event()
        mSetPos = false;
        move( p );
    }
    if ( interfaceGroup.hasKey( "StatisticsSize" ) )
    {
        QSize s = interfaceGroup.readEntry( "StatisticsSize", QSize() );
        resize( s );
    }
    else
    {
        // Improve the chance that we have a decent sized dialog
        // the first time it's shown
        resize( 600, 450 );
    }
}

InterfaceStatisticsDialog::~InterfaceStatisticsDialog()
{
    KConfig *config = mConfig.data();
    KConfigGroup interfaceGroup( config, "Interface_" + mInterface->getName() );
    interfaceGroup.writeEntry( "StatisticsPos", pos() );
    interfaceGroup.writeEntry( "StatisticsSize", size() );
    config->sync();
}

bool InterfaceStatisticsDialog::event( QEvent *e )
{
    /* If we do not explicitly call size() and move() at least once then
     * hiding and showing the dialog will cause it to forget its previous
     * size and position. */
    if ( e->type() == QEvent::Move )
    {
        if ( mSetPos && !pos().isNull() )
        {
            mSetPos = false;
            move( pos() );
        }
    }

    return KDialog::event( e );
}

void InterfaceStatisticsDialog::updateDays()
{
    QList<StatisticEntry *> dayStatistics = mInterface->getStatistics()->getDayStatistics();
    QList<QString> vheaders;

    dailyModel->removeRows( 0, dailyModel->rowCount() );

    foreach ( StatisticEntry *iterator, dayStatistics )
    {
        QDate date( iterator->year, iterator->month,  iterator->day );
        vheaders << KGlobal::locale()->formatDate( date );
        QStandardItem *tx = new QStandardItem( KIO::convertSize( iterator->txBytes ) );
        QStandardItem *rx = new QStandardItem( KIO::convertSize( iterator->rxBytes ) );
        QStandardItem *total = new QStandardItem( KIO::convertSize( iterator->rxBytes + iterator->txBytes ) );
        QList<QStandardItem *> row;
        row << tx << rx << total;
        dailyModel->appendRow( row );
    }

    dailyModel->setVerticalHeaderLabels( vheaders );

    if ( dailyModel->rowCount() > 0 )
    {
        QModelIndex scrollIndex = dailyModel->item( dailyModel->rowCount() - 1, 2 )->index();
        ui.tableDaily->setCurrentIndex( scrollIndex );
        ui.tableDaily->scrollTo( scrollIndex );
    }
}

void InterfaceStatisticsDialog::updateMonths()
{
    QList<StatisticEntry *> monthStatistics = mInterface->getStatistics()->getMonthStatistics();
    QList<QString> vheaders;

    monthlyModel->removeRows( 0, monthlyModel->rowCount() );

    foreach ( StatisticEntry* iterator, monthStatistics )
    {
        const KCalendarSystem* calendar = KGlobal::locale()->calendar();
        vheaders << calendar->monthName( iterator->month, iterator->year ) + " " + QString::number( iterator->year );
        QStandardItem *tx = new QStandardItem( KIO::convertSize( iterator->txBytes ) );
        QStandardItem *rx = new QStandardItem( KIO::convertSize( iterator->rxBytes ) );
        QStandardItem *total = new QStandardItem( KIO::convertSize( iterator->rxBytes + iterator->txBytes ) );
        QList<QStandardItem *> row;
        row << tx << rx << total;
        monthlyModel->appendRow( row );
    }

    monthlyModel->setVerticalHeaderLabels( vheaders );

    if ( monthlyModel->rowCount() > 0 )
    {
        QModelIndex scrollIndex = monthlyModel->item( monthlyModel->rowCount() - 1, 2 )->index();
        ui.tableMonthly->setCurrentIndex( scrollIndex );
        ui.tableMonthly->scrollTo( scrollIndex );
    }
}

void InterfaceStatisticsDialog::updateYears()
{
    QList<StatisticEntry *> yearStatistics = mInterface->getStatistics()->getYearStatistics();
    QList<QString> vheaders;

    yearlyModel->removeRows( 0, yearlyModel->rowCount() );

    foreach ( StatisticEntry* iterator, yearStatistics )
    {
        vheaders << QString::number( iterator->year );
        QStandardItem *tx = new QStandardItem( KIO::convertSize( iterator->txBytes ) );
        QStandardItem *rx = new QStandardItem( KIO::convertSize( iterator->rxBytes ) );
        QStandardItem *total = new QStandardItem( KIO::convertSize( iterator->rxBytes + iterator->txBytes ) );
        QList<QStandardItem *> row;
        row << tx << rx << total;
        yearlyModel->appendRow( row );
    }

    yearlyModel->setVerticalHeaderLabels( vheaders );

    if ( yearlyModel->rowCount() > 0 )
    {
        QModelIndex scrollIndex = yearlyModel->item( yearlyModel->rowCount() - 1, 2 )->index();
        ui.tableYearly->setCurrentIndex( scrollIndex );
        ui.tableYearly->scrollTo( scrollIndex );
    }
}

void InterfaceStatisticsDialog::updateCurrentEntry()
{
    int lastRow = dailyModel->rowCount() - 1;
    if ( lastRow > -1 )
    {
        const StatisticEntry* currentEntry = mInterface->getStatistics()->getCurrentDay();
        dailyModel->item( lastRow, 0 )->setText( KIO::convertSize( currentEntry->txBytes ) );
        dailyModel->item( lastRow, 1 )->setText( KIO::convertSize( currentEntry->rxBytes ) );
        dailyModel->item( lastRow, 2 )->setText( KIO::convertSize( currentEntry->rxBytes + currentEntry->txBytes ) );
    }

    lastRow = monthlyModel->rowCount() - 1;
    if ( lastRow > -1 )
    {
        const StatisticEntry* currentEntry = mInterface->getStatistics()->getCurrentMonth();
        monthlyModel->item( lastRow, 0 )->setText( KIO::convertSize( currentEntry->txBytes ) );
        monthlyModel->item( lastRow, 1 )->setText( KIO::convertSize( currentEntry->rxBytes ) );
        monthlyModel->item( lastRow, 2 )->setText( KIO::convertSize( currentEntry->rxBytes + currentEntry->txBytes ) );
    }

    lastRow = yearlyModel->rowCount() - 1;
    if ( lastRow > -1 )
    {
        const StatisticEntry* currentEntry = mInterface->getStatistics()->getCurrentYear();
        yearlyModel->item( lastRow, 0 )->setText( KIO::convertSize( currentEntry->txBytes ) );
        yearlyModel->item( lastRow, 1 )->setText( KIO::convertSize( currentEntry->rxBytes ) );
        yearlyModel->item( lastRow, 2 )->setText( KIO::convertSize( currentEntry->rxBytes + currentEntry->txBytes ) );
    }
}

#include "interfacestatisticsdialog.moc"
