/* This file is part of KNemo
   Copyright (C) 2004, 2005 Percy Leonhardt <percy@eris23.de>
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
#ifndef INTERFACETRAY_H
#define INTERFACETRAY_H

#include <KSystemTrayIcon>

#include "interface.h"

class InterfaceTray : public KSystemTrayIcon
{
    Q_OBJECT
public:
    /**
     * Default Constructor
     */
    InterfaceTray( Interface* interface, const QString& icon, QWidget* parent = 0 );

    /**
     * Default Destructor
     */
    virtual ~InterfaceTray();

    void updateToolTip();

protected:
    bool event( QEvent * );

private:
    Interface* mInterface;
    QMap<quint32, QString> mToolTips;

    QString toolTipData();
    void setupToolTipArray();

private Q_SLOTS:
    void slotQuit();
};

#endif // INTERFACETRAY_H
