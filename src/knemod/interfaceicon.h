/* This file is part of KNemo
   Copyright (C) 2004, 2005 Percy Leonhardt <percy@eris23.de>
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

#ifndef INTERFACEICON_H
#define INTERFACEICON_H

class Interface;
class InterfaceTray;
class KActionCollection;
class QAction;

/**
 * This is the logical representation of the systemtray icon. It handles
 * creation and deletion of the real icon, setting the tooltip, setting
 * the correct icon image and displaying of the settings dialog.
 *
 * @short Logical representation of the systemtray icon
 * @author Percy Leonhardt <percy@eris23.de>
 */

class InterfaceIcon : public QObject
{
    Q_OBJECT
public:
    /**
     * Default Constructor
     */
    InterfaceIcon( Interface* interface );

    /**
     * Default Destructor
     */
    virtual ~InterfaceIcon();

Q_SIGNALS:
    void statisticsSelected();

public Q_SLOTS:
    /*
     * Creates or deletes the tray icon
     */
    void updateTrayStatus();

    /*
     * Update tool tip text
     */
    void updateToolTip();

    /*
     * Fill the context menu with entries if the user configured
     * start and stop command
     */
    void updateMenu();

    void configChanged();

private Q_SLOTS:
    /*
     * Called when the user selects 'Configure KNemo' from the context menu
     */
    void showConfigDialog();

    /*
     * Returns a string with a compact transfer rate
     * This should not be more than 4 chars, including the units
     */
    QString compactTrayText( unsigned long );

    void showStatus();
    void showGraph();
    void showStatistics();

private:
    /*
     * Changes the icon displayed in the tray
     */
    void updateIconImage( int status );

    bool conStatusChanged();
    QList<qreal> barLevels( QList<unsigned long>& rxHist, QList<unsigned long>& txHist );
    void updateBarIcon( bool doUpdate = false );
    void updateTextIcon( bool doUpdate = false );
    // the interface this icon belongs to
    Interface* mInterface;
    // the real tray icon
    InterfaceTray* mTray;
    KActionCollection* commandActions;
    QAction* statusAction;
    QAction* plotterAction;
    QAction* statisticsAction;
    QAction* configAction;
    QString m_rxText;
    QString m_txText;
    int iconWidth;
    int histSize;
    int m_rxPrevBarHeight;
    int m_txPrevBarHeight;
    int barWidth;
    int leftMargin;
    int midMargin;
    QList<unsigned long>m_rxHist;
    QList<unsigned long>m_txHist;
    unsigned int m_maxRate;
};

#endif // INTERFACEICON_H
