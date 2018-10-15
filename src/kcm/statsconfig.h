/* This file is part of KNemo
   Copyright (C) 2010 John Stamp <jstamp@users.sourceforge.net>

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

#ifndef STATSCONFIG_H
#define STATSCONFIG_H

#include <QDialog>
#include "data.h"
#include "ui_statscfg.h"

class StatsConfig : public QDialog
{
Q_OBJECT
public:
    StatsConfig( const InterfaceSettings *settings, const KCalendarSystem *calendar, const StatsRule &rule, bool addRule = true );
    StatsRule settings();
private:
    Ui::StatsCfg mDlg;
    const InterfaceSettings *mSettings;
    const KCalendarSystem *mCal;
    bool mAddRule;
    void setControls( const StatsRule &s );
private Q_SLOTS:
    void setDefaults( QAbstractButton* );
    void enableItems();
    void accept();
};

#endif
