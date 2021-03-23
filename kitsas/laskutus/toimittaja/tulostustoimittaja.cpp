/*
   Copyright (C) 2019 Arto Hyvättinen

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
#include "tulostustoimittaja.h"

#include <QPrintDialog>
#include <QPageLayout>
#include <QPainter>
#include <QPrinter>
#include "db/kirjanpito.h"
#include "../tulostus/laskuntulostaja.h"

TulostusToimittaja::TulostusToimittaja(QObject *parent)
    : AbstraktiToimittaja(parent)
{

}


void TulostusToimittaja::toimita()
{
    if( tulostusKaynnissa_)
        return;
    tulostusKaynnissa_ = true;

    QPageLayout vanhaleiska = kp()->printer()->pageLayout();
    QPageLayout uusileiska = vanhaleiska;
    uusileiska.setUnits(QPageLayout::Millimeter);
    uusileiska.setMargins( QMarginsF(5.0,5.0,5.0,5.0));
    kp()->printer()->setPageLayout(uusileiska);

    QPrintDialog printDialog( kp()->printer() );
    if( printDialog.exec())
    {
        QPainter painter( kp()->printer() );
        while( !vapaa()) {
            LaskunTulostaja tulostaja(kp());
            Tosite tosite;
            tosite.lataa(tositeMap());
            tulostaja.tulosta( tosite, kp()->printer(), &painter );
            if( jonossa() > 1 )
                kp()->printer()->newPage();
            merkkaaToimitetuksi();
        }
    } else {
        peru();
    }

    kp()->printer()->setPageLayout(vanhaleiska);
    tulostusKaynnissa_ = false;
}
