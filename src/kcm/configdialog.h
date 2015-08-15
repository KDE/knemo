/* This file is part of KNemo
   Copyright (C) 2004, 2005, 2006 Percy Leonhardt <percy@eris23.de>
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

#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <KCModule>

#include "data.h"
#include "ui_configdlg.h"
#include <QStandardItemModel>

class QTreeWidgetItem;
class KCalendarSystem;

/**
 * This is the configuration dialog for KNemo
 * It is implemented as a control center module so that it is still
 * possible to configure KNemo even when there is no icon visible
 * in the system tray.
 *
 * @short Configuration dialog for KNemo
 * @author Percy Leonhardt <percy@eris23.de>
 */

class StatsRuleModel : public QStandardItemModel
{
    Q_OBJECT
    public:
        StatsRuleModel( QObject *parent = 0 ) :
            QStandardItemModel( parent ) {}
        virtual ~StatsRuleModel() {}
        void setCalendar( const KCalendarSystem *cal );
        QModelIndex addRule( const StatsRule &s );
        void modifyRule( const QModelIndex &index, const StatsRule &s );
        QList<StatsRule> getRules();
    private:
        QString dateText( const StatsRule &s );
        const KCalendarSystem *mCalendar;
};

class WarnModel : public QStandardItemModel
{
    Q_OBJECT
    public:
        WarnModel( QObject *parent = 0 ) : QStandardItemModel( parent ) {}
        virtual ~WarnModel() {}
        QModelIndex addWarn( const WarnRule &w );
        void modifyWarn( const QModelIndex &index, const WarnRule &warn );
        QList<WarnRule> getRules();
    private:
        QString ruleText( const WarnRule &warn );
};

class ConfigDialog : public KCModule
{
    Q_OBJECT
public:
    /**
     * Default Constructor
     */
    ConfigDialog( QWidget *parent, const QVariantList &args );

    /**
     * Default Destructor
     */
    virtual ~ConfigDialog();

    void load();
    void save();
    void defaults();

private Q_SLOTS:
    void buttonNewSelected();
    void buttonAllSelected();
    void buttonDeleteSelected();
    void buttonAddToolTipSelected();
    void buttonRemoveToolTipSelected();
    void buttonNotificationsSelected();
    void interfaceSelected( int row );
    void aliasChanged( const QString& text );
    void iconThemeChanged( int set );
    void comboHidingChanged( int val );
    void checkBoxStatisticsToggled( bool on );
    void checkBoxStartKNemoToggled( bool on );
    void colorButtonChanged();
    void advancedButtonClicked();
    void addStatsClicked();
    void modifyStatsClicked();
    void removeStatsClicked();
    void addWarnClicked();
    void modifyWarnClicked();
    void removeWarnClicked();
    void moveTips( QListWidget *from, QListWidget *to );

private:
    void setupToolTipTab();
    void setupToolTipMap();
    void updateControls( InterfaceSettings *settings );
    InterfaceSettings * getItemSettings();
    int findIndexFromName( const QString& internalName );
    QString findNameFromIndex( int index );
    QPixmap textIcon( QString incomingText, QString outgoingText, int status );
    QPixmap barIcon( int status );
    void updateWarnText( int oldCount );

    int mToolTipContent;
    bool mLock;
    Ui::ConfigDlg* mDlg;
    const KCalendarSystem* mCalendar;
    int mMaxDay;
    StatsRuleModel *statsModel;
    WarnModel *warnModel;

    QMap<QString, InterfaceSettings *> mSettingsMap;
    QMap<quint32, QString> mToolTips;
    QList<QString> mDeletedIfaces;
};

#endif // CONFIGDIALOG_H
