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

#include <QDBusInterface>
#include <QDBusMessage>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

#include <KCalendarSystem>
#include <KColorScheme>
#include <KConfigGroup>
#include <QInputDialog>
#include <kio/global.h>
#include <KMessageBox>
#include <KPluginFactory>
#include <KNotifyConfigWidget>
#include <Plasma/Theme>
#include <math.h>

#include "ui_configdlg.h"
#include "config-knemo.h"
#include "configdialog.h"
#include "statsconfig.h"
#include "warnconfig.h"
#include "themeconfig.h"
#include "utils.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netdb.h>

#ifdef __linux__
  #include <netlink/route/rtnl.h>
  #include <netlink/route/link.h>
  #include <netlink/route/route.h>
#endif


K_PLUGIN_FACTORY(KNemoFactory, registerPlugin<ConfigDialog>(QLatin1String("knemo"));)
K_EXPORT_PLUGIN(KNemoFactory("kcm_knemo"))

Q_DECLARE_METATYPE( KNemoTheme )
Q_DECLARE_METATYPE( StatsRule )
Q_DECLARE_METATYPE( WarnRule )


static bool themesLessThan( const KNemoTheme& s1, const KNemoTheme& s2 )
{
    if ( s1.name < s2.name )
        return true;
    else
        return false;
}

static QString periodText( int c, int u )
{
    QString units;
    switch ( u )
    {
        case KNemoStats::Hour:
            units = i18np( "%1 hour", "%1 hours", c );
            break;
        case KNemoStats::Day:
            units = i18np( "%1 day", "%1 days", c );
            break;
        case KNemoStats::Week:
            units = i18np( "%1 week", "%1 weeks", c );
            break;
        case KNemoStats::Month:
            units = i18np( "%1 month", "%1 months", c );
            break;
        case KNemoStats::BillPeriod:
            units = i18np( "%1 billing period", "%1 billing periods", c );
            break;
        case KNemoStats::Year:
            units = i18np( "%1 year", "%1 years", c );
            break;
        default:
            units = i18n( "Invalid period" );
            ;;
    }
    return units;
}

void StatsRuleModel::setCalendar( const KCalendarSystem *cal )
{
    mCalendar = cal;
}

QString StatsRuleModel::dateText( const StatsRule &s )
{
    QString dateStr = mCalendar->formatDate( s.startDate, KLocale::LongDate );
    if ( !mCalendar->isValid( s.startDate ) )
        dateStr = i18n( "Invalid Date" );
    return dateStr;
}

QList<StatsRule> StatsRuleModel::getRules()
{
    QList<StatsRule> statsRules;
    for ( int i = 0; i < rowCount(); ++i )
    {
        statsRules << item( i, 0 )->data( Qt::UserRole ).value<StatsRule>();
    }
    return statsRules;
}

QModelIndex StatsRuleModel::addRule( const StatsRule &s )
{
    QList<QStandardItem*> items;
    QStandardItem *item = new QStandardItem( dateText( s ) );
    QVariant v;
    v.setValue( s );
    item->setData( v, Qt::UserRole );
    item->setData( s.startDate, Qt::UserRole + 1 );
    items << item;

    item = new QStandardItem( periodText( s.periodCount, s.periodUnits ) );
    items << item;
    appendRow( items );
    return indexFromItem (items[0] );
}

void StatsRuleModel::modifyRule( const QModelIndex &index, const StatsRule &s )
{
    QVariant v;
    v.setValue( s );
    item( index.row(), 0 )->setData( v, Qt::UserRole );
    item( index.row(), 0 )->setData( s.startDate, Qt::UserRole + 1 );
    item( index.row(), 0 )->setData( dateText( s ), Qt::DisplayRole );
    item( index.row(), 1 )->setData( periodText( s.periodCount, s.periodUnits ), Qt::DisplayRole );
}

QString WarnModel::ruleText( const WarnRule &warn )
{
    QString warnText;
    quint64 siz = warn.threshold * pow( 1024, warn.trafficUnits );
    switch ( warn.trafficDirection )
    {
        case KNemoStats::TrafficIn:
            if ( warn.trafficType == KNemoStats::Peak )
                warnText = i18n( "peak incoming traffic > %1" ).arg( KIO::convertSize( siz ) );
            else if ( warn.trafficType == KNemoStats::Offpeak )
                warnText = i18n( "off-peak incoming traffic > %1" ).arg( KIO::convertSize( siz ) );
            else
                warnText = i18n( "incoming traffic > %1" ).arg( KIO::convertSize( siz ) );
            break;
        case KNemoStats::TrafficOut:
            if ( warn.trafficType == KNemoStats::Peak )
                warnText = i18n( "peak outgoing traffic > %1" ).arg( KIO::convertSize( siz ) );
            else if ( warn.trafficType == KNemoStats::Offpeak )
                warnText = i18n( "off-peak outgoing traffic > %1" ).arg( KIO::convertSize( siz ) );
            else
                warnText = i18n( "outgoing traffic > %1" ).arg( KIO::convertSize( siz ) );
            break;
        case KNemoStats::TrafficTotal:
            if ( warn.trafficType == KNemoStats::Peak )
                warnText = i18n( "peak incoming and outgoing traffic > %1" ).arg( KIO::convertSize( siz ) );
            else if ( warn.trafficType == KNemoStats::Offpeak )
                warnText = i18n( "off-peak incoming and outgoing traffic > %1" ).arg( KIO::convertSize( siz ) );
            else
                warnText = i18n( "incoming and outgoing traffic > %1" ).arg( KIO::convertSize( siz ) );
    }
    return warnText;
}

QList<WarnRule> WarnModel::getRules()
{
    QList<WarnRule> warnRules;
    for ( int i = 0; i < rowCount(); ++i )
    {
        warnRules << item( i, 0 )->data( Qt::UserRole ).value<WarnRule>();
    }
    return warnRules;
}

QModelIndex WarnModel::addWarn( const WarnRule &warn )
{
    QList<QStandardItem*> items;
    QStandardItem *item = new QStandardItem( ruleText( warn ) );
    QVariant v;
    v.setValue( warn );
    item->setData( v, Qt::UserRole );
    items << item;
    item = new QStandardItem( periodText( warn.periodCount, warn.periodUnits ) );
    items << item;
    appendRow( items );
    return indexFromItem( items[0] );
}

void WarnModel::modifyWarn( const QModelIndex &index, const WarnRule &warn )
{
    QVariant v;
    v.setValue( warn );
    item( index.row(), 0 )->setData( v, Qt::UserRole );
    item( index.row(), 0 )->setData( ruleText( warn ), Qt::DisplayRole );
    item( index.row(), 1 )->setData( periodText( warn.periodCount, warn.periodUnits ), Qt::DisplayRole );
}


ConfigDialog::ConfigDialog( QWidget *parent, const QVariantList &args )
    : KCModule( parent, args ),
      mLock( false ),
      mDlg( new Ui::ConfigDlg() ),
      mCalendar( 0 )
{
    migrateKde4Conf();

    setupToolTipMap();

    QWidget *main = new QWidget( this );
    QVBoxLayout* top = new QVBoxLayout( this );
    mDlg->setupUi( main );
    top->addWidget( main );
    statsModel = new StatsRuleModel( this );
    QStringList l;
    l << i18n( "Start Date" ) << i18n( "Period" );
    statsModel->setHorizontalHeaderLabels( l );
    QSortFilterProxyModel *proxy = new QSortFilterProxyModel( mDlg->statsView );
    proxy->setSourceModel( statsModel );
    proxy->setSortRole( Qt::UserRole + 1 );
    mDlg->statsView->setModel( proxy );
    mDlg->statsView->sortByColumn( 0, Qt::AscendingOrder );

    warnModel = new WarnModel( this );
    l.clear();
    l << i18n( "Alert" ) << i18n( "Period" );
    warnModel->setHorizontalHeaderLabels( l );
    mDlg->warnView->setModel( warnModel );

    QList<KNemoTheme> themes = findThemes();
    qSort( themes.begin(), themes.end(), themesLessThan );
    foreach ( KNemoTheme theme, themes )
        mDlg->comboBoxIconTheme->addItem( theme.name, QVariant::fromValue( theme ) );

    // We want these hardcoded and at the bottom of the list
    KNemoTheme systemTheme;
    systemTheme.name = i18n( "System Theme" );
    systemTheme.comment = i18n( "Use the current icon theme's network status icons" );
    systemTheme.internalName = SYSTEM_THEME;

    KNemoTheme textTheme;
    textTheme.name = i18n( "Text" );
    textTheme.comment = i18n( "KNemo theme that shows the upload/download speed as text" );
    textTheme.internalName = TEXT_THEME;

    KNemoTheme netloadTheme;
    netloadTheme.name = i18n( "Netload" );
    netloadTheme.comment = i18n( "KNemo theme that shows the upload/download speed as bar graphs" );
    netloadTheme.internalName = NETLOAD_THEME;

    // Leave this out for now.  Looks like none of the KDE icon themes provide
    // status/network-* icons.
    //mDlg->comboBoxIconTheme->addItem( systemTheme.name, QVariant::fromValue( systemTheme ) );
    mDlg->comboBoxIconTheme->addItem( netloadTheme.name, QVariant::fromValue( netloadTheme ) );
    mDlg->comboBoxIconTheme->addItem( textTheme.name, QVariant::fromValue( textTheme ) );

    InterfaceSettings s;
    int index = findIndexFromName( s.iconTheme );
    if ( index < 0 )
        index = findIndexFromName( TEXT_THEME );
    mDlg->comboBoxIconTheme->setCurrentIndex( index );

    for ( size_t i = 0; i < sizeof(pollIntervals)/sizeof(double); i++ )
        mDlg->comboBoxPoll->addItem( i18n( "%1 sec", pollIntervals[i] ), pollIntervals[i] );

    mDlg->pushButtonNew->setIcon( QIcon::fromTheme( QLatin1String("list-add") ) );
    mDlg->pushButtonAll->setIcon( QIcon::fromTheme( QLatin1String("document-new") ) );
    mDlg->pushButtonDelete->setIcon( QIcon::fromTheme( QLatin1String("list-remove") ) );
    mDlg->pushButtonAddToolTip->setIcon( QIcon::fromTheme( QLatin1String("arrow-right") ) );
    mDlg->pushButtonRemoveToolTip->setIcon( QIcon::fromTheme( QLatin1String("arrow-left") ) );

    setButtons( KCModule::Default | KCModule::Apply );

    connect( mDlg->checkBoxStartKNemo, SIGNAL( toggled( bool ) ),
             this, SLOT( checkBoxStartKNemoToggled( bool ) ) );

    // Interface
    connect( mDlg->listBoxInterfaces, SIGNAL( currentRowChanged( int ) ),
             this, SLOT( interfaceSelected( int ) ) );
    connect( mDlg->pushButtonNew, SIGNAL( clicked() ),
             this, SLOT( buttonNewSelected() ) );
    connect( mDlg->pushButtonAll, SIGNAL( clicked() ),
             this, SLOT( buttonAllSelected() ) );
    connect( mDlg->pushButtonDelete, SIGNAL( clicked() ),
             this, SLOT( buttonDeleteSelected() ) );

    // Interface - Icon Appearance
    connect( mDlg->comboHiding, SIGNAL( activated( int ) ),
             this, SLOT( comboHidingChanged( int ) ) );
    connect( mDlg->comboBoxIconTheme, SIGNAL( activated( int ) ),
             this, SLOT( iconThemeChanged( int ) ) );
    connect( mDlg->advancedButton, SIGNAL( clicked() ),
             this, SLOT( advancedButtonClicked() ) );

    // Interface - Statistics
    connect( mDlg->checkBoxStatistics, SIGNAL( toggled( bool ) ),
             this, SLOT( checkBoxStatisticsToggled ( bool ) ) );
    connect( mDlg->addStats, SIGNAL( clicked() ),
             this, SLOT( addStatsClicked() ) );
    connect( mDlg->modifyStats, SIGNAL( clicked() ),
             this, SLOT( modifyStatsClicked() ) );
    connect( mDlg->removeStats, SIGNAL( clicked() ),
             this, SLOT( removeStatsClicked() ) );
    connect( mDlg->addWarn, SIGNAL( clicked() ),
             this, SLOT( addWarnClicked() ) );
    connect( mDlg->modifyWarn, SIGNAL( clicked() ),
             this, SLOT( modifyWarnClicked() ) );
    connect( mDlg->removeWarn, SIGNAL( clicked() ),
             this, SLOT( removeWarnClicked() ) );

    // ToolTip
    connect( mDlg->pushButtonAddToolTip, SIGNAL( clicked() ),
             this, SLOT( buttonAddToolTipSelected() ) );
    connect( mDlg->pushButtonRemoveToolTip, SIGNAL( clicked() ),
             this, SLOT( buttonRemoveToolTipSelected() ) );

    // General
    connect( mDlg->pushButtonNotifications, SIGNAL( clicked() ),
             this, SLOT( buttonNotificationsSelected() ) );
    connect( mDlg->comboBoxPoll, SIGNAL( currentIndexChanged( int ) ),
             this, SLOT( changed() ) );
    connect( mDlg->numInputSaveInterval, SIGNAL( valueChanged( int ) ),
             this, SLOT( changed() ) );
    connect( mDlg->useBitrate, SIGNAL( toggled( bool ) ),
             this, SLOT( changed() ) );
}

ConfigDialog::~ConfigDialog()
{
    delete mDlg;
}

void ConfigDialog::load()
{
    mSettingsMap.clear();
    mDlg->listBoxInterfaces->clear();
    KSharedConfig::Ptr config = KSharedConfig::openConfig( QLatin1String("knemorc") );

    KConfigGroup generalGroup( config, confg_general );
    bool startKNemo = generalGroup.readEntry( conf_autoStart, true );
    mDlg->checkBoxStartKNemo->setChecked( startKNemo );
    GeneralSettings g;
    double pollVal = clamp<double>(generalGroup.readEntry( conf_pollInterval, g.pollInterval ), 0.1, 2.0 );
    pollVal = validatePoll( pollVal );
    int index = mDlg->comboBoxPoll->findData( pollVal );
    if ( index >= 0 )
        mDlg->comboBoxPoll->setCurrentIndex( index );
    mDlg->numInputSaveInterval->setValue( clamp<int>(generalGroup.readEntry( conf_saveInterval, g.saveInterval ), 0, 300 ) );
    mDlg->useBitrate->setChecked( generalGroup.readEntry( conf_useBitrate, g.useBitrate ) );
    mToolTipContent = generalGroup.readEntry( conf_toolTipContent, g.toolTipContent );

    QStringList list = generalGroup.readEntry( conf_interfaces, QStringList() );

    // Get defaults from the struct
    InterfaceSettings s;
    foreach ( QString interface, list )
    {
        QString group( confg_interface );
        group += interface;
        InterfaceSettings* settings = new InterfaceSettings();
        if ( config->hasGroup( group ) )
        {
            KConfigGroup interfaceGroup( config, group );
            settings->minVisibleState = interfaceGroup.readEntry( conf_minVisibleState, s.minVisibleState );
            settings->trafficThreshold = clamp<int>(interfaceGroup.readEntry( conf_trafficThreshold, s.trafficThreshold ), 0, 1000 );
            settings->iconTheme = interfaceGroup.readEntry( conf_iconTheme, s.iconTheme );
            KColorScheme scheme(QPalette::Active, KColorScheme::View);
            settings->barScale = interfaceGroup.readEntry( conf_barScale, s.barScale );
            settings->maxRate = interfaceGroup.readEntry( conf_maxRate, s.maxRate );
            settings->calendarSystem = static_cast<KLocale::CalendarSystem>(interfaceGroup.readEntry( conf_calendarSystem, static_cast<int>(KLocale::QDateCalendar) ));
            settings->activateStatistics = interfaceGroup.readEntry( conf_activateStatistics, s.activateStatistics );
            int statsRuleCount = interfaceGroup.readEntry( conf_statsRules, 0 );
            for ( int i = 0; i < statsRuleCount; ++i )
            {
                group = QString::fromLatin1( "%1%2 #%3" ).arg( confg_statsRule ).arg( interface ).arg( i );
                if ( config->hasGroup( group ) )
                {
                    KConfigGroup statsGroup( config, group );
                    StatsRule stats;

                    stats.startDate = statsGroup.readEntry( conf_statsStartDate, QDate() );
                    stats.periodUnits = clamp<int>(statsGroup.readEntry( conf_statsPeriodUnits, stats.periodUnits ), KNemoStats::Day, KNemoStats::Year );
                    stats.periodCount = clamp<int>(statsGroup.readEntry( conf_statsPeriodCount, stats.periodCount ), 1, 1000 );
                    stats.logOffpeak = statsGroup.readEntry( conf_logOffpeak,stats.logOffpeak );
                    stats.offpeakStartTime = QTime::fromString( statsGroup.readEntry( conf_offpeakStartTime, stats.offpeakStartTime.toString( Qt::ISODate ) ), Qt::ISODate );
                    stats.offpeakEndTime = QTime::fromString( statsGroup.readEntry( conf_offpeakEndTime, stats.offpeakEndTime.toString( Qt::ISODate ) ), Qt::ISODate );
                    stats.weekendIsOffpeak = statsGroup.readEntry( conf_weekendIsOffpeak, stats.weekendIsOffpeak );
                    stats.weekendDayStart = statsGroup.readEntry( conf_weekendDayStart, stats.weekendDayStart );
                    stats.weekendDayEnd = statsGroup.readEntry( conf_weekendDayEnd, stats.weekendDayEnd );
                    stats.weekendTimeStart = QTime::fromString( statsGroup.readEntry( conf_weekendTimeStart, stats.weekendTimeStart.toString( Qt::ISODate ) ), Qt::ISODate );
                    stats.weekendTimeEnd = QTime::fromString( statsGroup.readEntry( conf_weekendTimeEnd, stats.weekendTimeEnd.toString( Qt::ISODate ) ), Qt::ISODate );
                    settings->statsRules << stats;
                }
            }

            int warnRuleCount = interfaceGroup.readEntry( conf_warnRules, 0 );
            for ( int i = 0; i < warnRuleCount; ++i )
            {
                group = QString::fromLatin1( "%1%2 #%3" ).arg( confg_warnRule ).arg( interface ).arg( i );
                if ( config->hasGroup( group ) )
                {
                    KConfigGroup warnGroup( config, group );
                    WarnRule warn;

                    warn.periodUnits = clamp<int>(warnGroup.readEntry( conf_warnPeriodUnits, warn.periodUnits ), KNemoStats::Hour, KNemoStats::Year );
                    warn.periodCount = clamp<int>(warnGroup.readEntry( conf_warnPeriodCount, warn.periodCount ), 1, 1000 );
                    warn.trafficType = clamp<int>(warnGroup.readEntry( conf_warnTrafficType, warn.trafficType ), KNemoStats::Peak, KNemoStats::PeakOffpeak );
                    warn.trafficDirection = clamp<int>(warnGroup.readEntry( conf_warnTrafficDirection, warn.trafficDirection ), KNemoStats::TrafficIn, KNemoStats::TrafficTotal );
                    warn.trafficUnits = clamp<int>(warnGroup.readEntry( conf_warnTrafficUnits, warn.trafficUnits ), KNemoStats::UnitB, KNemoStats::UnitG );
                    warn.threshold = clamp<double>(warnGroup.readEntry( conf_warnThreshold, warn.threshold ), 0.0, 9999.0 );
                    warn.customText = warnGroup.readEntry( conf_warnCustomText, warn.customText ).trimmed();

                    settings->warnRules << warn;
                }
            }
        }
        mSettingsMap.insert( interface, settings );
        mDlg->listBoxInterfaces->addItem( interface );
        mDlg->pushButtonDelete->setEnabled( true );
        mDlg->ifaceTab->setEnabled( true );
    }

    // These things need to be here so that 'Reset' from the control
    // center is handled correctly.
    setupToolTipTab();

    // In case the user opened the control center via the context menu
    // this call to the daemon will deliver the interface the menu
    // belongs to. This way we can preselect the appropriate entry in the list.
    QString selectedInterface = QString::null;
    QDBusMessage reply = QDBusInterface(QLatin1String("org.kde.knemo"), QLatin1String("/knemo"), QLatin1String("org.kde.knemo")).call(QLatin1String("getSelectedInterface"));
    if ( reply.arguments().count() )
    {
        selectedInterface = reply.arguments().first().toString();
    }

    if ( selectedInterface != QString::null )
    {
        // Try to preselect the interface.
        int i;
        for ( i = 0; i < mDlg->listBoxInterfaces->count(); i++ )
        {
            if ( mDlg->listBoxInterfaces->item( i )->text() == selectedInterface )
            {
                // Found it.
                mDlg->listBoxInterfaces->setCurrentRow( i );
                break;
            }
        }
        if ( i == mDlg->listBoxInterfaces->count() )
        {
            // Not found. Select first entry in list.
            mDlg->listBoxInterfaces->setCurrentRow( 0 );
        }
    }
    else if ( mDlg->listBoxInterfaces->count() )
    {
        // No interface from KNemo. Select first entry in list.
        mDlg->listBoxInterfaces->setCurrentRow( 0 );
    }
}

void ConfigDialog::save()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig( QLatin1String("knemorc") );

    QStringList list;

    // Remove interfaces from the config that were deleted during this session
    foreach ( QString delIface, mDeletedIfaces )
    {
        if ( !mSettingsMap.contains( delIface ) )
        {
            config->deleteGroup( confg_interface + delIface );
            config->deleteGroup( confg_plotter + delIface );
        }
    }

    QStringList groupList = config->groupList();
    foreach ( QString tempDel, groupList )
    {
        if ( tempDel.contains( confg_statsRule ) || tempDel.contains( confg_warnRule ) )
            config->deleteGroup( tempDel );
    }

    foreach ( QString it, mSettingsMap.keys() )
    {
        list.append( it );
        InterfaceSettings* settings = mSettingsMap.value( it );
        KConfigGroup interfaceGroup( config, confg_interface + it );

        // Preserve settings set by the app before delete
        QPoint plotterPos = interfaceGroup.readEntry( conf_plotterPos, QPoint() );
        QSize plotterSize = interfaceGroup.readEntry( conf_plotterSize, QSize() );
        QPoint statisticsPos = interfaceGroup.readEntry( conf_statisticsPos, QPoint() );
        QSize statisticsSize = interfaceGroup.readEntry( conf_statisticsSize, QSize() );
        QPoint statusPos = interfaceGroup.readEntry( conf_statusPos, QPoint() );
        QSize statusSize = interfaceGroup.readEntry( conf_statusSize, QSize() );
        QByteArray hourState = interfaceGroup.readEntry( conf_hourState, QByteArray() );
        QByteArray dayState = interfaceGroup.readEntry( conf_dayState, QByteArray() );
        QByteArray weekState = interfaceGroup.readEntry( conf_weekState, QByteArray() );
        QByteArray monthState = interfaceGroup.readEntry( conf_monthState, QByteArray() );
        QByteArray billingState = interfaceGroup.readEntry( conf_billingState, QByteArray() );
        QByteArray yearState = interfaceGroup.readEntry( conf_yearState, QByteArray() );

        // Make sure we don't get crufty commands left over
        interfaceGroup.deleteGroup();

        if ( !plotterPos.isNull() )
            interfaceGroup.writeEntry( conf_plotterPos, plotterPos );
        if ( !plotterSize.isEmpty() )
            interfaceGroup.writeEntry( conf_plotterSize, plotterSize );
        if ( !statisticsPos.isNull() )
            interfaceGroup.writeEntry( conf_statisticsPos, statisticsPos );
        if ( !statisticsSize.isEmpty() )
            interfaceGroup.writeEntry( conf_statisticsSize, statisticsSize );
        if ( !statusPos.isNull() )
            interfaceGroup.writeEntry( conf_statusPos, statusPos );
        if ( !statusSize.isEmpty() )
            interfaceGroup.writeEntry( conf_statusSize, statusSize );
        if ( !hourState.isNull() )
            interfaceGroup.writeEntry( conf_hourState, hourState );
        if ( !dayState.isNull() )
            interfaceGroup.writeEntry( conf_dayState, dayState );
        if ( !weekState.isNull() )
            interfaceGroup.writeEntry( conf_weekState, weekState );
        if ( !monthState.isNull() )
            interfaceGroup.writeEntry( conf_monthState, monthState );
        if ( !billingState.isNull() )
            interfaceGroup.writeEntry( conf_billingState, billingState );
        if ( !yearState.isNull() )
            interfaceGroup.writeEntry( conf_yearState, yearState );

        interfaceGroup.writeEntry( conf_minVisibleState, settings->minVisibleState );
        interfaceGroup.writeEntry( conf_trafficThreshold, settings->trafficThreshold );
        interfaceGroup.writeEntry( conf_iconTheme, settings->iconTheme );
        if ( settings->iconTheme == TEXT_THEME ||
             settings->iconTheme == NETLOAD_THEME
           )
        {
            if ( settings->iconTheme == NETLOAD_THEME )
            {
                interfaceGroup.writeEntry( conf_barScale, settings->barScale );
            }
            if ( settings->iconTheme == NETLOAD_THEME && settings->barScale )
            {
                interfaceGroup.writeEntry( conf_maxRate, settings->maxRate );
            }
        }
        interfaceGroup.writeEntry( conf_activateStatistics, settings->activateStatistics );
        interfaceGroup.writeEntry( conf_calendarSystem, static_cast<int>(settings->calendarSystem) );
        interfaceGroup.writeEntry( conf_statsRules, settings->statsRules.count() );
        for ( int i = 0; i < settings->statsRules.count(); i++ )
        {
            QString group = QString::fromLatin1( "%1%2 #%3" ).arg( confg_statsRule ).arg( it ).arg( i );
            KConfigGroup statsGroup( config, group );
            statsGroup.writeEntry( conf_statsStartDate, settings->statsRules[i].startDate );
            statsGroup.writeEntry( conf_statsPeriodUnits, settings->statsRules[i].periodUnits );
            statsGroup.writeEntry( conf_statsPeriodCount, settings->statsRules[i].periodCount );
            statsGroup.writeEntry( conf_logOffpeak, settings->statsRules[i].logOffpeak );
            if ( settings->statsRules[i].logOffpeak )
            {
                statsGroup.writeEntry( conf_offpeakStartTime, settings->statsRules[i].offpeakStartTime.toString( Qt::ISODate ) );
                statsGroup.writeEntry( conf_offpeakEndTime, settings->statsRules[i].offpeakEndTime.toString( Qt::ISODate ) );
                statsGroup.writeEntry( conf_weekendIsOffpeak, settings->statsRules[i].weekendIsOffpeak );
                if ( settings->statsRules[i].weekendIsOffpeak )
                {
                    statsGroup.writeEntry( conf_weekendDayStart, settings->statsRules[i].weekendDayStart );
                    statsGroup.writeEntry( conf_weekendDayEnd, settings->statsRules[i].weekendDayEnd );
                    statsGroup.writeEntry( conf_weekendTimeStart, settings->statsRules[i].weekendTimeStart.toString( Qt::ISODate ) );
                    statsGroup.writeEntry( conf_weekendTimeEnd, settings->statsRules[i].weekendTimeEnd.toString( Qt::ISODate ) );
                }
            }
        }
        interfaceGroup.writeEntry( conf_warnRules, settings->warnRules.count() );
        for ( int i = 0; i < settings->warnRules.count(); i++ )
        {
            QString group = QString::fromLatin1( "%1%2 #%3" ).arg( confg_warnRule ).arg( it ).arg( i );
            KConfigGroup warnGroup( config, group );
            if ( settings->statsRules.count() == 0 && settings->warnRules[i].periodUnits == KNemoStats::BillPeriod )
            {
                warnGroup.writeEntry( conf_warnPeriodUnits, static_cast<int>(KNemoStats::Month) );
            }
            else
            {
                warnGroup.writeEntry( conf_warnPeriodUnits, settings->warnRules[i].periodUnits );
            }
            warnGroup.writeEntry( conf_warnPeriodCount, settings->warnRules[i].periodCount );
            warnGroup.writeEntry( conf_warnTrafficType, settings->warnRules[i].trafficType );
            warnGroup.writeEntry( conf_warnTrafficDirection, settings->warnRules[i].trafficDirection );
            warnGroup.writeEntry( conf_warnTrafficUnits, settings->warnRules[i].trafficUnits );
            warnGroup.writeEntry( conf_warnThreshold, settings->warnRules[i].threshold );
            warnGroup.writeEntry( conf_warnCustomText, settings->warnRules[i].customText.trimmed() );
        }
    }

    KConfigGroup generalGroup( config, confg_general );
    generalGroup.writeEntry( conf_firstStart, false );
    generalGroup.writeEntry( conf_autoStart, mDlg->checkBoxStartKNemo->isChecked() );
    generalGroup.writeEntry( conf_pollInterval, mDlg->comboBoxPoll->itemData( mDlg->comboBoxPoll->currentIndex() ).value<double>() );
    generalGroup.writeEntry( conf_saveInterval, mDlg->numInputSaveInterval->value() );
    generalGroup.writeEntry( conf_useBitrate, mDlg->useBitrate->isChecked() );
    generalGroup.writeEntry( conf_toolTipContent, mToolTipContent );
    generalGroup.writeEntry( conf_interfaces, list );

    config->sync();
    QDBusMessage reply = QDBusInterface(QLatin1String("org.kde.knemo"), QLatin1String("/knemo"), QLatin1String("org.kde.knemo")).call(QLatin1String("reparseConfiguration"));
}

void ConfigDialog::defaults()
{
    // Set these values before we check for default interfaces
    mSettingsMap.clear();
    mDlg->listBoxInterfaces->clear();
    mDlg->pushButtonDelete->setEnabled( false );

    InterfaceSettings emptySettings;
    updateControls( &emptySettings );

    // Default interface
    void *cache = NULL;

#ifdef __linux__
	struct nl_sock *rtsock = nl_socket_alloc();
	int c = nl_connect(rtsock, NETLINK_ROUTE);
    if ( c >= 0 )
    {
	    rtnl_route_alloc_cache( rtsock, AF_UNSPEC, 0, reinterpret_cast<nl_cache**>(&cache) );
    }
#endif

    QString interface = getDefaultRoute( AF_INET, NULL, cache );
    if ( interface.isEmpty() )
        interface = getDefaultRoute( AF_INET6, NULL, cache );
#ifdef __linux__
    nl_cache_free( static_cast<nl_cache*>(cache) );
    nl_close( rtsock );
    nl_socket_free( rtsock );
#endif

    if ( interface.isEmpty() )
    {
        mDlg->ifaceTab->setEnabled( false );
        mDlg->pixmapError->clear();
        mDlg->pixmapDisconnected->clear();
        mDlg->pixmapConnected->clear();
        mDlg->pixmapIncoming->clear();
        mDlg->pixmapOutgoing->clear();
        mDlg->pixmapTraffic->clear();
    }
    else
    {
        InterfaceSettings* settings = new InterfaceSettings();
        mSettingsMap.insert( interface, settings );
        mDlg->listBoxInterfaces->addItem( interface );
        mDlg->listBoxInterfaces->setCurrentRow( 0 );
        mDlg->pushButtonDelete->setEnabled( true );
        mDlg->ifaceTab->setEnabled( true );
    }

    // Default general settings
    GeneralSettings g;
    int index = mDlg->comboBoxPoll->findData( g.pollInterval );
    if ( index >= 0 )
        mDlg->comboBoxPoll->setCurrentIndex( index );
    mDlg->numInputSaveInterval->setValue( g.saveInterval );
    mDlg->useBitrate->setChecked( g.useBitrate );

    // Default tool tips
    mToolTipContent = g.toolTipContent;
    setupToolTipTab();

    changed( true );
}

void ConfigDialog::checkBoxStartKNemoToggled( bool on )
{
    if ( on )
    {
        KSharedConfig::Ptr config = KSharedConfig::openConfig( QLatin1String("knemorc") );
        KConfigGroup generalGroup( config, confg_general );
        if ( generalGroup.readEntry( conf_firstStart, true ) )
        {
            // Populate the dialog with some default values if the user starts
            // KNemo for the very first time.
            defaults();
        }
    }

    if (!mLock) changed( true );
}



/******************************************
 *                                        *
 * Interface tab                          *
 *                                        *
 ******************************************/

InterfaceSettings * ConfigDialog::getItemSettings()
{
    if ( !mDlg->listBoxInterfaces->currentItem() )
        return NULL;

    QListWidgetItem* selected = mDlg->listBoxInterfaces->currentItem();

    return mSettingsMap[selected->text()];
}

QString ConfigDialog::findNameFromIndex( int index )
{
    KNemoTheme theme = mDlg->comboBoxIconTheme->itemData( index ).value<KNemoTheme>();
    return theme.internalName;
}

int ConfigDialog::findIndexFromName( const QString& internalName )
{
    for( int i = 0; i < mDlg->comboBoxIconTheme->count(); i++ )
    {
        KNemoTheme theme = mDlg->comboBoxIconTheme->itemData( i ).value<KNemoTheme>();
        if ( theme.internalName == internalName )
            return i;
    }
    return -1;
}

void ConfigDialog::updateWarnText( int oldCount )
{
    // If the billing periods go away, the warn period will change to months
    // This only changes the text displayed in the model, so it can change
    // back if a billing period reappears.
    if ( ! statsModel->rowCount() )
    {
        QList<WarnRule> warnRules = warnModel->getRules();
        for ( int i = 0; i < warnRules.count(); ++i )
        {
            if ( warnRules[i].periodUnits == KNemoStats::BillPeriod )
            {
                warnModel->item( i, 1 )->setData( periodText( warnRules[i].periodCount, KNemoStats::Month ), Qt::DisplayRole );
            }
        }
    }
    else if ( oldCount == 0 )
    {
        QList<WarnRule> warnRules = warnModel->getRules();
        for ( int i = 0; i < warnRules.count(); ++i )
        {
            if ( warnRules[i].periodUnits == KNemoStats::BillPeriod )
                warnModel->item( i, 1 )->setData( periodText( warnRules[i].periodCount, warnRules[i].periodUnits ), Qt::DisplayRole );
        }
    }
}

void ConfigDialog::updateControls( InterfaceSettings *settings )
{
    mLock = true;
    int index = findIndexFromName( settings->iconTheme );
    if ( index < 0 )
        index = findIndexFromName( TEXT_THEME );
    mDlg->comboBoxIconTheme->setCurrentIndex( index );
    iconThemeChanged( index );
    switch ( settings->minVisibleState )
    {
        case KNemoIface::Connected:
            index = 1;
            break;
        case KNemoIface::Available:
            index = 2;
            break;
        case KNemoIface::MaxState:
            index = 3;
            break;
        default:
            index = 0;
    }

    mDlg->comboHiding->setCurrentIndex( index );
    comboHidingChanged( index );
    mDlg->checkBoxStatistics->setChecked( settings->activateStatistics );

    if ( !mCalendar || mCalendar->calendarSystem() != settings->calendarSystem )
        mCalendar = KCalendarSystem::create( settings->calendarSystem );

    statsModel->removeRows(0, statsModel->rowCount() );
    statsModel->setCalendar( mCalendar );
    foreach( StatsRule s, settings->statsRules )
    {
        statsModel->addRule( s );
    }
    if ( statsModel->rowCount() )
    {
        QSortFilterProxyModel* proxy = static_cast<QSortFilterProxyModel*>(mDlg->statsView->model());
        QModelIndex index = statsModel->indexFromItem( statsModel->item( 0, 0 ) );
        mDlg->statsView->setCurrentIndex( proxy->mapFromSource( index ) );
    }
    mDlg->modifyStats->setEnabled( statsModel->rowCount() );
    mDlg->removeStats->setEnabled( statsModel->rowCount() );

    warnModel->removeRows(0, warnModel->rowCount() );
    foreach( WarnRule warn, settings->warnRules )
    {
        warnModel->addWarn( warn );
    }
    updateWarnText( statsModel->rowCount() );

    mDlg->modifyWarn->setEnabled( warnModel->rowCount() );
    mDlg->removeWarn->setEnabled( warnModel->rowCount() );
    if ( warnModel->rowCount() )
    {
        mDlg->warnView->setCurrentIndex( warnModel->indexFromItem ( warnModel->item( 0, 0 ) ) );
    }

    mLock = false;
}

void ConfigDialog::interfaceSelected( int row )
{
    if ( row < 0 )
        return;
    QString interface = mDlg->listBoxInterfaces->item( row )->text();
    InterfaceSettings* settings = mSettingsMap[interface];
    mDlg->ifaceTab->setEnabled( true );
    updateControls( settings );
}

void ConfigDialog::buttonNewSelected()
{
    bool ok = false;
    QString ifname = QInputDialog::getText( this, i18n( "Add new interface" ),
                                            i18n( "Please enter the name of the interface to be monitored.\nIt should be something like 'eth1', 'wlan2' or 'ppp0'." ),
                                            QLineEdit::Normal,
                                            QString::null,
                                            &ok );

    if ( ok )
    {
        QListWidgetItem *item = new QListWidgetItem( ifname );
        mDlg->listBoxInterfaces->addItem( item );
        InterfaceSettings *settings = new InterfaceSettings();
        KColorScheme scheme(QPalette::Active, KColorScheme::View);
        mSettingsMap.insert( ifname, settings );
        mDlg->listBoxInterfaces->setCurrentRow( mDlg->listBoxInterfaces->row( item ) );
        mDlg->pushButtonDelete->setEnabled( true );
        changed( true );
    }
}

void ConfigDialog::buttonAllSelected()
{
    QStringList ifaces;

#ifdef __linux__
    nl_cache * linkCache = NULL;
    nl_sock *rtsock = nl_socket_alloc();
    int c = nl_connect(rtsock, NETLINK_ROUTE);
    if ( c >= 0 )
    {
        rtnl_link_alloc_cache( rtsock, AF_UNSPEC, &linkCache );

        struct rtnl_link * rtlink;
        for ( rtlink = reinterpret_cast<struct rtnl_link *>(nl_cache_get_first( linkCache ));
              rtlink != NULL;
              rtlink = reinterpret_cast<struct rtnl_link *>(nl_cache_get_next( reinterpret_cast<struct nl_object *>(rtlink) ))
            )
        {
            QString ifname( QLatin1String(rtnl_link_get_name( rtlink )) );
            ifaces << ifname;
        }
    }
    nl_cache_free( linkCache );
    nl_close( rtsock );
    nl_socket_free( rtsock );
#else
    struct ifaddrs *ifaddr;
    struct ifaddrs *ifa;
    getifaddrs( &ifaddr );
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        QString ifname( ifa->ifa_name );
        ifaces << ifname;
    }
    freeifaddrs( ifaddr );
#endif

    ifaces.removeAll( QLatin1String("lo") );
    ifaces.removeAll( QLatin1String("lo0") );

    const KColorScheme scheme(QPalette::Active, KColorScheme::View);
    foreach ( QString ifname, ifaces )
    {
        if ( mSettingsMap.contains( ifname ) )
            continue;
        InterfaceSettings* settings = new InterfaceSettings();
        mSettingsMap.insert( ifname, settings );
        mDlg->listBoxInterfaces->addItem( ifname );
    }

    if ( mDlg->listBoxInterfaces->count() > 0 )
    {
        mDlg->listBoxInterfaces->setCurrentRow( 0 );
        mDlg->pushButtonDelete->setEnabled( true );
        mDlg->ifaceTab->setEnabled( true );
        QString iface = mDlg->listBoxInterfaces->item( 0 )->text();
    }
    changed( true );
}

void ConfigDialog::buttonDeleteSelected()
{
    if ( !mDlg->listBoxInterfaces->currentItem() )
        return;

    QListWidgetItem* selected = mDlg->listBoxInterfaces->currentItem();

    // To prevent bloat when we save
    if ( !mDeletedIfaces.contains( selected->text() ) )
        mDeletedIfaces << selected->text();
    mSettingsMap.remove( selected->text() );

    QListWidgetItem *taken = mDlg->listBoxInterfaces->takeItem( mDlg->listBoxInterfaces->row( selected ) );
    delete taken;

    if ( mDlg->listBoxInterfaces->count() < 1 )
    {
        InterfaceSettings emptySettings;
        updateControls( &emptySettings );
        mDlg->pushButtonDelete->setEnabled( false );
        mDlg->ifaceTab->setEnabled( false );
        mDlg->pixmapError->clear();
        mDlg->pixmapDisconnected->clear();
        mDlg->pixmapConnected->clear();
        mDlg->pixmapIncoming->clear();
        mDlg->pixmapOutgoing->clear();
        mDlg->pixmapTraffic->clear();
    }
    changed( true );
}


/******************************************
 *                                        *
 * Interface tab - Icon Appearance        *
 *                                        *
 ******************************************/


void ConfigDialog::comboHidingChanged( int val )
{
    InterfaceSettings* settings = getItemSettings();
    if ( !settings )
        return;

    switch ( val )
    {
        case 0:
            // Do not hide
            settings->minVisibleState = KNemoIface::UnknownState;
            break;
        case 1:
            // Hide when disconnected
            settings->minVisibleState = KNemoIface::Connected;
            break;
        case 2:
            // Hide when unavailable
            settings->minVisibleState = KNemoIface::Available;
            break;
        case 3:
            // Always hide
            settings->minVisibleState = KNemoIface::MaxState;
            break;
    }

    if (!mLock) changed( true );
}

void ConfigDialog::iconThemeChanged( int set )
{
    InterfaceSettings* settings = getItemSettings();
    if ( !settings )
        return;

    KNemoTheme curTheme = mDlg->comboBoxIconTheme->itemData( mDlg->comboBoxIconTheme->currentIndex() ).value<KNemoTheme>();

    if ( curTheme.internalName == TEXT_THEME ||
         curTheme.internalName == NETLOAD_THEME )
    {
        if ( curTheme.internalName == TEXT_THEME )
        {
            QString f1 = QStringLiteral("0.0K");
            QString f2 = QStringLiteral("123K");
            QString f3 = QStringLiteral("12K");

            settings->iconTheme = TEXT_THEME;
            Plasma::Theme theme;
            mDlg->pixmapError->setPixmap( genTextIcon( f1, f1, theme.smallestFont(), KNemoIface::Unavailable ) );
            mDlg->pixmapDisconnected->setPixmap( genTextIcon( f1, f1, theme.smallestFont(), KNemoIface::Available ) );
            mDlg->pixmapConnected->setPixmap( genTextIcon( f1, f1, theme.smallestFont(), KNemoIface::Connected ) );
            mDlg->pixmapIncoming->setPixmap( genTextIcon( f2, f1, theme.smallestFont(), KNemoIface::Connected ) );
            mDlg->pixmapOutgoing->setPixmap( genTextIcon( f1, f3, theme.smallestFont(), KNemoIface::Connected ) );
            mDlg->pixmapTraffic->setPixmap( genTextIcon( f2, f3, theme.smallestFont(), KNemoIface::Connected ) );
        }
        else
        {
            settings->iconTheme = NETLOAD_THEME;
            mDlg->pixmapError->setPixmap( genBarIcon( 0, 0, KNemoIface::Unavailable ) );
            mDlg->pixmapDisconnected->setPixmap( genBarIcon( 0, 0, KNemoIface::Available ) );
            mDlg->pixmapConnected->setPixmap( genBarIcon( 0, 0, KNemoIface::Connected ) );
            mDlg->pixmapIncoming->setPixmap( genBarIcon( 0.75, 0, KNemoIface::Connected ) );
            mDlg->pixmapOutgoing->setPixmap( genBarIcon( 0, 0.75, KNemoIface::Connected ) );
            mDlg->pixmapTraffic->setPixmap( genBarIcon( 0.75, 0.75, KNemoIface::Connected ) );
            mDlg->pixmapError->setMinimumHeight(getIconSize().height());
        }
    }
    else
    {
        settings->iconTheme = findNameFromIndex( set );
        QString iconName;
        if ( settings->iconTheme == SYSTEM_THEME )
            iconName = QLatin1String("network-");
        else
            iconName = QLatin1String("knemo-") + settings->iconTheme + QLatin1Char('-');
        QSize iconSize = getIconSize();
        mDlg->pixmapError->setPixmap( QIcon::fromTheme( iconName + ICON_ERROR ).pixmap( iconSize ) );
        mDlg->pixmapDisconnected->setPixmap( QIcon::fromTheme( iconName + ICON_OFFLINE ).pixmap( iconSize ) );
        mDlg->pixmapConnected->setPixmap( QIcon::fromTheme( iconName + ICON_IDLE ).pixmap( iconSize ) );
        mDlg->pixmapIncoming->setPixmap( QIcon::fromTheme( iconName + ICON_RX ).pixmap( iconSize ) );
        mDlg->pixmapOutgoing->setPixmap( QIcon::fromTheme( iconName + ICON_TX ).pixmap( iconSize ) );
        mDlg->pixmapTraffic->setPixmap( QIcon::fromTheme( iconName + ICON_RX_TX ).pixmap( iconSize ) );
    }
    if (!mLock) changed( true );
}

void ConfigDialog::advancedButtonClicked()
{
    InterfaceSettings* settings = getItemSettings();
    if ( !settings )
        return;

    ThemeConfig dlg( *settings );
    if ( dlg.exec() )
    {
        InterfaceSettings s = dlg.settings();
        settings->trafficThreshold = s.trafficThreshold;

        settings->barScale = s.barScale;
        settings->maxRate = s.maxRate;

        changed( true );
    }
}

void ConfigDialog::addStatsClicked()
{
    InterfaceSettings* settings = getItemSettings();
    if ( !settings )
        return;

    StatsRule rule;
    int oldRuleCount = statsModel->rowCount();
    StatsConfig dlg( settings, mCalendar, rule, true );
    if ( dlg.exec() )
    {
        rule = dlg.settings();
        QSortFilterProxyModel* proxy = static_cast<QSortFilterProxyModel*>(mDlg->statsView->model());
        QModelIndex index = statsModel->addRule( rule );
        mDlg->statsView->setCurrentIndex( proxy->mapFromSource( index ) );
        settings->statsRules = statsModel->getRules();
        mDlg->modifyStats->setEnabled( true );
        mDlg->removeStats->setEnabled( true );
        updateWarnText( oldRuleCount );
        changed( true );
    }
}

void ConfigDialog::modifyStatsClicked()
{
    InterfaceSettings* settings = getItemSettings();
    if ( !settings || mDlg->statsView->model()->rowCount() < 1 )
        return;

    QModelIndex index = mDlg->statsView->selectionModel()->currentIndex();
    if ( !index.isValid() )
        return;
    QSortFilterProxyModel* proxy = static_cast<QSortFilterProxyModel*>(mDlg->statsView->model());
    index = proxy->mapToSource( index );
    StatsRule s = statsModel->item( index.row(), 0 )->data( Qt::UserRole ).value<StatsRule>();
    StatsConfig dlg( settings, mCalendar, s, false );
    if ( dlg.exec() )
    {
        s = dlg.settings();
        statsModel->modifyRule( index, s );
        settings->statsRules = statsModel->getRules();
        changed( true );
    }
}

void ConfigDialog::removeStatsClicked()
{
    InterfaceSettings* settings = getItemSettings();
    if ( !settings || mDlg->statsView->model()->rowCount() < 1 )
        return;

    QModelIndex index = mDlg->statsView->selectionModel()->currentIndex();
    if ( !index.isValid() )
        return;
    QSortFilterProxyModel* proxy = static_cast<QSortFilterProxyModel*>(mDlg->statsView->model());
    index = proxy->mapToSource( index );
    statsModel->removeRow( index.row() );
    settings->statsRules = statsModel->getRules();
    mDlg->modifyStats->setEnabled( statsModel->rowCount() );
    mDlg->removeStats->setEnabled( statsModel->rowCount() );
    updateWarnText( statsModel->rowCount() );
    changed( true );
}

void ConfigDialog::addWarnClicked()
{
    InterfaceSettings* settings = getItemSettings();
    if ( !settings )
        return;

    WarnRule warn;
    WarnConfig dlg( settings, warn, true );
    if ( dlg.exec() )
    {
        warn = dlg.settings();
        QModelIndex index = warnModel->addWarn( warn );
        mDlg->warnView->setCurrentIndex( index );
        settings->warnRules = warnModel->getRules();
        changed( true );
        mDlg->modifyWarn->setEnabled( true );
        mDlg->removeWarn->setEnabled( true );
    }
}

void ConfigDialog::modifyWarnClicked()
{
    InterfaceSettings* settings = getItemSettings();
    if ( !settings || mDlg->warnView->model()->rowCount() < 1 )
        return;

    const QModelIndex index = mDlg->warnView->selectionModel()->currentIndex();
    if ( !index.isValid() )
        return;
    WarnRule warn = mDlg->warnView->model()->data( index.sibling( index.row(), 0 ), Qt::UserRole ).value<WarnRule>();
    WarnConfig dlg( settings, warn, false );
    if ( dlg.exec() )
    {
        warn = dlg.settings();
        warnModel->modifyWarn( index, warn );
        settings->warnRules = warnModel->getRules();
        changed( true );
    }
}

void ConfigDialog::removeWarnClicked()
{
    InterfaceSettings* settings = getItemSettings();
    if ( !settings || mDlg->warnView->model()->rowCount() < 1 )
        return;

    const QModelIndex index = mDlg->warnView->selectionModel()->currentIndex();
    if ( !index.isValid() )
        return;
    warnModel->removeRow( index.row() );
    settings->warnRules = warnModel->getRules();
    mDlg->modifyWarn->setEnabled( warnModel->rowCount() );
    mDlg->removeWarn->setEnabled( warnModel->rowCount() );
    changed( true );
}



/******************************************
 *                                        *
 * Interface tab - Statistics             *
 *                                        *
 ******************************************/

void ConfigDialog::checkBoxStatisticsToggled( bool on )
{
    InterfaceSettings* settings = getItemSettings();
    if ( !settings )
        return;

    settings->activateStatistics = on;
    if (!mLock) changed( true );
}



/******************************************
 *                                        *
 * ToolTip tab                            *
 *                                        *
 ******************************************/

void ConfigDialog::setupToolTipMap()
{
    // Cannot make this data static as the i18n macro doesn't seem
    // to work when called to early i.e. before setting the catalogue.
    mToolTips.insert( INTERFACE, i18n( "Interface" ) );
    mToolTips.insert( STATUS, i18n( "Status" ) );
    mToolTips.insert( UPTIME, i18n( "Connection Time" ) );
    mToolTips.insert( IP_ADDRESS, i18n( "IP Address" ) );
    mToolTips.insert( SCOPE, i18n( "Scope & Flags" ) );
    mToolTips.insert( HW_ADDRESS, i18n( "MAC Address" ) );
    mToolTips.insert( BCAST_ADDRESS, i18n( "Broadcast Address" ) );
    mToolTips.insert( GATEWAY, i18n( "Default Gateway" ) );
    mToolTips.insert( PTP_ADDRESS, i18n( "PtP Address" ) );
    mToolTips.insert( RX_PACKETS, i18n( "Packets Received" ) );
    mToolTips.insert( TX_PACKETS, i18n( "Packets Sent" ) );
    mToolTips.insert( RX_BYTES, i18n( "Bytes Received" ) );
    mToolTips.insert( TX_BYTES, i18n( "Bytes Sent" ) );
    mToolTips.insert( DOWNLOAD_SPEED, i18n( "Download Speed" ) );
    mToolTips.insert( UPLOAD_SPEED, i18n( "Upload Speed" ) );
    mToolTips.insert( ESSID, i18n( "ESSID" ) );
    mToolTips.insert( MODE, i18n( "Mode" ) );
    mToolTips.insert( FREQUENCY, i18n( "Frequency" ) );
    mToolTips.insert( BIT_RATE, i18n( "Bit Rate" ) );
    mToolTips.insert( ACCESS_POINT, i18n( "Access Point" ) );
    mToolTips.insert( LINK_QUALITY, i18n( "Link Quality" ) );
#ifndef __linux__
    mToolTips.insert( NICK_NAME, i18n( "Nickname" ) );
#endif
    mToolTips.insert( ENCRYPTION, i18n( "Encryption" ) );
}

void ConfigDialog::setupToolTipTab()
{
    mDlg->listBoxDisplay->clear();
    mDlg->listBoxAvailable->clear();

    foreach ( QString tip, mToolTips )
    {
        if ( mToolTipContent & mToolTips.key( tip ) )
            mDlg->listBoxDisplay->addItem( tip );
        else
            mDlg->listBoxAvailable->addItem( tip );
    }

    if ( mDlg->listBoxDisplay->count() > 0 )
        mDlg->listBoxDisplay->item( 0 )->setSelected( true );

    if ( mDlg->listBoxAvailable->count() > 0 )
        mDlg->listBoxAvailable->item( 0 )->setSelected( true );

    mDlg->pushButtonRemoveToolTip->setEnabled( (mDlg->listBoxDisplay->count() > 0) );
    mDlg->pushButtonAddToolTip->setEnabled( (mDlg->listBoxAvailable->count() > 0) );
}

void ConfigDialog::moveTips( QListWidget *from, QListWidget* to )
{
    QList<QListWidgetItem *> selectedItems = from->selectedItems();

    foreach ( QListWidgetItem *selected, selectedItems )
    {
        quint32 key = mToolTips.key( selected->text() );

        int newIndex = -1;
        int count = to->count();
        for ( int i = 0; i < count; i++ )
        {
            QListWidgetItem *item = to->item( i );
            if ( mToolTips.key( item->text() ) > key )
            {
                newIndex = i;
                break;
            }
        }
        if ( newIndex < 0 )
            newIndex = count;

        selected->setSelected( false );
        from->takeItem( from->row( selected ) );
        to->insertItem( newIndex, selected );
        mDlg->pushButtonAddToolTip->setEnabled( (mDlg->listBoxAvailable->count() > 0) );
        mDlg->pushButtonRemoveToolTip->setEnabled( (mDlg->listBoxDisplay->count() > 0) );
        changed( true );
    }
    mToolTipContent = 0;
    for ( int i = 0; i < mDlg->listBoxDisplay->count(); i++ )
        mToolTipContent += mToolTips.key( mDlg->listBoxDisplay->item( i )->text() );
}

void ConfigDialog::buttonAddToolTipSelected()
{
    // Support extended selection
    if ( mDlg->listBoxAvailable->count() == 0 )
        return;

    moveTips( mDlg->listBoxAvailable, mDlg->listBoxDisplay );
}

void ConfigDialog::buttonRemoveToolTipSelected()
{
    // Support extended selection
    if ( mDlg->listBoxDisplay->count() == 0 )
        return;

    moveTips( mDlg->listBoxDisplay, mDlg->listBoxAvailable );
}



/******************************************
 *                                        *
 * General tab                            *
 *                                        *
 ******************************************/

void ConfigDialog::buttonNotificationsSelected()
{
    KNotifyConfigWidget::configure( this, QLatin1String("knemo") );
}

#include "configdialog.moc"
