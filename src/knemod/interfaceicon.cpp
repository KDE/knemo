/* This file is part of KNemo
   Copyright (C) 2004, 2005 Percy Leonhardt <percy@eris23.de>
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

#include <math.h>
#include <unistd.h>

#include <QPixmapCache>

#include <QAction>
#include <KActionCollection>
#include <KConfigGroup>
#include <KHelpMenu>
#include <QIcon>
#include <KLocalizedString>
#include <QMenu>
#include <KAboutData>
#include <KProcess>
#include <Plasma/Theme>

#include "global.h"
#include "utils.h"
#include "interface.h"
#include "knemodaemon.h"
#include "interfaceicon.h"
#include "interfacetray.h"

#define SHRINK_MAX 0.75
#define HISTSIZE_STORE 0.5

InterfaceIcon::InterfaceIcon( Interface* interface )
    : QObject(),
      mInterface( interface ),
      mTray( 0L ),
      m_rxPrevBarHeight( 0 ),
      m_txPrevBarHeight( 0 )
{
    statusAction = new QAction( i18n( "Show &Status Dialog" ), this );
    plotterAction = new QAction( QIcon::fromTheme( QLatin1String("utilities-system-monitor") ),
                       i18n( "Show &Traffic Plotter" ), this );
    statisticsAction = new QAction( QIcon::fromTheme( QLatin1String("view-statistics") ),
                          i18n( "Show St&atistics" ), this );
    configAction = new QAction( QIcon::fromTheme( QLatin1String("configure") ),
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
    delete mTray;
}

void InterfaceIcon::configChanged()
{
    histSize = HISTSIZE_STORE/generalSettings->pollInterval;
    if ( histSize < 2 )
        histSize = 3;

    for ( int i=0; i < histSize; i++ )
    {
        m_rxHist.append( 0 );
        m_txHist.append( 0 );
    }

    m_maxRate = mInterface->settings().maxRate;

    updateTrayStatus();

    if ( mTray != 0L )
    {
        updateMenu();
        if ( mInterface->settings().iconTheme == TEXT_THEME )
             updateTextIcon( true );
        else if ( mInterface->settings().iconTheme == NETLOAD_THEME )
             updateBarIcon( true );
    }
}

void InterfaceIcon::updateIconImage( int status )
{
    if ( mTray == 0L || mInterface->settings().iconTheme == TEXT_THEME )
        return;

    QString iconName;
    if ( mInterface->settings().iconTheme == SYSTEM_THEME )
        iconName = QStringLiteral("network-");
    else
        iconName = QLatin1String("knemo-") + mInterface->settings().iconTheme + QLatin1Char('-');

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
    mTray->setIconByName( iconName );
}

QList<qreal> InterfaceIcon::barLevels( QList<unsigned long>& rxHist, QList<unsigned long>& txHist )
{
    // get the average rate of the two, then calculate based on the higher
    // return ratios for both.
    unsigned long histTotal = 0;
    foreach( unsigned long j, rxHist )
    {
        histTotal += j;
    }
    unsigned long rxAvgRate = histTotal / histSize;

    histTotal = 0;
    foreach( unsigned long j, txHist )
    {
        histTotal += j;
    }
    unsigned long txAvgRate = histTotal / histSize;

    // Adjust the scale of the bar icons when we don't have a fixed ceiling.
    if ( !mInterface->settings().barScale )
    {
        QList<unsigned long>rxSortedMax( rxHist );
        QList<unsigned long>txSortedMax( txHist );
        qSort( rxSortedMax );
        qSort( txSortedMax );
        int multiplier = 1024;
        if ( generalSettings->useBitrate )
            multiplier = 1000;
        if( qMax(rxAvgRate, txAvgRate) > m_maxRate )
        {
            m_maxRate = qMax(rxAvgRate, txAvgRate);
        }
        else if( qMax( rxSortedMax.last(), txSortedMax.last() ) < m_maxRate * SHRINK_MAX
                && m_maxRate * SHRINK_MAX >= multiplier )
        {
            m_maxRate *= SHRINK_MAX;
        }
    }

    QList<qreal> levels;
    qreal rxLevel = static_cast<double>(rxAvgRate)/m_maxRate;
    if ( rxLevel > 1.0 )
        rxLevel = 1.0;
    levels.append(rxLevel);
    qreal txLevel = static_cast<double>(txAvgRate)/m_maxRate;
    if ( txLevel > 1.0 )
        txLevel = 1.0;
    levels.append(txLevel);
    return levels;
}

bool InterfaceIcon::conStatusChanged()
{
    return !( mInterface->backendData()->status &
              mInterface->backendData()->prevStatus & 
              (KNemoIface::Connected | KNemoIface::Available | KNemoIface::Unavailable) );
}

void InterfaceIcon::updateBarIcon( bool doUpdate )
{
    // Has color changed?
    if ( conStatusChanged() )
    {
        doUpdate = true;
    }

    QSize iconSize = getIconSize();

    // If either of the bar heights have changed since the last time, we have
    // to do an update.
    QList<qreal> levels = barLevels( m_rxHist, m_txHist );
    int barHeight = static_cast<int>(round(iconSize.height() * levels.at(0)) + 0.5);
    if ( barHeight != m_rxPrevBarHeight )
    {
        doUpdate = true;
        m_rxPrevBarHeight = barHeight;
    }
    barHeight = static_cast<int>(round(iconSize.height() * levels.at(1)) + 0.5);
    if ( barHeight != m_txPrevBarHeight )
    {
        doUpdate = true;
        m_txPrevBarHeight = barHeight;
    }

    if ( !doUpdate )
        return;

    const BackendData * data = mInterface->backendData();

    mTray->setIconByPixmap( genBarIcon( levels.at(0), levels.at(1), data->status ) );
    QPixmapCache::clear();
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

void InterfaceIcon::updateTextIcon( bool doUpdate )
{
    // Has color changed?
    if ( conStatusChanged() )
    {
        doUpdate = true;
    }

    // Has text changed?
    QString byteText = compactTrayText( mInterface->rxRate() );
    if ( byteText != m_rxText )
    {
        doUpdate = true;
        m_rxText = byteText;
    }
    byteText = compactTrayText( mInterface->txRate() );
    if ( byteText != m_txText )
    {
        doUpdate = true;
        m_txText = byteText;
    }

    if ( !doUpdate )
        return;

    mTray->setIconByPixmap( genTextIcon(m_rxText, m_txText, plasmaTheme->smallestFont(), mInterface->backendData()->status) );
    QPixmapCache::clear();
}

void InterfaceIcon::updateToolTip()
{
    if ( mTray == 0L )
        return;
    m_rxHist.prepend( mInterface->rxRate() );
    m_txHist.prepend( mInterface->txRate() );
    while ( m_rxHist.count() > histSize )
    {
        m_rxHist.removeLast();
        m_txHist.removeLast();
    }


    if ( mInterface->settings().iconTheme == TEXT_THEME )
        updateTextIcon();
    else if ( mInterface->settings().iconTheme == NETLOAD_THEME )
        updateBarIcon();
    mTray->updateToolTip();
}

void InterfaceIcon::updateMenu()
{
    QMenu* menu = mTray->contextMenu();

    InterfaceSettings& settings = mInterface->settings();

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
    int minVisibleState = mInterface->settings().minVisibleState;

    QString title = ifaceName;

    if ( mTray != 0L && currentStatus < minVisibleState )
    {
        delete mTray;
        mTray = 0L;
    }
    else if ( mTray == 0L && currentStatus >= minVisibleState )
    {
        mTray = new InterfaceTray( mInterface, ifaceName );
        QMenu* menu = mTray->contextMenu();

        menu->removeAction( menu->actions().at( 0 ) );
        // FIXME: title for QMenu?
        //menu->addTitle( QIcon::fromTheme( QLatin1String("knemo") ), i18n( "KNemo - %1", title ) );
        menu->addAction( statusAction );
        menu->addAction( plotterAction );
        menu->addAction( configAction );
        KHelpMenu* helpMenu( new KHelpMenu( menu, KAboutData::applicationData(), false ) );
        menu->addMenu( helpMenu->menu() )->setIcon( QIcon::fromTheme( QLatin1String("help-contents") ) );

        if ( mInterface->settings().iconTheme == TEXT_THEME )
            updateTextIcon();
        else if ( mInterface->settings().iconTheme == NETLOAD_THEME )
            updateBarIcon();
        else
            updateIconImage( mInterface->ifaceState() );
        updateMenu();
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
    process << QLatin1String("kcmshell5") << QLatin1String("kcm_knemo");
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

#include "moc_interfaceicon.cpp"
