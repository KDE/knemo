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

#include "statsconfig.h"
#include <QPushButton>
#include <KCalendarSystem>
#include <KGlobal>
#include <KMessageBox>

StatsConfig::StatsConfig( const InterfaceSettings * settings, const KCalendarSystem *calendar, const StatsRule &rule, bool addRule ) : QDialog(),
    mSettings( settings ),
    mCal( calendar ),
    mAddRule( addRule )
{
    // Do this for the sake of KDateEdit
    KGlobal::locale()->setCalendarSystem( mCal->calendarSystem() );

    mDlg.setupUi( this );

    for ( int i = 1; i <= mCal->daysInWeek( QDate::currentDate() ); ++i )
    {
        mDlg.weekendStartDay->addItem( mCal->weekDayName( i ) );
        mDlg.weekendStopDay->addItem( mCal->weekDayName( i ) );
    }

    mDlg.periodUnits->addItem( i18n( "Days" ), KNemoStats::Day );
    mDlg.periodUnits->addItem( i18n( "Weeks" ), KNemoStats::Week );
    mDlg.periodUnits->addItem( i18n( "Months" ), KNemoStats::Month );
    //mDlg.periodUnits->addItem( i18n( "Years" ), KNemoStats::Year );

    connect( mDlg.buttonBox, SIGNAL( accepted() ), SLOT( accept() ) );
    connect( mDlg.buttonBox, SIGNAL( rejected() ), SLOT( reject() ) );
    connect( mDlg.buttonBox, SIGNAL( clicked( QAbstractButton* ) ), SLOT( setDefaults( QAbstractButton* ) ) );
    connect( mDlg.logOffpeak, SIGNAL( toggled( bool ) ), SLOT( enableItems() ) );
    connect( mDlg.doWeekend, SIGNAL( toggled( bool ) ), SLOT( enableItems() ) );

    QDate dt = rule.startDate;
    if ( !dt.isValid() )
        dt = QDate::currentDate().addDays( 1 - mCal->day( QDate::currentDate() ) );
    mDlg.startDate->setDate( dt );
    setControls( rule );
}

void StatsConfig::setControls( const StatsRule &s )
{
    mDlg.periodCount->setValue( s.periodCount );
    int index = mDlg.periodUnits->findData( s.periodUnits );
    mDlg.periodUnits->setCurrentIndex( index );
    mDlg.logOffpeak->setChecked( s.logOffpeak );
    mDlg.startTime->setTime( s.offpeakStartTime );
    mDlg.stopTime->setTime( s.offpeakEndTime );
    mDlg.doWeekend->setChecked( s.weekendIsOffpeak );
    mDlg.weekendStartDay->setCurrentIndex( s.weekendDayStart - 1 );
    mDlg.weekendStopDay->setCurrentIndex( s.weekendDayEnd - 1 );
    mDlg.weekendStartTime->setTime( s.weekendTimeStart );
    mDlg.weekendStopTime->setTime( s.weekendTimeEnd );
}

void StatsConfig::setDefaults( QAbstractButton* button )
{
    if (static_cast<QPushButton*>(button) == mDlg.buttonBox->button(QDialogButtonBox::RestoreDefaults) ) {
        StatsRule s;
        mDlg.startDate->setDate( QDate::currentDate().addDays( 1 - mCal->day( QDate::currentDate() ) ) );
        setControls( s );
    }
}

StatsRule StatsConfig::settings()
{
    StatsRule rule;
    rule.startDate = mDlg.startDate->date();
    rule.periodUnits = mDlg.periodUnits->itemData( mDlg.periodUnits->currentIndex() ).toInt();
    rule.periodCount = mDlg.periodCount->value();
    rule.logOffpeak = mDlg.logOffpeak->isChecked();
    rule.offpeakStartTime = mDlg.startTime->time();
    rule.offpeakEndTime = mDlg.stopTime->time();
    rule.weekendIsOffpeak = mDlg.doWeekend->isChecked();
    rule.weekendDayStart = mDlg.weekendStartDay->currentIndex() + 1;
    rule.weekendDayEnd = mDlg.weekendStopDay->currentIndex() + 1;
    rule.weekendTimeStart = mDlg.weekendStartTime->time();
    rule.weekendTimeEnd = mDlg.weekendStopTime->time();
    return rule;
}

void StatsConfig::enableItems()
{
    bool enabledItems;
    if ( mDlg.logOffpeak->isChecked() && mDlg.doWeekend->isChecked() )
    {
        enabledItems = true;
    }
    else
    {
        enabledItems = false;
    }

    mDlg.label_9->setEnabled( enabledItems );
    mDlg.label_10->setEnabled( enabledItems );
    mDlg.weekendStartDay->setEnabled( enabledItems );
    mDlg.weekendStopDay->setEnabled( enabledItems );
    mDlg.weekendStartTime->setEnabled( enabledItems );
    mDlg.weekendStopTime->setEnabled( enabledItems );
}

void StatsConfig::accept()
{
    bool duplicateEntry = false;
    StatsRule testRule = settings();
    QList<StatsRule> statsRules = mSettings->statsRules;
    foreach ( StatsRule rule, statsRules )
    {
        if ( rule == testRule )
        {
            duplicateEntry = true;
            break;
        }
    }
    if ( duplicateEntry )
    {
        KMessageBox::sorry( 0,
                            i18n( "Another rule already starts on %1. "
                                  "Please choose another date.",
                                  mCal->formatDate( mDlg.startDate->date(), KLocale::LongDate ) )
                          );
    } else {
        QDialog::accept();
    }
}

#include "moc_statsconfig.cpp"
