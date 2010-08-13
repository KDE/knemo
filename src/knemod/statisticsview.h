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

#ifndef STATISTICSVIEW_H
#define STATISTICSVIEW_H

#include <QTableView>

class StatisticsView : public QTableView
{
Q_OBJECT
    public:
        StatisticsView( QWidget * parent = 0 );
        virtual ~StatisticsView();
        void setModel( QAbstractItemModel * );
        void haveOffpeak( bool op ) { mOffpeak = op; }

    private:
        bool mFollow;
        bool mOffpeak;
        QString peakString;
        QString offpeakString;
        QString totalString;

    protected:
        virtual void hideEvent( QHideEvent * );
        virtual void keyPressEvent( QKeyEvent * e );
        virtual void mousePressEvent( QMouseEvent * );
        virtual bool viewportEvent( QEvent * );

    private slots:
        void updateSum();
        void showSum( const QPoint &p );
};

#endif
