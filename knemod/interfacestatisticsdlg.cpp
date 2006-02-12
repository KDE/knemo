#include <kdialog.h>
#include <klocale.h>
/****************************************************************************
** Form implementation generated from reading ui file '/home/percy/src/network/knemo/knemod/interfacestatisticsdlg.ui'
**
** Created: Tue Jan 24 23:18:51 2006
**      by: The User Interface Compiler ($Id: qt/main.cpp   3.3.5   edited Aug 31 12:13 $)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#include "interfacestatisticsdlg.h"

#include <qvariant.h>
#include <qpushbutton.h>
#include <qtabwidget.h>
#include <qwidget.h>
#include <qtable.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

/*
 *  Constructs a InterfaceStatisticsDlg as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
InterfaceStatisticsDlg::InterfaceStatisticsDlg( QWidget* parent, const char* name, bool modal, WFlags fl )
    : QDialog( parent, name, modal, fl )
{
    if ( !name )
	setName( "InterfaceStatisticsDlg" );
    InterfaceStatisticsDlgLayout = new QGridLayout( this, 1, 1, 11, 6, "InterfaceStatisticsDlgLayout"); 

    buttonClose = new QPushButton( this, "buttonClose" );
    buttonClose->setDefault( TRUE );

    InterfaceStatisticsDlgLayout->addWidget( buttonClose, 1, 1 );

    tabWidget = new QTabWidget( this, "tabWidget" );

    daily = new QWidget( tabWidget, "daily" );
    dailyLayout = new QVBoxLayout( daily, 11, 6, "dailyLayout"); 

    tableDaily = new QTable( daily, "tableDaily" );
    tableDaily->setNumCols( tableDaily->numCols() + 1 );
    tableDaily->horizontalHeader()->setLabel( tableDaily->numCols() - 1, tr2i18n( "Sent" ) );
    tableDaily->setNumCols( tableDaily->numCols() + 1 );
    tableDaily->horizontalHeader()->setLabel( tableDaily->numCols() - 1, tr2i18n( "Received" ) );
    tableDaily->setNumCols( tableDaily->numCols() + 1 );
    tableDaily->horizontalHeader()->setLabel( tableDaily->numCols() - 1, tr2i18n( "Total" ) );
    tableDaily->setNumRows( 1 );
    tableDaily->setNumCols( 3 );
    tableDaily->setReadOnly( TRUE );
    dailyLayout->addWidget( tableDaily );

    buttonClearDaily = new QPushButton( daily, "buttonClearDaily" );
    dailyLayout->addWidget( buttonClearDaily );
    tabWidget->insertTab( daily, QString::fromLatin1("") );

    monthy = new QWidget( tabWidget, "monthy" );
    monthyLayout = new QVBoxLayout( monthy, 11, 6, "monthyLayout"); 

    tableMonthly = new QTable( monthy, "tableMonthly" );
    tableMonthly->setNumCols( tableMonthly->numCols() + 1 );
    tableMonthly->horizontalHeader()->setLabel( tableMonthly->numCols() - 1, tr2i18n( "Sent" ) );
    tableMonthly->setNumCols( tableMonthly->numCols() + 1 );
    tableMonthly->horizontalHeader()->setLabel( tableMonthly->numCols() - 1, tr2i18n( "Received" ) );
    tableMonthly->setNumCols( tableMonthly->numCols() + 1 );
    tableMonthly->horizontalHeader()->setLabel( tableMonthly->numCols() - 1, tr2i18n( "Total" ) );
    tableMonthly->setNumRows( 1 );
    tableMonthly->setNumCols( 3 );
    monthyLayout->addWidget( tableMonthly );

    buttonClearMonthly = new QPushButton( monthy, "buttonClearMonthly" );
    monthyLayout->addWidget( buttonClearMonthly );
    tabWidget->insertTab( monthy, QString::fromLatin1("") );

    yearly = new QWidget( tabWidget, "yearly" );
    yearlyLayout = new QVBoxLayout( yearly, 11, 6, "yearlyLayout"); 

    tableYearly = new QTable( yearly, "tableYearly" );
    tableYearly->setNumCols( tableYearly->numCols() + 1 );
    tableYearly->horizontalHeader()->setLabel( tableYearly->numCols() - 1, tr2i18n( "Sent" ) );
    tableYearly->setNumCols( tableYearly->numCols() + 1 );
    tableYearly->horizontalHeader()->setLabel( tableYearly->numCols() - 1, tr2i18n( "Received" ) );
    tableYearly->setNumCols( tableYearly->numCols() + 1 );
    tableYearly->horizontalHeader()->setLabel( tableYearly->numCols() - 1, tr2i18n( "Total" ) );
    tableYearly->setNumRows( 1 );
    tableYearly->setNumCols( 3 );
    tableYearly->setReadOnly( TRUE );
    yearlyLayout->addWidget( tableYearly );

    buttonClearYearly = new QPushButton( yearly, "buttonClearYearly" );
    yearlyLayout->addWidget( buttonClearYearly );
    tabWidget->insertTab( yearly, QString::fromLatin1("") );

    InterfaceStatisticsDlgLayout->addMultiCellWidget( tabWidget, 0, 0, 0, 2 );
    spacer28 = new QSpacerItem( 211, 31, QSizePolicy::Expanding, QSizePolicy::Minimum );
    InterfaceStatisticsDlgLayout->addItem( spacer28, 1, 0 );
    spacer29 = new QSpacerItem( 201, 31, QSizePolicy::Expanding, QSizePolicy::Minimum );
    InterfaceStatisticsDlgLayout->addItem( spacer29, 1, 2 );
    languageChange();
    resize( QSize(490, 502).expandedTo(minimumSizeHint()) );
    clearWState( WState_Polished );

    // signals and slots connections
    connect( buttonClose, SIGNAL( clicked() ), this, SLOT( close() ) );
}

/*
 *  Destroys the object and frees any allocated resources
 */
InterfaceStatisticsDlg::~InterfaceStatisticsDlg()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void InterfaceStatisticsDlg::languageChange()
{
    setCaption( tr2i18n( "Statistics" ) );
    buttonClose->setText( tr2i18n( "&Close" ) );
    buttonClose->setAccel( QKeySequence( tr2i18n( "Alt+C" ) ) );
    tableDaily->horizontalHeader()->setLabel( 0, tr2i18n( "Sent" ) );
    tableDaily->horizontalHeader()->setLabel( 1, tr2i18n( "Received" ) );
    tableDaily->horizontalHeader()->setLabel( 2, tr2i18n( "Total" ) );
    buttonClearDaily->setText( tr2i18n( "Clear daily statistics" ) );
    tabWidget->changeTab( daily, tr2i18n( "Daily" ) );
    tableMonthly->horizontalHeader()->setLabel( 0, tr2i18n( "Sent" ) );
    tableMonthly->horizontalHeader()->setLabel( 1, tr2i18n( "Received" ) );
    tableMonthly->horizontalHeader()->setLabel( 2, tr2i18n( "Total" ) );
    buttonClearMonthly->setText( tr2i18n( "Clear monthly statistics" ) );
    tabWidget->changeTab( monthy, tr2i18n( "Monthly" ) );
    tableYearly->horizontalHeader()->setLabel( 0, tr2i18n( "Sent" ) );
    tableYearly->horizontalHeader()->setLabel( 1, tr2i18n( "Received" ) );
    tableYearly->horizontalHeader()->setLabel( 2, tr2i18n( "Total" ) );
    buttonClearYearly->setText( tr2i18n( "Clear yearly statistics" ) );
    tabWidget->changeTab( yearly, tr2i18n( "Yearly" ) );
}

#include "interfacestatisticsdlg.moc"
