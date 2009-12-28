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
#include <KMessageBox>

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
      mWasShown( false ),
      mSetPos( true ),
      mIsMonths( true ),
      mConfig( KGlobal::config() ),
      mInterface( interface )
{
    // TODO: Test for a KDE release that contains SVN commit 1013534
    KGlobal::locale()->setCalendar( mInterface->getSettings().calendar );

    mCalendar = KCalendarSystem::create( mInterface->getSettings().calendar );
    setCaption( i18n( "%1 Statistics", interface->getName() ) );
    setButtons( Reset | Close );

    ui.setupUi( mainWidget() );

    dailyModel = new QStandardItemModel( this );
    weeklyModel = new QStandardItemModel( this );
    monthlyModel = new QStandardItemModel( this );
    yearlyModel = new QStandardItemModel( this );

    ui.tableDaily->setModel( dailyModel );
    ui.tableWeekly->setModel( weeklyModel );
    ui.tableMonthly->setModel( monthlyModel );
    ui.tableYearly->setModel( yearlyModel );

    QStringList headerList;
    headerList << i18n( "Sent" ) << i18n( "Received" ) << i18n( "Total" );
    dailyModel->setHorizontalHeaderLabels( headerList );
    weeklyModel->setHorizontalHeaderLabels( headerList );
    monthlyModel->setHorizontalHeaderLabels( headerList );
    yearlyModel->setHorizontalHeaderLabels( headerList );

    connect( this, SIGNAL( resetClicked() ), SLOT( confirmReset() ) );

    // Restore window size and position.
    KConfig *config = mConfig.data();
    KConfigGroup interfaceGroup( config, confg_interface + mInterface->getName() );
    if ( interfaceGroup.hasKey( conf_statisticsPos ) )
    {
        QPoint p = interfaceGroup.readEntry( conf_statisticsPos, QPoint() );
        // See comment in event()
        mSetPos = false;
        move( p );
    }
    if ( interfaceGroup.hasKey( conf_statisticsSize ) )
    {
        QSize s = interfaceGroup.readEntry( conf_statisticsSize, QSize() );
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
    if ( mWasShown )
    {
        KConfig *config = mConfig.data();
        KConfigGroup interfaceGroup( config, confg_interface + mInterface->getName() );
        interfaceGroup.writeEntry( conf_statisticsPos, pos() );
        interfaceGroup.writeEntry( conf_statisticsSize, size() );
        config->sync();
    }
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
    if ( e->type() == QEvent::Show )
        mWasShown = true;

    return KDialog::event( e );
}

void InterfaceStatisticsDialog::confirmReset()
{
    if ( KMessageBox::questionYesNo( this, i18n( "Do you want to reset all statistics?" ) ) == KMessageBox::Yes )
    {
        dailyModel->removeRows( 0, dailyModel->rowCount() );
        weeklyModel->removeRows( 0, weeklyModel->rowCount() );
        monthlyModel->removeRows( 0, monthlyModel->rowCount() );
        yearlyModel->removeRows( 0, yearlyModel->rowCount() );
        emit clearStatistics();
    }
}

void InterfaceStatisticsDialog::updateEntry( const StatisticEntry* entry,
        QStandardItemModel* model )
{
    int lastRow = model->rowCount() - 1;
    if ( lastRow > -1 )
    {
        model->item( lastRow, 0 )->setText( KIO::convertSize( entry->txBytes ) );
        model->item( lastRow, 1 )->setText( KIO::convertSize( entry->rxBytes ) );
        model->item( lastRow, 2 )->setText( KIO::convertSize( entry->rxBytes + entry->txBytes ) );
    }
}

void InterfaceStatisticsDialog::updateModel( const QList<StatisticEntry *>& statistics,
        QStandardItemModel* model, QTableView * view, bool fullRebuild, int group )
{
    QList<QString> vheaders;

    if ( fullRebuild )
    {
        model->removeRows( 0, model->rowCount() );
        mIsMonths = true;
    }

    int bottom = model->rowCount() - 1;
    bottom = bottom < 0 ? 0: bottom;
    for ( int i = bottom; i < statistics.count(); i++ )
    {
        StatisticEntry * entry = statistics.at( i );
        QString tempHeader;
        switch (group)
        {
            case InterfaceStatistics::Day:
                vheaders << mCalendar->formatDate( entry->date, KLocale::ShortDate );
                break;
            case InterfaceStatistics::Week:
                vheaders << mCalendar->formatDate( entry->date, KLocale::ShortDate );
                break;
            case InterfaceStatistics::Month:
                // Format for simple period
                // Starts on the first of the month, lasts exactly one month
                if ( mCalendar->day( entry->date ) == 1 &&
                     entry->span == mCalendar->daysInMonth( entry->date ) )
                    tempHeader = QString( "%1 %2" )
                        .arg( mCalendar->monthName( entry->date, KCalendarSystem::ShortName ) )
                        .arg( mCalendar->year( entry->date ) );
                // Format for complex period
                else
                {
                    mIsMonths = false;
                    QDate endDate = entry->date.addDays( entry->span - 1 );
                    tempHeader = QString( "%1 %2 - %4 %5 %6" )
                        .arg( mCalendar->day( entry->date ) )
                        .arg( mCalendar->monthName( entry->date, KCalendarSystem::ShortName ) )
                        .arg( mCalendar->day( endDate ) )
                        .arg( mCalendar->monthName( endDate, KCalendarSystem::ShortName ) )
                        .arg( mCalendar->year( endDate ) );
                }
                vheaders << tempHeader;
                break;
            case InterfaceStatistics::Year:
                vheaders << QString::number( mCalendar->year( entry->date ) );
        }
        QStandardItem *tx = new QStandardItem( KIO::convertSize( entry->txBytes ) );
        QStandardItem *rx = new QStandardItem( KIO::convertSize( entry->rxBytes ) );
        QStandardItem *total = new QStandardItem( KIO::convertSize( entry->rxBytes + entry->txBytes ) );
        QList<QStandardItem *> row;
        row << tx << rx << total;
        model->appendRow( row );
    }

    model->setVerticalHeaderLabels( vheaders );

    if ( mIsMonths )
        ui.tabWidget->setTabText( ui.tabWidget->indexOf(ui.monthly), i18n( "Months", 0 ) );
    else
        ui.tabWidget->setTabText( ui.tabWidget->indexOf(ui.monthly), i18n( "Billing Periods", 0 ) );

    if ( model->rowCount() > 0 )
    {
        QModelIndex scrollIndex = model->item( model->rowCount() - 1, 0 )->index();
        view->selectionModel()->setCurrentIndex( scrollIndex, QItemSelectionModel::NoUpdate );
    }
    view->verticalHeader()->setResizeMode( QHeaderView::ResizeToContents );
}

void InterfaceStatisticsDialog::updateDays()
{
    updateModel( mInterface->getStatistics()->getDayStatistics(),
                 dailyModel, ui.tableDaily, false,
                 InterfaceStatistics::Day );
}

void InterfaceStatisticsDialog::updateWeeks()
{
    updateModel( mInterface->getStatistics()->getWeekStatistics(),
                 weeklyModel, ui.tableWeekly, false,
                 InterfaceStatistics::Week );
}

void InterfaceStatisticsDialog::updateMonths( bool forceFullUpdate )
{
    updateModel( mInterface->getStatistics()->getMonthStatistics(),
                 monthlyModel, ui.tableMonthly, forceFullUpdate,
                 InterfaceStatistics::Month );
}

void InterfaceStatisticsDialog::updateYears()
{
    updateModel( mInterface->getStatistics()->getYearStatistics(),
                 yearlyModel, ui.tableYearly, false,
                 InterfaceStatistics::Year );
}

void InterfaceStatisticsDialog::updateCurrentEntry()
{
    updateEntry( mInterface->getStatistics()->getCurrentDay(), dailyModel );
    updateEntry( mInterface->getStatistics()->getCurrentWeek(), weeklyModel );
    updateEntry( mInterface->getStatistics()->getCurrentMonth(), monthlyModel );
    updateEntry( mInterface->getStatistics()->getCurrentYear(), yearlyModel );
}

#include "interfacestatisticsdialog.moc"
