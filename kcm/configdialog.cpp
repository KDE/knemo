/* This file is part of KNemo
   Copyright (C) 2004, 2006 Percy Leonhardt <percy@eris23.de>

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

#include <qfile.h>
#include <qdict.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qcstring.h>
#include <qlistbox.h>
#include <qcheckbox.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qwhatsthis.h>
#include <qpushbutton.h>
#include <qstringlist.h>
#include <qdatastream.h>

#include <kglobal.h>
#include <kconfig.h>
#include <klocale.h>
#include <knuminput.h>
#include <dcopclient.h>
#include <kiconloader.h>
#include <kfiledialog.h>
#include <kcolorbutton.h>
#include <kinputdialog.h>
#include <kapplication.h>
#include <knotifydialog.h>
#include <kstandarddirs.h>
#include <kgenericfactory.h>
#include <kdirselectdialog.h>

#include "configdlg.h"
#include "configdialog.h"

const QString ConfigDialog::ICON_DISCONNECTED = "network_disconnected";
const QString ConfigDialog::ICON_CONNECTED = "network_connected";
const QString ConfigDialog::ICON_INCOMING = "network_incoming";
const QString ConfigDialog::ICON_OUTGOING = "network_outgoing";
const QString ConfigDialog::ICON_TRAFFIC = "network_traffic";
const QString ConfigDialog::SUFFIX_PPP = "_ppp";
const QString ConfigDialog::SUFFIX_LAN = "_lan";
const QString ConfigDialog::SUFFIX_WLAN = "_wlan";

typedef KGenericFactory<ConfigDialog, QWidget> KNemoFactory;
K_EXPORT_COMPONENT_FACTORY( kcm_knemo, KNemoFactory("kcm_knemo") )

ConfigDialog::ConfigDialog( QWidget *parent, const char *name, const QStringList& )
    : KCModule( KNemoFactory::instance(), parent, name ),
      mLock( false ),
      mDlg( new ConfigDlg( this ) ),
      mColorVLines( 0x04FB1D ),
      mColorHLines( 0x04FB1D ),
      mColorIncoming( 0x1889FF ),
      mColorOutgoing( 0xFF7F08 ),
      mColorBackground( 0x313031 )
{
    KGlobal::locale()->insertCatalogue("kcm_knemo");
    setupToolTipArray();
    load();

    QVBoxLayout* top = new QVBoxLayout(this);
    mDlg->pushButtonNew->setPixmap( SmallIcon( "filenew" ) );
    mDlg->pushButtonDelete->setPixmap( SmallIcon( "editdelete" ) );
    mDlg->pushButtonAddCommand->setPixmap( SmallIcon( "filenew" ) );
    mDlg->pushButtonRemoveCommand->setPixmap( SmallIcon( "editdelete" ) );
    mDlg->pushButtonUp->setPixmap( SmallIcon( "1uparrow" ) );
    mDlg->pushButtonDown->setPixmap( SmallIcon( "1downarrow" ) );
    mDlg->pushButtonAddToolTip->setPixmap( SmallIcon( "1rightarrow" ) );
    mDlg->pushButtonRemoveToolTip->setPixmap( SmallIcon( "1leftarrow" ) );
    mDlg->listViewCommands->setSorting( -1 );
    QWhatsThis::add( mDlg->listViewCommands,
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

    mSettingsDict.setAutoDelete( true );
    setButtons( KCModule::Default | KCModule::Apply );

    connect( mDlg->pushButtonNew, SIGNAL( clicked() ),
             this, SLOT( buttonNewSelected() ) );
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
    connect( mDlg->pushButtonStatisticsDir, SIGNAL( clicked() ),
             this, SLOT( buttonStatisticsDirSelected() ) );
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
    connect( mDlg->spinBoxTrafficThreshold, SIGNAL( valueChanged( int ) ),
             this, SLOT( spinBoxTrafficValueChanged ( int ) ) );
    connect( mDlg->checkBoxCustom, SIGNAL( toggled( bool ) ),
             this, SLOT( checkBoxCustomToggled ( bool ) ) );
    connect( mDlg->listBoxInterfaces, SIGNAL( highlighted( const QString& ) ),
             this, SLOT( interfaceSelected( const QString& ) ) );
    connect( mDlg->listViewCommands, SIGNAL( selectionChanged() ),
             this, SLOT( listViewCommandsSelectionChanged() ) );
    connect( mDlg->listViewCommands, SIGNAL( itemRenamed( QListViewItem*, int, const QString& ) ),
             this, SLOT( listViewCommandsRenamed( QListViewItem*, int, const QString& ) ) );

    // connect the plotter widgets
    connect( mDlg->checkBoxTopBar, SIGNAL( toggled( bool ) ),
             this, SLOT( checkBoxToggled( bool ) ) );
    connect( mDlg->checkBoxLabels, SIGNAL( toggled( bool ) ),
             this, SLOT( checkBoxToggled( bool ) ) );
    connect( mDlg->checkBoxVLines, SIGNAL( toggled( bool ) ),
             this, SLOT( checkBoxToggled( bool ) ) );
    connect( mDlg->checkBoxHLines, SIGNAL( toggled( bool ) ),
             this, SLOT( checkBoxToggled( bool ) ) );
    connect( mDlg->checkBoxIncoming, SIGNAL( toggled( bool ) ),
             this, SLOT( checkBoxToggled( bool ) ) );
    connect( mDlg->checkBoxOutgoing, SIGNAL( toggled( bool ) ),
             this, SLOT( checkBoxToggled( bool ) ) );
    connect( mDlg->checkBoxVLinesScroll, SIGNAL( toggled( bool ) ),
             this, SLOT( checkBoxToggled( bool ) ) );
    connect( mDlg->checkBoxAutoDetection, SIGNAL( toggled( bool ) ),
             this, SLOT( checkBoxToggled( bool ) ) );
    connect( mDlg->spinBoxCount, SIGNAL( valueChanged( int ) ),
             this, SLOT( spinBoxValueChanged( int ) ) );
    connect( mDlg->spinBoxPixel, SIGNAL( valueChanged( int ) ),
             this, SLOT( spinBoxValueChanged( int ) ) );
    connect( mDlg->spinBoxDistance, SIGNAL( valueChanged( int ) ),
             this, SLOT( spinBoxValueChanged( int ) ) );
    connect( mDlg->spinBoxFontSize, SIGNAL( valueChanged( int ) ),
             this, SLOT( spinBoxValueChanged( int ) ) );
    connect( mDlg->spinBoxMinValue, SIGNAL( valueChanged( int ) ),
             this, SLOT( spinBoxValueChanged( int ) ) );
    connect( mDlg->spinBoxMaxValue, SIGNAL( valueChanged( int ) ),
             this, SLOT( spinBoxValueChanged( int ) ) );
    connect( mDlg->numInputPollInterval, SIGNAL( valueChanged( int ) ),
             this, SLOT( spinBoxValueChanged( int ) ) );
    connect( mDlg->numInputSaveInterval, SIGNAL( valueChanged( int ) ),
             this, SLOT( spinBoxValueChanged( int ) ) );
    connect( mDlg->kColorButtonVLines, SIGNAL( changed( const QColor& ) ),
             this, SLOT( kColorButtonChanged( const QColor& ) ) );
    connect( mDlg->kColorButtonHLines, SIGNAL( changed( const QColor& ) ),
             this, SLOT( kColorButtonChanged( const QColor& ) ) );
    connect( mDlg->kColorButtonIncoming, SIGNAL( changed( const QColor& ) ),
             this, SLOT( kColorButtonChanged( const QColor& ) ) );
    connect( mDlg->kColorButtonOutgoing, SIGNAL( changed( const QColor& ) ),
             this, SLOT( kColorButtonChanged( const QColor& ) ) );
    connect( mDlg->kColorButtonBackground, SIGNAL( changed( const QColor& ) ),
             this, SLOT( kColorButtonChanged( const QColor& ) ) );

    // In case the user opened the control center via the context menu
    // this call to the daemon will deliver the interface the menu
    // belongs to. This way we can preselect the appropriate entry in the list.
    QCString replyType;
    QByteArray replyData, arg;
    QString selectedInterface = QString::null;
    if ( kapp->dcopClient()->call( "kded", "knemod", "getSelectedInterface()", arg, replyType, replyData ) )
    {
        QDataStream reply( replyData,  IO_ReadOnly );
        reply >> selectedInterface;
    }

    if ( selectedInterface != QString::null )
    {
        // Try to preselect the interface.
        unsigned int i;
        for ( i = 0; i < mDlg->listBoxInterfaces->count(); i++ )
        {
            QListBoxItem* item = mDlg->listBoxInterfaces->item( i );
            if ( item->text() == selectedInterface )
            {
                // Found it.
                mDlg->listBoxInterfaces->setSelected( i, true );
                break;
            }
        }
        if ( i == mDlg->listBoxInterfaces->count() )
            // Not found. Select first entry in list.
            mDlg->listBoxInterfaces->setSelected( 0, true );
    }
    else
        // No interface from daemon. Select first entry in list.
        mDlg->listBoxInterfaces->setSelected( 0, true );

    top->add( mDlg );
}

ConfigDialog::~ConfigDialog()
{
    delete mDlg;
}

void ConfigDialog::load()
{
    mSettingsDict.clear();
    mDlg->listBoxInterfaces->clear();
    KConfig* config = new KConfig( "knemorc", true );

    config->setGroup( "General" );
    mDlg->numInputPollInterval->setValue( config->readNumEntry( "PollInterval", 1 ) );
    mDlg->numInputSaveInterval->setValue( config->readNumEntry( "SaveInterval", 60 ) );
    mDlg->lineEditStatisticsDir->setText( config->readEntry( "StatisticsDir", KGlobal::dirs()->saveLocation( "data", "knemo/" ) ) );
    mToolTipContent = config->readNumEntry( "ToolTipContent", 2 );
    QStrList list;
    int numEntries = config->readListEntry( "Interfaces", list );

    if ( numEntries == 0 )
        return;

    char* interface;
    for ( interface = list.first(); interface; interface = list.next() )
    {
        QString group( "Interface_" );
        group += interface;
        InterfaceSettings* settings = new InterfaceSettings();
        if ( config->hasGroup( group ) )
        {
            config->setGroup( group );
            settings->alias = config->readEntry( "Alias" );
            settings->iconSet = config->readNumEntry( "IconSet", 0 );
            settings->customCommands = config->readBoolEntry( "CustomCommands" );
            settings->hideWhenNotAvailable = config->readBoolEntry( "HideWhenNotAvailable" );
            settings->hideWhenNotExisting = config->readBoolEntry( "HideWhenNotExisting" );
            settings->activateStatistics = config->readBoolEntry( "ActivateStatistics" );
            settings->trafficThreshold = config->readNumEntry( "TrafficThreshold", 0 );
            int numCommands = config->readNumEntry( "NumCommands" );
            for ( int i = 0; i < numCommands; i++ )
            {
                QString entry;
                InterfaceCommand cmd;
                entry = QString( "RunAsRoot%1" ).arg( i + 1 );
                cmd.runAsRoot = config->readBoolEntry( entry );
                entry = QString( "Command%1" ).arg( i + 1 );
                cmd.command = config->readEntry( entry );
                entry = QString( "MenuText%1" ).arg( i + 1 );
                cmd.menuText = config->readEntry( entry );
                settings->commands.append( cmd );
            }
        }
        mSettingsDict.insert( interface, settings );
        mDlg->listBoxInterfaces->insertItem( interface );
    }

    // Set the plotter widgets
    config->setGroup( "PlotterSettings" );
    mDlg->spinBoxPixel->setValue( config->readNumEntry( "Pixel", 1 ) );
    mDlg->spinBoxCount->setValue( config->readNumEntry( "Count", 5 ) );
    mDlg->spinBoxDistance->setValue( config->readNumEntry( "Distance", 30 ) );
    mDlg->spinBoxFontSize->setValue( config->readNumEntry( "FontSize", 8 ) );
    mDlg->spinBoxMinValue->setValue( config->readNumEntry( "MinimumValue", 0 ) );
    mDlg->spinBoxMaxValue->setValue( config->readNumEntry( "MaximumValue", 1 ) );
    mDlg->checkBoxLabels->setChecked( config->readBoolEntry( "Labels", true ) );
    mDlg->checkBoxTopBar->setChecked( config->readBoolEntry( "TopBar", false ) );
    mDlg->checkBoxVLines->setChecked( config->readBoolEntry( "VerticalLines", true ) );
    mDlg->checkBoxHLines->setChecked( config->readBoolEntry( "HorizontalLines", true ) );
    mDlg->checkBoxIncoming->setChecked( config->readBoolEntry( "ShowIncoming", true ) );
    mDlg->checkBoxOutgoing->setChecked( config->readBoolEntry( "ShowOutgoing", true ) );
    mDlg->checkBoxAutoDetection->setChecked( config->readBoolEntry( "AutomaticDetection", true ) );
    mDlg->checkBoxVLinesScroll->setChecked( config->readBoolEntry( "VerticalLinesScroll", true ) );
    mDlg->kColorButtonVLines->setColor( config->readColorEntry( "ColorVLines", &mColorVLines ) );
    mDlg->kColorButtonHLines->setColor( config->readColorEntry( "ColorHLines", &mColorHLines ) );
    mDlg->kColorButtonIncoming->setColor( config->readColorEntry( "ColorIncoming", &mColorIncoming ) );
    mDlg->kColorButtonOutgoing->setColor( config->readColorEntry( "ColorOutgoing", &mColorOutgoing ) );
    mDlg->kColorButtonBackground->setColor( config->readColorEntry( "ColorBackground", &mColorBackground ) );

    delete config;

    // These things need to be here so that 'Reset' from the control
    // center is handled correctly.
    setupToolTipTab();
}

void ConfigDialog::save()
{
    KConfig* config = new KConfig( "knemorc", false );

    QStringList list;
    QDictIterator<InterfaceSettings> it( mSettingsDict );
    for ( ; it.current(); ++it )
    {
        list.append( it.currentKey() );
        InterfaceSettings* settings = it.current();
        config->setGroup( "Interface_" + it.currentKey() );
        if ( settings->alias.isEmpty() )
            config->deleteEntry( "Alias" );
        else
            config->writeEntry( "Alias", settings->alias );
        config->writeEntry( "IconSet", settings->iconSet );
        config->writeEntry( "CustomCommands", settings->customCommands );
        config->writeEntry( "HideWhenNotAvailable", settings->hideWhenNotAvailable );
        config->writeEntry( "HideWhenNotExisting", settings->hideWhenNotExisting );
        config->writeEntry( "ActivateStatistics", settings->activateStatistics );
        config->writeEntry( "TrafficThreshold", settings->trafficThreshold );
        config->writeEntry( "NumCommands", settings->commands.size() );
        for ( uint i = 0; i < settings->commands.size(); i++ )
        {
            QString entry;
            entry = QString( "RunAsRoot%1" ).arg( i + 1 );
            config->writeEntry( entry, settings->commands[i].runAsRoot );
            entry = QString( "Command%1" ).arg( i + 1 );
            config->writeEntry( entry, settings->commands[i].command );
            entry = QString( "MenuText%1" ).arg( i + 1 );
            config->writeEntry( entry, settings->commands[i].menuText );
        }
    }

    config->setGroup( "General" );
    config->writeEntry( "PollInterval", mDlg->numInputPollInterval->value() );
    config->writeEntry( "SaveInterval", mDlg->numInputSaveInterval->value() );
    config->writeEntry( "StatisticsDir",  mDlg->lineEditStatisticsDir->text() );
    config->writeEntry( "ToolTipContent", mToolTipContent );
    config->writeEntry( "Interfaces", list );

    config->setGroup( "PlotterSettings" );
    config->writeEntry( "Pixel", mDlg->spinBoxPixel->value() );
    config->writeEntry( "Count", mDlg->spinBoxCount->value() );
    config->writeEntry( "Distance", mDlg->spinBoxDistance->value() );
    config->writeEntry( "FontSize", mDlg->spinBoxFontSize->value() );
    config->writeEntry( "MinimumValue", mDlg->spinBoxMinValue->value() );
    config->writeEntry( "MaximumValue", mDlg->spinBoxMaxValue->value() );
    config->writeEntry( "Labels", mDlg->checkBoxLabels->isChecked() );
    config->writeEntry( "TopBar", mDlg->checkBoxTopBar->isChecked() );
    config->writeEntry( "VerticalLines", mDlg->checkBoxVLines->isChecked() );
    config->writeEntry( "HorizontalLines", mDlg->checkBoxHLines->isChecked() );
    config->writeEntry( "ShowIncoming", mDlg->checkBoxIncoming->isChecked() );
    config->writeEntry( "ShowOutgoing", mDlg->checkBoxOutgoing->isChecked() );
    config->writeEntry( "AutomaticDetection", mDlg->checkBoxAutoDetection->isChecked() );
    config->writeEntry( "VerticalLinesScroll", mDlg->checkBoxVLinesScroll->isChecked() );
    config->writeEntry( "ColorVLines", mDlg->kColorButtonVLines->color() );
    config->writeEntry( "ColorHLines", mDlg->kColorButtonHLines->color() );
    config->writeEntry( "ColorIncoming", mDlg->kColorButtonIncoming->color() );
    config->writeEntry( "ColorOutgoing", mDlg->kColorButtonOutgoing->color() );
    config->writeEntry( "ColorBackground", mDlg->kColorButtonBackground->color() );

    config->sync();
    delete config;
    kapp->dcopClient()->send( "kded", "knemod", "reparseConfiguration()", "" );
}

void ConfigDialog::defaults()
{
    // Default interfaces
    QFile proc( "/proc/net/dev" );
    if ( proc.open( IO_ReadOnly ) )
    {
        mSettingsDict.clear();
        mDlg->listBoxInterfaces->clear();

        QString file =  proc.readAll();
        QStringList content = QStringList::split( "\n", file );
        if ( content.count() > 2 )
        {
            for ( unsigned int i = 2; i < content.count(); i++ )
            {
                QString interface = content[i].simplifyWhiteSpace();
                interface = interface.left( interface.find( ':' ) );
                if ( interface == "lo" )
                    continue;

                InterfaceSettings* settings = new InterfaceSettings();
                settings->customCommands = false;
                settings->hideWhenNotAvailable = false;
                settings->hideWhenNotExisting = false;
                settings->activateStatistics = false;
                mSettingsDict.insert( interface, settings );
                mDlg->listBoxInterfaces->insertItem( interface );
            }
            if ( mDlg->listBoxInterfaces->count() > 0 )
            {
                mDlg->listBoxInterfaces->setSelected( 0, true );
            }
            else
            {
                mDlg->lineEditAlias->setText( QString::null );
                mDlg->comboBoxIconSet->setCurrentItem( 0 );
                mDlg->checkBoxNotConnected->setChecked( false );
                mDlg->checkBoxNotExisting->setChecked( false );
                mDlg->checkBoxStatistics->setChecked( false );
                mDlg->checkBoxCustom->setChecked( false );
            }
        }
        proc.close();
    }

    // Default misc settings
    mDlg->numInputPollInterval->setValue( 1 );
    mDlg->numInputSaveInterval->setValue( 60 );
    mDlg->lineEditStatisticsDir->setText( KGlobal::dirs()->saveLocation( "data", "knemo/" ) );

    // Default tool tips
    mToolTipContent = 2;
    setupToolTipTab();

    // Default plotter settings
    mDlg->spinBoxPixel->setValue( 1 );
    mDlg->spinBoxCount->setValue( 5 );
    mDlg->spinBoxDistance->setValue( 30 );
    mDlg->spinBoxFontSize->setValue( 8 );
    mDlg->spinBoxMinValue->setValue( 0 );
    mDlg->spinBoxMaxValue->setValue( 1 );
    mDlg->checkBoxLabels->setChecked( true );
    mDlg->checkBoxTopBar->setChecked( false );
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
        mDlg->listBoxInterfaces->insertItem( ifname );
        mSettingsDict.insert( ifname, new InterfaceSettings() );
        mDlg->listBoxInterfaces->setSelected( mDlg->listBoxInterfaces->count() - 1, true );
        changed( true );
    }
}

void ConfigDialog::buttonDeleteSelected()
{
    QListBoxItem* selected = mDlg->listBoxInterfaces->selectedItem();

    if ( selected == 0 )
        return;

    // TODO: find a better way than blocking signals
    mSettingsDict.remove( selected->text() );
    mDlg->lineEditAlias->blockSignals( true );
    mDlg->lineEditAlias->setText( QString::null );
    mDlg->lineEditAlias->blockSignals( false );
    mDlg->comboBoxIconSet->blockSignals( true );
    mDlg->comboBoxIconSet->setCurrentItem( 0 );
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
    mDlg->listBoxInterfaces->removeItem( mDlg->listBoxInterfaces->index( selected ) );
    changed( true );
}

void ConfigDialog::buttonAddCommandSelected()
{
    QListBoxItem* selected = mDlg->listBoxInterfaces->selectedItem();

    if ( selected == 0 )
        return;

    KNemoCheckListItem* item = new KNemoCheckListItem( mDlg->listViewCommands );
    item->setRenameEnabled( 1, true );
    item->setRenameEnabled( 2, true );
    connect( item, SIGNAL( stateChanged( KNemoCheckListItem*, bool ) ),
             this, SLOT( listViewCommandsCheckListItemChanged( KNemoCheckListItem*, bool ) ) );
    InterfaceSettings* settings = mSettingsDict[selected->text()];

    QValueVector<InterfaceCommand> cmds;
    QListViewItem* i = mDlg->listViewCommands->firstChild();
    for ( ; i != 0; i = i->nextSibling() )
    {
        InterfaceCommand cmd;
        cmd.runAsRoot = static_cast<KNemoCheckListItem*>( i )->isOn();
        cmd.menuText = i->text( 1 );
        cmd.command = i->text( 2 );
        cmds.append( cmd );
    }

    settings->commands = cmds;
    if (!mLock) changed( true );
}

void ConfigDialog::buttonRemoveCommandSelected()
{
    QListBoxItem* selected = mDlg->listBoxInterfaces->selectedItem();

    if ( selected == 0 )
        return;

    delete mDlg->listViewCommands->selectedItem();
    InterfaceSettings* settings = mSettingsDict[selected->text()];

    QValueVector<InterfaceCommand> cmds;
    QListViewItem* i = mDlg->listViewCommands->firstChild();
    for ( ; i != 0; i = i->nextSibling() )
    {
        InterfaceCommand cmd;
        cmd.runAsRoot = static_cast<KNemoCheckListItem*>( i )->isOn();
        cmd.menuText = i->text( 1 );
        cmd.command = i->text( 2 );
        cmds.append( cmd );
    }

    settings->commands = cmds;
    if (!mLock) changed( true );
}

void ConfigDialog::buttonCommandUpSelected()
{
    QListBoxItem* selected = mDlg->listBoxInterfaces->selectedItem();

    if ( selected == 0 )
        return;

    QListViewItem* item = mDlg->listViewCommands->selectedItem();
    if ( item )
    {
        QListViewItem* previous = item->itemAbove();
        if ( previous )
        {
            // We can move one up.
            previous = previous->itemAbove();
            if ( previous )
                item->moveItem( previous );
            else
            {
                mDlg->listViewCommands->takeItem( item );
                mDlg->listViewCommands->insertItem( item );
                mDlg->listViewCommands->setSelected( item, true );
            }
        }
    }

    InterfaceSettings* settings = mSettingsDict[selected->text()];

    QValueVector<InterfaceCommand> cmds;
    QListViewItem* i = mDlg->listViewCommands->firstChild();
    for ( ; i != 0; i = i->nextSibling() )
    {
        InterfaceCommand cmd;
        cmd.runAsRoot = static_cast<KNemoCheckListItem*>( i )->isOn();
        cmd.menuText = i->text( 1 );
        cmd.command = i->text( 2 );
        cmds.append( cmd );
    }

    settings->commands = cmds;
    if (!mLock) changed( true );
}

void ConfigDialog::buttonCommandDownSelected()
{
    QListBoxItem* selected = mDlg->listBoxInterfaces->selectedItem();

    if ( selected == 0 )
        return;

    QListViewItem* item = mDlg->listViewCommands->selectedItem();
    if ( item )
    {
        QListViewItem* next = item->itemBelow();
        if ( next )
        {
            // We can move one down.
            item->moveItem( next );
        }
    }

    InterfaceSettings* settings = mSettingsDict[selected->text()];

    QValueVector<InterfaceCommand> cmds;
    QListViewItem* i = mDlg->listViewCommands->firstChild();
    for ( ; i != 0; i = i->nextSibling() )
    {
        InterfaceCommand cmd;
        cmd.runAsRoot = static_cast<KNemoCheckListItem*>( i )->isOn();
        cmd.menuText = i->text( 1 );
        cmd.command = i->text( 2 );
        cmds.append( cmd );
    }

    settings->commands = cmds;
    if (!mLock) changed( true );
}

void ConfigDialog::buttonAddToolTipSelected()
{
    // Support extended selection
    for ( int k = mDlg->listBoxAvailable->count() - 1; k >= 0; k-- )
    {
        if ( !mDlg->listBoxAvailable->isSelected( k ) )
            continue;

        QListBoxItem* selected = mDlg->listBoxAvailable->item( k );

        if ( selected == 0 )
            continue;

        // Find the index of the selected item in the tooltip array.
        int itemIndex = 0;
        for ( int i = 0; mToolTips[i].first != QString::null; i++ )
        {
            if ( mToolTips[i].first == selected->text() )
            {
                itemIndex = i;
                break;
            }
        }

        // Find the first item in the display list which has a larger
        // index in the tooltip array. We have to insert the selected
        // item just before this one.
        int newIndex = -1;
        for ( unsigned int i = 0; i < mDlg->listBoxDisplay->count(); i++ )
        {
            // For every item in the display list find its index in
            // the tooltip array.
            int siblingIndex = 0;
            QListBoxItem* item = mDlg->listBoxDisplay->item( i );
            for ( int j = 0; mToolTips[j].first != QString::null; j++ )
            {
                if ( mToolTips[j].first == item->text() )
                {
                    siblingIndex = j;
                    break;
                }
            }

            // Check if the index of this item is larger than the index
            // of the selected item.
            if ( siblingIndex > itemIndex )
            {
                // Insert the selected item at position newIndex placing it
                // directly in front of the sibling.
                newIndex = i;
                break;
            }
        }

        mDlg->listBoxAvailable->setSelected( selected, false );
        mDlg->listBoxAvailable->takeItem( selected );
        mDlg->listBoxDisplay->insertItem( selected, newIndex );
        if ( mDlg->listBoxAvailable->count() == 0 )
            mDlg->pushButtonAddToolTip->setEnabled( false );
        if ( mDlg->listBoxDisplay->count() == 1 )
            mDlg->pushButtonRemoveToolTip->setEnabled( true );

        mToolTipContent += mToolTips[itemIndex].second;
        changed( true );
    }
}

void ConfigDialog::buttonRemoveToolTipSelected()
{
    // Support extended selection
    for ( int k = mDlg->listBoxDisplay->count() - 1; k >= 0; k-- )
    {
        if ( !mDlg->listBoxDisplay->isSelected( k ) )
            continue;

        QListBoxItem* selected = mDlg->listBoxDisplay->item( k );

        if ( selected == 0 )
            continue;

        // Find the index of the selected item in the tooltip array.
        int itemIndex = 0;
        for ( int i = 0; mToolTips[i].first != QString::null; i++ )
        {
            if ( mToolTips[i].first == selected->text() )
            {
                itemIndex = i;
                break;
            }
        }

        // Find the first item in the available list which has a larger
        // index in the tooltip array. We have to insert the selected
        // item just before this one.
        int newIndex = -1;
        for ( unsigned int i = 0; i < mDlg->listBoxAvailable->count(); i++ )
        {
            // For every item in the available list find its index in
            // the tooltip array.
            int siblingIndex = 0;
            QListBoxItem* item = mDlg->listBoxAvailable->item( i );
            for ( int j = 0; mToolTips[j].first != QString::null; j++ )
            {
                if ( mToolTips[j].first == item->text() )
                {
                    siblingIndex = j;
                    break;
                }
            }

            // Check if the index of this item is larger than the index
            // of the selected item.
            if ( siblingIndex > itemIndex )
            {
                // Insert the selected item at position newIndex placing it
                // directly in front of the sibling.
                newIndex = i;
                break;
            }
        }

        mDlg->listBoxDisplay->setSelected( selected, false );
        mDlg->listBoxDisplay->takeItem( selected );
        mDlg->listBoxAvailable->insertItem( selected, newIndex );
        if ( mDlg->listBoxDisplay->count() == 0 )
            mDlg->pushButtonRemoveToolTip->setEnabled( false );
        if ( mDlg->listBoxAvailable->count() == 1 )
            mDlg->pushButtonAddToolTip->setEnabled( true );

        mToolTipContent -= mToolTips[itemIndex].second;
        changed( true );
    }
}

void ConfigDialog::buttonNotificationsSelected()
{
    KNotifyDialog dialog( this );
    dialog.addApplicationEvents( "knemo" );
    dialog.exec();
}

void ConfigDialog:: buttonStatisticsDirSelected()
{
    KURL url = KDirSelectDialog::selectDirectory();
    if ( url.path() != QString::null )
    {
        mDlg->lineEditStatisticsDir->setText( url.path() );
        changed( true );
    }
}

void ConfigDialog::interfaceSelected( const QString& interface )
{
    InterfaceSettings* settings = mSettingsDict[interface];
    mLock = true;
    mDlg->lineEditAlias->setText( settings->alias );
    mDlg->comboBoxIconSet->setCurrentItem( settings->iconSet );
    mDlg->checkBoxCustom->setChecked( settings->customCommands );
    mDlg->checkBoxNotConnected->setChecked( settings->hideWhenNotAvailable );
    mDlg->checkBoxNotExisting->setChecked( settings->hideWhenNotExisting );
    mDlg->checkBoxStatistics->setChecked( settings->activateStatistics );
    mDlg->spinBoxTrafficThreshold->setValue( settings->trafficThreshold );

    mDlg->listViewCommands->clear();
    for ( int i = settings->commands.size() - 1; i >= 0; i-- )
    {
        KNemoCheckListItem* item = new KNemoCheckListItem( mDlg->listViewCommands );
        item->setOn( settings->commands[i].runAsRoot );
        item->setText( 1, settings->commands[i].menuText );
        item->setRenameEnabled( 1, true );
        item->setText( 2, settings->commands[i].command );
        item->setRenameEnabled( 2, true );
        connect( item, SIGNAL( stateChanged( KNemoCheckListItem*, bool ) ),
                 this, SLOT( listViewCommandsCheckListItemChanged( KNemoCheckListItem*, bool ) ) );
    }

    iconSetChanged( settings->iconSet ); // to update iconset preview
    mLock = false;
}

void ConfigDialog::aliasChanged( const QString& text )
{
    QListBoxItem* selected = mDlg->listBoxInterfaces->selectedItem();

    if ( selected == 0 )
        return;

    InterfaceSettings* settings = mSettingsDict[selected->text()];
    settings->alias = text;
    if (!mLock) changed( true );
}

void ConfigDialog::iconSetChanged( int set )
{
    QListBoxItem* selected = mDlg->listBoxInterfaces->selectedItem();

    if ( selected == 0 )
        return;

    InterfaceSettings* settings = mSettingsDict[selected->text()];
    settings->iconSet = set;

    // Update the preview of the iconset.
    QString suffix;
    switch ( set )
    {
    case NETWORK:
        suffix = SUFFIX_LAN;
        break;
    case WIRELESS:
        suffix = SUFFIX_WLAN;
        break;
    case MODEM:
        suffix = SUFFIX_PPP;
        break;
    default:
        suffix = ""; // use standard icons
    }

    mDlg->pixmapDisconnected->setPixmap( SmallIcon( ICON_DISCONNECTED + suffix ) );
    mDlg->pixmapConnected->setPixmap( SmallIcon( ICON_CONNECTED + suffix ) );
    mDlg->pixmapIncoming->setPixmap( SmallIcon( ICON_INCOMING + suffix ) );
    mDlg->pixmapOutgoing->setPixmap( SmallIcon( ICON_OUTGOING + suffix ) );
    mDlg->pixmapTraffic->setPixmap( SmallIcon( ICON_TRAFFIC + suffix ) );
    if (!mLock) changed( true );
}

void ConfigDialog::checkBoxNotConnectedToggled( bool on )
{
    QListBoxItem* selected = mDlg->listBoxInterfaces->selectedItem();

    if ( selected == 0 )
        return;

    InterfaceSettings* settings = mSettingsDict[selected->text()];
    settings->hideWhenNotAvailable = on;
    if (!mLock) changed( true );
}

void ConfigDialog::checkBoxNotExistingToggled( bool on )
{
    QListBoxItem* selected = mDlg->listBoxInterfaces->selectedItem();

    if ( selected == 0 )
        return;

    InterfaceSettings* settings = mSettingsDict[selected->text()];
    settings->hideWhenNotExisting = on;
    if (!mLock) changed( true );
}


void ConfigDialog::checkBoxStatisticsToggled( bool on )
{
    QListBoxItem* selected = mDlg->listBoxInterfaces->selectedItem();

    if ( selected == 0 )
        return;

    InterfaceSettings* settings = mSettingsDict[selected->text()];
    settings->activateStatistics = on;
    if (!mLock) changed( true );
}

void ConfigDialog::spinBoxTrafficValueChanged( int value )
{
    QListBoxItem* selected = mDlg->listBoxInterfaces->selectedItem();

    if ( selected == 0 )
        return;

    InterfaceSettings* settings = mSettingsDict[selected->text()];
    settings->trafficThreshold = value;
    if (!mLock) changed( true );
}

void ConfigDialog::checkBoxCustomToggled( bool on )
{
    QListBoxItem* selected = mDlg->listBoxInterfaces->selectedItem();

    if ( selected == 0 )
        return;

    InterfaceSettings* settings = mSettingsDict[selected->text()];
    settings->customCommands = on;
    if ( on )
        if ( mDlg->listViewCommands->selectedItem() )
            mDlg->pushButtonRemoveCommand->setEnabled( true );
    else
        mDlg->pushButtonRemoveCommand->setEnabled( false );

    if (!mLock) changed( true );
}

void ConfigDialog::setupToolTipTab()
{
    mDlg->listBoxDisplay->clear();
    mDlg->listBoxAvailable->clear();

    for ( int i = 0; mToolTips[i].first != QString::null; i++ )
    {
        if ( mToolTipContent & mToolTips[i].second )
            mDlg->listBoxDisplay->insertItem( mToolTips[i].first );
        else
            mDlg->listBoxAvailable->insertItem( mToolTips[i].first );
    }

    if ( mDlg->listBoxDisplay->count() > 0 )
    {
        mDlg->listBoxDisplay->setSelected( 0, true );
        mDlg->pushButtonRemoveToolTip->setEnabled( true );
    }
    else
        mDlg->pushButtonRemoveToolTip->setEnabled( false );

    if ( mDlg->listBoxAvailable->count() > 0 )
    {
        mDlg->listBoxAvailable->setSelected( 0, true );
        mDlg->pushButtonAddToolTip->setEnabled( true );
    }
    else
        mDlg->pushButtonAddToolTip->setEnabled( false );
}

void ConfigDialog::setupToolTipArray()
{
    // Cannot make this data static as the i18n macro doesn't seem
    // to work when called to early i.e. before setting the catalogue.
    mToolTips[0] = QPair<QString, int>( i18n( "Interface" ), INTERFACE );
    mToolTips[1] = QPair<QString, int>( i18n( "Alias" ), ALIAS );
    mToolTips[2] = QPair<QString, int>( i18n( "Status" ), STATUS );
    mToolTips[3] = QPair<QString, int>( i18n( "Uptime" ), UPTIME );
    mToolTips[4] = QPair<QString, int>( i18n( "IP-Address" ), IP_ADDRESS );
    mToolTips[5] = QPair<QString, int>( i18n( "Subnet Mask" ), SUBNET_MASK );
    mToolTips[6] = QPair<QString, int>( i18n( "HW-Address" ), HW_ADDRESS );
    mToolTips[7] = QPair<QString, int>( i18n( "Broadcast Address" ), BCAST_ADDRESS );
    mToolTips[8] = QPair<QString, int>( i18n( "Default Gateway" ), GATEWAY );
    mToolTips[9] = QPair<QString, int>( i18n( "PtP-Address" ), PTP_ADDRESS );
    mToolTips[10] = QPair<QString, int>( i18n( "Packets Received" ), RX_PACKETS );
    mToolTips[11] = QPair<QString, int>( i18n( "Packets Sent" ), TX_PACKETS );
    mToolTips[12] = QPair<QString, int>( i18n( "Bytes Received" ), RX_BYTES );
    mToolTips[13] = QPair<QString, int>( i18n( "Bytes Sent" ), TX_BYTES );
    mToolTips[14] = QPair<QString, int>( i18n( "Download Speed" ), DOWNLOAD_SPEED );
    mToolTips[15] = QPair<QString, int>( i18n( "Upload Speed" ), UPLOAD_SPEED );
    mToolTips[16] = QPair<QString, int>( i18n( "ESSID" ), ESSID );
    mToolTips[17] = QPair<QString, int>( i18n( "Mode" ), MODE );
    mToolTips[18] = QPair<QString, int>( i18n( "Frequency" ), FREQUENCY );
    mToolTips[19] = QPair<QString, int>( i18n( "Bit Rate" ), BIT_RATE );
    mToolTips[20] = QPair<QString, int>( i18n( "Signal/Noise" ), SIGNAL_NOISE );
    mToolTips[21] = QPair<QString, int>( i18n( "Link Quality" ), LINK_QUALITY );
    mToolTips[22] = QPair<QString, int>();
}

void ConfigDialog::checkBoxToggled( bool )
{
    changed( true );
}

void ConfigDialog::spinBoxValueChanged( int )
{
    changed( true );
}

void ConfigDialog::kColorButtonChanged( const QColor& )
{
    changed( true );
}

void ConfigDialog::listViewCommandsSelectionChanged()
{
    QListViewItem* item = mDlg->listViewCommands->selectedItem();
    if ( item )
        mDlg->pushButtonRemoveCommand->setEnabled( true );
    else
        mDlg->pushButtonRemoveCommand->setEnabled( false );
}

void ConfigDialog::listViewCommandsCheckListItemChanged( KNemoCheckListItem* item, bool state )
{
    QListBoxItem* selected = mDlg->listBoxInterfaces->selectedItem();

    if ( selected == 0 )
        return;

    // Find the row of the item.
    int row = 0;
    bool foundItem = false;
    QListViewItem* i = mDlg->listViewCommands->firstChild();
    for ( ; i != 0; i = i->nextSibling() )
    {
        if ( i == item )
        {
            foundItem = true;
            break;
        }
        row++;
    }

    if ( foundItem )
    {
        InterfaceSettings* settings = mSettingsDict[selected->text()];
        InterfaceCommand& cmd = settings->commands[row];
        cmd.runAsRoot = state;

        if (!mLock) changed( true );
    }
}

void ConfigDialog::listViewCommandsRenamed( QListViewItem* item, int col, const QString & text )
{
    QListBoxItem* selected = mDlg->listBoxInterfaces->selectedItem();

    if ( selected == 0 )
        return;

    // Find the row of the item.
    int row = 0;
    bool foundItem = false;
    QListViewItem* i = mDlg->listViewCommands->firstChild();
    for ( ; i != 0; i = i->nextSibling() )
    {
        if ( i == item )
        {
            foundItem = true;
            break;
        }
        row++;
    }

    if ( foundItem )
    {
        InterfaceSettings* settings = mSettingsDict[selected->text()];
        InterfaceCommand& cmd = settings->commands[row];
        if ( col == 1 )
            cmd.menuText = text;
        else if ( col == 2 )
            cmd.command = text;

        if (!mLock) changed( true );
    }
}


#include "configdialog.moc"
