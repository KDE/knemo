/* This file is part of KNemo
   Copyright (C) 2004, 2005 Percy Leonhardt <percy@eris23.de>

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

#include <unistd.h>

#include <qpixmap.h>

#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kprocess.h>
#include <kpopupmenu.h>
#include <kiconloader.h>
#include <knotifyclient.h>

#include "data.h"
#include "interface.h"
#include "knemodaemon.h"
#include "interfaceicon.h"
#include "interfacetray.h"
#include "interfacemonitor.h"
#include "interfacetooltip.h"

const QString InterfaceIcon::ICON_DISCONNECTED = "network_disconnected";
const QString InterfaceIcon::ICON_CONNECTED = "network_connected";
const QString InterfaceIcon::ICON_INCOMING = "network_incoming";
const QString InterfaceIcon::ICON_OUTGOING = "network_outgoing";
const QString InterfaceIcon::ICON_TRAFFIC = "network_traffic";
const QString InterfaceIcon::SUFFIX_PPP = "_ppp";
const QString InterfaceIcon::SUFFIX_LAN = "_lan";
const QString InterfaceIcon::SUFFIX_WLAN = "_wlan";

InterfaceIcon::InterfaceIcon( Interface* interface )
    : QObject(),
      mInterface( interface ),
      mTray( 0L )
{
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

    // If the user wants something different than the default icons
    // append the correct suffix to the filename.
    QString suffix;
    if ( mInterface->getSettings().iconSet == Interface::NETWORK )
    {
        suffix = SUFFIX_LAN;
    }
    else if ( mInterface->getSettings().iconSet == Interface::WIRELESS )
    {
        suffix = SUFFIX_WLAN;
    }
    else if ( mInterface->getSettings().iconSet == Interface::MODEM )
    {
        suffix = SUFFIX_PPP;
    }
    else
    {
        suffix = ""; // use standard icons
    }

    // Now set the correct icon depending on the status of the interface.
    if ( status == Interface::NOT_AVAILABLE ||
         status == Interface::NOT_EXISTING )
    {
        mTray->setPixmap( mTray->loadIcon( ICON_DISCONNECTED + suffix ) );
    }
    else if ( ( status & Interface::RX_TRAFFIC ) &&
              ( status & Interface::TX_TRAFFIC ) )
    {
        mTray->setPixmap( mTray->loadIcon( ICON_TRAFFIC + suffix ) );
    }
    else if ( status & Interface::RX_TRAFFIC )
    {
        mTray->setPixmap( mTray->loadIcon( ICON_INCOMING + suffix ) );
    }
    else if ( status & Interface::TX_TRAFFIC )
    {
        mTray->setPixmap( mTray->loadIcon( ICON_OUTGOING + suffix ) );
    }
    else
    {
        mTray->setPixmap( mTray->loadIcon( ICON_CONNECTED + suffix ) );
    }
}

void InterfaceIcon::updateToolTip()
{
    if ( mTray == 0L )
        return;

    QString toolTip = mInterface->getSettings().alias;
    if ( toolTip == QString::null )
        toolTip = mInterface->getName();
    new InterfaceToolTip( mInterface,  mTray );
}

void InterfaceIcon::updateMenu()
{
    if ( mTray == 0L )
        return;

    // Remove all old entries.
    KPopupMenu* menu = mTray->contextMenu();
    int count = menu->count();
    for ( int i = 0; i < count - 6; i++ )
        menu->removeItemAt( 6 );

    InterfaceSettings& settings = mInterface->getSettings();

    // If the user wants statistics, add an entry to show them.
    if ( settings.activateStatistics )
    {
        menu->insertItem( i18n( "Open &Statistics" ), this,
                          SIGNAL( statisticsSelected() ) );
    }

    // If the user wants custom commands, add them.
    if ( settings.customCommands )
    {
        menu->insertSeparator();
        QValueVector<InterfaceCommand>::iterator it;
        for ( it = settings.commands.begin(); it != settings.commands.end(); it++ )
            (*it).id = menu->insertItem( (*it).menuText );
    }
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

        KNotifyClient::event( mTray->winId(), "knemo_disconnected",
                              title + ":\n" + i18n( "Not connected." ) );
        /* Wait half a second before deleting the tray so that the call
         * to the notification daemon has a chance to run before the
         * winId gets invalid.
         */
        usleep( 500000 );
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

        KNotifyClient::event( mTray->winId(), "knemo_notexisting",
                              title + ":\n" + i18n( "Not existing." ) );
        /* Wait half a second before deleting the tray so that the call
         * to the notification daemon has a chance to run before the
         * winId gets invalid.
         */
        usleep( 500000 );
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
        mTray = new InterfaceTray( mInterface->getName() );
        QToolTip::add( mTray, mInterface->getName() );
        KPopupMenu* menu = mTray->contextMenu();
        connect( menu, SIGNAL( activated( int ) ),
                 this, SLOT( menuActivated( int ) ) );
        connect( mTray, SIGNAL( leftClicked() ),
                 mInterface, SLOT( showStatusDialog() ) );
        connect( mTray, SIGNAL( graphSelected( bool ) ),
                 mInterface, SLOT( showSignalPlotter( bool ) ) );
        connect( mTray, SIGNAL( configSelected() ),
                 this, SLOT( showConfigDialog() ) );

        updateStatus( mInterface->getState() );
        updateToolTip();
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

        /* Wait half a second before calling the notification daemon
         * so that the winId of the tray is valid when used below.
         */
        usleep( 500000 );
        KNotifyClient::event( mTray->winId(), "knemo_connected",
                              title + ":\n" + i18n( "Connection established." ) );
    }
}

void InterfaceIcon::showConfigDialog()
{
    KNemoDaemon::sSelectedInterface = mInterface->getName();

    KProcess process;
    process << "kcmshell" << "kcm_knemo";
    process.start( KProcess::DontCare );
}

void InterfaceIcon::menuActivated( int id )
{
    InterfaceSettings& settings = mInterface->getSettings();
    QValueVector<InterfaceCommand>::iterator it;
    for ( it = settings.commands.begin(); it != settings.commands.end(); it++ )
    {
        if ( (*it).id == id )
        {
            KProcess process;
            if ( (*it).runAsRoot )
            {
                process << "kdesu";
                process << (*it).command;
            }
            else
                process << QStringList::split( ' ', (*it).command );

            process.start( KProcess::DontCare );
            break;
        }
    }
}

#include "interfaceicon.moc"
