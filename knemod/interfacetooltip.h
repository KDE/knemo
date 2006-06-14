/* This file is part of KNemo
   Copyright (C) 2004 Percy Leonhardt <percy@eris23.de>

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

#ifndef INTERFACETOOLTIP_H
#define INTERFACETOOLTIP_H

#include <qpair.h>
#include <qtooltip.h>

class Interface;

/**
 * This call is responsible for displaying the tooltip of an
 * interface icon. It is derived from QToolTip and overwrites
 * the function maybeTip(const QPoint & p) to update the tooltip
 * text just before it is going to be displayed.
 *
 * @short Display tooltip according to user settings
 * @author Percy Leonhardt <percy@eris23.de>
 */

class InterfaceToolTip : public QToolTip
{
public:
    /**
     * Default Constructor
     */
    InterfaceToolTip( Interface* interface, QWidget* parent = 0L );

    /**
     * Default Destructor
     */
    virtual ~InterfaceToolTip();

    /**
     * From the Qt documentation:
     * It is called when there is a possibility that a tool tip should
     * be shown and must decide whether there is a tool tip for the point p
     * in the widget that this QToolTip object relates to. If so, maybeTip()
     * must call tip() with the rectangle the tip applies to, the tip's text
     * and optionally the QToolTipGroup details and the geometry in screen
     * coordinates.
     */
    void maybeTip( const QPoint& );

private:
    void setupText( QString& text );
    void setupToolTipArray();

    // the interface this tooltip belongs to
    Interface* mInterface;
    // the tooltip text for each information
    QPair<QString, int> mToolTips[23];
};

#endif // INTERFACETOOLTIP_H
