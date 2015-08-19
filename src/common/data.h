/* This file is part of KNemo
   Copyright (C) 2004 Percy Leonhardt <percy@eris23.de>
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

#ifndef DATA_H
#define DATA_H

#include <QDate>
#include <QList>
#include <QMap>
#include <QStandardPaths>
#include <QString>

#include <KLocale>
#include <QDir>

class KCalendarSystem;

/**
 * This file contains data structures used to store information about
 * an interface. It is shared between the daemon and the control center
 * module.
 *
 * @short Shared data structures
 * @author Percy Leonhardt <percy@eris23.de>
 */


namespace KNemoIface {
    enum Type
    {
        UnknownType,
        Ethernet,
        PPP
    };

    enum IfaceState
    {
        UnknownState =  0,
        Unavailable  =  1,
        Available    =  2,
        Up           =  4,
        Connected    =  8,
        RxTraffic    = 16,
        TxTraffic    = 32,
        MaxState     = 256
    };
}

namespace KNemoStats
{
    enum TrafficUnits
    {
        UnitB = 0,
        UnitK,
        UnitM,
        UnitG
    };

    enum TrafficDirection
    {
        TrafficIn = 0,
        TrafficOut,
        TrafficTotal
    };

    enum WarnType
    {
        Peak = 0,
        Offpeak,
        PeakOffpeak
    };

    enum PeriodUnits
    {
        Hour = 0,
        Day,
        Week,
        Month,
        BillPeriod,
        Year,
        HourArchive
    };

    // Powers of 2
    enum TrafficType
    {
        AllTraffic = 0,
        OffpeakTraffic = 1
    };

};

static const QLatin1String NETLOAD_THEME("netloadtheme");
static const QLatin1String TEXT_THEME("texttheme");
static const QLatin1String SYSTEM_THEME("systemtheme");
static const QLatin1String ICON_ERROR("error");
static const QLatin1String ICON_OFFLINE("offline");
static const QLatin1String ICON_IDLE("idle");
static const QLatin1String ICON_RX("receive");
static const QLatin1String ICON_TX("transmit");
static const QLatin1String ICON_RX_TX("transmit-receive");

// config groups

static const QLatin1String confg_general("General");
static const QLatin1String confg_interface("Interface_");
static const QLatin1String confg_plotter("Plotter_");
static const QLatin1String confg_statsRule("StatsRule_");
static const QLatin1String confg_warnRule("WarnRule_");

static const QLatin1String conf_firstStart("FirstStart");
static const QLatin1String conf_autoStart("AutoStart");
static const QLatin1String conf_interfaces("Interfaces");

// interface
static const QLatin1String conf_alias("Alias");

// interface icon
static const QLatin1String conf_minVisibleState("MinVisibleState");
static const QLatin1String conf_trafficThreshold("TrafficThreshold");
static const QLatin1String conf_iconTheme("IconSet");
static const QLatin1String conf_barScale("BarScale");
static const QLatin1String conf_maxRate("MaxRate");

// interface statistics
static const QLatin1String conf_activateStatistics("ActivateStatistics");
static const QLatin1String conf_calendarSystem("CalendarSystem");
static const QLatin1String conf_statsRules("StatsRules");
static const QLatin1String conf_warnRules("WarnRules");

// interface billing
static const QLatin1String conf_statsStartDate("StartDate");
static const QLatin1String conf_statsPeriodUnits("PeriodUnits");
static const QLatin1String conf_statsPeriodCount("PeriodCount");
static const QLatin1String conf_logOffpeak("LogOffpeak");
static const QLatin1String conf_offpeakStartTime("OffpeakStartTime");
static const QLatin1String conf_offpeakEndTime("OffpeakEndTime");
static const QLatin1String conf_weekendIsOffpeak("WeekendIsOffpeak");
static const QLatin1String conf_weekendDayStart("WeekendDayStart");
static const QLatin1String conf_weekendDayEnd("WeekendDayEnd");
static const QLatin1String conf_weekendTimeStart("WeekendTimeStart");
static const QLatin1String conf_weekendTimeEnd("WeekendTimeEnd");

// warning
static const QLatin1String conf_warnPeriodUnits("PeriodUnits");
static const QLatin1String conf_warnPeriodCount("PeriodCount");
static const QLatin1String conf_warnTrafficType("TrafficType");
static const QLatin1String conf_warnTrafficDirection("TrafficDirection");
static const QLatin1String conf_warnTrafficUnits("TrafficUnits");
static const QLatin1String conf_warnThreshold("Threshold");
static const QLatin1String conf_warnCustomText("CustomText");

// tooltip
static const QLatin1String conf_toolTipContent("ToolTipContent");

// general
static const QLatin1String conf_pollInterval("PollInterval");
static const QLatin1String conf_saveInterval("SaveInterval");
static const QLatin1String conf_useBitrate("UseBitrate");
static const QLatin1String conf_plotterPos("PlotterPos");
static const QLatin1String conf_plotterSize("PlotterSize");
static const QLatin1String conf_statisticsPos("StatisticsPos");
static const QLatin1String conf_statisticsSize("StatisticsSize");
static const QLatin1String conf_statusPos("StatusPos");
static const QLatin1String conf_statusSize("StatusSize");
static const QLatin1String conf_hourState("HourState");
static const QLatin1String conf_dayState("DayState");
static const QLatin1String conf_weekState("WeekState");
static const QLatin1String conf_monthState("MonthState");
static const QLatin1String conf_billingState("BillingState");
static const QLatin1String conf_yearState("YearState");

enum ToolTipEnums
{
    INTERFACE        = 0x00000001,
    ALIAS            = 0x00000002,
    STATUS           = 0x00000004,
    UPTIME           = 0x00000008,
    IP_ADDRESS       = 0x00000010,
    SCOPE            = 0x00000020,
    HW_ADDRESS       = 0x00000040,
    PTP_ADDRESS      = 0x00000080,
    RX_PACKETS       = 0x00000100,
    TX_PACKETS       = 0x00000200,
    RX_BYTES         = 0x00000400,
    TX_BYTES         = 0x00000800,
    ESSID            = 0x00001000,
    MODE             = 0x00002000,
    FREQUENCY        = 0x00004000,
    BIT_RATE         = 0x00008000,
    ACCESS_POINT     = 0x00010000,
    LINK_QUALITY     = 0x00020000,
    BCAST_ADDRESS    = 0x00040000,
    GATEWAY          = 0x00080000,
    DOWNLOAD_SPEED   = 0x00100000,
    UPLOAD_SPEED     = 0x00200000,
    NICK_NAME        = 0x00400000,
    ENCRYPTION       = 0x00800000
};

static const int defaultTip = STATUS | IP_ADDRESS | RX_BYTES | TX_BYTES | ESSID | LINK_QUALITY | DOWNLOAD_SPEED | UPLOAD_SPEED | ENCRYPTION;

struct KNemoTheme
{
    QString name;
    QString comment;
    QString internalName;
};

struct AddrData
{
    int afType;
    QString broadcastAddress;
    int scope;
    QString ipv6Flags;
    QString label;
    bool hasPeer;
};

struct BackendData
{
    BackendData()
      : status( KNemoIface::UnknownState ),
        index( -1 ),
        interfaceType( KNemoIface::UnknownType ),
        isWireless( false ),
        prevRxPackets( 0L ),
        prevTxPackets( 0L ),
        rxPackets( 0L ),
        txPackets( 0L ),
        prevRxBytes( 0L ),
        prevTxBytes( 0L ),
        incomingBytes( 0L ),
        outgoingBytes( 0L ),
        rxBytes( 0L ),
        txBytes( 0L ),
        isEncrypted( false )
    {}

    int status;
    int index;
    KNemoIface::Type interfaceType;
    bool isWireless;
    unsigned long prevRxPackets;
    unsigned long prevTxPackets;
    unsigned long rxPackets;
    unsigned long txPackets;
    unsigned long prevRxBytes;
    unsigned long prevTxBytes;
    unsigned long incomingBytes;
    unsigned long outgoingBytes;
    QMap<QString, AddrData> addrData;
    QString hwAddress;
    QString ip4DefaultGateway;
    QString ip6DefaultGateway;
    QString rxString;
    QString txString;
    quint64 rxBytes;
    quint64 txBytes;

    QString essid;
    QString mode;
    QString frequency;
    QString channel;
    QString bitRate;
    QString linkQuality;
    QString accessPoint;
    QString prevAccessPoint;
    QString nickName;
    bool isEncrypted;
};

struct GeneralSettings
{
    GeneralSettings()
      : toolTipContent( defaultTip ),
        pollInterval( 1.0 ),
        saveInterval( 60 ),
        useBitrate( false ),
        statisticsDir( QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/knemo") )
    {}
    int toolTipContent;
    double pollInterval;
    int saveInterval;
    bool useBitrate;
    QDir statisticsDir;
};

class StatsRule
{
public:
    StatsRule()
        :
        periodCount( 1 ),
        periodUnits( KNemoStats::Month ),
        logOffpeak( false ),
        offpeakStartTime( QTime( 23, 0) ),
        offpeakEndTime( QTime( 7, 0) ),
        weekendIsOffpeak( false ),
        weekendDayStart( 5 ),
        weekendDayEnd( 1 ),
        weekendTimeStart( QTime( 23, 0) ),
        weekendTimeEnd( QTime( 7, 0) )
    {
    }
    bool operator==( StatsRule &r );
    bool isValid( KCalendarSystem *cal );
    QDate startDate;
    int periodCount;
    int periodUnits;
    bool logOffpeak;
    QTime offpeakStartTime;
    QTime offpeakEndTime;
    bool weekendIsOffpeak;
    int weekendDayStart;
    int weekendDayEnd;
    QTime weekendTimeStart;
    QTime weekendTimeEnd;
};

struct WarnRule
{
    WarnRule()
        : periodUnits( KNemoStats::Month ),
        periodCount( 1 ),
        trafficType( KNemoStats::PeakOffpeak ),
        trafficDirection( KNemoStats::TrafficIn ),
        trafficUnits( KNemoStats::UnitG ),
        threshold( 5.0 ),
        warnDone( false )
    {
    }
    bool operator==( WarnRule &r )
    {
        if ( periodUnits == r.periodUnits &&
             periodCount == r.periodCount &&
             trafficType == r.trafficType &&
             trafficDirection == r.trafficDirection &&
             trafficUnits == r.trafficUnits &&
             threshold == r.threshold )
            return true;
        else
            return false;
    }
    int periodUnits;
    uint periodCount;
    int trafficType;
    int trafficDirection;
    int trafficUnits;
    double threshold;
    QString customText;
    bool warnDone;
};

struct InterfaceSettings
{
    InterfaceSettings()
      : barScale( false ),
        maxRate( 4 ),
        trafficThreshold( 0 ),
        minVisibleState( KNemoIface::UnknownState ),
        activateStatistics( false ),
        calendarSystem( KLocale::QDateCalendar )
    {}

    QString iconTheme;
    bool barScale;
    unsigned int maxRate;
    unsigned int trafficThreshold;
    int minVisibleState;
    bool activateStatistics;
    QList<StatsRule> statsRules;
    QList<WarnRule> warnRules;
    KLocale::CalendarSystem calendarSystem;
    QString alias;
};

#ifndef __linux__
enum rt_scope_t
{
    RT_SCOPE_UNIVERSE=0,
    RT_SCOPE_SITE=200,
    RT_SCOPE_LINK=253,
    RT_SCOPE_HOST=254,
    RT_SCOPE_NOWHERE=255
};
#endif

static const double pollIntervals[] = { 0.1, 0.2, 0.25, 0.5, 1.0, 2.0 };

#endif // DATA_H
