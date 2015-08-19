/* This file is part of KNemo
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

   Portions taken from FreeSWITCH
       Copyright (c) 2007-2008, Thomas BERNARD <miniupnp@free.fr>

       Permission to use, copy, modify, and/or distribute this software for any
       purpose with or without fee is hereby granted, provided that the above
       copyright notice and this permission notice appear in all copies.
*/

#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <QDir>
#include <QPainter>
#include <QPalette>
#include <QUrl>
#include <QFontMetrics>
#include <QStandardPaths>
#include <KColorScheme>
#include <KConfigGroup>
#include <Kdelibs4ConfigMigrator>
#include <Kdelibs4Migration>
#include <KSharedConfig>
#include <Plasma/Theme>
#include "data.h"
#include "utils.h"

#ifdef __linux__
  #include <netlink/route/rtnl.h>
  #include <netlink/route/route.h>
#else
  #include <net/route.h>
  #include <netinet/in.h>
  #include <unistd.h>
  #define NEXTADDR(w, u) \
        if (rtm_addrs & (w)) {\
            l = sizeof(struct sockaddr); memmove(cp, &(u), l); cp += l;\
        }
  #define rtm m_rtmsg.m_rtm
#endif

#ifdef __linux__

QString ipv4gwi;
QString ipv6gwi;

QString ipv4gw;
QString ipv6gw;

void parseNetlinkRoute( struct nl_object *object, void * )
{
    struct rtnl_route *const route = reinterpret_cast<struct rtnl_route *>(object);

    int rtfamily = rtnl_route_get_family( route );

    if ( rtfamily == AF_INET ||
         rtfamily == AF_INET6 )
    {
        struct rtnl_nexthop *nh = NULL;
        struct nl_addr *addr = NULL;
        if ( rtnl_route_get_nnexthops( route ) > 0 )
        {
            nh = rtnl_route_nexthop_n ( route, 0 );
            addr = rtnl_route_nh_get_gateway( nh );
        }

        if ( addr )
        {
            char gwaddr[ INET6_ADDRSTRLEN ];
            char gwname[ IFNAMSIZ ];
            memset( gwaddr, 0, sizeof( gwaddr ) );
            struct in_addr * inad = reinterpret_cast<struct in_addr *>(nl_addr_get_binary_addr( addr ));
            nl_addr2str( addr, gwaddr, sizeof( gwaddr ) );
            inet_ntop( rtfamily, &inad->s_addr, gwaddr, sizeof( gwaddr ) );
            int oif = rtnl_route_nh_get_ifindex( nh );
            if_indextoname( oif, gwname );

            if ( rtfamily == AF_INET )
            {
                ipv4gw = QLatin1String(gwaddr);
                ipv4gwi = QLatin1String(gwname);
            }
            else if ( rtfamily == AF_INET6 )
            {
                ipv6gw = QLatin1String(gwaddr);
                ipv6gwi = QLatin1String(gwname);
            }
        }
    }
}

QString getNetlinkRoute( int afType, QString *defaultGateway, void *data )
{
    if ( !data )
        return QString();

    struct nl_cache* rtlcache = static_cast<struct nl_cache*>(data);

    if ( afType == AF_INET )
    {
        ipv4gw.clear();
        ipv4gwi.clear();
    }
    else if ( afType == AF_INET6 )
    {
        ipv6gw.clear();
        ipv6gwi.clear();
    }
    nl_cache_foreach( rtlcache, parseNetlinkRoute, NULL);

    if ( afType == AF_INET )
    {
        if ( defaultGateway )
            *defaultGateway = ipv4gw;
        return ipv4gwi;
    }
    else
    {
        if ( defaultGateway )
            *defaultGateway = ipv6gw;
        return ipv6gwi;
    }
}
#else

QString getSocketRoute( int afType, QString *defaultGateway )
{
    struct
    {
        struct rt_msghdr m_rtm;
        char m_space[ 512 ];
    } m_rtmsg;

    int s, seq, l, rtm_addrs, i;
    pid_t pid;
    struct sockaddr so_dst, so_mask;
    char *cp = m_rtmsg.m_space;
    struct sockaddr *gate = NULL, *sa;
    struct rt_msghdr *msg_hdr;

    char outBuf[ INET6_ADDRSTRLEN ];
    memset( &outBuf, 0, sizeof( outBuf ) );
    void *tempAddrPtr = NULL;
    QString ifname;

    pid = getpid();
    seq = 0;
    rtm_addrs = RTA_DST | RTA_NETMASK;

    memset( &so_dst, 0, sizeof( so_dst ) );
    memset( &so_mask, 0, sizeof( so_mask ) );
    memset( &rtm, 0, sizeof( struct rt_msghdr ) );

    rtm.rtm_type = RTM_GET;
    rtm.rtm_flags = RTF_UP | RTF_GATEWAY;
    rtm.rtm_version = RTM_VERSION;
    rtm.rtm_seq = ++seq;
    rtm.rtm_addrs = rtm_addrs;

    if ( afType == AF_INET )
    {
        so_dst.sa_family = AF_INET;
        so_mask.sa_family = AF_INET;
    }
    else
    {
        so_dst.sa_family = AF_INET6;
        so_mask.sa_family = AF_INET6;
    }

    NEXTADDR( RTA_DST, so_dst );
    NEXTADDR( RTA_NETMASK, so_mask );

    rtm.rtm_msglen = l = cp - reinterpret_cast<char *>(&m_rtmsg);

    s = socket(PF_ROUTE, SOCK_RAW, 0);

    if ( write( s, reinterpret_cast<char *>(&m_rtmsg), l ) < 0 )
    {
        close( s );
        return ifname;
    }

    do
    {
        l = read(s, reinterpret_cast<char *>(&m_rtmsg), sizeof( m_rtmsg ) );
    } while ( l > 0 && (rtm.rtm_seq != seq || rtm.rtm_pid != pid) );

    close( s );

    msg_hdr = &rtm;

    cp = reinterpret_cast<char *>(msg_hdr + 1);
    if ( msg_hdr->rtm_addrs )
    {
        for ( i = 1; i; i <<= 1 )
            if ( i & msg_hdr->rtm_addrs )
            {
                sa = reinterpret_cast<struct sockaddr *>(cp);
                if ( i == RTA_GATEWAY )
                {
                    gate = sa;
                    char tempname[ IFNAMSIZ ];
                    if_indextoname( msg_hdr->rtm_index, tempname );
                    ifname = tempname;
                }

                cp += SA_SIZE( sa );
            }
    }
    else
        return ifname;

    if ( AF_INET == afType )
        tempAddrPtr = & reinterpret_cast<struct sockaddr_in *>(gate)->sin_addr;
    else
        tempAddrPtr = & reinterpret_cast<struct sockaddr_in6 *>(gate)->sin6_addr;
    inet_ntop( gate->sa_family, tempAddrPtr, outBuf, sizeof( outBuf ) );
    if ( defaultGateway && strncmp( outBuf, "0.0.0.0", 7 ) != 0 )
        *defaultGateway = outBuf;
    return ifname;
}
#endif

QString getDefaultRoute( int afType, QString *defaultGateway, void *data )
{
#ifdef __linux__
    return getNetlinkRoute( afType, defaultGateway, data );
#else
    return getSocketRoute( afType, defaultGateway );
#endif
}

QList<KNemoTheme> findThemes()
{
    const QStringList dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("knemo/themes"), QStandardPaths::LocateDirectory);
    QStringList themelist;
    Q_FOREACH (const QString& dir, dirs) {
        const QStringList fileNames = QDir(dir).entryList(QStringList() << QLatin1String("*.desktop"));
        Q_FOREACH (const QString& file, fileNames) {
            themelist.append(dir + QLatin1Char('/') + file);
        }
    }

    QList<KNemoTheme> iconThemes;
    foreach ( QString themeFile, themelist )
    {
        KSharedConfig::Ptr conf = KSharedConfig::openConfig( themeFile );
        KConfigGroup cfg( conf, QLatin1String("Desktop Entry") );
        KNemoTheme theme;
        theme.name = cfg.readEntry(QLatin1String("Name"));
        theme.comment = cfg.readEntry(QLatin1String("Comment"));
        theme.internalName = cfg.readEntry( QLatin1String("X-KNemo-Theme") );
        iconThemes << theme;
    }
    return iconThemes;
}

QSize getIconSize()
{
    // This is borrowed from the plasma system tray:
    // plasma-workspace/applets/systemtray/package/contents/ui/main.qml
    // plasma-workspace/applets/systemtray/package/contents/code/Layout.js
    int preferredItemSize = 128;
    Plasma::Theme theme;
    int baseSize = theme.mSize(theme.defaultFont()).height();
    int suggestedsize = min(baseSize * 2, preferredItemSize);
    QSize resultingsize;

    if (suggestedsize < 16 || suggestedsize >= 64) {
        resultingsize = QSize(suggestedsize, suggestedsize);
    } else if (suggestedsize < 22) {
        resultingsize = QSize(16, 16);
    } else if (suggestedsize < 24) {
        resultingsize = QSize(22, 22);
    } else if (suggestedsize < 32) {
        resultingsize = QSize(24, 24);
    } else if (suggestedsize < 48) {
        resultingsize = QSize(32, 32);
    } else if (suggestedsize < 64) {
        resultingsize = QSize(48, 48);
    }

    return resultingsize;
}

QPixmap genTextIcon(const QString& incomingText, const QString& outgoingText, const QFont& font, int status)
{
    QSize iconSize = getIconSize();
    QPixmap textIcon(iconSize);
    QRect topRect( 0, 0, iconSize.width(), iconSize.height()/2 );
    QRect bottomRect( 0, iconSize.width()/2, iconSize.width(), iconSize.height()/2 );
    textIcon.fill( Qt::transparent );
    QPainter p( &textIcon );
    p.setBrush( Qt::NoBrush );
    p.setOpacity( 1.0 );
    QColor textColor;

    // rxFont and txFont should be the same size per poll period
    QFont rxFont = setIconFont( incomingText, font, iconSize.height() );
    QFont txFont = setIconFont( outgoingText, font, iconSize.height() );
    rxFont.setPointSizeF( qMin(rxFont.pointSizeF(), txFont.pointSizeF()) );

    if ( status & KNemoIface::Connected )
        textColor = KColorScheme(QPalette::Active).foreground(KColorScheme::NormalText).color();
    else if ( status & KNemoIface::Available )
        textColor = KColorScheme(QPalette::Active).foreground(KColorScheme::InactiveText).color();
    else
        textColor = KColorScheme(QPalette::Active).foreground(KColorScheme::NegativeText).color();

    p.setFont( rxFont );
    p.setPen( textColor );
    p.drawText( topRect, Qt::AlignCenter | Qt::AlignRight, incomingText );
    p.drawText( bottomRect, Qt::AlignCenter | Qt::AlignRight, outgoingText );
    return textIcon;
}

QPixmap genBarIcon(qreal rxLevel, qreal txLevel, int status)
{
    QSize iconSize = getIconSize();

    QColor rxColor;
    QColor txColor;
    QColor bgColor;
    if ( status & KNemoIface::Connected )
    {
        rxColor = KColorScheme(QPalette::Active, KColorScheme::Window).foreground(KColorScheme::ActiveText).color();
        txColor = KColorScheme(QPalette::Active, KColorScheme::Window).foreground(KColorScheme::NeutralText).color();
        bgColor = KColorScheme(QPalette::Active, KColorScheme::Window).foreground(KColorScheme::InactiveText).color();
        bgColor.setAlpha( 77 );
    }
    else if ( status & KNemoIface::Available )
    {
        bgColor = KColorScheme(QPalette::Active, KColorScheme::Window).foreground(KColorScheme::InactiveText).color();
        bgColor.setAlpha( 153 );
    }
    else
    {
        bgColor = KColorScheme(QPalette::Active, KColorScheme::Window).foreground(KColorScheme::NegativeText).color();
    }

    int barWidth = static_cast<int>(round(iconSize.width()/3.0) + 0.5);
    int margins = iconSize.width() - (barWidth*2);
    int midMargin = static_cast<int>(round(margins/3.0) + 0.5);
    int outerMargin = static_cast<int>(round((margins - midMargin)/2.0) + 0.5);
    midMargin = outerMargin + barWidth + midMargin;
    QPixmap barIcon( iconSize );
    barIcon.fill( Qt::transparent );

    int top = iconSize.height() - static_cast<int>(round(iconSize.height() * txLevel) + 0.5);
    QRect topLeftRect( outerMargin, 0, barWidth, top );
    QRect leftRect( outerMargin, top, barWidth, iconSize.height() );
    top = iconSize.height() - static_cast<int>(round(iconSize.height() * rxLevel) + 0.5);
    QRect topRightRect( midMargin, 0, barWidth, top );
    QRect rightRect( midMargin, top, barWidth, iconSize.height() );

    QPainter p( &barIcon );
    p.fillRect( leftRect, txColor );
    p.fillRect( rightRect, rxColor );
    p.fillRect( topLeftRect, bgColor );
    p.fillRect( topRightRect, bgColor );
    return barIcon;
}



QFont setIconFont( const QString& text, const QFont& font, int iconWidth )
{
    // Is there a better way to do this?
    QFont f( font );
    qreal pointSize = f.pointSizeF();
    QFontMetricsF fm( f );
    qreal w = fm.width( text );
    if ( w != iconWidth )
    {
        pointSize *= qreal( iconWidth ) / w;
        if ( pointSize < 0.5 )
            pointSize = 0.5;
        f.setPointSizeF( pointSize );
        fm = QFontMetricsF( f );
        while ( pointSize > 0.5 && fm.width( text ) > iconWidth )
        {
            pointSize -= 0.5;
            f.setPointSizeF( pointSize );
            fm = QFontMetricsF( f );
        }
    }

    // Don't want decender()...space too tight
    if ( fm.ascent() > iconWidth/2.0 )
    {
        pointSize *=  iconWidth / 2.0 / fm.ascent();
        if ( pointSize < 0.5 )
            pointSize = 0.5;
        f.setPointSizeF( pointSize );
        fm = QFontMetricsF( f );
        while ( pointSize > 0.5 && fm.ascent() > iconWidth/2.0 )
        {
            pointSize -= 0.5;
            f.setPointSizeF( pointSize );
            fm = QFontMetricsF( f );
        }
    }
    return f;
}

double validatePoll( double val )
{
    int siz = sizeof(pollIntervals)/sizeof(double);
    for ( int i = 0; i < siz; i++ )
    {
        if ( val <= pollIntervals[i] )
        {
            val = pollIntervals[i];
            return val;
        }
    }
    return GeneralSettings().pollInterval;
}

void migrateKde4Conf()
{
    // Kdelibs4Migration
    QStringList configFiles;
    configFiles << QLatin1String("knemorc") << QLatin1String("knemo.notifyrc");
    Kdelibs4ConfigMigrator migrator(QLatin1String("knemo")); // the same name defined in the aboutData
    migrator.setConfigFiles(configFiles);

    if (migrator.migrate()) {
        // look in knemorc; find configured stats path (or KDE4 default); migrate stats.
        KConfigGroup generalGroup( KSharedConfig::openConfig(), confg_general );
        Kdelibs4Migration dataMigrator;
        QString sourceBasePath = generalGroup.readEntry( QLatin1String("StatisticsDir"), dataMigrator.saveLocation("data", QLatin1String("knemo")) );
        QUrl testUrl(sourceBasePath);
        if ( testUrl.isLocalFile() )
            sourceBasePath = testUrl.toLocalFile();
        const QString targetBasePath = GeneralSettings().statisticsDir.absolutePath() + QLatin1Char('/');

        QDir sourceDir(sourceBasePath);
        if(sourceDir.exists()) {
            QStringList fileNames = sourceDir.entryList(QDir::Files);
            if (QDir().mkpath(targetBasePath)) {
                foreach (const QString &fileName, fileNames) {
                    QFile::copy(sourceBasePath + fileName, targetBasePath + fileName);
                }
            }
        }
    }
}
