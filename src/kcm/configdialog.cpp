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
#include <kconfig.h>
#include <klocale.h>
#include <knuminput.h>
#include <kiconloader.h>
#include <kfiledialog.h>
#include <kcolorbutton.h>
#include <kinputdialog.h>
#include <kapplication.h>
#include <KNotifyConfigWidget>
#include <kstandarddirs.h>
#include <kgenericfactory.h>
#include <kdirselectdialog.h>

#include "ui_configdlg.h"
#include "config-knemo.h"
#include "configdialog.h"
#include "utils.h"

#ifdef __linux__
  #include <netlink/route/rtnl.h>
  #include <netlink/route/link.h>
  #include <netlink/route/route.h>
#endif

const QString ConfigDialog::ICON_DISCONNECTED = "_disconnected";
const QString ConfigDialog::ICON_CONNECTED = "_connected";
const QString ConfigDialog::ICON_INCOMING = "_incoming";
const QString ConfigDialog::ICON_OUTGOING = "_outgoing";
const QString ConfigDialog::ICON_TRAFFIC = "_traffic";


K_PLUGIN_FACTORY(KNemoFactory, registerPlugin<ConfigDialog>();)
K_EXPORT_PLUGIN(KNemoFactory("kcm_knemo"))


ConfigDialog::ConfigDialog( QWidget *parent, const QVariantList &args )
    : KCModule( KNemoFactory::componentData(), parent, args ),
      mLock( false ),
      mDlg( new Ui::ConfigDlg() ),
      mColorVLines( 0x04FB1D ),
      mColorHLines( 0x04FB1D ),
      mColorIncoming( 0x1889FF ),
      mColorOutgoing( 0xFF7F08 ),
      mColorBackground( 0x313031 )
{
    mConfig = KSharedConfig::openConfig( "knemorc" );
    setupToolTipMap();

    QWidget *main = new QWidget( this );
    QVBoxLayout* top = new QVBoxLayout( this );
    mDlg->setupUi( main );
    top->addWidget( main );

    // Search for iconsets and add them to comboBoxIconSet
    KStandardDirs iconDirs;
    iconDirs.addResourceType("knemo_pics", "data", "knemo/pics");
    QStringList iconlist = iconDirs.findAllResources( "knemo_pics", "*.png" );

    foreach ( QString iconName, iconlist )
    {
        QRegExp rx( "pics\\/(.+)_(connected|disconnected|incoming|outgoing|traffic)\\.png" );
        if ( rx.indexIn( iconName ) > -1 )
            if ( !mIconSets.contains( rx.cap( 1 ) ) )
                mIconSets << rx.cap( 1 );
    }
    mIconSets.sort();
    // We want "Text" at the bottom of the list
    mIconSets << i18n( "Text" );
    mDlg->comboBoxIconSet->addItems( mIconSets );
    if ( mIconSets.contains( "monitor" ) )
        mDlg->comboBoxIconSet->setCurrentIndex( mIconSets.indexOf( "monitor" ) );

    mDlg->pushButtonNew->setIcon( SmallIcon( "list-add" ) );
    mDlg->pushButtonAll->setIcon( SmallIcon( "document-new" ) );
    mDlg->pushButtonDelete->setIcon( SmallIcon( "list-remove" ) );
    mDlg->pushButtonAddCommand->setIcon( SmallIcon( "list-add" ) );
    mDlg->pushButtonRemoveCommand->setIcon( SmallIcon( "list-remove" ) );
    mDlg->pushButtonUp->setIcon( SmallIcon( "arrow-up" ) );
    mDlg->pushButtonDown->setIcon( SmallIcon( "arrow-down" ) );
    mDlg->pushButtonAddToolTip->setIcon( SmallIcon( "arrow-right" ) );
    mDlg->pushButtonRemoveToolTip->setIcon( SmallIcon( "arrow-left" ) );
    //mDlg->listViewCommands->setSorting( -1 );
    mDlg->listViewCommands->setWhatsThis (
                     i18n("<p>In this area you can add the custom entries " \
                          "for your context menu: <ol><li>check <b>Display " \
                          "custom entries in context menu</b>;</li>" \
                          "<li>push on the <b>Add</b> button to add a new " \
                          "entry in the list;</li><li>edit the entry by " \
                          "double clicking in column <b>Menu text</b> and " \
                          "<b>Command</b>;</li><li>start from step 2 for " \
                          "every new entry</li>.</ol>If you need to execute " \
                          "the command as root user check the corresponding " \
                          "<b>Root</b> CheckBox.") );

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
    connect( mDlg->comboBoxIconSet, SIGNAL( activated( int ) ),
             this, SLOT( iconSetChanged( int ) ) );
    connect( mDlg->checkBoxNotConnected, SIGNAL( toggled( bool ) ),
             this, SLOT( checkBoxNotConnectedToggled ( bool ) ) );
    connect( mDlg->checkBoxNotExisting, SIGNAL( toggled( bool ) ),
             this, SLOT( checkBoxNotExistingToggled ( bool ) ) );
    connect( mDlg->checkBoxStatistics, SIGNAL( toggled( bool ) ),
             this, SLOT( checkBoxStatisticsToggled ( bool ) ) );
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

    // connect the plotter widgets
    connect( mDlg->checkBoxBottomBar, SIGNAL( toggled( bool ) ),
             this, SLOT( changed() ) );
    connect( mDlg->checkBoxLabels, SIGNAL( toggled( bool ) ),
             this, SLOT( changed() ) );
    connect( mDlg->checkBoxVLines, SIGNAL( toggled( bool ) ),
             this, SLOT( changed() ) );
    connect( mDlg->checkBoxHLines, SIGNAL( toggled( bool ) ),
             this, SLOT( changed() ) );
    connect( mDlg->checkBoxIncoming, SIGNAL( toggled( bool ) ),
             this, SLOT( changed() ) );
    connect( mDlg->checkBoxOutgoing, SIGNAL( toggled( bool ) ),
             this, SLOT( changed() ) );
    connect( mDlg->checkBoxVLinesScroll, SIGNAL( toggled( bool ) ),
             this, SLOT( changed() ) );
    connect( mDlg->checkBoxAutoDetection, SIGNAL( toggled( bool ) ),
             this, SLOT( changed() ) );
    connect( mDlg->spinBoxPixel, SIGNAL( valueChanged( int ) ),
             this, SLOT( changed() ) );
    connect( mDlg->spinBoxDistance, SIGNAL( valueChanged( int ) ),
             this, SLOT( changed() ) );
    connect( mDlg->spinBoxFontSize, SIGNAL( valueChanged( int ) ),
             this, SLOT( changed() ) );
    connect( mDlg->spinBoxMinValue, SIGNAL( valueChanged( int ) ),
             this, SLOT( changed() ) );
    connect( mDlg->spinBoxMaxValue, SIGNAL( valueChanged( int ) ),
             this, SLOT( changed() ) );
    connect( mDlg->numInputPollInterval, SIGNAL( valueChanged( int ) ),
             this, SLOT( changed() ) );
    connect( mDlg->numInputSaveInterval, SIGNAL( valueChanged( int ) ),
             this, SLOT( changed() ) );
    connect( mDlg->kColorButtonVLines, SIGNAL( changed( const QColor& ) ),
             this, SLOT( changed() ) );
    connect( mDlg->kColorButtonHLines, SIGNAL( changed( const QColor& ) ),
             this, SLOT( changed() ) );
    connect( mDlg->kColorButtonIncoming, SIGNAL( changed( const QColor& ) ),
             this, SLOT( colorButtonChanged() ) );
    connect( mDlg->kColorButtonOutgoing, SIGNAL( changed( const QColor& ) ),
             this, SLOT( colorButtonChanged() ) );
    connect( mDlg->kColorButtonBackground, SIGNAL( changed( const QColor& ) ),
             this, SLOT( changed() ) );
    connect( mDlg->spinBoxOpacity, SIGNAL( valueChanged( const int ) ),
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
    KConfig *config = mConfig.data();

    KConfigGroup generalGroup( config, "General" );
    bool startKNemo = generalGroup.readEntry( "AutoStart", true );
    mDlg->checkBoxStartKNemo->setChecked( startKNemo );
    mDlg->numInputPollInterval->setValue( clamp<int>(generalGroup.readEntry( "PollInterval", 1 ), 1, 60 ) );
    mDlg->numInputSaveInterval->setValue( clamp<int>(generalGroup.readEntry( "SaveInterval", 60 ), 1, 300 ) );
    mDlg->lineEditStatisticsDir->setUrl( generalGroup.readEntry( "StatisticsDir", KGlobal::dirs()->saveLocation( "data", "knemo/" ) ) );
    mToolTipContent = generalGroup.readEntry( "ToolTipContent", 2 );

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
            settings->iconSet = interfaceGroup.readEntry( "IconSet", "monitor" );
            settings->customCommands = interfaceGroup.readEntry( "CustomCommands", false );
            settings->hideWhenNotAvailable = interfaceGroup.readEntry( "HideWhenNotAvailable", false );
            settings->hideWhenNotExisting = interfaceGroup.readEntry( "HideWhenNotExisting", false );
            settings->activateStatistics = interfaceGroup.readEntry( "ActivateStatistics", false );
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
        mDlg->groupBoxIfaceMisc->setDisabled( false );
        mDlg->groupBoxIfaceMenu->setDisabled( false );
    }

    // enable or disable statistics entries
    updateStatisticsEntries();

    // Set the plotter widgets
    KConfigGroup plotterGroup( config, "PlotterSettings" );
    mDlg->spinBoxPixel->setValue( clamp<int>(plotterGroup.readEntry( "Pixel", 1 ), 1, 50 ) );
    mDlg->spinBoxDistance->setValue( clamp<int>(plotterGroup.readEntry( "Distance", 30 ), 10, 120 ) );
    mDlg->spinBoxFontSize->setValue( clamp<int>(plotterGroup.readEntry( "FontSize", 8 ), 5, 24 ) );
    mDlg->spinBoxMinValue->setValue( clamp<int>(plotterGroup.readEntry( "MinimumValue", 0 ), 0, 49999 ) );
    mDlg->spinBoxMaxValue->setValue( clamp<int>(plotterGroup.readEntry( "MaximumValue", 1 ), 1, 50000 ) );
    mDlg->checkBoxLabels->setChecked( plotterGroup.readEntry( "Labels", true ) );
    mDlg->checkBoxBottomBar->setChecked( plotterGroup.readEntry( "BottomBar", true ) );
    mDlg->checkBoxVLines->setChecked( plotterGroup.readEntry( "VerticalLines", true ) );
    mDlg->checkBoxHLines->setChecked( plotterGroup.readEntry( "HorizontalLines", true ) );
    mDlg->checkBoxIncoming->setChecked( plotterGroup.readEntry( "ShowIncoming", true ) );
    mDlg->checkBoxOutgoing->setChecked( plotterGroup.readEntry( "ShowOutgoing", true ) );
    mDlg->checkBoxAutoDetection->setChecked( plotterGroup.readEntry( "AutomaticDetection", true ) );
    mDlg->checkBoxVLinesScroll->setChecked( plotterGroup.readEntry( "VerticalLinesScroll", true ) );
    mDlg->kColorButtonVLines->setColor( plotterGroup.readEntry( "ColorVLines", mColorVLines ) );
    mDlg->kColorButtonHLines->setColor( plotterGroup.readEntry( "ColorHLines", mColorHLines ) );
    mDlg->kColorButtonIncoming->setColor( plotterGroup.readEntry( "ColorIncoming", mColorIncoming ) );
    mDlg->kColorButtonOutgoing->setColor( plotterGroup.readEntry( "ColorOutgoing", mColorOutgoing ) );
    mDlg->kColorButtonBackground->setColor( plotterGroup.readEntry( "ColorBackground", mColorBackground ) );
    mDlg->spinBoxOpacity->setValue( clamp<int>(plotterGroup.readEntry( "Opacity", 20 ), 0, 100 ) );

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
}

void ConfigDialog::save()
{
    KConfig *config = mConfig.data();

    QStringList list;

    // Remove interfaces from the config that were deleted during this session
    foreach ( QString delIface, mDeletedIfaces )
    {
        if ( !mSettingsMap.contains( delIface ) )
            config->deleteGroup( "Interface_" + delIface );
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

        interfaceGroup.writeEntry( "IconSet", settings->iconSet );
        interfaceGroup.writeEntry( "CustomCommands", settings->customCommands );
        interfaceGroup.writeEntry( "HideWhenNotAvailable", settings->hideWhenNotAvailable );
        interfaceGroup.writeEntry( "HideWhenNotExisting", settings->hideWhenNotExisting );
        interfaceGroup.writeEntry( "ActivateStatistics", settings->activateStatistics );
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

    KConfigGroup plotterGroup( config, "PlotterSettings" );
    plotterGroup.writeEntry( "Pixel", mDlg->spinBoxPixel->value() );
    plotterGroup.writeEntry( "Distance", mDlg->spinBoxDistance->value() );
    plotterGroup.writeEntry( "FontSize", mDlg->spinBoxFontSize->value() );
    plotterGroup.writeEntry( "MinimumValue", mDlg->spinBoxMinValue->value() );
    plotterGroup.writeEntry( "MaximumValue", mDlg->spinBoxMaxValue->value() );
    plotterGroup.writeEntry( "Labels", mDlg->checkBoxLabels->isChecked() );
    plotterGroup.writeEntry( "BottomBar", mDlg->checkBoxBottomBar->isChecked() );
    plotterGroup.writeEntry( "VerticalLines", mDlg->checkBoxVLines->isChecked() );
    plotterGroup.writeEntry( "HorizontalLines", mDlg->checkBoxHLines->isChecked() );
    plotterGroup.writeEntry( "ShowIncoming", mDlg->checkBoxIncoming->isChecked() );
    plotterGroup.writeEntry( "ShowOutgoing", mDlg->checkBoxOutgoing->isChecked() );
    plotterGroup.writeEntry( "AutomaticDetection", mDlg->checkBoxAutoDetection->isChecked() );
    plotterGroup.writeEntry( "VerticalLinesScroll", mDlg->checkBoxVLinesScroll->isChecked() );
    plotterGroup.writeEntry( "ColorVLines", mDlg->kColorButtonVLines->color() );
    plotterGroup.writeEntry( "ColorHLines", mDlg->kColorButtonHLines->color() );
    plotterGroup.writeEntry( "ColorIncoming", mDlg->kColorButtonIncoming->color() );
    plotterGroup.writeEntry( "ColorOutgoing", mDlg->kColorButtonOutgoing->color() );
    plotterGroup.writeEntry( "ColorBackground", mDlg->kColorButtonBackground->color() );
    plotterGroup.writeEntry( "Opacity", mDlg->spinBoxOpacity->value() );

    config->sync();
    QDBusMessage reply = QDBusInterface("org.kde.knemo", "/knemo", "org.kde.knemo").call("reparseConfiguration");
}

void ConfigDialog::defaults()
{
    // Set these values before we check for default interfaces
    mSettingsMap.clear();
    mDlg->listBoxInterfaces->clear();
    mDlg->pushButtonDelete->setDisabled( true );
    mDlg->groupBoxIfaceMisc->setDisabled( true );
    mDlg->groupBoxIfaceMenu->setDisabled( true );
    mDlg->lineEditAlias->setText( QString::null );
    mDlg->checkBoxNotConnected->setChecked( false );
    mDlg->checkBoxNotExisting->setChecked( false );
    mDlg->checkBoxStatistics->setChecked( false );
    mDlg->checkBoxCustom->setChecked( false );
    if ( mIconSets.contains( "monitor" ) )
        mDlg->comboBoxIconSet->setCurrentIndex( mIconSets.indexOf( "monitor" ) );
    else
        mDlg->comboBoxIconSet->setCurrentIndex( 0 );
    mDlg->pixmapDisconnected->clear();
    mDlg->pixmapConnected->clear();
    mDlg->pixmapIncoming->clear();
    mDlg->pixmapOutgoing->clear();
    mDlg->pixmapTraffic->clear();

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
        settings->customCommands = false;
        settings->hideWhenNotAvailable = false;
        settings->hideWhenNotExisting = false;
        settings->activateStatistics = false;
        settings->iconSet = "monitor";
        settings->alias = interface;
        mSettingsMap.insert( interface, settings );
        mDlg->listBoxInterfaces->addItem( interface );
        mDlg->lineEditAlias->setText( interface );
        mDlg->listBoxInterfaces->setCurrentRow( 0 );
        mDlg->pushButtonDelete->setDisabled( false );
        mDlg->groupBoxIfaceMisc->setDisabled( false );
        mDlg->groupBoxIfaceMenu->setDisabled( false );
    }

    // Default misc settings
    mDlg->numInputPollInterval->setValue( 1 );
    mDlg->numInputSaveInterval->setValue( 60 );
    mDlg->lineEditStatisticsDir->setUrl( KGlobal::dirs()->saveLocation( "data", "knemo/" ) );

    // Default tool tips
    mToolTipContent = 2;
    setupToolTipTab();

    // Default plotter settings
    mDlg->spinBoxPixel->setValue( 1 );
    mDlg->spinBoxDistance->setValue( 30 );
    mDlg->spinBoxFontSize->setValue( 8 );
    mDlg->spinBoxMinValue->setValue( 0 );
    mDlg->spinBoxMaxValue->setValue( 1 );
    mDlg->checkBoxLabels->setChecked( true );
    mDlg->checkBoxBottomBar->setChecked( true );
    mDlg->checkBoxVLines->setChecked( true );
    mDlg->checkBoxHLines->setChecked( true );
    mDlg->checkBoxIncoming->setChecked( true );
    mDlg->checkBoxOutgoing->setChecked( true );
    mDlg->checkBoxAutoDetection->setChecked( true );
    mDlg->checkBoxVLinesScroll->setChecked( true );
    mDlg->kColorButtonVLines->setColor( mColorVLines );
    mDlg->kColorButtonHLines->setColor( mColorHLines );
    mDlg->kColorButtonIncoming->setColor( mColorIncoming );
    mDlg->kColorButtonOutgoing->setColor( mColorOutgoing );
    mDlg->kColorButtonBackground->setColor( mColorBackground );
    mDlg->spinBoxOpacity->setValue( 20 );

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
        mSettingsMap.insert( ifname, new InterfaceSettings() );
        mDlg->listBoxInterfaces->setCurrentRow( mDlg->listBoxInterfaces->row( item ) );
        mDlg->pushButtonDelete->setDisabled( false );
        mDlg->groupBoxIfaceMisc->setDisabled( false );
        mDlg->groupBoxIfaceMenu->setDisabled( false );
        mDlg->lineEditAlias->setText( ifname );
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
            if ( "lo" == ifname || mSettingsMap.contains( ifname ) )
                continue;
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
        if ( "lo" == ifname || "lo0" == ifname || ifaces.contains( ifname ) || mSettingsMap.contains( ifname ) )
            continue;
        ifaces << ifname;
    }
    freeifaddrs( ifaddr );
#endif

    foreach ( QString ifname, ifaces )
    {
        InterfaceSettings* settings = new InterfaceSettings();
        settings->customCommands = false;
        settings->hideWhenNotAvailable = false;
        settings->hideWhenNotExisting = false;
        settings->activateStatistics = false;
        settings->iconSet = "monitor";
        settings->alias = ifname;
        mSettingsMap.insert( ifname, settings );
        mDlg->listBoxInterfaces->addItem( ifname );
    }

    if ( mDlg->listBoxInterfaces->count() > 0 )
    {
        mDlg->listBoxInterfaces->setCurrentRow( 0 );
        mDlg->pushButtonDelete->setDisabled( false );
        mDlg->groupBoxIfaceMisc->setDisabled( false );
        mDlg->groupBoxIfaceMenu->setDisabled( false );
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
    mDlg->comboBoxIconSet->blockSignals( true );
    if ( mIconSets.contains( "monitor" ) )
        mDlg->comboBoxIconSet->setCurrentIndex( mIconSets.indexOf( "monitor" ) );
    else
        mDlg->comboBoxIconSet->setCurrentIndex( 0 );
    mDlg->comboBoxIconSet->blockSignals( false );
    mDlg->checkBoxNotConnected->blockSignals( true );
    mDlg->checkBoxNotConnected->setChecked( false );
    mDlg->checkBoxNotConnected->blockSignals( false );
    mDlg->checkBoxNotExisting->blockSignals( true );
    mDlg->checkBoxNotExisting->setChecked( false );
    mDlg->checkBoxNotExisting->blockSignals( false );
    mDlg->checkBoxStatistics->blockSignals( true );
    mDlg->checkBoxStatistics->setChecked( false );
    mDlg->checkBoxStatistics->blockSignals( false );
    mDlg->checkBoxCustom->blockSignals( true );
    mDlg->checkBoxCustom->setChecked( false );
    mDlg->checkBoxCustom->blockSignals( false );
    QListWidgetItem *taken = mDlg->listBoxInterfaces->takeItem( mDlg->listBoxInterfaces->row( selected ) );
    delete taken;
    if ( mDlg->listBoxInterfaces->count() < 1 )
    {
        mDlg->pushButtonDelete->setDisabled( true );
        mDlg->groupBoxIfaceMisc->setDisabled( true );
        mDlg->groupBoxIfaceMenu->setDisabled( true );
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

void ConfigDialog::interfaceSelected( int row )
{
    if ( row < 0 )
        return;
    QString interface = mDlg->listBoxInterfaces->item( row )->text();
    InterfaceSettings* settings = mSettingsMap[interface];
    mLock = true;
    mDlg->lineEditAlias->setText( settings->alias );
    if ( mIconSets.contains( settings->iconSet ) )
        mDlg->comboBoxIconSet->setCurrentIndex( mIconSets.indexOf( settings->iconSet ) );
    else if ( settings->iconSet.isEmpty() )
        mDlg->comboBoxIconSet->setCurrentIndex( mDlg->comboBoxIconSet->count() - 1 );
    else
        mDlg->comboBoxIconSet->setCurrentIndex( 0 );
    mDlg->checkBoxCustom->setChecked( settings->customCommands );
    mDlg->checkBoxNotConnected->setChecked( settings->hideWhenNotAvailable );
    mDlg->checkBoxNotExisting->setChecked( settings->hideWhenNotExisting );
    mDlg->checkBoxStatistics->setChecked( settings->activateStatistics );
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

    // to update iconset preview
    if ( mIconSets.contains( settings->iconSet ) )
        iconSetChanged( mIconSets.indexOf( settings->iconSet ) );
    else
        iconSetChanged( 0 );
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
    if ( mDlg->comboBoxIconSet->count() - 1 == mDlg->comboBoxIconSet->currentIndex() )
        iconSetChanged( mDlg->comboBoxIconSet->currentIndex() );
    changed( true );
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
    KColorScheme scheme(QPalette::Active, KColorScheme::View);
    QPixmap sampleIcon( 22, 22 );
    sampleIcon.fill( Qt::transparent );
    QPainter p( &sampleIcon );
    p.setBrush( Qt::NoBrush );
    p.setOpacity( 1.0 );
    p.setFont( setIconFont( incomingText ) );
    if ( active )
        p.setPen( mDlg->kColorButtonIncoming->color() );
    else
        p.setPen( scheme.foreground( KColorScheme::InactiveText ).color() );
    p.drawText( sampleIcon.rect(), Qt::AlignTop | Qt::AlignRight, incomingText );
    p.setFont( setIconFont( outgoingText ) );
    if ( active )
        p.setPen( mDlg->kColorButtonOutgoing->color() );
    p.drawText( sampleIcon.rect(), Qt::AlignBottom | Qt::AlignRight, outgoingText );
    return sampleIcon;
}

void ConfigDialog::iconSetChanged( int set )
{
    if ( !mDlg->listBoxInterfaces->currentItem() )
        return;

    QListWidgetItem* selected = mDlg->listBoxInterfaces->currentItem();

    InterfaceSettings* settings = mSettingsMap[selected->text()];
    if ( mDlg->comboBoxIconSet->count() - 1 == mDlg->comboBoxIconSet->currentIndex() )
    {
        // empty iconSet = Text icon
        settings->iconSet.clear();
        // Update the preview of the iconset.
        mDlg->pixmapDisconnected->setPixmap( textIcon( "0B", "0B", false ) );
        mDlg->pixmapConnected->setPixmap( textIcon( "0B", "0B", true ) );
        mDlg->pixmapIncoming->setPixmap( textIcon( "123K", "0B", true ) );
        mDlg->pixmapOutgoing->setPixmap( textIcon( "0B", "12K", true ) );
        mDlg->pixmapTraffic->setPixmap( textIcon( "123K", "12K", true ) );
    }
    else
    {
        settings->iconSet = mDlg->comboBoxIconSet->itemText( set );
        // Update the preview of the iconset.
        KIconLoader::global()->addAppDir( "knemo" );
        mDlg->pixmapDisconnected->setPixmap( UserIcon( settings->iconSet + ICON_DISCONNECTED ) );
        mDlg->pixmapConnected->setPixmap( UserIcon( settings->iconSet + ICON_CONNECTED ) );
        mDlg->pixmapIncoming->setPixmap( UserIcon( settings->iconSet + ICON_INCOMING ) );
        mDlg->pixmapOutgoing->setPixmap( UserIcon( settings->iconSet + ICON_OUTGOING ) );
        mDlg->pixmapTraffic->setPixmap( UserIcon( settings->iconSet + ICON_TRAFFIC ) );
    }
    if (!mLock) changed( true );
}

void ConfigDialog::checkBoxNotConnectedToggled( bool on )
{
    if ( !mDlg->listBoxInterfaces->currentItem() )
        return;

    QListWidgetItem* selected = mDlg->listBoxInterfaces->currentItem();

    InterfaceSettings* settings = mSettingsMap[selected->text()];
    settings->hideWhenNotAvailable = on;
    if (!mLock) changed( true );
}

void ConfigDialog::checkBoxNotExistingToggled( bool on )
{
    if ( !mDlg->listBoxInterfaces->currentItem() )
        return;

    QListWidgetItem* selected = mDlg->listBoxInterfaces->currentItem();

    InterfaceSettings* settings = mSettingsMap[selected->text()];
    settings->hideWhenNotExisting = on;
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

    // enable or disable statistics entries
    updateStatisticsEntries();
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

void ConfigDialog::updateStatisticsEntries( void )
{
    bool statisticsActive = false;
    foreach ( InterfaceSettings *it, mSettingsMap )
    {
        if ( it->activateStatistics )
        {
            statisticsActive = true;
            break;
        }
    }

    mDlg->groupBoxStatistics->setEnabled( statisticsActive );
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
