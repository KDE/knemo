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
#include <KStandardDirs>

#include "data.h"
#include "interface.h"
#include "knemodaemon.h"
#include "interfaceicon.h"
#include "interfacetray.h"

Q_DECLARE_METATYPE(InterfaceCommand)

InterfaceIcon::InterfaceIcon( Interface* interface )
    : QObject(),
      mInterface( interface ),
      mTray( 0L )
{
    commandActions = new KActionCollection( this );
    statusAction = new KAction( i18n( "Show &Status Dialog" ), this );
    plotterAction = new KAction( KIcon( "utilities-system-monitor" ),
                       i18n( "Show &Traffic Plotter" ), this );
    statisticsAction = new KAction( KIcon( "view-statistics" ),
                          i18n( "Show St&atistics" ), this );
    configAction = new KAction( KIcon( "configure" ),
                       i18n( "&Configure KNemo..." ), this );

    connect( statusAction, SIGNAL( triggered() ),
             this, SLOT( showStatus() ) );
    connect( plotterAction, SIGNAL( triggered() ),
             this, SLOT( showGraph() ) );
    connect( statisticsAction, SIGNAL( triggered() ),
             this, SLOT( showStatistics() ) );
    connect( configAction, SIGNAL( triggered() ),
             this, SLOT( showConfigDialog() ) );
}

InterfaceIcon::~InterfaceIcon()
{
    if ( mTray != 0L )
        delete mTray;
}

void InterfaceIcon::configChanged( const QColor& incoming,
                                   const QColor& outgoing,
                                   const QColor& disabled )
{
    KConfigGroup cg( KGlobal::mainComponent().config(), "System Tray" );
    iconWidth = cg.readEntry( "systrayIconWidth", 22 );

    colorIncoming = incoming;
    colorOutgoing = outgoing;
    colorDisabled = disabled;

    updateTrayStatus();

    if ( mTray != 0L )
    {
        updateMenu();
        if ( mInterface->getSettings().iconTheme == TEXT_THEME )
             updateIconText( true );
    }
}

void InterfaceIcon::updateIconImage( int status )
{
    if ( mTray == 0L || mInterface->getSettings().iconTheme == TEXT_THEME )
        return;

    QString iconName;
    if ( mInterface->getSettings().iconTheme == SYSTEM_THEME )
        iconName = "network-";
    else
        iconName = "knemo-" + mInterface->getSettings().iconTheme + "-";

    // Now set the correct icon depending on the status of the interface.
    if ( status == KNemoIface::Available )
    {
        iconName += ICON_OFFLINE;
    }
    else if ( ( status & KNemoIface::RxTraffic ) &&
              ( status & KNemoIface::TxTraffic ) )
    {
        iconName += ICON_RX_TX;
    }
    else if ( status & KNemoIface::RxTraffic )
    {
        iconName += ICON_RX;
    }
    else if ( status & KNemoIface::TxTraffic )
    {
        iconName += ICON_TX;
    }
    else if ( status & KNemoIface::Connected )
    {
        iconName += ICON_IDLE;
    }
    else
    {
        iconName += ICON_ERROR;
    }
#ifdef USE_KNOTIFICATIONITEM
    mTray->setIconByPixmap( KIcon( iconName ) );
#else
    mTray->setIcon( KIcon( iconName ) );
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

void InterfaceIcon::updateIconText( bool doUpdate )
{
    const BackendData * data = mInterface->getData();

    unsigned long bytesPerSecond = data->incomingBytes / mInterface->getGeneralData().pollInterval;
    QString byteText = compactTrayText( bytesPerSecond );
    if ( byteText != textIncoming )
    {
        doUpdate = true;
        textIncoming = byteText;
    }
    bytesPerSecond = data->outgoingBytes / mInterface->getGeneralData().pollInterval;
    byteText = compactTrayText( bytesPerSecond );
    if ( byteText != textOutgoing )
    {
        doUpdate = true;
        textOutgoing = byteText;
    }
    int state = mInterface->getState();
    int prevState = mInterface->getPreviousState();
    // Need to change color, though text not changed
    if ( ( state >= KNemoIface::Connected && prevState <  KNemoIface::Connected ) ||
         ( state <  KNemoIface::Connected && prevState >= KNemoIface::Connected ) )
        doUpdate = true;

    if ( !doUpdate )
        return;

    QPixmap textIcon(iconWidth, iconWidth);
    textIcon.fill( Qt::transparent );
    QPainter p( &textIcon );
    p.setBrush( Qt::NoBrush );
    p.setOpacity( 1.0 );

    KColorScheme scheme(QPalette::Active, KColorScheme::View);
    p.setFont( setIconFont( byteText ) );
    if ( data->status & KNemoIface::Connected )
        p.setPen( colorIncoming );
    else
        p.setPen( colorDisabled );
    p.drawText( textIcon.rect(), Qt::AlignTop | Qt::AlignRight, textIncoming );

    p.setFont( setIconFont( byteText ) );
    if ( data->status & KNemoIface::Connected )
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
    if ( mInterface->getSettings().iconTheme == TEXT_THEME )
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
        menu->insertActions( statusAction, commandActions->actions() );
    }

    if ( settings.activateStatistics )
        menu->insertAction( configAction, statisticsAction );
    else
        menu->removeAction( statisticsAction );
}

void InterfaceIcon::updateTrayStatus()
{
    const QString ifaceName( mInterface->getName() );
    const BackendData * data = mInterface->getData();
    int currentStatus = data->status;
    bool hideWhenUnavailable = mInterface->getSettings().hideWhenUnavailable;
    bool hideWhenDisconnected = mInterface->getSettings().hideWhenDisconnected;

    QString title = mInterface->getSettings().alias;
    if ( title.isEmpty() )
        title = ifaceName;

    /* Remove the icon if
     * - the interface is not available and the option to hide it is selected
     * - the interface does not exist, the option to hide it is selected
     *   and the other option is not selected
     */
    if ( mTray != 0L &&
         ( ( (currentStatus < KNemoIface::Connected ) && hideWhenDisconnected ) ||
           ( (currentStatus < KNemoIface::Available ) && hideWhenUnavailable && !hideWhenDisconnected ) ) )
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
              ( currentStatus > KNemoIface::Available ||
                ( currentStatus == KNemoIface::Available && !hideWhenDisconnected ) ||
                ( !hideWhenUnavailable && !hideWhenDisconnected ) ) )
    {
        mTray = new InterfaceTray( mInterface, ifaceName );
        KMenu* menu = (KMenu *)mTray->contextMenu();

        menu->removeAction( menu->actions().at( 0 ) );
        menu->addTitle( KIcon( "knemo" ), i18n( "KNemo - " ) + title );
        menu->addAction( statusAction );
        menu->addAction( plotterAction );
        menu->addAction( configAction );
        KHelpMenu* helpMenu( new KHelpMenu( menu, KNemoDaemon::aboutData(), false ) );
        menu->addMenu( helpMenu->menu() )->setIcon( KIcon( "help-contents" ) );

        connect( menu, SIGNAL( triggered( QAction * ) ),
                 this, SLOT( menuTriggered( QAction * ) ) );

        if ( mInterface->getSettings().iconTheme == TEXT_THEME )
            updateIconText( true );
        else
            updateIconImage( mInterface->getState() );
        updateMenu();
#ifndef USE_KNOTIFICATIONITEM
        mTray->show();
#endif
    }
    else if ( mTray != 0L )
    {
        if ( mInterface->getSettings().iconTheme != TEXT_THEME )
            updateIconImage( mInterface->getState() );
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

void InterfaceIcon::showStatus()
{
    mInterface->showStatusDialog( true );
}

void InterfaceIcon::showGraph()
{
    mInterface->showSignalPlotter( true );
}

#include "interfaceicon.moc"
