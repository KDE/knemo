/* This file is part of KNemo
   Copyright (C) 2004, 2006 Percy Leonhardt <percy@eris23.de>
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

#ifndef INTERFACEPLOTTERDIALOG_H
#define INTERFACEPLOTTERDIALOG_H

#include <QDialog>
#include <KSharedConfig>

#include "plotterconfigdialog.h"

class FancyPlotterLabel;
class KSignalPlotter;
class QBoxLayout;

class InterfacePlotterDialog : public QDialog
{
Q_OBJECT
public:
    InterfacePlotterDialog( QString );
    virtual ~InterfacePlotterDialog();

    /**
     * Update the signal plotter with new data
     */
    void updatePlotter( const double, const double );
    void useBitrate( bool );

protected:
    bool event( QEvent *e );
    void resizeEvent( QResizeEvent* );

private Q_SLOTS:
    void configFinished();
    void saveConfig();
    void setPlotterUnits();

private:
    void showContextMenu( const QPoint& );
    void loadConfig();
    void configChanged();
    void configPlotter();
    void addBeams();

    KSharedConfig::Ptr mConfig;
    PlotterConfigDialog *mConfigDlg;
    QWidget *mLabelsWidget;
    bool mSetPos;
    bool mWasShown;
    bool mUseBitrate;
    int mMultiplier;
    bool mOutgoingVisible;
    bool mIncomingVisible;
    PlotterSettings mSettings;
    QString mName;
    KSignalPlotter *mPlotter;
    FancyPlotterLabel *mReceivedLabel;
    FancyPlotterLabel *mSentLabel;
    QBoxLayout *mLabelLayout;
    QChar mIndicatorSymbol;
    QList<KLocalizedString> mByteUnits;
    QList<KLocalizedString> mBitUnits;
};

#endif
