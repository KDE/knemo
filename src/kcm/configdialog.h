/* This file is part of KNemo
   Copyright (C) 2004, 2005, 2006 Percy Leonhardt <percy@eris23.de>
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

#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <kcmodule.h>

#include "data.h"
#include "ui_configdlg.h"

class QStandardItem;
class QStandardItemModel;
class QTimer;
class QTreeWidgetItem;

/**
 * This is the configuration dialog for KNemo
 * It is implemented as a control center module so that it is still
 * possible to configure KNemo even when there is no icon visible
 * in the system tray.
 *
 * @short Configuration dialog for KNemo
 * @author Percy Leonhardt <percy@eris23.de>
 */

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

private slots:
    void buttonNewSelected();
    void buttonAllSelected();
    void buttonDeleteSelected();
    void buttonAddCommandSelected();
    void buttonRemoveCommandSelected();
    void buttonCommandUpSelected();
    void buttonCommandDownSelected();
    void buttonAddToolTipSelected();
    void buttonRemoveToolTipSelected();
    void buttonNotificationsSelected();
    void interfaceSelected( int row );
    void aliasChanged( const QString& text );
    void iconSetChanged( int set );
    void backendChanged( int set );
    void checkBoxNotConnectedToggled( bool on );
    void checkBoxNotExistingToggled( bool on );
    void checkBoxStatisticsToggled( bool on );
    void checkBoxStartKNemoToggled( bool on );
    void spinBoxTrafficValueChanged( int value );
    void checkBoxCustomToggled( bool on );
    void listViewCommandsSelectionChanged( QTreeWidgetItem *current, QTreeWidgetItem *previous );
    void listViewCommandsChanged( QTreeWidgetItem* item, int column );

    /**
     * These three are generic.
     * They are used for all plotter settings to activate the
     * 'Apply' button when something was changed by the user.
     */
    void checkBoxToggled( bool );
    void spinBoxValueChanged( int );
    void kColorButtonChanged( const QColor& );

private:
    void setupToolTipTab();
    void setupToolTipArray();
    void updateStatisticsEntries( void );

    enum IconSet
    {
        MONITOR = 0,
        MODEM,
        NETWORK,
        WIRELESS
    };

    int mToolTipContent;
    bool mLock;
    Ui::ConfigDlg* mDlg;
    QColor mColorVLines;
    QColor mColorHLines;
    QColor mColorIncoming;
    QColor mColorOutgoing;
    QColor mColorBackground;
    KSharedConfigPtr mConfig;
    QMap<QString, InterfaceSettings *> mSettingsMap;
    QMap<quint32, QString> mToolTips;
    QList<QString> mDeletedIfaces;
    QStringList mIconSets;

    static const QString ICON_DISCONNECTED;
    static const QString ICON_CONNECTED;
    static const QString ICON_INCOMING;
    static const QString ICON_OUTGOING;
    static const QString ICON_TRAFFIC;
};

#endif // CONFIGDIALOG_H
