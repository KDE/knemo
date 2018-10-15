/* This file is part of KNemo
   Copyright (C) 2006 Percy Leonhardt <percy@eris23.de>
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

#include <QStandardItemModel>

#include <kio/global.h>
#include <KMessageBox>
#include <KSharedConfig>
#include <KConfigGroup>
#include <QPushButton>
#include <QSortFilterProxyModel>

#include "data.h"
#include "interface.h"
#include "interfacestatistics.h"
#include "interfacestatisticsdialog.h"
#include "statisticsmodel.h"


InterfaceStatisticsDialog::InterfaceStatisticsDialog( Interface* interface, QWidget* parent )
    : QDialog( parent ),
      mWasShown( false ),
      mSetPos( true ),
      mInterface( interface )
{
    setWindowTitle( i18n( "%1 Statistics", interface->ifaceName() ) );

    ui.setupUi( this );

    mBillingWidget = new QWidget();
    QVBoxLayout *bl = new QVBoxLayout( mBillingWidget );
    mBillingView = new StatisticsView( mBillingWidget );
    mBillingView->setEditTriggers( QAbstractItemView::NoEditTriggers );
    mBillingView->setSortingEnabled( true );
    mBillingView->horizontalHeader()->setStretchLastSection( true );
    mBillingView->verticalHeader()->setVisible( false );
    bl->addWidget( mBillingView );

    mStateKeys.insert( ui.tableHourly, conf_hourState );
    mStateKeys.insert( ui.tableDaily, conf_dayState );
    mStateKeys.insert( ui.tableWeekly, conf_weekState );
    mStateKeys.insert( ui.tableMonthly, conf_monthState );
    mStateKeys.insert( ui.tableYearly, conf_yearState );
    mStateKeys.insert( mBillingView, conf_billingState );

    configChanged();

    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup interfaceGroup( config, confg_interface + mInterface->ifaceName() );

    InterfaceStatistics *stat = mInterface->ifaceStatistics();
    setupTable( &interfaceGroup, ui.tableHourly,  stat->getStatistics( KNemoStats::Hour ) );
    setupTable( &interfaceGroup, ui.tableDaily,   stat->getStatistics( KNemoStats::Day ) );
    setupTable( &interfaceGroup, ui.tableWeekly,  stat->getStatistics( KNemoStats::Week ) );
    setupTable( &interfaceGroup, ui.tableMonthly, stat->getStatistics( KNemoStats::Month ) );
    setupTable( &interfaceGroup, ui.tableYearly,  stat->getStatistics( KNemoStats::Year ) );
    setupTable( &interfaceGroup, mBillingView,    stat->getStatistics( KNemoStats::BillPeriod ) );

    connect( ui.buttonBox, SIGNAL( rejected() ), SLOT( reject() ) );
    connect( ui.buttonBox, SIGNAL( clicked( QAbstractButton* ) ), SLOT( confirmReset( QAbstractButton* ) ) );

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
        KSharedConfig::Ptr config = KSharedConfig::openConfig();
        KConfigGroup interfaceGroup( config, confg_interface + mInterface->ifaceName() );

        interfaceGroup.writeEntry( conf_statisticsPos, pos() );
        interfaceGroup.writeEntry( conf_statisticsSize, size() );

        QHashIterator<QTableView*, QString> i( mStateKeys );
        while ( i.hasNext() )
        {
            i.next();
            interfaceGroup.writeEntry( i.value(), i.key()->horizontalHeader()->saveState() );
        }

        config->sync();
    }
}

void InterfaceStatisticsDialog::configChanged()
{
    bool billingTab = false;
    bool logOffpeak = false;
    KCalendarSystem *cal = mInterface->ifaceStatistics()->calendar();
    foreach ( StatsRule rule, mInterface->settings().statsRules )
    {
        if ( rule.periodCount != 1 ||
             rule.periodUnits != KNemoStats::Month ||
             cal->day( rule.startDate ) != 1 )
        {
            billingTab = true;
        }
        if ( rule.logOffpeak )
            logOffpeak = true;
    }

    ui.tableHourly->haveOffpeak( logOffpeak );
    ui.tableDaily->haveOffpeak( logOffpeak );
    ui.tableWeekly->haveOffpeak( logOffpeak );
    ui.tableMonthly->haveOffpeak( logOffpeak );
    ui.tableYearly->haveOffpeak( logOffpeak );
    mBillingView->haveOffpeak( logOffpeak );

    if ( billingTab && ui.tabWidget->count() < 6 )
    {
        ui.tabWidget->insertTab( 4, mBillingWidget, i18n( "Billing Periods" ) );
    }
    else if ( !billingTab && ui.tabWidget->count() > 5 )
       ui.tabWidget->removeTab( 4 );
}

void InterfaceStatisticsDialog::setupTable( KConfigGroup* group, QTableView *view, StatisticsModel *model )
{
    QSortFilterProxyModel *proxy = new QSortFilterProxyModel( view );
    proxy->setSourceModel( model );
    proxy->setSortRole( StatisticsModel::DataRole );
    view->setModel( proxy );
    view->sortByColumn( 0, Qt::AscendingOrder );

    QModelIndex sourceIndex = proxy->sourceModel()->index( proxy->rowCount() - 1, 0 );
    view->selectionModel()->setCurrentIndex( proxy->mapFromSource( sourceIndex ), QItemSelectionModel::NoUpdate );

    connect( model, SIGNAL( itemChanged( QStandardItem * ) ), view->viewport(), SLOT( update() ) );
    connect( proxy, SIGNAL( rowsInserted( const QModelIndex&, int, int ) ), this, SLOT( setCurrentSel() ) );

    QByteArray state = group->readEntry( mStateKeys.value( view ), QByteArray() );
    if ( state.isNull() )
        view->resizeColumnsToContents();
    else
        view->horizontalHeader()->restoreState( state );
    proxy->setDynamicSortFilter( true );
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
    if ( e->type() == QEvent::Show && !mWasShown )
    {
        mWasShown = true;
        // Set this here!  For some reason the QTableView in the first tab
        // will become ridiculously wide if we set this earlier.
        ui.tableHourly->horizontalHeader()->setStretchLastSection( true );
    }

    return QDialog::event( e );
}

void InterfaceStatisticsDialog::confirmReset( QAbstractButton* button)
{
    if (static_cast<QPushButton*>(button) == ui.buttonBox->button(QDialogButtonBox::Reset) ) {
        if ( KMessageBox::questionYesNo( this, i18n( "Do you want to reset all statistics?" ) ) == KMessageBox::Yes )
            emit clearStatistics();
    }
}

void InterfaceStatisticsDialog::setCurrentSel()
{
    QSortFilterProxyModel *proxy = static_cast<QSortFilterProxyModel*>( sender() );
    QTableView *tv = static_cast<QTableView*>( sender()->parent() );
    QModelIndex sourceIndex = proxy->sourceModel()->index( proxy->rowCount() - 1, 0 );

    tv->selectionModel()->setCurrentIndex( proxy->mapFromSource( sourceIndex ), QItemSelectionModel::NoUpdate );
}


#include "moc_interfacestatisticsdialog.cpp"
