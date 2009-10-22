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


#include "plotterconfigdialog.h"
#include "data.h"

#include <KConfig>

PlotterConfigDialog::PlotterConfigDialog( QWidget * parent, const QString& iface, PlotterSettings* settings ) : KDialog( parent ),
      mName( iface ),
      mSettings( settings )
{
    setButtons( Ok | Apply | Default | Close );
    ui.setupUi( mainWidget() );
    load();
    enableButtonApply( false );
    //enableButtonDefault( true );

    connect( this, SIGNAL( defaultClicked() ), SLOT( defaults() ) );
    connect( this, SIGNAL( applyClicked() ), SLOT( save() ) );
    connect( this, SIGNAL( okClicked() ), SLOT( save() ) );

    // connect the plotter widgets
    connect( ui.checkBoxBottomBar, SIGNAL( toggled( bool ) ),
             this, SLOT( changed() ) );
    connect( ui.checkBoxLabels, SIGNAL( toggled( bool ) ),
             this, SLOT( changed() ) );
    connect( ui.checkBoxVLines, SIGNAL( toggled( bool ) ),
             this, SLOT( changed() ) );
    connect( ui.checkBoxHLines, SIGNAL( toggled( bool ) ),
             this, SLOT( changed() ) );
    connect( ui.checkBoxIncoming, SIGNAL( toggled( bool ) ),
             this, SLOT( changed() ) );
    connect( ui.checkBoxOutgoing, SIGNAL( toggled( bool ) ),
             this, SLOT( changed() ) );
    connect( ui.checkBoxVLinesScroll, SIGNAL( toggled( bool ) ),
             this, SLOT( changed() ) );
    connect( ui.checkBoxAutoDetection, SIGNAL( toggled( bool ) ),
             this, SLOT( changed() ) );
    connect( ui.spinBoxPixel, SIGNAL( valueChanged( int ) ),
             this, SLOT( changed() ) );
    connect( ui.spinBoxDistance, SIGNAL( valueChanged( int ) ),
             this, SLOT( changed() ) );
    connect( ui.spinBoxFontSize, SIGNAL( valueChanged( int ) ),
             this, SLOT( changed() ) );
    connect( ui.spinBoxMinValue, SIGNAL( valueChanged( int ) ),
             this, SLOT( changed() ) );
    connect( ui.spinBoxMaxValue, SIGNAL( valueChanged( int ) ),
             this, SLOT( changed() ) );
    connect( ui.kColorButtonVLines, SIGNAL( changed( const QColor& ) ),
             this, SLOT( changed() ) );
    connect( ui.kColorButtonHLines, SIGNAL( changed( const QColor& ) ),
             this, SLOT( changed() ) );
    connect( ui.kColorButtonIncoming, SIGNAL( changed( const QColor& ) ),
             this, SLOT( changed() ) );
    connect( ui.kColorButtonOutgoing, SIGNAL( changed( const QColor& ) ),
             this, SLOT( changed() ) );
    connect( ui.kColorButtonBackground, SIGNAL( changed( const QColor& ) ),
             this, SLOT( changed() ) );
    connect( ui.spinBoxOpacity, SIGNAL( valueChanged( const int ) ),
             this, SLOT( changed() ) );
}

PlotterConfigDialog::~PlotterConfigDialog()
{
}

void PlotterConfigDialog::load()
{
    ui.spinBoxPixel->setValue( mSettings->pixel );
    ui.spinBoxDistance->setValue( mSettings->distance );
    ui.spinBoxFontSize->setValue( mSettings->fontSize );
    ui.spinBoxMinValue->setValue( mSettings->minimumValue );
    ui.spinBoxMaxValue->setValue( mSettings->maximumValue );
    ui.checkBoxLabels->setChecked( mSettings->labels );
    ui.checkBoxBottomBar->setChecked( mSettings->bottomBar );
    ui.checkBoxVLines->setChecked( mSettings->verticalLines );
    ui.checkBoxHLines->setChecked( mSettings->horizontalLines );
    ui.checkBoxIncoming->setChecked( mSettings->showIncoming );
    ui.checkBoxOutgoing->setChecked( mSettings->showOutgoing );
    ui.checkBoxAutoDetection->setChecked( mSettings->automaticDetection );
    ui.checkBoxVLinesScroll->setChecked( mSettings->verticalLinesScroll );
    ui.kColorButtonVLines->setColor( mSettings->colorVLines );
    ui.kColorButtonHLines->setColor( mSettings->colorHLines );
    ui.kColorButtonIncoming->setColor( mSettings->colorIncoming );
    ui.kColorButtonOutgoing->setColor( mSettings->colorOutgoing );
    ui.kColorButtonBackground->setColor( mSettings->colorBackground );
    ui.spinBoxOpacity->setValue( mSettings->opacity );
}

void PlotterConfigDialog::save()
{
    mSettings->pixel = ui.spinBoxPixel->value();
    mSettings->distance = ui.spinBoxDistance->value();
    mSettings->fontSize = ui.spinBoxFontSize->value();
    mSettings->minimumValue = ui.spinBoxMinValue->value();
    mSettings->maximumValue = ui.spinBoxMaxValue->value();
    mSettings->labels = ui.checkBoxLabels->isChecked();
    mSettings->bottomBar = ui.checkBoxBottomBar->isChecked();
    mSettings->verticalLines = ui.checkBoxVLines->isChecked();
    mSettings->horizontalLines = ui.checkBoxHLines->isChecked();
    mSettings->showIncoming = ui.checkBoxIncoming->isChecked();
    mSettings->showOutgoing = ui.checkBoxOutgoing->isChecked();
    mSettings->automaticDetection = ui.checkBoxAutoDetection->isChecked();
    mSettings->verticalLinesScroll = ui.checkBoxVLinesScroll->isChecked();
    mSettings->colorVLines = ui.kColorButtonVLines->color();
    mSettings->colorHLines = ui.kColorButtonHLines->color();
    mSettings->colorIncoming = ui.kColorButtonIncoming->color();
    mSettings->colorOutgoing = ui.kColorButtonOutgoing->color();
    mSettings->colorBackground = ui.kColorButtonBackground->color();
    mSettings->opacity = ui.spinBoxOpacity->value();
    emit saved();
}

void PlotterConfigDialog::defaults()
{
    enableButtonApply( true );
    PlotterSettings s;
    // Default plotter settings
    ui.spinBoxPixel->setValue( s.pixel );
    ui.spinBoxDistance->setValue( s.distance );
    ui.spinBoxFontSize->setValue( s.fontSize );
    ui.spinBoxMinValue->setValue( s.minimumValue );
    ui.spinBoxMaxValue->setValue( s.maximumValue );
    ui.checkBoxLabels->setChecked( s.labels );
    ui.checkBoxBottomBar->setChecked( s.bottomBar );
    ui.checkBoxVLines->setChecked( s.verticalLines );
    ui.checkBoxHLines->setChecked( s.horizontalLines );
    ui.checkBoxIncoming->setChecked( s.showIncoming );
    ui.checkBoxOutgoing->setChecked( s.showOutgoing );
    ui.checkBoxAutoDetection->setChecked( s.automaticDetection );
    ui.checkBoxVLinesScroll->setChecked( s.verticalLinesScroll );
    ui.kColorButtonVLines->setColor( s.colorVLines );
    ui.kColorButtonHLines->setColor( s.colorHLines );
    ui.kColorButtonIncoming->setColor( s.colorIncoming );
    ui.kColorButtonOutgoing->setColor( s.colorOutgoing );
    ui.kColorButtonBackground->setColor( s.colorBackground );
    ui.spinBoxOpacity->setValue( s.opacity );
}

void PlotterConfigDialog::changed()
{
    enableButtonApply( true );
}

#include "plotterconfigdialog.moc"

