/* This file is part of KNemo
   Copyright (C) 2004 Percy Leonhardt <percy@eris23.de>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <qdict.h>
#include <qpair.h>
#include <qstring.h>
#include <qlistview.h>

#include <kcmodule.h>

#include "data.h"

class QTimer;
class ConfigDlg;
class KNemoCheckListItem;

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
    ConfigDialog( QWidget *parent, const char *name, const QStringList& );

    /**
     * Default Destructor
     */
    virtual ~ConfigDialog();

    void load();
    void save();
    void defaults();

private slots:
    void buttonNewSelected();
    void buttonDeleteSelected();
    void buttonAddCommandSelected();
    void buttonRemoveCommandSelected();
    void buttonCommandUpSelected();
    void buttonCommandDownSelected();
    void buttonAddToolTipSelected();
    void buttonRemoveToolTipSelected();
    void buttonNotificationsSelected();
    void interfaceSelected( const QString& interface );
    void aliasChanged( const QString& text );
    void iconSetChanged( int set );
    void checkBoxNotConnectedToggled( bool on );
    void checkBoxNotExistingToggled( bool on );
    void spinBoxTrafficValueChanged( int value );
    void checkBoxCustomToggled( bool on );
    void listViewCommandsSelectionChanged();
    void listViewCommandsCheckListItemChanged( KNemoCheckListItem* item, bool state );
    void listViewCommandsRenamed( QListViewItem* item, int col, const QString & text );

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

    enum IconSet
    {
        MONITOR = 0,
        MODEM,
        NETWORK,
        WIRELESS
    };

    int mToolTipContent;
    bool mLock;
    ConfigDlg* mDlg;
    QColor mColorVLines;
    QColor mColorHLines;
    QColor mColorIncoming;
    QColor mColorOutgoing;
    QColor mColorBackground;
    QDict<InterfaceSettings> mSettingsDict;
    QPair<QString, int> mToolTips[23];

    static const QString ICON_DISCONNECTED;
    static const QString ICON_CONNECTED;
    static const QString ICON_INCOMING;
    static const QString ICON_OUTGOING;
    static const QString ICON_TRAFFIC;
    static const QString SUFFIX_PPP;
    static const QString SUFFIX_LAN;
    static const QString SUFFIX_WLAN;
};

class KNemoCheckListItem : public QObject, public QCheckListItem
{
    Q_OBJECT
public:
    KNemoCheckListItem( QListView* view )
        : QCheckListItem( view, QString::null, QCheckListItem::CheckBox )
    {}

signals:
    void stateChanged( KNemoCheckListItem*, bool );

protected:
    void stateChange( bool state )
    {
        emit stateChanged( this, state );
    }
};

#endif // CONFIGDIALOG_H
