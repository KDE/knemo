/* This file is part of KNemo
   Copyright (C) 2004, 2006 Percy Leonhardt <percy@eris23.de>
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

#ifndef INTERFACEPLOTTERDIALOG_H
#define INTERFACEPLOTTERDIALOG_H

#include <KDialog>
#include <KSharedConfig>

#include "plotterconfigdialog.h"

class FancyPlotterLabel;
class KSignalPlotter;
class QBoxLayout;

class InterfacePlotterDialog : public KDialog
{
Q_OBJECT
public:
    InterfacePlotterDialog( QString );
    virtual ~InterfacePlotterDialog();

    /**
     * Update the signal plotter with new data
     */
    void updatePlotter( const double, const double );

protected:
    bool event( QEvent *e );

private slots:
    void configFinished();
    void saveConfig();

private:
    void showContextMenu( const QPoint& );
    void loadConfig();
    void configChanged();
    void configPlotter();

    KSharedConfigPtr mConfig;
    PlotterConfigDialog *mConfigDlg;
    bool mSetPos;
    bool mWasShown;
    PlotterSettings mSettings;
    QString mName;
    KSignalPlotter *mPlotter;
    FancyPlotterLabel *mReceivedLabel;
    FancyPlotterLabel *mSentLabel;
    QBoxLayout *mLabelLayout;
    QString mUnit;
    QChar mIndicatorSymbol;
};

#endif
