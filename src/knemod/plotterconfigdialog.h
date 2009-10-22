/* This file is part of KNemo
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


#ifndef PLOTTERCONFIGDIALOG_H
#define PLOTTERCONFIGDIALOG_H

#include <KDialog>
#include <QColor>
#include "ui_plotterconfigdlg.h"

struct PlotterSettings
{
    PlotterSettings()
      : pixel( 1 ),
        distance( 30 ),
        fontSize( 8 ),
        minimumValue( 0 ),
        maximumValue( 8 ),
        labels( true ),
        bottomBar( true ),
        showIncoming( true ),
        showOutgoing( true ),
        verticalLines( true ),
        horizontalLines( true ),
        automaticDetection( true ),
        verticalLinesScroll( true ),
        colorVLines( 0x04FB1D ),
        colorHLines( 0x04FB1D ),
        colorIncoming( 0x1889FF ),
        colorOutgoing( 0xFF7F08 ),
        colorBackground(0x313031 ),
        opacity( 20 )
    {}

    int pixel;
    int distance;
    int fontSize;
    int minimumValue;
    int maximumValue;
    bool labels;
    bool bottomBar;
    bool showIncoming;
    bool showOutgoing;
    bool verticalLines;
    bool horizontalLines;
    bool automaticDetection;
    bool verticalLinesScroll;
    QColor colorVLines;
    QColor colorHLines;
    QColor colorIncoming;
    QColor colorOutgoing;
    QColor colorBackground;
    int opacity;
};

class PlotterConfigDialog : public KDialog
{
    Q_OBJECT
    public:
        PlotterConfigDialog( QWidget *parent, const QString& iface, PlotterSettings* settings );
        virtual ~PlotterConfigDialog();
    signals:
        void saved();
    private slots:
        void changed();
        void defaults();
        void save();
    private:
        void load();
        Ui::Form ui;

    QString mName;
    PlotterSettings *mSettings;
    /*QColor mColorVLines;
    QColor mColorHLines;
    QColor mColorIncoming;
    QColor mColorOutgoing;
    QColor mColorBackground;*/
};

#endif
