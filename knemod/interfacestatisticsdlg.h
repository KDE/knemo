/****************************************************************************
** Form interface generated from reading ui file '/home/percy/src/network/knemo/knemod/interfacestatisticsdlg.ui'
**
** Created: Tue Jan 24 23:18:51 2006
**      by: The User Interface Compiler ($Id: qt/main.cpp   3.3.5   edited Aug 31 12:13 $)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#ifndef INTERFACESTATISTICSDLG_H
#define INTERFACESTATISTICSDLG_H

#include <qvariant.h>
#include <qdialog.h>

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QSpacerItem;
class QPushButton;
class QTabWidget;
class QWidget;
class QTable;

class InterfaceStatisticsDlg : public QDialog
{
    Q_OBJECT

public:
    InterfaceStatisticsDlg( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
    ~InterfaceStatisticsDlg();

    QPushButton* buttonClose;
    QTabWidget* tabWidget;
    QWidget* daily;
    QTable* tableDaily;
    QPushButton* buttonClearDaily;
    QWidget* monthy;
    QTable* tableMonthly;
    QPushButton* buttonClearMonthly;
    QWidget* yearly;
    QTable* tableYearly;
    QPushButton* buttonClearYearly;

protected:
    QGridLayout* InterfaceStatisticsDlgLayout;
    QSpacerItem* spacer28;
    QSpacerItem* spacer29;
    QVBoxLayout* dailyLayout;
    QVBoxLayout* monthyLayout;
    QVBoxLayout* yearlyLayout;

protected slots:
    virtual void languageChange();

};

#endif // INTERFACESTATISTICSDLG_H
