/* This file is part of KNemo
   Copyright (C) 2004, 2006 Percy Leonhardt <percy@eris23.de>
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

#include <QDBusInterface>
#include <QPainter>

#include <KCalendarSystem>
#include <KColorScheme>
#include <KGenericFactory>
#include <KInputDialog>
#include <KMessageBox>
#include <KNotifyConfigWidget>
#include <KStandardDirs>

#include "ui_configdlg.h"
#include "config-knemo.h"
#include "configdialog.h"
#include "utils.h"

#include <math.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netdb.h>

#ifdef __linux__
  #include <netlink/route/rtnl.h>
  #include <netlink/route/link.h>
  #include <netlink/route/route.h>
#endif


K_PLUGIN_FACTORY(KNemoFactory, registerPlugin<ConfigDialog>();)
K_EXPORT_PLUGIN(KNemoFactory("kcm_knemo"))

Q_DECLARE_METATYPE( KNemoTheme )


static bool themesLessThan( const KNemoTheme& s1, const KNemoTheme& s2 )
{
    if ( s1.name < s2.name )
        return true;
    else
        return false;
}

ConfigDialog::ConfigDialog( QWidget *parent, const QVariantList &args )
    : KCModule( KNemoFactory::componentData(), parent, args ),
      mLock( false ),
      mDlg( new Ui::ConfigDlg() ),

      // If we're going to change KGlobal::locale()->calendar() we're
      // going to need to track the original calendar type.
      // TODO: Some of the calendars are a bit buggy, so default to Gregorian for now
      //mDefaultCalendarType( KGlobal::locale()->calendarType() ),
      mDefaultCalendarType( "gregorian" )
{
    mConfig = KSharedConfig::openConfig( "knemorc" );

    setupToolTipMap();

    QWidget *main = new QWidget( this );
    QVBoxLayout* top = new QVBoxLayout( this );
    mDlg->setupUi( main );
    top->addWidget( main );

    QList<KNemoTheme> themes = findThemes();
    qSort( themes.begin(), themes.end(), themesLessThan );
    foreach ( KNemoTheme theme, themes )
        mDlg->comboBoxIconTheme->addItem( theme.name, QVariant::fromValue( theme ) );

    // We want these hardcoded and at the bottom of the list
    KNemoTheme systemTheme;
    systemTheme.name = i18n( "System Theme" );
    systemTheme.comment = i18n( "Use the current icon theme's network status icons" );
    systemTheme.internalName = SYSTEM_THEME;
    // Leave this out for now.  Looks like none of the KDE icon themes provide
    // status/network-* icons.
    //mDlg->comboBoxIconTheme->addItem( systemTheme.name, QVariant::fromValue( systemTheme ) );
    KNemoTheme textTheme;
    textTheme.name = i18n( "Text Icon" );
    textTheme.comment = i18n( "KNemo theme that shows text of upload/download speed" );
    textTheme.internalName = TEXT_THEME;
    mDlg->comboBoxIconTheme->addItem( textTheme.name, QVariant::fromValue( textTheme ) );

    InterfaceSettings s;
    int index = findIndexFromName( s.iconTheme );
    if ( index < 0 )
        index = findIndexFromName( TEXT_THEME );
    mDlg->comboBoxIconTheme->setCurrentIndex( index );

    for ( size_t i = 0; i < sizeof(pollIntervals)/sizeof(double); i++ )
        mDlg->comboBoxPoll->addItem( i18n( "%1 sec", pollIntervals[i] ), pollIntervals[i] );

    mDlg->pushButtonNew->setIcon( SmallIcon( "list-add" ) );
    mDlg->pushButtonAll->setIcon( SmallIcon( "document-new" ) );
    mDlg->pushButtonDelete->setIcon( SmallIcon( "list-remove" ) );
    mDlg->pushButtonAddCommand->setIcon( SmallIcon( "list-add" ) );
    mDlg->pushButtonRemoveCommand->setIcon( SmallIcon( "list-remove" ) );
    mDlg->pushButtonUp->setIcon( SmallIcon( "arrow-up" ) );
    mDlg->pushButtonDown->setIcon( SmallIcon( "arrow-down" ) );
    mDlg->pushButtonAddToolTip->setIcon( SmallIcon( "arrow-right" ) );
    mDlg->pushButtonRemoveToolTip->setIcon( SmallIcon( "arrow-left" ) );

    mDlg->colorIncomingLabel->hide();
    mDlg->colorIncoming->hide();
    mDlg->colorOutgoingLabel->hide();
    mDlg->colorOutgoing->hide();
    mDlg->colorDisabledLabel->hide();
    mDlg->colorDisabled->hide();
    mDlg->colorUnavailable->hide();
    mDlg->colorUnavailable->hide();

    //mDlg->listViewCommands->setSorting( -1 );

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
    connect( mDlg->lineEditAlias, SIGNAL( textChanged( const QString& ) ),
             this, SLOT( aliasChanged( const QString& ) ) );

    // Interface - Icon Appearance
    connect( mDlg->checkBoxDisconnected, SIGNAL( toggled( bool ) ),
             this, SLOT( checkBoxDisconnectedToggled ( bool ) ) );
    connect( mDlg->checkBoxUnavailable, SIGNAL( toggled( bool ) ),
             this, SLOT( checkBoxUnavailableToggled ( bool ) ) );
    connect( mDlg->spinBoxTrafficThreshold, SIGNAL( valueChanged( int ) ),
             this, SLOT( spinBoxTrafficValueChanged ( int ) ) );
    connect( mDlg->comboBoxIconTheme, SIGNAL( activated( int ) ),
             this, SLOT( iconThemeChanged( int ) ) );
    connect( mDlg->colorIncoming, SIGNAL( changed( const QColor& ) ),
             this, SLOT( colorButtonChanged() ) );
    connect( mDlg->colorOutgoing, SIGNAL( changed( const QColor& ) ),
             this, SLOT( colorButtonChanged() ) );
    connect( mDlg->colorDisabled, SIGNAL( changed( const QColor& ) ),
             this, SLOT( colorButtonChanged() ) );
    connect( mDlg->colorUnavailable, SIGNAL( changed( const QColor& ) ),
             this, SLOT( colorButtonChanged() ) );

    // Interface - Statistics
    connect( mDlg->checkBoxStatistics, SIGNAL( toggled( bool ) ),
             this, SLOT( checkBoxStatisticsToggled ( bool ) ) );
    connect( mDlg->checkBoxCustomBilling, SIGNAL( toggled( bool ) ),
             this, SLOT( checkBoxCustomBillingToggled( bool ) ) );
    connect( mDlg->billingStartInput, SIGNAL( dateEntered( const QDate& ) ),
             this, SLOT( billingStartInputChanged( const QDate& ) ) );
    connect( mDlg->billingMonthsInput, SIGNAL( valueChanged( int ) ),
             this, SLOT( billingMonthsInputChanged( int ) ) );
    connect( mDlg->warnThreshold, SIGNAL( valueChanged( double ) ),
             this, SLOT( warnThresholdChanged ( double ) ) );
    connect( mDlg->warnRxTx, SIGNAL( toggled( bool ) ),
             this, SLOT( warnRxTxToggled ( bool ) ) );

    // Interface - Context Menu
    connect( mDlg->checkBoxCustom, SIGNAL( toggled( bool ) ),
             this, SLOT( checkBoxCustomToggled ( bool ) ) );
    connect( mDlg->listViewCommands, SIGNAL( currentItemChanged( QTreeWidgetItem*, QTreeWidgetItem* ) ),
             this, SLOT( listViewCommandsSelectionChanged( QTreeWidgetItem*, QTreeWidgetItem* ) ) );
    connect( mDlg->listViewCommands, SIGNAL( itemChanged( QTreeWidgetItem*, int ) ),
             this, SLOT( listViewCommandsChanged( QTreeWidgetItem*, int ) ) );
    connect( mDlg->pushButtonAddCommand, SIGNAL( clicked() ),
             this, SLOT( buttonAddCommandSelected() ) );
    connect( mDlg->pushButtonRemoveCommand, SIGNAL( clicked() ),
             this, SLOT( buttonRemoveCommandSelected() ) );
    connect( mDlg->pushButtonUp, SIGNAL( clicked() ),
             this, SLOT( buttonCommandUpSelected() ) );
    connect( mDlg->pushButtonDown, SIGNAL( clicked() ),
             this, SLOT( buttonCommandDownSelected() ) );

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
}

ConfigDialog::~ConfigDialog()
{
    delete mDlg;
}

void ConfigDialog::setMaxDay()
{
    QDate date = QDate::currentDate();
    mMaxDay = mCalendar->daysInMonth( date );
    if ( mCalendar->isLeapYear( date ) )
        date = date.addDays( 0 - mCalendar->dayOfYear( date ) );
    int months = mCalendar->monthsInYear( date );
    for ( int i = 1; i < months; i++ )
    {
        QDate month;
        mCalendar->setYMD( month, mCalendar->year( date ), i, 1 );
        int days = mCalendar->daysInMonth( month );
        if ( days < mMaxDay )
            mMaxDay = days;
    }
}

void ConfigDialog::load()
{
    mSettingsMap.clear();
    mDlg->listBoxInterfaces->clear();
    KConfig *config = mConfig.data();

    KConfigGroup generalGroup( config, confg_general );
    bool startKNemo = generalGroup.readEntry( conf_autoStart, true );
    mDlg->checkBoxStartKNemo->setChecked( startKNemo );
    double pollVal = clamp<double>(generalGroup.readEntry( conf_pollInterval, 1.0 ), 0.1, 2.0 );
    pollVal = validatePoll( pollVal );
    int index = mDlg->comboBoxPoll->findData( pollVal );
    if ( index >= 0 )
        mDlg->comboBoxPoll->setCurrentIndex( index );
    mDlg->numInputSaveInterval->setValue( clamp<int>(generalGroup.readEntry( conf_saveInterval, 60 ), 0, 300 ) );
    mDlg->lineEditStatisticsDir->setUrl( generalGroup.readEntry( conf_statisticsDir, KGlobal::dirs()->saveLocation( "data", "knemo/" ) ) );
    mToolTipContent = generalGroup.readEntry( conf_toolTipContent, defaultTip );

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
            settings->alias = interfaceGroup.readEntry( conf_alias ).trimmed();
            settings->hideWhenDisconnected = interfaceGroup.readEntry( conf_hideWhenNotAvail, s.hideWhenDisconnected );
            settings->hideWhenUnavailable = interfaceGroup.readEntry( conf_hideWhenNotExist, s.hideWhenUnavailable );
            settings->trafficThreshold = clamp<int>(interfaceGroup.readEntry( conf_trafficThreshold, s.trafficThreshold ), 0, 1000 );
            settings->iconTheme = interfaceGroup.readEntry( conf_iconTheme, s.iconTheme );
            settings->colorIncoming = interfaceGroup.readEntry( conf_colorIncoming, s.colorIncoming );
            settings->colorOutgoing = interfaceGroup.readEntry( conf_colorOutgoing, s.colorOutgoing );
            KColorScheme scheme(QPalette::Active, KColorScheme::View);
            settings->colorDisabled = interfaceGroup.readEntry( conf_colorDisabled, scheme.foreground( KColorScheme::InactiveText ).color() );
            settings->colorUnavailable = interfaceGroup.readEntry( conf_colorUnavailable, scheme.foreground( KColorScheme::InactiveText ).color() );
            settings->activateStatistics = interfaceGroup.readEntry( conf_activateStatistics, s.activateStatistics );
            settings->customBilling = interfaceGroup.readEntry( conf_customBilling, s.customBilling );
            settings->calendar = interfaceGroup.readEntry( conf_calendar, mDefaultCalendarType );
            settings->billingMonths = clamp<int>(interfaceGroup.readEntry( conf_billingMonths, s.billingMonths ), 1, 6 );
            settings->warnThreshold = clamp<double>(interfaceGroup.readEntry( conf_billingWarnThresh, s.warnThreshold ), 0.0, 9999.0 );
            settings->warnTotalTraffic = interfaceGroup.readEntry( conf_billingWarnRxTx, s.warnTotalTraffic );

             // If no start date saved, default to first of month.
            mCalendar = KCalendarSystem::create( settings->calendar );
            QDate startDate = QDate::currentDate().addDays( 1 - mCalendar->day( QDate::currentDate() ) );
            settings->billingStart = interfaceGroup.readEntry( conf_billingStart, startDate );

            // If date is saved but very old, update it to current period
            QDate currentDate = QDate::currentDate();
            QDate nextMonthStart = settings->billingStart;
            if ( nextMonthStart <= currentDate )
            {
                int length = settings->billingMonths;
                while ( nextMonthStart <= currentDate )
                {
                    settings->billingStart = nextMonthStart;
                    for ( int i = 0; i < length; i++ )
                        nextMonthStart = nextMonthStart.addDays( mCalendar->daysInMonth( nextMonthStart ) );
                }
            }

            settings->customCommands = interfaceGroup.readEntry( conf_customCommands, s.customCommands );
            int numCommands = interfaceGroup.readEntry( conf_numCommands, s.numCommands );
            for ( int i = 0; i < numCommands; i++ )
            {
                QString entry;
                InterfaceCommand cmd;
                entry = QString( "%1%2" ).arg( conf_runAsRoot ).arg( i + 1 );
                cmd.runAsRoot = interfaceGroup.readEntry( entry, false );
                entry = QString( "%1%2" ).arg( conf_command ).arg( i + 1 );
                cmd.command = interfaceGroup.readEntry( entry );
                entry = QString( "%1%2" ).arg( conf_menuText ).arg( i + 1 );
                cmd.menuText = interfaceGroup.readEntry( entry );
                settings->commands.append( cmd );
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
    QDBusMessage reply = QDBusInterface("org.kde.knemo", "/knemo", "org.kde.knemo").call("getSelectedInterface");
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
    KConfig *config = mConfig.data();

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
        if ( !settings->alias.trimmed().isEmpty() )
            interfaceGroup.writeEntry( conf_alias, settings->alias );

        interfaceGroup.writeEntry( conf_hideWhenNotAvail, settings->hideWhenDisconnected );
        interfaceGroup.writeEntry( conf_hideWhenNotExist, settings->hideWhenUnavailable );
        interfaceGroup.writeEntry( conf_trafficThreshold, settings->trafficThreshold );
        interfaceGroup.writeEntry( conf_iconTheme, settings->iconTheme );
        if ( settings->iconTheme == TEXT_THEME )
        {
            interfaceGroup.writeEntry( conf_colorIncoming, settings->colorIncoming );
            interfaceGroup.writeEntry( conf_colorOutgoing, settings->colorOutgoing );
            interfaceGroup.writeEntry( conf_colorDisabled, settings->colorDisabled );
            interfaceGroup.writeEntry( conf_colorUnavailable, settings->colorUnavailable );
        }
        interfaceGroup.writeEntry( conf_activateStatistics, settings->activateStatistics );
        interfaceGroup.writeEntry( conf_customBilling, settings->customBilling );
        if ( settings->customBilling )
        {
            interfaceGroup.writeEntry( conf_billingStart, mDlg->billingStartInput->date() );
            interfaceGroup.writeEntry( conf_billingMonths, settings->billingMonths );
            interfaceGroup.writeEntry( conf_calendar, settings->calendar );
        }
        interfaceGroup.writeEntry( conf_billingWarnThresh, settings->warnThreshold );
        interfaceGroup.writeEntry( conf_billingWarnRxTx, settings->warnTotalTraffic );

        interfaceGroup.writeEntry( conf_customCommands, settings->customCommands );
        interfaceGroup.writeEntry( conf_numCommands, settings->commands.size() );
        for ( int i = 0; i < settings->commands.size(); i++ )
        {
            QString entry;
            entry = QString( "%1%2" ).arg( conf_runAsRoot ).arg( i + 1 );
            interfaceGroup.writeEntry( entry, settings->commands[i].runAsRoot );
            entry = QString( "%1%2" ).arg( conf_command ).arg( i + 1 );
            interfaceGroup.writeEntry( entry, settings->commands[i].command );
            entry = QString( "%1%2" ).arg( conf_menuText ).arg( i + 1 );
            interfaceGroup.writeEntry( entry, settings->commands[i].menuText );
        }
    }

    KConfigGroup generalGroup( config, confg_general );
    generalGroup.writeEntry( conf_firstStart, false );
    generalGroup.writeEntry( conf_autoStart, mDlg->checkBoxStartKNemo->isChecked() );
    generalGroup.writeEntry( conf_pollInterval, mDlg->comboBoxPoll->itemData( mDlg->comboBoxPoll->currentIndex() ).value<double>() );
    generalGroup.writeEntry( conf_saveInterval, mDlg->numInputSaveInterval->value() );
    generalGroup.writeEntry( conf_statisticsDir,  mDlg->lineEditStatisticsDir->url().url() );
    generalGroup.writeEntry( conf_toolTipContent, mToolTipContent );
    generalGroup.writeEntry( conf_interfaces, list );

    config->sync();
    QDBusMessage reply = QDBusInterface("org.kde.knemo", "/knemo", "org.kde.knemo").call("reparseConfiguration");
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
	nl_handle *rtsock = nl_handle_alloc();
	int c = nl_connect(rtsock, NETLINK_ROUTE);
    if ( c >= 0 )
    {
	    cache = rtnl_route_alloc_cache( rtsock );
    }
#endif

    QString interface = getDefaultRoute( AF_INET, NULL, cache );
    if ( interface.isEmpty() )
        interface = getDefaultRoute( AF_INET6, NULL, cache );
#ifdef __linux__
    nl_cache_free( static_cast<nl_cache*>(cache) );
    nl_close( rtsock );
    nl_handle_destroy( rtsock );
#endif

    if ( interface.isEmpty() )
    {
        mDlg->aliasLabel->setEnabled( false );
        mDlg->lineEditAlias->setEnabled( false );
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
        QDate startDate = QDate::currentDate().addDays( 1 - mCalendar->day( QDate::currentDate() ) );
        settings->billingStart = startDate;
        KColorScheme scheme(QPalette::Active, KColorScheme::View);
        settings->colorDisabled = scheme.foreground( KColorScheme::InactiveText ).color();
        settings->colorUnavailable = scheme.foreground( KColorScheme::InactiveText ).color();
        mSettingsMap.insert( interface, settings );
        mDlg->listBoxInterfaces->addItem( interface );
        mDlg->listBoxInterfaces->setCurrentRow( 0 );
        mDlg->pushButtonDelete->setEnabled( true );
        mDlg->aliasLabel->setEnabled( false );
        mDlg->lineEditAlias->setEnabled( false );
        mDlg->ifaceTab->setEnabled( true );
    }

    // Default general settings
    int index = mDlg->comboBoxPoll->findData( 1.0 );
    if ( index >= 0 )
        mDlg->comboBoxPoll->setCurrentIndex( index );
    mDlg->numInputSaveInterval->setValue( 60 );
    mDlg->lineEditStatisticsDir->setUrl( KGlobal::dirs()->saveLocation( "data", "knemo/" ) );

    // Default tool tips
    mToolTipContent = defaultTip;
    setupToolTipTab();

    changed( true );
}

void ConfigDialog::checkBoxStartKNemoToggled( bool on )
{
    if ( on )
    {
        KConfig *config = mConfig.data();
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

void ConfigDialog::updateControls( InterfaceSettings *settings )
{
    mLock = true;
    mDlg->lineEditAlias->setText( settings->alias );
    int index = findIndexFromName( settings->iconTheme );
    if ( index < 0 )
        index = findIndexFromName( TEXT_THEME );
    mDlg->comboBoxIconTheme->setCurrentIndex( index );
    iconThemeChanged( index );
    mDlg->colorIncoming->setColor( settings->colorIncoming );
    mDlg->colorOutgoing->setColor( settings->colorOutgoing );
    mDlg->colorDisabled->setColor( settings->colorDisabled );
    mDlg->colorUnavailable->setColor( settings->colorUnavailable );
    mDlg->checkBoxCustom->setChecked( settings->customCommands );
    mDlg->checkBoxDisconnected->setChecked( settings->hideWhenDisconnected );
    mDlg->checkBoxUnavailable->setChecked( settings->hideWhenUnavailable );
    mDlg->checkBoxStatistics->setChecked( settings->activateStatistics );
    mDlg->warnThreshold->setValue( settings->warnThreshold );
    if ( settings->warnTotalTraffic )
        mDlg->warnRxTx->setChecked( true );
    else
        mDlg->warnRx->setChecked( true );
    if ( settings->customBilling )
        mDlg->checkBoxCustomBilling->setChecked( true );
    else
        mDlg->checkBoxCustomBilling->setChecked( false );

    if ( settings->calendar.isEmpty() )
    {
        // TODO: Test for a KDE release that contains SVN commit 1013534
        KGlobal::locale()->setCalendar( mDefaultCalendarType );

        mCalendar = KCalendarSystem::create( mDefaultCalendarType );
    }
    else
    {
        // TODO: Test for a KDE release that contains SVN commit 1013534
        KGlobal::locale()->setCalendar( settings->calendar );

        mCalendar = KCalendarSystem::create( settings->calendar );
    }
    setMaxDay();
    mDlg->billingStartInput->setDate( settings->billingStart );
    mDlg->spinBoxTrafficThreshold->setValue( settings->trafficThreshold );

    mDlg->listViewCommands->clear();
    QList<QTreeWidgetItem *>items;
    foreach ( InterfaceCommand command, settings->commands )
    {
        QTreeWidgetItem* item = new QTreeWidgetItem();
        enum Qt::CheckState checkState = Qt::Unchecked;
        if ( command.runAsRoot )
            checkState = Qt::Checked;
        item->setCheckState( 0, checkState );
        item->setFlags( item->flags() | Qt::ItemIsEditable );
        item->setText( 1, command.menuText );
        item->setText( 2, command.command );
        items << item;
    }
    mDlg->listViewCommands->addTopLevelItems( items );
    mLock = false;
}

void ConfigDialog::interfaceSelected( int row )
{
    if ( row < 0 )
        return;
    QString interface = mDlg->listBoxInterfaces->item( row )->text();
    InterfaceSettings* settings = mSettingsMap[interface];
    mDlg->ifaceTab->setEnabled( true );
    mDlg->aliasLabel->setEnabled( true );
    mDlg->lineEditAlias->setEnabled( true );
    updateControls( settings );
}

void ConfigDialog::buttonNewSelected()
{
    bool ok = false;
    QString ifname = KInputDialog::getText( i18n( "Add new interface" ),
                                            i18n( "Please enter the name of the interface to be monitored.\nIt should be something like 'eth1', 'wlan2' or 'ppp0'." ),
                                            QString::null,
                                            &ok );

    if ( ok )
    {
        QListWidgetItem *item = new QListWidgetItem( ifname );
        mDlg->listBoxInterfaces->addItem( item );
        InterfaceSettings *settings = new InterfaceSettings();
        mCalendar = KCalendarSystem::create( mDefaultCalendarType );
        settings->billingStart = QDate::currentDate().addDays( 1 - mCalendar->day( QDate::currentDate() ) );
        KColorScheme scheme(QPalette::Active, KColorScheme::View);
        settings->colorDisabled = scheme.foreground( KColorScheme::InactiveText ).color();
        settings->colorUnavailable = scheme.foreground( KColorScheme::InactiveText ).color();
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
    nl_handle *rtsock = nl_handle_alloc();
    int c = nl_connect(rtsock, NETLINK_ROUTE);
    if ( c >= 0 )
    {
        linkCache = rtnl_link_alloc_cache( rtsock );

        struct rtnl_link * rtlink;
        for ( rtlink = reinterpret_cast<struct rtnl_link *>(nl_cache_get_first( linkCache ));
              rtlink != NULL;
              rtlink = reinterpret_cast<struct rtnl_link *>(nl_cache_get_next( reinterpret_cast<struct nl_object *>(rtlink) ))
            )
        {
            QString ifname( rtnl_link_get_name( rtlink ) );
            ifaces << ifname;
        }
    }
    nl_cache_free( linkCache );
    nl_close( rtsock );
    nl_handle_destroy( rtsock );
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

    ifaces.removeAll( "lo" );
    ifaces.removeAll( "lo0" );

    foreach ( QString ifname, ifaces )
    {
        if ( mSettingsMap.contains( ifname ) )
            continue;
        InterfaceSettings* settings = new InterfaceSettings();
        mCalendar = KCalendarSystem::create( mDefaultCalendarType );
        settings->billingStart = QDate::currentDate().addDays( 1 - mCalendar->day( QDate::currentDate() ) );
        KColorScheme scheme(QPalette::Active, KColorScheme::View);
        settings->colorDisabled = scheme.foreground( KColorScheme::InactiveText ).color();
        settings->colorUnavailable = scheme.foreground( KColorScheme::InactiveText ).color();
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
        mDlg->aliasLabel->setEnabled( false );
        mDlg->lineEditAlias->setEnabled( false );
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

void ConfigDialog::aliasChanged( const QString& text )
{
    InterfaceSettings* settings = getItemSettings();
    if ( !settings )
        return;

    settings->alias = text;
    if (!mLock) changed( true );
}


/******************************************
 *                                        *
 * Interface tab - Icon Appearance        *
 *                                        *
 ******************************************/

QPixmap ConfigDialog::textIcon( QString incomingText, QString outgoingText, int status )
{
    QPixmap sampleIcon( 22, 22 );
    sampleIcon.fill( Qt::transparent );
    QRect topRect( 0, 0, 22, 11 );
    QRect bottomRect( 0, 11, 22, 11 );
    QPainter p( &sampleIcon );
    p.setBrush( Qt::NoBrush );
    p.setOpacity( 1.0 );
    QFont rxFont = setIconFont( incomingText, 22 );
    QFont txFont = setIconFont( outgoingText, 22 );
    if ( rxFont.pointSizeF() > txFont.pointSizeF() )
        rxFont.setPointSizeF( txFont.pointSizeF() );
    p.setFont( rxFont );
    if ( status >= KNemoIface::Connected )
        p.setPen( mDlg->colorIncoming->color() );
    else if ( status == KNemoIface::Available )
        p.setPen( mDlg->colorDisabled->color() );
    else
        p.setPen( mDlg->colorUnavailable->color() );
    p.drawText( topRect, Qt::AlignCenter | Qt::AlignRight, incomingText );
    p.setFont( rxFont );
    if ( status >= KNemoIface::Connected )
        p.setPen( mDlg->colorOutgoing->color() );
    p.drawText( bottomRect, Qt::AlignCenter | Qt::AlignRight, outgoingText );
    return sampleIcon;
}

void ConfigDialog::checkBoxDisconnectedToggled( bool on )
{
    InterfaceSettings* settings = getItemSettings();
    if ( !settings )
        return;

    settings->hideWhenDisconnected = on;
    if (!mLock) changed( true );
}

void ConfigDialog::checkBoxUnavailableToggled( bool on )
{
    InterfaceSettings* settings = getItemSettings();
    if ( !settings )
        return;

    settings->hideWhenUnavailable = on;
    if (!mLock) changed( true );
}

void ConfigDialog::spinBoxTrafficValueChanged( int value )
{
    InterfaceSettings* settings = getItemSettings();
    if ( !settings )
        return;

    settings->trafficThreshold = value;
    if (!mLock) changed( true );
}

void ConfigDialog::iconThemeChanged( int set )
{
    InterfaceSettings* settings = getItemSettings();
    if ( !settings )
        return;

    KNemoTheme curTheme = mDlg->comboBoxIconTheme->itemData( mDlg->comboBoxIconTheme->currentIndex() ).value<KNemoTheme>();
    if ( curTheme.internalName == TEXT_THEME )
    {
        settings->iconTheme = TEXT_THEME;
        mDlg->pixmapError->setPixmap( textIcon( "0.0K", "0.0K", KNemoIface::Unavailable ) );
        mDlg->pixmapDisconnected->setPixmap( textIcon( "0.0K", "0.0K", KNemoIface::Available ) );
        mDlg->pixmapConnected->setPixmap( textIcon( "0.0K", "0.0K", KNemoIface::Connected ) );
        mDlg->pixmapIncoming->setPixmap( textIcon( "123K", "0.0K", KNemoIface::Connected ) );
        mDlg->pixmapOutgoing->setPixmap( textIcon( "0.0K", "12K", KNemoIface::Connected ) );
        mDlg->pixmapTraffic->setPixmap( textIcon( "123K", "12K", KNemoIface::Connected ) );
        mDlg->colorIncoming->show();
        mDlg->colorIncomingLabel->show();
        mDlg->colorOutgoing->show();
        mDlg->colorOutgoingLabel->show();
        mDlg->colorDisabled->show();
        mDlg->colorDisabledLabel->show();
        mDlg->colorUnavailable->show();
        mDlg->colorUnavailableLabel->show();
    }
    else
    {
        settings->iconTheme = findNameFromIndex( set );
        QString iconName;
        if ( settings->iconTheme == SYSTEM_THEME )
            iconName = "network-";
        else
            iconName = "knemo-" + settings->iconTheme + "-";
        mDlg->pixmapError->setPixmap( KIcon( iconName + ICON_ERROR ).pixmap( 22 ) );
        mDlg->pixmapDisconnected->setPixmap( KIcon( iconName + ICON_OFFLINE ).pixmap( 22 ) );
        mDlg->pixmapConnected->setPixmap( KIcon( iconName + ICON_IDLE ).pixmap( 22 ) );
        mDlg->pixmapIncoming->setPixmap( KIcon( iconName + ICON_RX ).pixmap( 22 ) );
        mDlg->pixmapOutgoing->setPixmap( KIcon( iconName + ICON_TX ).pixmap( 22 ) );
        mDlg->pixmapTraffic->setPixmap( KIcon( iconName + ICON_RX_TX ).pixmap( 22 ) );
        mDlg->colorIncoming->hide();
        mDlg->colorIncomingLabel->hide();
        mDlg->colorOutgoing->hide();
        mDlg->colorOutgoingLabel->hide();
        mDlg->colorDisabled->hide();
        mDlg->colorDisabledLabel->hide();
        mDlg->colorUnavailable->hide();
        mDlg->colorUnavailableLabel->hide();
    }
    if (!mLock) changed( true );
}

void ConfigDialog::colorButtonChanged()
{
    InterfaceSettings* settings = getItemSettings();
    if ( !settings )
        return;

    if ( mDlg->colorIncoming->color().isValid() )
        settings->colorIncoming = mDlg->colorIncoming->color();
    if ( mDlg->colorOutgoing->color().isValid() )
        settings->colorOutgoing = mDlg->colorOutgoing->color();
    if ( mDlg->colorDisabled->color().isValid() )
        settings->colorDisabled = mDlg->colorDisabled->color();
    if ( mDlg->colorUnavailable->color().isValid() )
        settings->colorUnavailable = mDlg->colorUnavailable->color();

    KNemoTheme curTheme = mDlg->comboBoxIconTheme->itemData( mDlg->comboBoxIconTheme->currentIndex() ).value<KNemoTheme>();
    if ( curTheme.internalName == TEXT_THEME )
        iconThemeChanged( mDlg->comboBoxIconTheme->currentIndex() );
    if ( !mLock) changed( true );
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

void ConfigDialog::checkBoxCustomBillingToggled( bool on )
{
    InterfaceSettings* settings = getItemSettings();
    if ( !settings )
        return;

    settings->customBilling = on;
    mDlg->billingStartInput->setDate( QDate::currentDate().addDays( 1 - mCalendar->day( QDate::currentDate() ) ) );
    mDlg->billingMonthsInput->setValue( 1 );
    if (!mLock) changed( true );
}

void ConfigDialog::billingStartInputChanged( const QDate& date )
{
    InterfaceSettings* settings = getItemSettings();
    if ( !settings )
        return;

    // KDateEdit doesn't guarantee a valid date
    if ( !date.isValid() ||
         date > QDate::currentDate() ||
         mCalendar->day( date ) > mMaxDay )
    {
        KMessageBox::error( this, i18n( "The billing day of the month can be any day from 1 - %1, and the complete date must be a valid, non-future date.", QString::number( mMaxDay ) ) );
        mDlg->billingStartInput->setDate( settings->billingStart );
    }
    else
    {
        settings->billingStart = date;
        if (!mLock) changed( true );
    }
}

void ConfigDialog::billingMonthsInputChanged( int value )
{
    InterfaceSettings* settings = getItemSettings();
    if ( !settings )
        return;

    settings->billingMonths = value;
    if (!mLock) changed( true );
}

void ConfigDialog::warnThresholdChanged( double val )
{
    InterfaceSettings* settings = getItemSettings();
    if ( !settings )
        return;

    settings->warnThreshold = round(val*10.0)/10.0;
    bool enable = settings->warnThreshold > 0.0;
    mDlg->warnRx->setEnabled( enable );
    mDlg->warnRxTx->setEnabled( enable );
    if (!mLock) changed( true );
}

void ConfigDialog::warnRxTxToggled( bool on )
{
    InterfaceSettings* settings = getItemSettings();
    if ( !settings )
        return;

    settings->warnTotalTraffic = on;
    if (!mLock) changed( true );
}



/******************************************
 *                                        *
 * Interface tab - Context Menu           *
 *                                        *
 ******************************************/

void ConfigDialog::checkBoxCustomToggled( bool on )
{
    InterfaceSettings* settings = getItemSettings();
    if ( !settings )
        return;

    settings->customCommands = on;
    if ( on )
    {
        if ( mDlg->listViewCommands->currentItem() )
            mDlg->pushButtonRemoveCommand->setEnabled( true );
    }
    else
        mDlg->pushButtonRemoveCommand->setEnabled( false );

    if (!mLock) changed( true );
}

void ConfigDialog::buttonAddCommandSelected()
{
    InterfaceSettings* settings = getItemSettings();
    if ( !settings )
        return;

    InterfaceCommand cmd;
    cmd.runAsRoot = false;
    cmd.menuText = QString();
    cmd.command = QString();
    settings->commands.append( cmd );

    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setCheckState( 0, Qt::Unchecked );
    item->setFlags( item->flags() | Qt::ItemIsEditable );
    mDlg->listViewCommands->addTopLevelItem( item );

    if (!mLock) changed( true );
}

void ConfigDialog::buttonRemoveCommandSelected()
{
    InterfaceSettings* settings = getItemSettings();
    if ( !settings )
        return;

    if ( !mDlg->listViewCommands->currentItem() )
        return;

    QTreeWidgetItem *item = mDlg->listViewCommands->currentItem();
    int index = mDlg->listViewCommands->indexOfTopLevelItem( item );
    mDlg->listViewCommands->takeTopLevelItem( index );
    delete item;

    QList<InterfaceCommand> cmds;
    QTreeWidgetItemIterator i( mDlg->listViewCommands );
    while ( QTreeWidgetItem * item = *i )
    {
        InterfaceCommand cmd;
        cmd.runAsRoot = item->checkState( 0 );
        cmd.menuText = item->text( 1 );
        cmd.command = item->text( 2 );
        cmds.append( cmd );
        ++i;
    }

    settings->commands = cmds;
    if (!mLock) changed( true );
}

void ConfigDialog::buttonCommandUpSelected()
{
    InterfaceSettings* settings = getItemSettings();
    if ( !settings )
        return;

    if ( !mDlg->listViewCommands->currentItem() )
        return;

    QTreeWidgetItem* item = mDlg->listViewCommands->currentItem();
    int index = mDlg->listViewCommands->indexOfTopLevelItem( item );
    if ( index == 0 )
        return;

    mDlg->listViewCommands->takeTopLevelItem( index );
    mDlg->listViewCommands->insertTopLevelItem( index - 1, item );
    mDlg->listViewCommands->setCurrentItem( item );

    QList<InterfaceCommand> cmds;
    QTreeWidgetItemIterator i( mDlg->listViewCommands );
    while ( QTreeWidgetItem * item = *i )
    {
        InterfaceCommand cmd;
        cmd.runAsRoot = item->checkState( 0 );
        cmd.menuText = item->text( 1 );
        cmd.command = item->text( 2 );
        cmds.append( cmd );
        ++i;
    }

    settings->commands = cmds;
    if (!mLock) changed( true );
}

void ConfigDialog::buttonCommandDownSelected()
{
    InterfaceSettings* settings = getItemSettings();
    if ( !settings )
        return;

    if ( !mDlg->listViewCommands->currentItem() )
        return;

    QTreeWidgetItem* item = mDlg->listViewCommands->currentItem();
    int index = mDlg->listViewCommands->indexOfTopLevelItem( item );
    if ( index == mDlg->listViewCommands->topLevelItemCount() - 1 )
        return;

    mDlg->listViewCommands->takeTopLevelItem( index );
    mDlg->listViewCommands->insertTopLevelItem( index + 1, item );
    mDlg->listViewCommands->setCurrentItem( item );

    QList<InterfaceCommand> cmds;
    QTreeWidgetItemIterator i( mDlg->listViewCommands );
    while ( QTreeWidgetItem * item = *i )
    {
        InterfaceCommand cmd;
        cmd.runAsRoot = item->checkState( 0 );
        cmd.menuText = item->text( 1 );
        cmd.command = item->text( 2 );
        cmds.append( cmd );
        ++i;
    }

    settings->commands = cmds;
    if (!mLock) changed( true );
}

void ConfigDialog::listViewCommandsSelectionChanged( QTreeWidgetItem* item, QTreeWidgetItem* )
{
    mDlg->pushButtonRemoveCommand->setEnabled( item != NULL );
}

void ConfigDialog::listViewCommandsChanged( QTreeWidgetItem* item, int column )
{
    InterfaceSettings* settings = getItemSettings();
    if ( !settings )
        return;

    int row = mDlg->listViewCommands->indexOfTopLevelItem( item );
    InterfaceCommand& cmd = settings->commands[row];
    switch ( column )
    {
        case 0:
            cmd.runAsRoot = item->checkState( 0 );
            break;
        case 1:
            cmd.menuText = item->text( 1 );
            break;
        case 2:
            cmd.command = item->text( 2 );
    }

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
#ifndef USE_KNOTIFICATIONITEM
    mToolTips.insert( ALIAS, i18n( "Alias" ) );
#endif
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
    KNotifyConfigWidget::configure( this, "knemo" );
}

#include "configdialog.moc"
