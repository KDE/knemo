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

#include <sys/socket.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <QDBusInterface>
#include <qlabel.h>
#include <qlayout.h>
#include <qcheckbox.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qwhatsthis.h>
#include <qtabwidget.h>
#include <QPainter>
#include <qpushbutton.h>
#include <qstringlist.h>
#include <qdatastream.h>

#include <KColorScheme>
#include <kglobal.h>
#include <KGlobalSettings>
#include <KCalendarSystem>
#include <kconfig.h>
#include <klocale.h>
#include <knuminput.h>
#include <kiconloader.h>
#include <kfiledialog.h>
#include <kcolorbutton.h>
#include <kinputdialog.h>
#include <kapplication.h>
#include <KMessageBox>
#include <KNotifyConfigWidget>
#include <kstandarddirs.h>
#include <kgenericfactory.h>
#include <kdirselectdialog.h>
#include <math.h>

#include "ui_configdlg.h"
#include "config-knemo.h"
#include "configdialog.h"
#include "utils.h"

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

    int index = findIndexFromName( "monitor" );
    if ( index < 0 )
        index = findIndexFromName( TEXT_THEME );
    mDlg->comboBoxIconTheme->setCurrentIndex( index );

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

    //mDlg->listViewCommands->setSorting( -1 );

    setButtons( KCModule::Default | KCModule::Apply );

    connect( mDlg->pushButtonNew, SIGNAL( clicked() ),
             this, SLOT( buttonNewSelected() ) );
    connect( mDlg->pushButtonAll, SIGNAL( clicked() ),
             this, SLOT( buttonAllSelected() ) );
    connect( mDlg->pushButtonDelete, SIGNAL( clicked() ),
             this, SLOT( buttonDeleteSelected() ) );
    connect( mDlg->pushButtonAddCommand, SIGNAL( clicked() ),
             this, SLOT( buttonAddCommandSelected() ) );
    connect( mDlg->pushButtonRemoveCommand, SIGNAL( clicked() ),
             this, SLOT( buttonRemoveCommandSelected() ) );
    connect( mDlg->pushButtonUp, SIGNAL( clicked() ),
             this, SLOT( buttonCommandUpSelected() ) );
    connect( mDlg->pushButtonDown, SIGNAL( clicked() ),
             this, SLOT( buttonCommandDownSelected() ) );
    connect( mDlg->pushButtonAddToolTip, SIGNAL( clicked() ),
             this, SLOT( buttonAddToolTipSelected() ) );
    connect( mDlg->pushButtonRemoveToolTip, SIGNAL( clicked() ),
             this, SLOT( buttonRemoveToolTipSelected() ) );
    connect( mDlg->pushButtonNotifications, SIGNAL( clicked() ),
             this, SLOT( buttonNotificationsSelected() ) );
    connect( mDlg->lineEditAlias, SIGNAL( textChanged( const QString& ) ),
             this, SLOT( aliasChanged( const QString& ) ) );
    connect( mDlg->comboBoxIconTheme, SIGNAL( activated( int ) ),
             this, SLOT( iconThemeChanged( int ) ) );
    connect( mDlg->colorIncoming, SIGNAL( changed( const QColor& ) ),
             this, SLOT( colorButtonChanged() ) );
    connect( mDlg->colorOutgoing, SIGNAL( changed( const QColor& ) ),
             this, SLOT( colorButtonChanged() ) );
    connect( mDlg->colorDisabled, SIGNAL( changed( const QColor& ) ),
             this, SLOT( colorButtonChanged() ) );
    connect( mDlg->checkBoxDisconnected, SIGNAL( toggled( bool ) ),
             this, SLOT( checkBoxDisconnectedToggled ( bool ) ) );
    connect( mDlg->checkBoxUnavailable, SIGNAL( toggled( bool ) ),
             this, SLOT( checkBoxUnavailableToggled ( bool ) ) );
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
    connect( mDlg->checkBoxStartKNemo, SIGNAL( toggled( bool ) ),
             this, SLOT( checkBoxStartKNemoToggled( bool ) ) );
    connect( mDlg->spinBoxTrafficThreshold, SIGNAL( valueChanged( int ) ),
             this, SLOT( spinBoxTrafficValueChanged ( int ) ) );
    connect( mDlg->checkBoxCustom, SIGNAL( toggled( bool ) ),
             this, SLOT( checkBoxCustomToggled ( bool ) ) );
    connect( mDlg->listBoxInterfaces, SIGNAL( currentRowChanged( int ) ),
             this, SLOT( interfaceSelected( int ) ) );
    connect( mDlg->listViewCommands, SIGNAL( currentItemChanged( QTreeWidgetItem*, QTreeWidgetItem* ) ),
             this, SLOT( listViewCommandsSelectionChanged( QTreeWidgetItem*, QTreeWidgetItem* ) ) );
    connect( mDlg->listViewCommands, SIGNAL( itemChanged( QTreeWidgetItem*, int ) ),
             this, SLOT( listViewCommandsChanged( QTreeWidgetItem*, int ) ) );
    connect( mDlg->numInputPollInterval, SIGNAL( valueChanged( int ) ),
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

    KConfigGroup generalGroup( config, "General" );
    bool startKNemo = generalGroup.readEntry( "AutoStart", true );
    mDlg->checkBoxStartKNemo->setChecked( startKNemo );
    mDlg->numInputPollInterval->setValue( clamp<int>(generalGroup.readEntry( "PollInterval", 1 ), 1, 60 ) );
    mDlg->numInputSaveInterval->setValue( clamp<int>(generalGroup.readEntry( "SaveInterval", 60 ), 0, 300 ) );
    mDlg->lineEditStatisticsDir->setUrl( generalGroup.readEntry( "StatisticsDir", KGlobal::dirs()->saveLocation( "data", "knemo/" ) ) );
    mToolTipContent = generalGroup.readEntry( "ToolTipContent", defaultTip );

    QStringList list = generalGroup.readEntry( "Interfaces", QStringList() );

    foreach ( QString interface, list )
    {
        QString group( "Interface_" );
        group += interface;
        InterfaceSettings* settings = new InterfaceSettings();
        if ( config->hasGroup( group ) )
        {
            KConfigGroup interfaceGroup( config, group );
            settings->alias = interfaceGroup.readEntry( "Alias" ).trimmed();
            settings->iconTheme = interfaceGroup.readEntry( "IconSet", "monitor" );
            settings->colorIncoming = interfaceGroup.readEntry( "ColorIncoming", QColor( 0x1889FF ) );
            settings->colorOutgoing = interfaceGroup.readEntry( "ColorOutgoing", QColor( 0xFF7F08 ) );
            KColorScheme scheme(QPalette::Active, KColorScheme::View);
            settings->colorDisabled = interfaceGroup.readEntry( "ColorDisabled", scheme.foreground( KColorScheme::InactiveText ).color() );
            settings->customCommands = interfaceGroup.readEntry( "CustomCommands", false );
            settings->hideWhenDisconnected = interfaceGroup.readEntry( "HideWhenNotAvailable", false );
            settings->hideWhenUnavailable = interfaceGroup.readEntry( "HideWhenNotExisting", false );
            settings->activateStatistics = interfaceGroup.readEntry( "ActivateStatistics", false );
            settings->warnThreshold = clamp<double>(interfaceGroup.readEntry( "BillingWarnThreshold", 0.0 ), 0.0, 9999.0 );
            settings->warnTotalTraffic = interfaceGroup.readEntry( "BillingWarnRxTx", false );

            settings->calendar = interfaceGroup.readEntry( "Calendar", "" );

            if ( settings->calendar.isEmpty() )
                mCalendar = KCalendarSystem::create( mDefaultCalendarType );
            else
                mCalendar = KCalendarSystem::create( settings->calendar );

            settings->customBilling = interfaceGroup.readEntry( "CustomBilling", false );
            settings->billingMonths = clamp<int>(interfaceGroup.readEntry( "BillingMonths", 1 ), 1, 6 );

             // If no start date saved, default to first of month.
            QDate startDate = QDate::currentDate().addDays( 1 - mCalendar->day( QDate::currentDate() ) );
            settings->billingStart = interfaceGroup.readEntry( "BillingStart", startDate );

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

            settings->trafficThreshold = clamp<int>(interfaceGroup.readEntry( "TrafficThreshold", 0 ), 0, 1000 );
            int numCommands = interfaceGroup.readEntry( "NumCommands", 0 );
            for ( int i = 0; i < numCommands; i++ )
            {
                QString entry;
                InterfaceCommand cmd;
                entry = QString( "RunAsRoot%1" ).arg( i + 1 );
                cmd.runAsRoot = interfaceGroup.readEntry( entry, false );
                entry = QString( "Command%1" ).arg( i + 1 );
                cmd.command = interfaceGroup.readEntry( entry );
                entry = QString( "MenuText%1" ).arg( i + 1 );
                cmd.menuText = interfaceGroup.readEntry( entry );
                settings->commands.append( cmd );
            }
        }
        mSettingsMap.insert( interface, settings );
        mDlg->listBoxInterfaces->addItem( interface );
        mDlg->pushButtonDelete->setDisabled( false );
        mDlg->ifaceTab->setDisabled( false );
    }

    // These things need to be here so that 'Reset' from the control
    // center is handled correctly.
    setupToolTipTab();

    // No dcop call if KNemo is not activated by the user. Otherwise
    // load-on-demand will start KNemo.
    if ( mDlg->checkBoxStartKNemo->isChecked() )
    {
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
            // No interface from daemon. Select first entry in list.
            mDlg->listBoxInterfaces->setCurrentRow( 0 );
        }
    }
    else
    {
        // Started from control center. Select first entry in list.
        mDlg->listBoxInterfaces->setCurrentRow( 0 );
    }
    setMaxDay();
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
            config->deleteGroup( "Interface_" + delIface );
            config->deleteGroup( "Plotter_" + delIface );
        }
    }

    foreach ( QString it, mSettingsMap.keys() )
    {
        list.append( it );
        InterfaceSettings* settings = mSettingsMap.value( it );
        KConfigGroup interfaceGroup( config, "Interface_" + it );

        // Preserve settings set by the app before delete
        QPoint plotterPos = interfaceGroup.readEntry( "PlotterPos", QPoint() );
        QSize plotterSize = interfaceGroup.readEntry( "PlotterSize", QSize() );
        QPoint statisticsPos = interfaceGroup.readEntry( "StatisticsPos", QPoint() );
        QSize statisticsSize = interfaceGroup.readEntry( "StatisticsSize", QSize() );
        QPoint statusPos = interfaceGroup.readEntry( "StatusPos", QPoint() );
        QSize statusSize = interfaceGroup.readEntry( "StatusSize", QSize() );

        // Make sure we don't get crufty commands left over
        interfaceGroup.deleteGroup();

        if ( !plotterPos.isNull() )
            interfaceGroup.writeEntry( "PlotterPos", plotterPos );
        if ( !plotterSize.isEmpty() )
            interfaceGroup.writeEntry( "PlotterSize", plotterSize );
        if ( !statisticsPos.isNull() )
            interfaceGroup.writeEntry( "StatisticsPos", statisticsPos );
        if ( !statisticsSize.isEmpty() )
            interfaceGroup.writeEntry( "StatisticsSize", statisticsSize );
        if ( !statusPos.isNull() )
            interfaceGroup.writeEntry( "StatusPos", statusPos );
        if ( !statusSize.isEmpty() )
            interfaceGroup.writeEntry( "StatusSize", statusSize );
        if ( !settings->alias.trimmed().isEmpty() )
            interfaceGroup.writeEntry( "Alias", settings->alias );

        interfaceGroup.writeEntry( "IconSet", settings->iconTheme );
        interfaceGroup.writeEntry( "ColorIncoming", settings->colorIncoming );
        interfaceGroup.writeEntry( "ColorOutgoing", settings->colorOutgoing );
        interfaceGroup.writeEntry( "ColorDisabled", settings->colorDisabled );
        interfaceGroup.writeEntry( "CustomCommands", settings->customCommands );
        interfaceGroup.writeEntry( "HideWhenNotAvailable", settings->hideWhenDisconnected );
        interfaceGroup.writeEntry( "HideWhenNotExisting", settings->hideWhenUnavailable );
        interfaceGroup.writeEntry( "ActivateStatistics", settings->activateStatistics );
        interfaceGroup.writeEntry( "BillingWarnThreshold", settings->warnThreshold );
        interfaceGroup.writeEntry( "BillingWarnRxTx", settings->warnTotalTraffic );
        interfaceGroup.writeEntry( "CustomBilling", settings->customBilling );
        if ( settings->customBilling )
        {
            interfaceGroup.writeEntry( "BillingStart", mDlg->billingStartInput->date() );
            if ( settings->billingMonths > 0 )
                interfaceGroup.writeEntry( "BillingMonths", settings->billingMonths );
            if ( !settings->calendar.isEmpty() )
                interfaceGroup.writeEntry( "Calendar", settings->calendar );
        }

        interfaceGroup.writeEntry( "TrafficThreshold", settings->trafficThreshold );
        interfaceGroup.writeEntry( "NumCommands", settings->commands.size() );
        for ( int i = 0; i < settings->commands.size(); i++ )
        {
            QString entry;
            entry = QString( "RunAsRoot%1" ).arg( i + 1 );
            interfaceGroup.writeEntry( entry, settings->commands[i].runAsRoot );
            entry = QString( "Command%1" ).arg( i + 1 );
            interfaceGroup.writeEntry( entry, settings->commands[i].command );
            entry = QString( "MenuText%1" ).arg( i + 1 );
            interfaceGroup.writeEntry( entry, settings->commands[i].menuText );
        }
    }

    KConfigGroup generalGroup( config, "General" );
    generalGroup.writeEntry( "FirstStart", false );
    generalGroup.writeEntry( "AutoStart", mDlg->checkBoxStartKNemo->isChecked() );
    generalGroup.writeEntry( "PollInterval", mDlg->numInputPollInterval->value() );
    generalGroup.writeEntry( "SaveInterval", mDlg->numInputSaveInterval->value() );
    generalGroup.writeEntry( "StatisticsDir",  mDlg->lineEditStatisticsDir->url().url() );
    generalGroup.writeEntry( "ToolTipContent", mToolTipContent );
    generalGroup.writeEntry( "Interfaces", list );

    config->sync();
    QDBusMessage reply = QDBusInterface("org.kde.knemo", "/knemo", "org.kde.knemo").call("reparseConfiguration");
}

void ConfigDialog::defaults()
{
    // Set these values before we check for default interfaces
    mSettingsMap.clear();
    mDlg->listBoxInterfaces->clear();
    mDlg->pushButtonDelete->setDisabled( true );
    mDlg->ifaceTab->setDisabled( true );
    mDlg->lineEditAlias->setText( QString::null );
    mDlg->checkBoxDisconnected->setChecked( false );
    mDlg->checkBoxUnavailable->setChecked( false );
    mDlg->checkBoxStatistics->setChecked( false );

    // TODO: Test for a KDE release that contains SVN commit 1013534
    KGlobal::locale()->setCalendar( mDefaultCalendarType );

    mCalendar = KCalendarSystem::create( mDefaultCalendarType );
    setMaxDay();

    mDlg->checkBoxCustomBilling->setChecked( false );
    QDate startDate = QDate::currentDate().addDays( 1 - mCalendar->day( QDate::currentDate() ) );
    mDlg->billingStartInput->setDate( startDate );

    mDlg->checkBoxCustom->setChecked( false );
    int index = findIndexFromName( "monitor" );
    if ( index < 0 )
        index = findIndexFromName( TEXT_THEME );
    mDlg->comboBoxIconTheme->setCurrentIndex( index );
    mDlg->pixmapError->clear();
    mDlg->pixmapDisconnected->clear();
    mDlg->pixmapConnected->clear();
    mDlg->pixmapIncoming->clear();
    mDlg->pixmapOutgoing->clear();
    mDlg->pixmapTraffic->clear();
    mDlg->colorIncoming->setColor( QColor( 0x1889FF ) );
    mDlg->colorOutgoing->setColor( QColor( 0xFF7F08 ) );
    KColorScheme scheme(QPalette::Active, KColorScheme::View);
    mDlg->colorDisabled->setColor( scheme.foreground( KColorScheme::InactiveText ).color() );

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

    if ( !interface.isEmpty() )
    {
        InterfaceSettings* settings = new InterfaceSettings();
        settings->customBilling = false;
        settings->billingStart = startDate;
        settings->colorDisabled = scheme.foreground( KColorScheme::InactiveText ).color();
        mSettingsMap.insert( interface, settings );
        mDlg->listBoxInterfaces->addItem( interface );
        mDlg->listBoxInterfaces->setCurrentRow( 0 );
        mDlg->pushButtonDelete->setDisabled( false );
        mDlg->ifaceTab->setDisabled( false );
    }

    // Default misc settings
    mDlg->numInputPollInterval->setValue( 1 );
    mDlg->numInputSaveInterval->setValue( 60 );
    mDlg->lineEditStatisticsDir->setUrl( KGlobal::dirs()->saveLocation( "data", "knemo/" ) );

    // Default tool tips
    mToolTipContent = defaultTip;
    setupToolTipTab();

    changed( true );
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
        settings->billingStart = QDate::currentDate().addDays( 1 - mCalendar->day( QDate::currentDate() ) );
        mSettingsMap.insert( ifname, settings );
        mDlg->listBoxInterfaces->setCurrentRow( mDlg->listBoxInterfaces->row( item ) );
        mDlg->pushButtonDelete->setDisabled( false );
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
        mSettingsMap.insert( ifname, settings );
        mDlg->listBoxInterfaces->addItem( ifname );
    }

    if ( mDlg->listBoxInterfaces->count() > 0 )
    {
        mDlg->listBoxInterfaces->setCurrentRow( 0 );
        mDlg->pushButtonDelete->setDisabled( false );
        mDlg->ifaceTab->setDisabled( false );
        QString iface = mDlg->listBoxInterfaces->item( 0 )->text();
        mDlg->lineEditAlias->blockSignals( true );
        mDlg->lineEditAlias->setText( mSettingsMap[iface]->alias );
        mDlg->lineEditAlias->blockSignals( false );
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

    // TODO: find a better way than blocking signals
    mSettingsMap.remove( selected->text() );
    mDlg->lineEditAlias->blockSignals( true );
    mDlg->lineEditAlias->setText( QString::null );
    mDlg->lineEditAlias->blockSignals( false );
    mDlg->comboBoxIconTheme->blockSignals( true );
    int index = findIndexFromName( "monitor" );
    if ( index < 0 )
        index = findIndexFromName( TEXT_THEME );
    mDlg->comboBoxIconTheme->setCurrentIndex( index );
    mDlg->comboBoxIconTheme->blockSignals( false );
    mDlg->checkBoxDisconnected->blockSignals( true );
    mDlg->checkBoxDisconnected->setChecked( false );
    mDlg->checkBoxDisconnected->blockSignals( false );
    mDlg->checkBoxUnavailable->blockSignals( true );
    mDlg->checkBoxUnavailable->setChecked( false );
    mDlg->checkBoxUnavailable->blockSignals( false );
    mDlg->checkBoxStatistics->blockSignals( true );
    mDlg->checkBoxStatistics->setChecked( false );
    mDlg->checkBoxStatistics->blockSignals( false );
    mDlg->billingStartInput->blockSignals( true );

    // TODO: Test for a KDE release that contains SVN commit 1013534
    KGlobal::locale()->setCalendar( mDefaultCalendarType );

    mCalendar = KCalendarSystem::create( mDefaultCalendarType );
    setMaxDay();
    QDate startDate = QDate::currentDate().addDays( 1 - mCalendar->day( QDate::currentDate() ) );
    mDlg->billingStartInput->setDate( startDate );
    mDlg->billingStartInput->blockSignals( false );
    mDlg->checkBoxCustom->blockSignals( true );
    mDlg->checkBoxCustom->setChecked( false );
    mDlg->checkBoxCustom->blockSignals( false );
    QListWidgetItem *taken = mDlg->listBoxInterfaces->takeItem( mDlg->listBoxInterfaces->row( selected ) );
    delete taken;
    if ( mDlg->listBoxInterfaces->count() < 1 )
    {
        mDlg->pushButtonDelete->setDisabled( true );
        mDlg->ifaceTab->setDisabled( true );
        mDlg->pixmapError->clear();
        mDlg->pixmapDisconnected->clear();
        mDlg->pixmapConnected->clear();
        mDlg->pixmapIncoming->clear();
        mDlg->pixmapOutgoing->clear();
        mDlg->pixmapTraffic->clear();
    }
    changed( true );
}

void ConfigDialog::buttonAddCommandSelected()
{
    if ( !mDlg->listBoxInterfaces->currentItem() )
        return;

    QListWidgetItem* selected = mDlg->listBoxInterfaces->currentItem();

    InterfaceSettings* settings = mSettingsMap.value( selected->text() );

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
    if ( !mDlg->listBoxInterfaces->currentItem() ||
         !mDlg->listViewCommands->currentItem() )
        return;

    QListWidgetItem* selected = mDlg->listBoxInterfaces->currentItem();

    QTreeWidgetItem *item = mDlg->listViewCommands->currentItem();
    int index = mDlg->listViewCommands->indexOfTopLevelItem( item );
    mDlg->listViewCommands->takeTopLevelItem( index );
    delete item;

    InterfaceSettings* settings = mSettingsMap[selected->text()];

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
    if ( !mDlg->listBoxInterfaces->currentItem() ||
         !mDlg->listViewCommands->currentItem() )
        return;

    QListWidgetItem* selected = mDlg->listBoxInterfaces->currentItem();
    QTreeWidgetItem* item = mDlg->listViewCommands->currentItem();
    int index = mDlg->listViewCommands->indexOfTopLevelItem( item );
    if ( index == 0 )
        return;

    mDlg->listViewCommands->takeTopLevelItem( index );
    mDlg->listViewCommands->insertTopLevelItem( index - 1, item );
    mDlg->listViewCommands->setCurrentItem( item );

    InterfaceSettings* settings = mSettingsMap[selected->text()];

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
    if ( !mDlg->listBoxInterfaces->currentItem() ||
         !mDlg->listViewCommands->currentItem() )
        return;

    QListWidgetItem* selected = mDlg->listBoxInterfaces->currentItem();
    QTreeWidgetItem* item = mDlg->listViewCommands->currentItem();
    int index = mDlg->listViewCommands->indexOfTopLevelItem( item );
    if ( index == mDlg->listViewCommands->topLevelItemCount() - 1 )
        return;

    mDlg->listViewCommands->takeTopLevelItem( index );
    mDlg->listViewCommands->insertTopLevelItem( index + 1, item );
    mDlg->listViewCommands->setCurrentItem( item );

    InterfaceSettings* settings = mSettingsMap[selected->text()];

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

void ConfigDialog::buttonAddToolTipSelected()
{
    // Support extended selection
    if ( mDlg->listBoxAvailable->count() == 0 )
        return;

    QList<QListWidgetItem *> selectedItems = mDlg->listBoxAvailable->selectedItems();

    foreach ( QListWidgetItem *selected, selectedItems )
    {
        quint32 key = mToolTips.key( selected->text() );

        int newIndex = -1;
        int count = mDlg->listBoxDisplay->count();
        for ( int i = 0; i < count; i++ )
        {
            QListWidgetItem *item = mDlg->listBoxDisplay->item( i );
            if ( mToolTips.key( item->text() ) > key )
            {
                newIndex = i;
                break;
            }
        }
        if ( newIndex < 0 )
            newIndex = count;

        selected->setSelected( false );
        mDlg->listBoxAvailable->takeItem( mDlg->listBoxAvailable->row( selected ) );
        mDlg->listBoxDisplay->insertItem( newIndex, selected );
        if ( mDlg->listBoxAvailable->count() == 0 )
            mDlg->pushButtonAddToolTip->setEnabled( false );
        if ( mDlg->listBoxDisplay->count() == 1 )
            mDlg->pushButtonRemoveToolTip->setEnabled( true );

        mToolTipContent += mToolTips.key( selected->text() );
        changed( true );
    }
}

void ConfigDialog::buttonRemoveToolTipSelected()
{
    // Support extended selection
    if ( mDlg->listBoxDisplay->count() == 0 )
        return;

    QList<QListWidgetItem *> selectedItems = mDlg->listBoxDisplay->selectedItems();

    foreach ( QListWidgetItem *selected, selectedItems )
    {
        quint32 key = mToolTips.key( selected->text() );

        int newIndex = -1;
        int count = mDlg->listBoxAvailable->count();
        for ( int i = 0; i < count; i++ )
        {
            QListWidgetItem *item = mDlg->listBoxAvailable->item( i );
            if ( mToolTips.key( item->text() ) > key )
            {
                newIndex = i;
                break;
            }
        }
        if ( newIndex < 0 )
            newIndex = count;

        selected->setSelected( false );
        mDlg->listBoxDisplay->takeItem( mDlg->listBoxDisplay->row( selected ) );
        mDlg->listBoxAvailable->insertItem( newIndex, selected );
        if ( mDlg->listBoxDisplay->count() == 0 )
            mDlg->pushButtonRemoveToolTip->setEnabled( false );
        if ( mDlg->listBoxAvailable->count() == 1 )
            mDlg->pushButtonAddToolTip->setEnabled( true );

        mToolTipContent -= mToolTips.key( selected->text() );
        changed( true );
    }
}

void ConfigDialog::buttonNotificationsSelected()
{
    KNotifyConfigWidget::configure( this, "knemo" );
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

void ConfigDialog::interfaceSelected( int row )
{
    if ( row < 0 )
        return;
    QString interface = mDlg->listBoxInterfaces->item( row )->text();
    InterfaceSettings* settings = mSettingsMap[interface];
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

void ConfigDialog::aliasChanged( const QString& text )
{
    if ( !mDlg->listBoxInterfaces->currentItem() )
        return;

    QListWidgetItem* selected = mDlg->listBoxInterfaces->currentItem();

    InterfaceSettings* settings = mSettingsMap[selected->text()];
    settings->alias = text;
    if (!mLock) changed( true );
}

void ConfigDialog::colorButtonChanged()
{
    if ( !mDlg->listBoxInterfaces->currentItem() )
        return;

    QListWidgetItem* selected = mDlg->listBoxInterfaces->currentItem();

    InterfaceSettings* settings = mSettingsMap[selected->text()];
    if ( mDlg->colorIncoming->color().isValid() )
        settings->colorIncoming = mDlg->colorIncoming->color();
    if ( mDlg->colorOutgoing->color().isValid() )
        settings->colorOutgoing = mDlg->colorOutgoing->color();
    if ( mDlg->colorDisabled->color().isValid() )
        settings->colorDisabled = mDlg->colorDisabled->color();

    KNemoTheme curTheme = mDlg->comboBoxIconTheme->itemData( mDlg->comboBoxIconTheme->currentIndex() ).value<KNemoTheme>();
    if ( curTheme.internalName == TEXT_THEME )
        iconThemeChanged( mDlg->comboBoxIconTheme->currentIndex() );
    if ( !mLock) changed( true );
}

QFont ConfigDialog::setIconFont( QString text )
{
    QFont f = KGlobalSettings::generalFont();
    float pointSize = f.pointSizeF();
    QFontMetrics fm( f );
    int w = fm.width( text );
    if ( w > 22 )
    {
        pointSize *= float( 22 ) / float( w );
        f.setPointSizeF( pointSize );
    }

    fm = QFontMetrics( f );
    // Don't want decender()...space too tight
    // +1 for base line +1 for space between lines
    int h = fm.ascent() + 2;
    if ( h > 11 )
    {
        pointSize *= float( 11 ) / float( h );
        f.setPointSizeF( pointSize );
    }
    return f;
}

QPixmap ConfigDialog::textIcon( QString incomingText, QString outgoingText, bool active )
{
    QPixmap sampleIcon( 22, 22 );
    sampleIcon.fill( Qt::transparent );
    QPainter p( &sampleIcon );
    p.setBrush( Qt::NoBrush );
    p.setOpacity( 1.0 );
    p.setFont( setIconFont( incomingText ) );
    if ( active )
        p.setPen( mDlg->colorIncoming->color() );
    else
        p.setPen( mDlg->colorDisabled->color() );
    p.drawText( sampleIcon.rect(), Qt::AlignTop | Qt::AlignRight, incomingText );
    p.setFont( setIconFont( outgoingText ) );
    if ( active )
        p.setPen( mDlg->colorOutgoing->color() );
    p.drawText( sampleIcon.rect(), Qt::AlignBottom | Qt::AlignRight, outgoingText );
    return sampleIcon;
}

void ConfigDialog::iconThemeChanged( int set )
{
    if ( !mDlg->listBoxInterfaces->currentItem() )
        return;

    QListWidgetItem* selected = mDlg->listBoxInterfaces->currentItem();

    InterfaceSettings* settings = mSettingsMap[selected->text()];
    KNemoTheme curTheme = mDlg->comboBoxIconTheme->itemData( mDlg->comboBoxIconTheme->currentIndex() ).value<KNemoTheme>();
    if ( curTheme.internalName == TEXT_THEME )
    {
        settings->iconTheme = TEXT_THEME;
        mDlg->pixmapError->setPixmap( textIcon( "0B", "0B", false ) );
        mDlg->pixmapDisconnected->setPixmap( textIcon( "0B", "0B", false ) );
        mDlg->pixmapConnected->setPixmap( textIcon( "0B", "0B", true ) );
        mDlg->pixmapIncoming->setPixmap( textIcon( "123K", "0B", true ) );
        mDlg->pixmapOutgoing->setPixmap( textIcon( "0B", "12K", true ) );
        mDlg->pixmapTraffic->setPixmap( textIcon( "123K", "12K", true ) );
        mDlg->colorIncoming->show();
        mDlg->colorIncomingLabel->show();
        mDlg->colorOutgoing->show();
        mDlg->colorOutgoingLabel->show();
        mDlg->colorDisabled->show();
        mDlg->colorDisabledLabel->show();
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
    }
    if (!mLock) changed( true );
}

void ConfigDialog::checkBoxDisconnectedToggled( bool on )
{
    if ( !mDlg->listBoxInterfaces->currentItem() )
        return;

    QListWidgetItem* selected = mDlg->listBoxInterfaces->currentItem();

    InterfaceSettings* settings = mSettingsMap[selected->text()];
    settings->hideWhenDisconnected = on;
    if (!mLock) changed( true );
}

void ConfigDialog::checkBoxUnavailableToggled( bool on )
{
    if ( !mDlg->listBoxInterfaces->currentItem() )
        return;

    QListWidgetItem* selected = mDlg->listBoxInterfaces->currentItem();

    InterfaceSettings* settings = mSettingsMap[selected->text()];
    settings->hideWhenUnavailable = on;
    if (!mLock) changed( true );
}

void ConfigDialog::checkBoxCustomBillingToggled( bool on )
{
    if ( !mDlg->listBoxInterfaces->currentItem() )
        return;

    QListWidgetItem* selected = mDlg->listBoxInterfaces->currentItem();

    InterfaceSettings* settings = mSettingsMap[selected->text()];
    settings->customBilling = on;
    mDlg->billingStartInput->setDate( QDate::currentDate().addDays( 1 - mCalendar->day( QDate::currentDate() ) ) );
    mDlg->billingMonthsInput->setValue( 1 );

    if (!mLock) changed( true );
}

void ConfigDialog::warnThresholdChanged( double val )
{
    if ( !mDlg->listBoxInterfaces->currentItem() )
        return;

    QListWidgetItem* selected = mDlg->listBoxInterfaces->currentItem();

    InterfaceSettings* settings = mSettingsMap[selected->text()];
    settings->warnThreshold = round(val*10.0)/10.0;

    mDlg->warnRx->setEnabled( ( settings->warnThreshold > 0.0 ) );
    mDlg->warnRxTx->setEnabled( ( settings->warnThreshold > 0.0 ) );
    if (!mLock) changed( true );
}

void ConfigDialog::warnRxTxToggled( bool on )
{
    if ( !mDlg->listBoxInterfaces->currentItem() )
        return;

    QListWidgetItem* selected = mDlg->listBoxInterfaces->currentItem();

    InterfaceSettings* settings = mSettingsMap[selected->text()];
    settings->warnTotalTraffic = on;
    if (!mLock) changed( true );
}

void ConfigDialog::checkBoxStatisticsToggled( bool on )
{
    if ( !mDlg->listBoxInterfaces->currentItem() )
        return;

    QListWidgetItem* selected = mDlg->listBoxInterfaces->currentItem();

    InterfaceSettings* settings = mSettingsMap[selected->text()];
    settings->activateStatistics = on;
    if (!mLock) changed( true );
}

void ConfigDialog::billingStartInputChanged( const QDate& date )
{
    QListWidgetItem* selected = mDlg->listBoxInterfaces->currentItem();
    InterfaceSettings* settings = mSettingsMap[selected->text()];

    // KDateEdit doesn't guarantee a valid date
    if ( !date.isValid() ||
         date > QDate::currentDate() ||
         mCalendar->day( date ) > mMaxDay )
    {
        KMessageBox::error( this, i18n( "The billing day of the month can be any day from 1 - %1, and the complete date must be a valid, non-future date." ).arg( QString::number( mMaxDay ) ) );
        mDlg->billingStartInput->setDate( settings->billingStart );
    }
    else
    {
        settings->billingStart = date;
        changed( true );
    }
}

void ConfigDialog::billingMonthsInputChanged( int value )
{
    if ( !mDlg->listBoxInterfaces->currentItem() )
        return;

    QListWidgetItem* selected = mDlg->listBoxInterfaces->currentItem();

    InterfaceSettings* settings = mSettingsMap[selected->text()];
    settings->billingMonths = value;
}

void ConfigDialog::checkBoxStartKNemoToggled( bool on )
{
    if ( on )
    {
        KConfig *config = mConfig.data();
        KConfigGroup generalGroup( config, "General" );
        if ( generalGroup.readEntry( "FirstStart", true ) )
        {
            // Populate the dialog with some default values if the user starts
            // KNemo for the very first time.
            defaults();
        }
    }

    if (!mLock) changed( true );
}

void ConfigDialog::spinBoxTrafficValueChanged( int value )
{
    if ( !mDlg->listBoxInterfaces->currentItem() )
        return;

    QListWidgetItem* selected = mDlg->listBoxInterfaces->currentItem();

    InterfaceSettings* settings = mSettingsMap[selected->text()];
    settings->trafficThreshold = value;
    if (!mLock) changed( true );
}

void ConfigDialog::checkBoxCustomToggled( bool on )
{
    if ( !mDlg->listBoxInterfaces->currentItem() )
        return;

    QListWidgetItem* selected = mDlg->listBoxInterfaces->currentItem();

    InterfaceSettings* settings = mSettingsMap[selected->text()];
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
    {
        mDlg->listBoxDisplay->item( 0 )->setSelected( true );
        mDlg->pushButtonRemoveToolTip->setEnabled( true );
    }
    else
        mDlg->pushButtonRemoveToolTip->setEnabled( false );

    if ( mDlg->listBoxAvailable->count() > 0 )
    {
        mDlg->listBoxAvailable->item( 0 )->setSelected( true );
        mDlg->pushButtonAddToolTip->setEnabled( true );
    }
    else
        mDlg->pushButtonAddToolTip->setEnabled( false );
}

void ConfigDialog::setupToolTipMap()
{
    // Cannot make this data static as the i18n macro doesn't seem
    // to work when called to early i.e. before setting the catalogue.
    mToolTips.insert( INTERFACE, i18n( "Interface" ) );
#ifndef USE_KNOTIFICATIONITEM
    mToolTips.insert( ALIAS, i18n( "Alias" ) );
#endif
    mToolTips.insert( STATUS, i18n( "Status" ) );
    mToolTips.insert( UPTIME, i18n( "Uptime" ) );
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
    mToolTips.insert( NICK_NAME, i18n( "Nickname" ) );
    mToolTips.insert( ENCRYPTION, i18n( "Encryption" ) );
}

void ConfigDialog::listViewCommandsSelectionChanged( QTreeWidgetItem* item, QTreeWidgetItem* )
{
    if ( item )
        mDlg->pushButtonRemoveCommand->setEnabled( true );
    else
        mDlg->pushButtonRemoveCommand->setEnabled( false );
}

void ConfigDialog::listViewCommandsChanged( QTreeWidgetItem* item, int column )
{
    if ( !mDlg->listBoxInterfaces->currentItem() )
        return;

    QListWidgetItem* selected = mDlg->listBoxInterfaces->currentItem();

    int row = mDlg->listViewCommands->indexOfTopLevelItem( item );

    InterfaceSettings* settings = mSettingsMap[selected->text()];
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

#include "configdialog.moc"
