/* This file is part of KNemo
   Copyright (C) 2006 Percy Leonhardt <percy@eris23.de>

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

#include <qwidget.h>

#include "interfacestatisticsdlg.h"

class QTimer;
class Interface;

/**
 * This class shows the statistics dialog. It contains the tables for the
 * different statistics.
 *
 * @short Statistics dialog
 * @author Percy Leonhardt <percy@eris23.de>
 */

class InterfaceStatisticsDialog : public InterfaceStatisticsDlg
{
    Q_OBJECT
public:
    /**
     * Default Constructor
     */
    InterfaceStatisticsDialog( Interface* interface,
                               QWidget* parent = 0L, const char* name = 0L );

    /**
     * Default Destructor
     */
    virtual ~InterfaceStatisticsDialog();

signals:
    void clearDailyStatisticsClicked();
    void clearMonthlyStatisticsClicked();
    void clearYearlyStatisticsClicked();

public slots:
    void updateDays();
    void updateMonths();
    void updateYears();
    void updateCurrentEntry();

private:
    Interface* mInterface;
};

#endif // INTERFACESTATISTICSDIALOG_H
