/* This file is part of KNemo
   Copyright (C) 2004, 2005 Percy Leonhardt <percy@eris23.de>
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

#include <unistd.h>

#include <KHelpMenu>
#include <KIcon>
#include <KAction>
#include <KActionCollection>
#include <KLocale>
#include <KMenu>
#include <KProcess>
#include <KNotification>
#include <KStandardDirs>

#include "data.h"
#include "interface.h"
#include "knemodaemon.h"
#include "interfaceicon.h"
#include "interfacetray.h"

const QString InterfaceIcon::ICON_DISCONNECTED = "_disconnected";
const QString InterfaceIcon::ICON_CONNECTED = "_connected";
const QString InterfaceIcon::ICON_INCOMING = "_incoming";
const QString InterfaceIcon::ICON_OUTGOING = "_outgoing";
const QString InterfaceIcon::ICON_TRAFFIC = "_traffic";

Q_DECLARE_METATYPE(InterfaceCommand)

InterfaceIcon::InterfaceIcon( Interface* interface )
    : QObject(),
      mInterface( interface ),
      mTray( 0L )
{
    commandActions = new KActionCollection( this );
    plotterAction = new KAction( KIcon( "utilities-system-monitor" ),
                       i18n( "&Open Traffic Plotter" ), this );
    statisticsAction = new KAction( KIcon( "view-statistics" ),
                          i18n( "Open &Statistics" ), this );
    configAction = new KAction( KIcon( "configure" ),
                       i18n( "&Configure KNemo..." ), this );
}

InterfaceIcon::~InterfaceIcon()
{
    if ( mTray != 0L )
        delete mTray;
}

void InterfaceIcon::updateStatus( int status )
{
    if ( mTray == 0L )
        return;

    // We need the iconset name in all cases.
    QString iconSet = mInterface->getSettings().iconSet;

    // Now set the correct icon depending on the status of the interface.
    if ( status == Interface::NOT_AVAILABLE ||
         status == Interface::NOT_EXISTING )
    {
        mTray->setIcon( UserIcon( iconSet + ICON_DISCONNECTED ) );
    }
    else if ( ( status & Interface::RX_TRAFFIC ) &&
              ( status & Interface::TX_TRAFFIC ) )
    {
        mTray->setIcon( UserIcon( iconSet + ICON_TRAFFIC ) );
    }
    else if ( status & Interface::RX_TRAFFIC )
    {
        mTray->setIcon( UserIcon( iconSet + ICON_INCOMING ) );
    }
    else if ( status & Interface::TX_TRAFFIC )
    {
        mTray->setIcon( UserIcon( iconSet + ICON_OUTGOING ) );
    }
    else
    {
        mTray->setIcon( UserIcon( iconSet + ICON_CONNECTED ) );
    }
}

void InterfaceIcon::updateToolTip()
{
    if ( mTray == 0L )
        return;
    mTray->updateToolTip();
}

void InterfaceIcon::updateMenu()
{
    if ( mTray == 0L )
        return;

    // Remove all old entries.
    KMenu* menu = (KMenu*)mTray->contextMenu();
    QList<QAction *> actions = menu->actions();
    foreach ( QAction* action, commandActions->actions() )
        menu->removeAction( action );
    commandActions->clear();
    menu->removeAction( statisticsAction );

    InterfaceSettings& settings = mInterface->getSettings();

    // If the user wants custom commands, add them.
    if ( settings.customCommands )
    {
        int i = 0;
        foreach ( InterfaceCommand command, settings.commands )
        {
            QAction *action = new QAction( command.menuText, this );
            action->setData( QVariant::fromValue( command ) );
            commandActions->addAction( QString( "command%1" ).arg( i ), action );
            ++i;
        }
        QAction* sep = menu->addSeparator();
        commandActions->addAction( "sep", sep );
        menu->insertActions( plotterAction, commandActions->actions() );
    }

    if ( settings.activateStatistics )
        menu->insertAction( configAction, statisticsAction );
}

void InterfaceIcon::updateTrayStatus( int previousState )
{
    bool interfaceExists = mInterface->getData().existing;
    bool interfaceAvailable = mInterface->getData().available;
    bool hideWhenNotExisting = mInterface->getSettings().hideWhenNotExisting;
    bool hideWhenNotAvailable = mInterface->getSettings().hideWhenNotAvailable;

    // notification 'interface not available'
    if ( !interfaceAvailable && mTray != 0L &&
         previousState == Interface::AVAILABLE )
    {
        /* When KNemo is starting we don't show the change in connection
         * status as this would be annoying when KDE starts.
         */
        QString title;
        if ( mInterface->getSettings().alias != QString::null )
            title = mInterface->getSettings().alias;
        else
            title = mInterface->getName();

        KNotification::event( "disconnected",
                       title + ":\n" + i18n( "Not connected." ) );
    }

    // notification 'interface not existing'
    if ( !interfaceExists && mTray != 0L &&
         previousState != Interface::UNKNOWN_STATE )
    {
        /* When KNemo is starting we don't show the change in connection
         * status as this would be annoying when KDE starts.
         */
        QString title;
        if ( mInterface->getSettings().alias != QString::null )
            title = mInterface->getSettings().alias;
        else
            title = mInterface->getName();

        KNotification::event( "notexisting",
                              title + ":\n" + i18n( "Not existing." ) );
    }

    /* Remove the icon if
     * - the interface is not available and the option to hide it is selected
     * - the interface does not exist, the option to hide it is selected
     *   and the other option is not selected
     */
    if ( mTray != 0L &&
         ( ( !interfaceAvailable && hideWhenNotAvailable ) ||
           ( !interfaceExists && hideWhenNotExisting && !hideWhenNotAvailable ) ) )
    {
        delete mTray;
        mTray = 0L;
    }
    /* Create the icon if
     * - the interface is available
     * - the interface is not available and the option to hide it is not
     *   selected and the interface does exist
     * - the interface does not exist and the option to hide it is not selected
     *   and the other option is not selected
     */
    else if ( mTray == 0L &&
              ( interfaceAvailable ||
                ( !interfaceAvailable && !hideWhenNotAvailable && interfaceExists ) ||
                ( !interfaceExists && !hideWhenNotExisting && !hideWhenNotAvailable ) ) )
    {
        mTray = new InterfaceTray( mInterface, mInterface->getName() );
        KMenu* menu = (KMenu *)mTray->contextMenu();

        menu->removeAction( menu->actions().at( 0 ) );
        menu->addTitle( KIcon( "knemo" ), "KNemo - " + mInterface->getName() );
        menu->addAction( plotterAction );
        menu->addAction( statisticsAction );
        menu->addAction( configAction );
        KHelpMenu* helpMenu( new KHelpMenu( menu, KNemoDaemon::aboutData(), false ) );
        menu->addMenu( helpMenu->menu() )->setIcon( KIcon( "help-contents" ) );

        connect( menu, SIGNAL( triggered( QAction * ) ),
                 this, SLOT( menuTriggered( QAction * ) ) );
        connect( mTray, SIGNAL( activated( QSystemTrayIcon::ActivationReason ) ),
                 this, SLOT( iconActivated( QSystemTrayIcon::ActivationReason ) ) );
        connect( plotterAction, SIGNAL( triggered() ),
                 this, SLOT( showGraph() ) );
        connect( statisticsAction, SIGNAL( triggered() ),
                 this, SLOT( showStatistics() ) );
        connect( configAction, SIGNAL( triggered() ),
                 this, SLOT( showConfigDialog() ) );

        updateStatus( mInterface->getState() );
        updateMenu();
        mTray->show();
    }

    // notification 'interface available'
    if ( interfaceAvailable && mTray != 0L &&
         previousState != Interface::UNKNOWN_STATE )
    {
        /* When KNemo is starting we don't show the change in connection
         * status as this would be annoying when KDE starts.
         */
        QString title;
        if ( mInterface->getSettings().alias != QString::null )
            title = mInterface->getSettings().alias;
        else
            title = mInterface->getName();

        if ( mInterface->getData().wirelessDevice )
        {
            KNotification::event( "connected",
                                  title + ":\n" + i18n( "Connection established to\n" ) +
                                  mInterface->getWirelessData().essid );
        }
        else
        {
            KNotification::event( "connected",
                                  title + ":\n" + i18n( "Connection established." ) );
        }
    }
}

void InterfaceIcon::showConfigDialog()
{
    KNemoDaemon::sSelectedInterface = mInterface->getName();

    KProcess process;
    process << "kcmshell4" << "kcm_knemo";
    process.startDetached();
}

void InterfaceIcon::iconActivated( QSystemTrayIcon::ActivationReason reason )
{
    switch (reason)
    {
     case QSystemTrayIcon::Trigger:
         mInterface->showStatusDialog();
         break;
     case QSystemTrayIcon::MiddleClick:
         mInterface->showSignalPlotter( true );
         break;
     default:
         ;
     }
}

void InterfaceIcon::menuTriggered( QAction *action )
{
    if ( !action->data().canConvert<InterfaceCommand>() )
        return;

    InterfaceCommand command = action->data().value<InterfaceCommand>();
    KProcess process;
    if ( command.runAsRoot )
    {
        process << KStandardDirs::findExe("kdesu");
        process << command.command;
    }
    else
        process << command.command.split( " " );

    process.startDetached();
}

void InterfaceIcon::showStatistics()
{
    emit statisticsSelected();
}

void InterfaceIcon::showGraph()
{
    mInterface->showSignalPlotter( false );
}

#include "interfaceicon.moc"
