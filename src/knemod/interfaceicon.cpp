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

#include <QDebug>
#include <QPainter>
#include <KColorScheme>
#include <KConfigGroup>
#include <KGlobalSettings>
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

void InterfaceIcon::configChanged( const QColor& incoming,
                                   const QColor& outgoing,
                                   const QColor& disabled,
                                   int status )
{
    KConfigGroup cg( KGlobal::mainComponent().config(), "System Tray" );
    iconWidth = cg.readEntry( "systrayIconWidth", 22 );

    colorIncoming = incoming;
    colorOutgoing = outgoing;
    colorDisabled = disabled;

    // UNKNOWN_STATE to avoid notification
    updateTrayStatus( Interface::UNKNOWN_STATE );

    // handle changed iconset by user
    if ( mTray != 0L )
    {
        if ( mInterface->getSettings().iconSet == TEXTICON )
            updateIconText( true );
        else
            updateIconImage( status );
        updateMenu();
    }
}

void InterfaceIcon::updateIconImage( int status )
{
    // We need the iconset name in all cases.
    QString iconSet = mInterface->getSettings().iconSet;

    if ( mTray == 0L || iconSet == TEXTICON )
        return;

    // Now set the correct icon depending on the status of the interface.
    if ( status == Interface::NOT_AVAILABLE ||
         status == Interface::NOT_EXISTING )
    {
        iconSet += ICON_DISCONNECTED;
    }
    else if ( ( status & Interface::RX_TRAFFIC ) &&
              ( status & Interface::TX_TRAFFIC ) )
    {
        iconSet += ICON_TRAFFIC;
    }
    else if ( status & Interface::RX_TRAFFIC )
    {
        iconSet += ICON_INCOMING;
    }
    else if ( status & Interface::TX_TRAFFIC )
    {
        iconSet += ICON_OUTGOING;
    }
    else
    {
        iconSet += ICON_CONNECTED;
    }
#ifdef USE_KNOTIFICATIONITEM
    mTray->setIconByPixmap( UserIcon( iconSet ) );
#else
    mTray->setIcon( UserIcon( iconSet ) );
#endif
}

QFont InterfaceIcon::setIconFont( QString text )
{
    QFont f = KGlobalSettings::generalFont();
    float pointSize = f.pointSizeF();
    QFontMetrics fm( f );
    int w = fm.width( text );
    if ( w > iconWidth )
    {
        pointSize *= float( iconWidth ) / float( w );
        f.setPointSizeF( pointSize );
    }

    fm = QFontMetrics( f );
    // Don't want decender()...space too tight
    // +1 for base line +1 for space between lines
    int h = fm.ascent() + 2;
    if ( h > iconWidth/2 )
    {
        pointSize *= float( iconWidth/2 ) / float( h );
        f.setPointSizeF( pointSize );
    }
    return f;
}

QString InterfaceIcon::compactTrayText(unsigned long bytes )
{
    QString byteString;
    // Space is tight, so no space between number and units, and the complete
    // string should be no more than 4 chars.
    if ( bytes < 922 ) // 922B = 0.9K
        byteString = i18n( "%1B" ).arg( bytes );
    else if ( bytes < 10189 ) // < 9.95K
        byteString = i18n( "%1K" ).arg( QString::number( bytes/1024.0, 'f', 1 ) );
    else if ( bytes < 1023488 ) // < 999.5
        byteString = i18n( "%1K" ).arg( QString::number( bytes/1024.0, 'f', 0 ) );
    else if ( bytes < 10433331 ) // < 9.95M
        byteString = i18n( "%1M" ).arg( QString::number( bytes/1048576.0, 'f', 1 ) );
    else if ( bytes < 1048051712 ) // < 999.5G
        byteString = i18n( "%1M" ).arg( QString::number( bytes/1048576.0, 'f', 0 ) );
    else if ( bytes < 10683731148.0 ) // < 9.95G
        // xgettext: no-c-format
        byteString = i18n( "%1G" ).arg( QString::number( bytes/1073741824.0, 'f', 1 ) );
    else
        // xgettext: no-c-format
        byteString = i18n( "%1G" ).arg( QString::number( bytes/1073741824.0, 'f', 0) );
    return byteString;
}

void InterfaceIcon::updateIconText( bool proceed )
{
    const BackendData * data = mInterface->getData();

    unsigned long bytesPerSecond = data->incomingBytes / mInterface->getGeneralData().pollInterval;
    QString byteText = compactTrayText( bytesPerSecond );
    if ( byteText != textIncoming )
    {
        proceed = true;
        textIncoming = byteText;
    }
    bytesPerSecond = data->outgoingBytes / mInterface->getGeneralData().pollInterval;
    byteText = compactTrayText( bytesPerSecond );
    if ( byteText != textOutgoing )
    {
        proceed = true;
        textOutgoing = byteText;
    }

    // Do we even need to repaint?
    if ( !proceed )
        return;

    QPixmap textIcon(iconWidth, iconWidth);
    textIcon.fill( Qt::transparent );
    QPainter p( &textIcon );
    p.setBrush( Qt::NoBrush );
    p.setOpacity( 1.0 );

    KColorScheme scheme(QPalette::Active, KColorScheme::View);
    p.setFont( setIconFont( byteText ) );
    if ( data->isAvailable )
        p.setPen( colorIncoming );
    else
        p.setPen( colorDisabled );
    p.drawText( textIcon.rect(), Qt::AlignTop | Qt::AlignRight, textIncoming );

    p.setFont( setIconFont( byteText ) );
    if ( data->isAvailable )
        p.setPen( colorOutgoing );
    p.drawText( textIcon.rect(), Qt::AlignBottom | Qt::AlignRight, textOutgoing );
#ifdef USE_KNOTIFICATIONITEM
    mTray->setIconByPixmap( textIcon );
#else
    mTray->setIcon( textIcon );
#endif
}

void InterfaceIcon::updateToolTip()
{
    if ( mTray == 0L )
        return;
    if ( mInterface->getSettings().iconSet == TEXTICON )
        updateIconText();
    mTray->updateToolTip();
}

void InterfaceIcon::updateMenu()
{
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
    const QString ifaceName( mInterface->getName() );
    const BackendData * data = mInterface->getData();
    bool interfaceExists = data->isExisting;
    bool interfaceAvailable = data->isAvailable;
    bool hideWhenNotExisting = mInterface->getSettings().hideWhenNotExisting;
    bool hideWhenNotAvailable = mInterface->getSettings().hideWhenNotAvailable;

    QString title = mInterface->getSettings().alias;
    if ( title.isEmpty() )
        title = ifaceName;

    if ( mTray != 0L )
    {
        // notification 'interface not available'
        if ( !interfaceAvailable && previousState == Interface::AVAILABLE )
        {
            /* When KNemo is starting we don't show the change in connection
             * status as this would be annoying when KDE starts.
             */
            KNotification::event( "disconnected",
                           title + ": " + i18n( "Disconnected" ) );
        }

        // notification 'interface nonexistent'
        if ( !interfaceExists && previousState != Interface::UNKNOWN_STATE )
        {
            /* When KNemo is starting we don't show the change in connection
             * status as this would be annoying when KDE starts.
             */
            KNotification::event( "nonexistent",
                                  title + ": " + i18n( "Nonexistent" ) );
        }
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
        mTray = new InterfaceTray( mInterface, ifaceName );
        KMenu* menu = (KMenu *)mTray->contextMenu();

        menu->removeAction( menu->actions().at( 0 ) );
        menu->addTitle( KIcon( "knemo" ), i18n( "KNemo - " ) + title );
        menu->addAction( plotterAction );
        menu->addAction( statisticsAction );
        menu->addAction( configAction );
        KHelpMenu* helpMenu( new KHelpMenu( menu, KNemoDaemon::aboutData(), false ) );
        menu->addMenu( helpMenu->menu() )->setIcon( KIcon( "help-contents" ) );

        connect( menu, SIGNAL( triggered( QAction * ) ),
                 this, SLOT( menuTriggered( QAction * ) ) );
        connect( plotterAction, SIGNAL( triggered() ),
                 this, SLOT( showGraph() ) );
        connect( statisticsAction, SIGNAL( triggered() ),
                 this, SLOT( showStatistics() ) );
        connect( configAction, SIGNAL( triggered() ),
                 this, SLOT( showConfigDialog() ) );

        if ( mInterface->getSettings().iconSet == TEXTICON )
            updateIconText( true );
        else
            updateIconImage( mInterface->getState() );
        updateMenu();
#ifndef USE_KNOTIFICATIONITEM
        mTray->show();
#endif
    }

    if ( mTray != 0L )
    {
        // notification 'interface available'
        if ( interfaceAvailable && previousState != Interface::UNKNOWN_STATE )
        {
            /* When KNemo is starting we don't show the change in connection
             * status as this would be annoying.
             */
            if ( mInterface->getData()->isWireless )
            {
                KNotification::event( "connected",
                                      title + ": " +
                                      i18n( "Connected to %1", mInterface->getData()->essid ) );
            }
            else
            {
                KNotification::event( "connected",
                                      title + ": " + i18n( "Connected" ) );
            }
        }

        // Tray text may need to appear active/inactive
        // Force an update
        if ( mInterface->getSettings().iconSet == TEXTICON )
             updateIconText( true );
    }
}

void InterfaceIcon::showConfigDialog()
{
    KNemoDaemon::sSelectedInterface = mInterface->getName();

    KProcess process;
    process << "kcmshell4" << "kcm_knemo";
    process.startDetached();
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
        process << "/bin/sh" << "-c" << command.command;

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
