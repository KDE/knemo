/* This file is part of KNemo
   Copyright (C) 2006 Percy Leonhardt <percy@eris23.de>
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

#ifndef INTERFACESTATISTICSDIALOG_H
#define INTERFACESTATISTICSDIALOG_H

#include <QDialog>
#include "ui_interfacestatisticsdlg.h"

class StatisticsModel;
class Interface;


/**
 * This class shows the statistics dialog. It contains the tables for the
 * different statistics.
 *
 * @short Statistics dialog
 * @author Percy Leonhardt <percy@eris23.de>
 */

class InterfaceStatisticsDialog : public QDialog
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

    void configChanged();

Q_SIGNALS:
    void clearStatistics();

public Q_SLOTS:
    void confirmReset(QAbstractButton* button);

protected:
    bool event( QEvent *e );

private:
    void setupTable( KConfigGroup* group, QTableView * view, StatisticsModel* model );

    Ui::InterfaceStatisticsDlg ui;
    bool mWasShown;
    bool mSetPos;
    QWidget *mBillingWidget;
    StatisticsView *mBillingView;
    KSharedConfig::Ptr mConfig;
    Interface* mInterface;
    QHash<QTableView*, QString> mStateKeys;

private Q_SLOTS:
    void setCurrentSel();
};

#endif // INTERFACESTATISTICSDIALOG_H
