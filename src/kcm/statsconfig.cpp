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
#include <KCalendarSystem>
#include <KMessageBox>

StatsConfig::StatsConfig( const InterfaceSettings * settings, const KCalendarSystem *calendar, const StatsRule &rule, bool addRule ) : KDialog(),
    mSettings( settings ),
    mCal( calendar ),
    mAddRule( addRule )
{
    // Do this for the sake of KDateEdit
    KGlobal::locale()->setCalendar( mCal->calendarType() );

    mDlg.setupUi( mainWidget() );
    setButtons( KDialog::Default | KDialog::Ok | KDialog::Cancel );

    mDlg.periodUnits->addItem( i18n( "Days" ), KNemoStats::Day );
    mDlg.periodUnits->addItem( i18n( "Weeks" ), KNemoStats::Week );
    mDlg.periodUnits->addItem( i18n( "Months" ), KNemoStats::Month );
    //mDlg.periodUnits->addItem( i18n( "Years" ), KNemoStats::Year );

    connect( this, SIGNAL( defaultClicked() ), SLOT( setDefaults() ) );

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
}

void StatsConfig::setDefaults()
{
    StatsRule s;
    mDlg.startDate->setDate( QDate::currentDate().addDays( 1 - mCal->day( QDate::currentDate() ) ) );
    setControls( s );
}

StatsRule StatsConfig::getSettings()
{
    StatsRule rule;
    rule.startDate = mDlg.startDate->date();
    rule.periodUnits = mDlg.periodUnits->itemData( mDlg.periodUnits->currentIndex() ).toInt();
    rule.periodCount = mDlg.periodCount->value();
    return rule;
}

void StatsConfig::slotButtonClicked( int button )
{
    if ( mAddRule && ( (button == Ok) || (button == Apply) ) )
    {
        bool duplicateEntry = false;
        StatsRule testRule = getSettings();
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
            KMessageBox::sorry( 0,
                                i18n( "Another rule already starts on %1. "
                                      "Please choose another date.",
                                      mCal->formatDate( mDlg.startDate->date(), KLocale::LongDate ) )
                              );
        else
            KDialog::slotButtonClicked( button );
    }
    else
        KDialog::slotButtonClicked( button );
}

#include "statsconfig.moc"
