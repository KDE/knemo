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

#include "themeconfig.h"

ThemeConfig::ThemeConfig( const InterfaceSettings s ) : KDialog(),
    settings( s )
{
    mDlg.setupUi( mainWidget() );
    setButtons( KDialog::Default | KDialog::Ok | KDialog::Cancel );

    if ( settings.iconTheme != NETLOAD_THEME )
        mDlg.checkBarScale->hide();
    if ( settings.iconTheme != NETLOAD_THEME &&
         settings.iconTheme != TEXT_THEME )
    {
        mDlg.rateGroup->hide();
        mDlg.maxRateGroup->hide();
    }

    mDlg.txMaxRate->setValue( settings.outMaxRate );
    mDlg.rxMaxRate->setValue( settings.inMaxRate );

    mDlg.checkBarScale->setChecked( settings.barScale );
    mDlg.checkDynColor->setChecked( settings.dynamicColor );
    mDlg.colorIncomingMax->setColor( settings.colorIncomingMax );
    mDlg.colorOutgoingMax->setColor( settings.colorOutgoingMax );
    updateRateGroup();

    connect( this, SIGNAL( defaultClicked() ), SLOT( setDefaults() ) );
    connect( mDlg.checkBarScale, SIGNAL( toggled( bool ) ), SLOT( updateRateGroup() ) );
    connect( mDlg.checkDynColor, SIGNAL( toggled( bool ) ), SLOT( updateRateGroup() ) );
}

void ThemeConfig::setDefaults()
{
    InterfaceSettings s;

    mDlg.txMaxRate->setValue( s.outMaxRate );
    mDlg.rxMaxRate->setValue( s.inMaxRate );

    mDlg.checkBarScale->setChecked( s.barScale );
    mDlg.checkDynColor->setChecked( s.dynamicColor );
    mDlg.colorIncomingMax->setColor( s.colorIncomingMax );
    mDlg.colorOutgoingMax->setColor( s.colorOutgoingMax );
}

void ThemeConfig::updateRateGroup()
{
    if ( mDlg.checkBarScale->isChecked() || mDlg.checkDynColor->isChecked() )
        mDlg.maxRateGroup->setEnabled( true );
    else
        mDlg.maxRateGroup->setEnabled( false );
}

InterfaceSettings ThemeConfig::getSettings()
{
    settings.outMaxRate = mDlg.txMaxRate->value();
    settings.inMaxRate = mDlg.rxMaxRate->value();

    settings.barScale = mDlg.checkBarScale->isChecked();
    settings.dynamicColor = mDlg.checkDynColor->isChecked();
    settings.colorIncomingMax = mDlg.colorIncomingMax->color();
    settings.colorOutgoingMax = mDlg.colorOutgoingMax->color();

    return settings;
}
