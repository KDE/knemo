/* This file is part of KNemo
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


#include "plotterconfigdialog.h"
#include "global.h"
#include <QPushButton>

PlotterConfigDialog::PlotterConfigDialog( QWidget * parent, const QString& iface, PlotterSettings* settings ) : QDialog( parent ),
      mName( iface ),
      mSettings( settings )
{
    QString suffix;
    if ( generalSettings->useBitrate )
    {
        suffix = i18n( " kbit/s" );
    }
    else
    {
        suffix = i18n( " KiB/s" );
    }
    ui.setupUi( this );
    ui.spinBoxMinValue->setSuffix( suffix );
    ui.spinBoxMaxValue->setSuffix( suffix );
    load();
    ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
    //enableButtonDefault( true );

    connect( ui.buttonBox, SIGNAL( accepted() ), SLOT( save() ) );
    connect( ui.buttonBox, SIGNAL( clicked( QAbstractButton* ) ), SLOT( defaults( QAbstractButton* ) ) );

    // connect the plotter widgets
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
    connect( ui.spinBoxMinValue, SIGNAL( valueChanged( double ) ),
             this, SLOT( changed() ) );
    connect( ui.spinBoxMaxValue, SIGNAL( valueChanged( double ) ),
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
    ui.checkBoxVLines->setChecked( mSettings->verticalLines );
    ui.checkBoxHLines->setChecked( mSettings->horizontalLines );
    ui.checkBoxIncoming->setChecked( mSettings->showIncoming );
    ui.checkBoxOutgoing->setChecked( mSettings->showOutgoing );
    ui.checkBoxAutoDetection->setChecked( !mSettings->automaticDetection );
    ui.checkBoxVLinesScroll->setChecked( mSettings->verticalLinesScroll );
}

void PlotterConfigDialog::save()
{
    mSettings->pixel = ui.spinBoxPixel->value();
    mSettings->distance = ui.spinBoxDistance->value();
    mSettings->fontSize = ui.spinBoxFontSize->value();
    mSettings->minimumValue = ui.spinBoxMinValue->value();
    mSettings->maximumValue = ui.spinBoxMaxValue->value();
    mSettings->labels = ui.checkBoxLabels->isChecked();
    mSettings->verticalLines = ui.checkBoxVLines->isChecked();
    mSettings->horizontalLines = ui.checkBoxHLines->isChecked();
    mSettings->showIncoming = ui.checkBoxIncoming->isChecked();
    mSettings->showOutgoing = ui.checkBoxOutgoing->isChecked();
    mSettings->automaticDetection = !ui.checkBoxAutoDetection->isChecked();
    mSettings->verticalLinesScroll = ui.checkBoxVLinesScroll->isChecked();
    ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
    emit saved();
}

void PlotterConfigDialog::defaults( QAbstractButton* button )
{
    if (static_cast<QPushButton*>(button) == ui.buttonBox->button(QDialogButtonBox::RestoreDefaults) ) {
        ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
        PlotterSettings s;
        // Default plotter settings
        ui.spinBoxPixel->setValue( s.pixel );
        ui.spinBoxDistance->setValue( s.distance );
        ui.spinBoxFontSize->setValue( s.fontSize );
        ui.spinBoxMinValue->setValue( s.minimumValue );
        ui.spinBoxMaxValue->setValue( s.maximumValue );
        ui.checkBoxLabels->setChecked( s.labels );
        ui.checkBoxVLines->setChecked( s.verticalLines );
        ui.checkBoxHLines->setChecked( s.horizontalLines );
        ui.checkBoxIncoming->setChecked( s.showIncoming );
        ui.checkBoxOutgoing->setChecked( s.showOutgoing );
        ui.checkBoxAutoDetection->setChecked( !s.automaticDetection );
        ui.checkBoxVLinesScroll->setChecked( s.verticalLinesScroll );
    } else if (static_cast<QPushButton*>(button) == ui.buttonBox->button(QDialogButtonBox::Ok) ) {
        QDialog::accept();
    } else if (static_cast<QPushButton*>(button) == ui.buttonBox->button(QDialogButtonBox::Apply) ) {
        save();
    } else if (static_cast<QPushButton*>(button) == ui.buttonBox->button(QDialogButtonBox::Cancel) ) {
        QDialog::reject();
    }

}

void PlotterConfigDialog::changed()
{
    ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
}

#include "moc_plotterconfigdialog.cpp"

