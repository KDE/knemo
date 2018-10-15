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

#include "themeconfig.h"

#include <QPushButton>

ThemeConfig::ThemeConfig( const InterfaceSettings s ) : QDialog(),
    mSettings( s )
{
    mDlg.setupUi( this );

    if ( mSettings.iconTheme != NETLOAD_THEME )
    {
        mDlg.checkBarScale->hide();
        mDlg.rateGroup->hide();
    }

    mDlg.spinBoxTrafficThreshold->setValue( mSettings.trafficThreshold );

    mDlg.maxRate->setValue( mSettings.maxRate );

    mDlg.checkBarScale->setChecked( mSettings.barScale );

    connect( mDlg.buttonBox, SIGNAL( accepted() ), SLOT( accept() ) );
    connect( mDlg.buttonBox, SIGNAL( rejected() ), SLOT( reject() ) );
    connect( mDlg.buttonBox, SIGNAL( clicked( QAbstractButton* ) ), SLOT( setDefaults( QAbstractButton* ) ) );
}

void ThemeConfig::setDefaults( QAbstractButton* button )
{
    if (static_cast<QPushButton*>(button) == mDlg.buttonBox->button(QDialogButtonBox::RestoreDefaults) ) {
        InterfaceSettings s;

        mDlg.spinBoxTrafficThreshold->setValue( s.trafficThreshold );

        mDlg.maxRate->setValue( s.maxRate );

        mDlg.checkBarScale->setChecked( s.barScale );
    }
}

InterfaceSettings ThemeConfig::settings()
{
    mSettings.trafficThreshold = mDlg.spinBoxTrafficThreshold->value();

    mSettings.maxRate = mDlg.maxRate->value();

    mSettings.barScale = mDlg.checkBarScale->isChecked();

    return mSettings;
}

#include "moc_themeconfig.cpp"
