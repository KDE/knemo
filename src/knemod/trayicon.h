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
#ifndef TRAYICON_H
#define TRAYICON_H

#include "config-knemo.h"

#include <KStatusNotifierItem>

class Interface;

class TrayIcon : public KStatusNotifierItem
{
    Q_OBJECT
public:
    /**
     * Default Constructor
     */
    TrayIcon( Interface* interface, const QString &id, QWidget* parent = 0 );

    /**
     * Default Destructor
     */
    virtual ~TrayIcon();

    void configChanged();
    void processUpdate();

public Q_SLOTS:
    void activate(const QPoint &pos);

private:
    bool conStatusChanged();
    QList<qreal> barLevels( QList<unsigned long>& rxHist, QList<unsigned long>& txHist );
    void updateBarIcon( bool force = false );
    QString compactTrayText( unsigned long );
    void updateTextIcon( bool force = false );
    void updateImgIcon( int status );
    QString toolTipData();
    QString formatTip( const QString& field, const QString& data, bool insertBreak );


    Interface* mInterface;
    QMap<int, QString> mScope;

    QAction* statusAction;
    QAction* plotterAction;
    QAction* statisticsAction;
    QAction* configAction;
    QString m_rxText;
    QString m_txText;
    int m_rxPrevBarHeight;
    int m_txPrevBarHeight;
    int histSize;
    QList<unsigned long>m_rxHist;
    QList<unsigned long>m_txHist;
    unsigned int m_maxRate;

private Q_SLOTS:
    void togglePlotter();
    void slotQuit();
    /*
     * Called when the user selects 'Configure KNemo' from the context menu
     */
    void showConfigDialog();
};

#endif // TRAYICON_H
