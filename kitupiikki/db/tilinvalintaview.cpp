/*
   Copyright (C) 2018 Arto Hyvättinen

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/


#include <QMouseEvent>
#include <QModelIndex>

#include <QPoint>
#include <QCursor>

#include "tilinvalintaview.h"
#include "tilimodel.h"


TilinValintaView::TilinValintaView(QWidget *parent) :
    QTableView(parent), ilmoitettuTili(-1)
{
    setMouseTracking(true);
}

void TilinValintaView::paivitaInfo()
{
    QPoint pos = viewport()->mapFromGlobal( QCursor::pos());

    QModelIndex index = indexAt(  pos  );


    if( model()->rowCount(QModelIndex()) == 1)
        index = model()->index(0,0);

    if( index.isValid())
    {
        int indeksi = index.data( TiliModel::IdRooli ).toInt();
        if( indeksi != ilmoitettuTili)
        {
            ilmoitettuTili = indeksi;
            emit tiliHiirenAlla(ilmoitettuTili);
        }
    }

}

void TilinValintaView::mouseMoveEvent(QMouseEvent * /* event */)
{
    paivitaInfo();
}
