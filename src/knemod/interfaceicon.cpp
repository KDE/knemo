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

#include <math.h>
#include <unistd.h>

#include <QPainter>

#include <KAction>
#include <KActionCollection>
#include <KColorScheme>
#include <KConfigGroup>
#include <KHelpMenu>
#include <KIcon>
#include <KLocale>
#include <KMenu>
#include <KProcess>
#include <KStandardDirs>

#include "global.h"
#include "utils.h"
#include "interface.h"
#include "knemodaemon.h"
#include "interfaceicon.h"
#include "interfacetray.h"

#define SHRINK_MAX 0.75
#define HISTSIZE_STORE 0.5

Q_DECLARE_METATYPE(InterfaceCommand)

InterfaceIcon::InterfaceIcon( Interface* interface )
    : QObject(),
      mInterface( interface ),
      mTray( 0L ),
      barIncoming( 0 ),
      barOutgoing( 0 )
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
    // Prevent the "Destroyed while process is still running" message
    foreach( KProcess* process, processList )
    {
        process->kill();
        process->waitForFinished( 2000 );
    }
    delete mTray;
}

void InterfaceIcon::configChanged()
{
    KConfigGroup cg( KGlobal::mainComponent().config(), "System Tray" );
    iconWidth = cg.readEntry( "systrayIconWidth", 22 );

    barWidth = iconWidth/3;
    int margins = iconWidth - (barWidth*2);
    midMargin = margins/3;
    int rightMargin = (margins - midMargin)/2;
    leftMargin = margins-midMargin - rightMargin;
    midMargin = leftMargin + barWidth+ midMargin;
    histSize = HISTSIZE_STORE/generalSettings->pollInterval;
    if ( histSize < 2 )
        histSize = 2;

    for ( int i=0; i < histSize; i++ )
    {
        inHist.append( 0 );
        outHist.append( 0 );
    }

    inMaxRate = mInterface->settings().inMaxRate;
    outMaxRate = mInterface->settings().outMaxRate;

    updateTrayStatus();

    if ( mTray != 0L )
    {
        updateMenu();
        if ( mInterface->settings().iconTheme == TEXT_THEME )
             updateIconText( true );
        else if ( mInterface->settings().iconTheme == NETLOAD_THEME )
             updateBars( true );
    }
}

void InterfaceIcon::updateIconImage( int status )
{
    if ( mTray == 0L || mInterface->settings().iconTheme == TEXT_THEME )
        return;

    QString iconName;
    if ( mInterface->settings().iconTheme == SYSTEM_THEME )
        iconName = "network-";
    else
        iconName = "knemo-" + mInterface->settings().iconTheme + "-";

    // Now set the correct icon depending on the status of the interface.
    if ( ( status & KNemoIface::RxTraffic ) &&
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
    else if ( status & KNemoIface::Available )
    {
        iconName += ICON_OFFLINE;
    }
    else
    {
        iconName += ICON_ERROR;
    }
#ifdef HAVE_KSTATUSNOTIFIERITEM
    mTray->setIconByName( iconName );
#else
    mTray->setIcon( KIcon( iconName ) );
#endif
}

int InterfaceIcon::calcHeight( QList<unsigned long>& hist, unsigned int& net_max )
{
    unsigned long histcalculate = 0;
    unsigned long rate = 0;

    foreach( unsigned long j, hist )
    {
        histcalculate += j;
    }
    rate = histcalculate / histSize;

    /* update maximum */
    if ( !mInterface->settings().barScale )
    {
        QList<unsigned long>sortedMax( hist );
        qSort( sortedMax );
        unsigned long max = sortedMax.last();
        int multiplier = 1024;
        if ( generalSettings->useBitrate )
            multiplier = 1000;
        if( rate > net_max )
        {
            net_max = rate;
        }
        else if( max < net_max * SHRINK_MAX
                && net_max * SHRINK_MAX >= multiplier )
        {
            net_max *= SHRINK_MAX;
        }
    }
    qreal ratio = static_cast<double>(rate)/net_max;
    if ( ratio > 1.0 )
        ratio = 1.0;
    return ratio*iconWidth;
}

QColor InterfaceIcon::calcColor( QList<unsigned long>& hist, const QColor& low, const QColor& high, int hival )
{
    const BackendData * data = mInterface->backendData();

    if ( data->status & KNemoIface::Connected )
    {
        if ( ! mInterface->settings().dynamicColor )
            return low;
    }
    else if ( data->status & KNemoIface::Available )
        return mInterface->settings().colorDisabled;
    else if ( data->status & KNemoIface::Unavailable )
        return mInterface->settings().colorUnavailable;

    unsigned long histcalculate = 0;
    unsigned long rate = 0;
    if ( mInterface->settings().iconTheme == NETLOAD_THEME )
    {
        foreach( unsigned long j, hist )
        {
            histcalculate += j;
        }
        rate = histcalculate / histSize;
    }
    else
        rate = hist[0];

    int lowH, lowS, lowV;
    int hiH, hiS, hiV;
    int difH, difS, difV;

    low.getHsv( &lowH, &lowS, &lowV );
    high.getHsv( &hiH, &hiS, &hiV );

    difH = hiH - lowH;
    difS = hiS - lowS;
    difV = hiV - lowV;

    qreal percentage = static_cast<qreal>(rate)/hival;
    if ( percentage > 1.0 )
        percentage = 1.0;
    QColor retcolor;
    retcolor.setHsv( lowH + ( percentage*difH ), lowS + ( percentage*difS), lowV + (percentage*difV ) );
    return retcolor;
}

void InterfaceIcon::updateBars( bool doUpdate )
{
    // Has color changed?
    QColor rxColor = calcColor( inHist, mInterface->settings().colorIncoming, mInterface->settings().colorIncomingMax, mInterface->settings().inMaxRate );
    QColor txColor = calcColor( outHist, mInterface->settings().colorOutgoing, mInterface->settings().colorOutgoingMax, mInterface->settings().outMaxRate );
    if ( rxColor != colorIncoming )
    {
        doUpdate = true;
        colorIncoming = rxColor;
    }
    if ( txColor != colorOutgoing )
    {
        doUpdate = true;
        colorOutgoing = txColor;
    }

    // Has height changed?
    int rateIn = calcHeight( inHist, inMaxRate );
    int rateOut = calcHeight( outHist, outMaxRate );
    if ( rateIn != barIncoming )
    {
        doUpdate = true;
        barIncoming = rateIn;
    }
    if ( rateOut != barOutgoing )
    {
        doUpdate = true;
        barOutgoing = rateOut;
    }

    if ( !doUpdate )
        return;

    QPixmap barIcon(iconWidth, iconWidth);

    QLinearGradient inGrad( midMargin, 0, midMargin+barWidth, 0 );
    QLinearGradient topInGrad( midMargin, 0, midMargin+barWidth, 0 );
    QLinearGradient outGrad( leftMargin, 0, leftMargin+barWidth, 0 );
    QLinearGradient topOutGrad( leftMargin, 0, leftMargin+barWidth, 0 );

    int top = iconWidth - barOutgoing;

    QRect topLeftRect( leftMargin, 0, barWidth, top );
    QRect leftRect( leftMargin, top, barWidth, iconWidth );
    top = iconWidth - barIncoming;
    QRect topRightRect( midMargin, 0, barWidth, top );
    QRect rightRect( midMargin, top, barWidth, iconWidth );

    barIcon.fill( Qt::transparent );
    QPainter p( &barIcon );
    p.setOpacity( 1.0 );

    QColor topColor = mInterface->settings().colorUnavailable;
    QColor topColorD = mInterface->settings().colorUnavailable.darker();
    topColor.setAlpha( 128 );
    topColorD.setAlpha( 128 );
    topInGrad.setColorAt(0, topColorD);
    topInGrad.setColorAt(1, topColor );
    topOutGrad.setColorAt(0, topColorD);
    topOutGrad.setColorAt(1, topColor );

    inGrad.setColorAt(0, rxColor );
    inGrad.setColorAt(1, rxColor.darker() );
    outGrad.setColorAt(0, txColor );
    outGrad.setColorAt(1, txColor.darker() );

    QBrush brush( inGrad );
    p.setBrush( brush );
    p.fillRect( rightRect, inGrad );
    brush = QBrush( topInGrad );
    p.fillRect( topRightRect, topInGrad );
    brush = QBrush( outGrad );
    p.fillRect( leftRect, outGrad );
    brush = QBrush( topOutGrad );
    p.fillRect( topLeftRect, topOutGrad );
#ifdef HAVE_KSTATUSNOTIFIERITEM
    mTray->setIconByPixmap( barIcon );
#else
    mTray->setIcon( barIcon );
#endif
}

QString InterfaceIcon::compactTrayText(unsigned long data )
{
    QString dataString;
    // Space is tight, so no space between number and units, and the complete
    // string should be no more than 4 chars.
    /* Visually confusing to display bytes
    if ( bytes < 922 ) // 922B = 0.9K
        byteString = i18n( "%1B", bytes );
    */
    double multiplier = 1024;
    if ( generalSettings->useBitrate )
        multiplier = 1000;

    int precision = 0;
    if ( data < multiplier*9.95 ) // < 9.95K
    {
        precision = 1;
    }
    if ( data < multiplier*999.5 ) // < 999.5K
    {
        if ( generalSettings->useBitrate )
            dataString = i18n( "%1k", QString::number( data/multiplier, 'f', precision ) );
        else
            dataString = i18n( "%1K", QString::number( data/multiplier, 'f', precision ) );
        return dataString;
    }

    if ( data < pow(multiplier, 2)*9.95 ) // < 9.95M
        precision = 1;
    if ( data < pow(multiplier, 2)*999.5 ) // < 999.5M
    {
        dataString = i18n( "%1M", QString::number( data/pow(multiplier, 2), 'f', precision ) );
        return dataString;
    }

    if ( data < pow(multiplier, 3)*9.95 ) // < 9.95G
        precision = 1;
    // xgettext: no-c-format
    dataString = i18n( "%1G", QString::number( data/pow(multiplier, 3), 'f', precision) );
    return dataString;
}

void InterfaceIcon::updateIconText( bool doUpdate )
{
    // Has color changed?
    QColor rxColor = calcColor( inHist, mInterface->settings().colorIncoming, mInterface->settings().colorIncomingMax, mInterface->settings().inMaxRate );
    QColor txColor = calcColor( outHist, mInterface->settings().colorOutgoing, mInterface->settings().colorOutgoingMax, mInterface->settings().outMaxRate );
    if ( rxColor != colorIncoming )
    {
        doUpdate = true;
        colorIncoming = rxColor;
    }
    if ( rxColor != colorOutgoing )
    {
        doUpdate = true;
        colorOutgoing = txColor;
    }

    // Has text changed?
    QString byteText = compactTrayText( mInterface->rxRate() );
    if ( byteText != textIncoming )
    {
        doUpdate = true;
        textIncoming = byteText;
    }
    byteText = compactTrayText( mInterface->txRate() );
    if ( byteText != textOutgoing )
    {
        doUpdate = true;
        textOutgoing = byteText;
    }

    if ( !doUpdate )
        return;

    QPixmap textIcon(iconWidth, iconWidth);
    QRect topRect( 0, 0, iconWidth, iconWidth/2 );
    QRect bottomRect( 0, iconWidth/2, iconWidth, iconWidth/2 );
    textIcon.fill( Qt::transparent );
    QPainter p( &textIcon );
    p.setBrush( Qt::NoBrush );
    p.setOpacity( 1.0 );

    KColorScheme scheme(QPalette::Active, KColorScheme::View);

    // rxFont and txFont should be the same size per poll period
    QFont rxFont = setIconFont( textIncoming, mInterface->settings().iconFont, iconWidth );
    QFont txFont = setIconFont( textOutgoing, mInterface->settings().iconFont, iconWidth );
    if ( rxFont.pointSizeF() > txFont.pointSizeF() )
        rxFont.setPointSizeF( txFont.pointSizeF() );

    p.setFont( rxFont );
    p.setPen( rxColor );
    p.drawText( topRect, Qt::AlignCenter | Qt::AlignRight, textIncoming );

    p.setFont( rxFont );
    p.setPen( txColor );
    p.drawText( bottomRect, Qt::AlignCenter | Qt::AlignRight, textOutgoing );
#ifdef HAVE_KSTATUSNOTIFIERITEM
    mTray->setIconByPixmap( textIcon );
#else
    mTray->setIcon( textIcon );
#endif
}

void InterfaceIcon::updateToolTip()
{
    if ( mTray == 0L )
        return;
    inHist.prepend( mInterface->rxRate() );
    outHist.prepend( mInterface->txRate() );
    while ( inHist.count() > histSize )
    {
        inHist.removeLast();
        outHist.removeLast();
    }


    if ( mInterface->settings().iconTheme == TEXT_THEME )
        updateIconText();
    else if ( mInterface->settings().iconTheme == NETLOAD_THEME )
        updateBars();
    mTray->updateToolTip();
}

void InterfaceIcon::updateMenu()
{
    // Remove all old entries.
    KMenu* menu = static_cast<KMenu*>(mTray->contextMenu());
    QList<QAction *> actions = menu->actions();
    foreach ( QAction* action, commandActions->actions() )
        menu->removeAction( action );
    commandActions->clear();

    InterfaceSettings& settings = mInterface->settings();

    // If the user wants custom commands, add them.
    if ( settings.commands.size() > 0 )
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
    const QString ifaceName( mInterface->ifaceName() );
    const BackendData * data = mInterface->backendData();
    int currentStatus = data->status;
    bool hideWhenUnavailable = mInterface->settings().hideWhenUnavailable;
    bool hideWhenDisconnected = mInterface->settings().hideWhenDisconnected;

    QString title = mInterface->settings().alias;
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
              ( currentStatus & KNemoIface::Connected ||
                ( currentStatus & KNemoIface::Available && !hideWhenDisconnected ) ||
                ( !hideWhenUnavailable && !hideWhenDisconnected ) ) )
    {
        mTray = new InterfaceTray( mInterface, ifaceName );
        KMenu* menu = static_cast<KMenu*>(mTray->contextMenu());

        menu->removeAction( menu->actions().at( 0 ) );
        menu->addTitle( KIcon( "knemo" ), i18n( "KNemo - %1", title ) );
        menu->addAction( statusAction );
        menu->addAction( plotterAction );
        menu->addAction( configAction );
        KHelpMenu* helpMenu( new KHelpMenu( menu, KNemoDaemon::aboutData(), false ) );
        menu->addMenu( helpMenu->menu() )->setIcon( KIcon( "help-contents" ) );

        connect( menu, SIGNAL( triggered( QAction * ) ),
                 this, SLOT( menuTriggered( QAction * ) ) );

        if ( mInterface->settings().iconTheme == TEXT_THEME )
            updateIconText();
        else if ( mInterface->settings().iconTheme == NETLOAD_THEME )
            updateBars();
        else
            updateIconImage( mInterface->ifaceState() );
        updateMenu();
#ifndef HAVE_KSTATUSNOTIFIERITEM
        mTray->show();
#endif
    }
    else if ( mTray != 0L )
    {
        if ( mInterface->settings().iconTheme != TEXT_THEME &&
             mInterface->settings().iconTheme != NETLOAD_THEME )
            updateIconImage( mInterface->ifaceState() );
    }
}

void InterfaceIcon::showConfigDialog()
{
    KNemoDaemon::sSelectedInterface = mInterface->ifaceName();

    KProcess process;
    process << "kcmshell4" << "kcm_knemo";
    process.startDetached();
}

void InterfaceIcon::menuTriggered( QAction *action )
{
    if ( !action->data().canConvert<InterfaceCommand>() )
        return;

    InterfaceCommand command = action->data().value<InterfaceCommand>();
    KProcess *process = new KProcess( this );
    if ( command.runAsRoot )
        *process << KStandardDirs::findExe("kdesu") << command.command;
    else
        process->setShellCommand( command.command );

    processList << process;
    connect( process, SIGNAL( finished( int, QProcess::ExitStatus ) ), this, SLOT( processFinished() ) );
    process->start();
}

void InterfaceIcon::processFinished()
{
    processList.removeAll( static_cast<KProcess*>(sender()) );
    static_cast<KProcess*>(sender())->deleteLater();
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
