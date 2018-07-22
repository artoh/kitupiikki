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
#include "naytinview.h"

NaytinView::NaytinView(QWidget *parent)
    : QGraphicsView(parent)
{

}


void NaytinView::nayta(const QByteArray &data)
{
    if( data.startsWith("%PDF"))
    {
        vaihdaScene( new PdfScene(data, this) );
        emit sisaltoVaihtunut("pdf");
    }

}

void NaytinView::vaihdaScene(NaytinScene *uusi)
{
    if( scene_)
        scene_->deleteLater();

    scene_ = uusi;
    setScene(uusi);
    scene_->piirraLeveyteen( width() - 20.0 );
}

void NaytinView::resizeEvent(QResizeEvent * /*event*/)
{
    if( scene_)
        scene_->piirraLeveyteen( width() - 20.0);
}
