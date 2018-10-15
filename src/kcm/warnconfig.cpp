/* This file is part of KNemo
   Copyright (C) 2010 John Stamp <jstamp@users.sourceforge.net>

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

#include <math.h>
#include "warnconfig.h"
#include <QPushButton>
#include <KMessageBox>

WarnConfig::WarnConfig( const InterfaceSettings *settings, const WarnRule &warn, bool addRule )
    : QDialog(),
    mSettings( settings ),
    mAddRule( addRule )
{
    mDlg.setupUi( this );

    QList<StatsRule> statsRules = settings->statsRules;
    bool offpeakTracking = false;
    foreach( StatsRule stats, statsRules )
    {
        if ( stats.logOffpeak )
        {
            offpeakTracking = true;
            break;
        }
    }
    if ( !offpeakTracking )
    {
        mDlg.trafficTypeLabel->hide();
        mDlg.trafficType->hide();
    }

    mDlg.trafficUnits->addItem( i18n( "KiB" ), KNemoStats::UnitK );
    mDlg.trafficUnits->addItem( i18n( "MiB" ), KNemoStats::UnitM );
    mDlg.trafficUnits->addItem( i18n( "GiB" ), KNemoStats::UnitG );

    mDlg.periodUnits->addItem( i18n( "Hours" ), KNemoStats::Hour );
    mDlg.periodUnits->addItem( i18n( "Days" ), KNemoStats::Day );
    mDlg.periodUnits->addItem( i18n( "Weeks" ), KNemoStats::Week );
    mDlg.periodUnits->addItem( i18n( "Months" ), KNemoStats::Month );
    if ( settings->statsRules.count() )
        mDlg.periodUnits->addItem( i18n( "Billing Periods" ), KNemoStats::BillPeriod );
    //mDlg.periodUnits->addItem( i18n( "Years" ), KNemoStats::Year );

    mDlg.legend->setText( i18n( "<i>%i</i> = interface,<br/>"
                "<i>%t</i> = traffic threshold, <i>%c</i> = current traffic" ) );

    connect( mDlg.buttonBox, SIGNAL( accepted() ), SLOT( accept() ) );
    connect( mDlg.buttonBox, SIGNAL( rejected() ), SLOT( reject() ) );
    connect( mDlg.buttonBox, SIGNAL( clicked( QAbstractButton* ) ), SLOT( setDefaults( QAbstractButton* ) ) );
    connect( mDlg.threshold, SIGNAL( valueChanged( double ) ), this, SLOT( thresholdChanged( double ) ) );

    setControls( warn );
}

void WarnConfig::setControls( const WarnRule &warn )
{
    mDlg.trafficType->setCurrentIndex( warn.trafficType );
    mDlg.trafficDirection->setCurrentIndex( warn.trafficDirection );
    mDlg.threshold->setValue( warn.threshold );
    int index = mDlg.trafficUnits->findData( warn.trafficUnits );
    mDlg.trafficUnits->setCurrentIndex( index );
    mDlg.periodCount->setValue( warn.periodCount );
    index = mDlg.periodUnits->findData( warn.periodUnits );
    if ( index < 0 )
    {
        index = mDlg.periodUnits->findData( KNemoStats::Month );
    }
    mDlg.periodUnits->setCurrentIndex( index );
    mDlg.customTextEdit->setPlainText( warn.customText );
    mDlg.customTextCheck->setChecked( !warn.customText.trimmed().isEmpty() );
}

WarnRule WarnConfig::settings()
{
    WarnRule warn;
    warn.trafficType = mDlg.trafficType->currentIndex();
    warn.trafficDirection = mDlg.trafficDirection->currentIndex();
    warn.threshold = mDlg.threshold->value();
    warn.trafficUnits = mDlg.trafficUnits->itemData( mDlg.trafficUnits->currentIndex() ).toInt();
    warn.periodCount = mDlg.periodCount->value();
    warn.periodUnits = mDlg.periodUnits->itemData( mDlg.periodUnits->currentIndex() ).toInt();
    if ( mDlg.customTextCheck->isChecked() )
        warn.customText = mDlg.customTextEdit->toPlainText().trimmed();
    else
        warn.customText = QString();
    return warn;
}

void WarnConfig::thresholdChanged( double val )
{
    double test = round(val*10.0)/10.0;
    if ( val != test )
        mDlg.threshold->setValue( test );
}

void WarnConfig::setDefaults(QAbstractButton* button)
{
    if (static_cast<QPushButton*>(button) == mDlg.buttonBox->button(QDialogButtonBox::RestoreDefaults) ) {
        WarnRule warn;
        setControls( warn );
    }
}

void WarnConfig::accept()
{
    WarnRule testRule = settings();
    bool duplicateEntry = false;
    QList<WarnRule> warnRules = mSettings->warnRules;
    foreach ( WarnRule rule, warnRules )
    {
        if ( rule == testRule )
        {
            duplicateEntry = true;
            break;
        }
    }
    if ( duplicateEntry )
        KMessageBox::sorry( 0, i18n( "This traffic notification rule already exists." ) );
    else
        QDialog::accept();
}

#include "moc_warnconfig.cpp"
