/* This file is part of KNemo
   Copyright (C) 2006 Percy Leonhardt <percy@eris23.de>
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

#ifndef INTERFACESTATISTICSDIALOG_H
#define INTERFACESTATISTICSDIALOG_H

#include <KDialog>
#include "ui_interfacestatisticsdlg.h"

class KCalendarSystem;
class QStandardItemModel;
class Interface;


/**
 * This class shows the statistics dialog. It contains the tables for the
 * different statistics.
 *
 * @short Statistics dialog
 * @author Percy Leonhardt <percy@eris23.de>
 */

class InterfaceStatisticsDialog : public KDialog
{
    Q_OBJECT
public:
    /**
     * Default Constructor
     */
    InterfaceStatisticsDialog( Interface* interface,
                               QWidget* parent = 0L );

    /**
     * Default Destructor
     */
    virtual ~InterfaceStatisticsDialog();

signals:
    void clearStatistics();

public slots:
    void updateDays();
    void updateWeeks();
    void updateMonths( bool );
    void updateYears();
    void updateCurrentEntry();
    void confirmReset();

protected:
    bool event( QEvent *e );

private:
    void updateEntry( const StatisticEntry* entry, QStandardItemModel* model );
    void updateModel( const QList<StatisticEntry *>& statistics,
                      QStandardItemModel* model, QTableView* view,
                      bool fullRebuild, int group );

    Ui::InterfaceStatisticsDlg ui;
    bool mWasShown;
    bool mSetPos;
    bool mIsMonths;
    KSharedConfigPtr mConfig;
    const KCalendarSystem* mCalendar;
    Interface* mInterface;
    QStandardItemModel* dailyModel;
    QStandardItemModel* weeklyModel;
    QStandardItemModel* monthlyModel;
    QStandardItemModel* yearlyModel;
};

#endif // INTERFACESTATISTICSDIALOG_H
