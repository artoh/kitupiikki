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
#include "raporttiscene.h"
#include <QPainter>


RaporttiScene::RaporttiScene(QObject *parent) :
    PdfScene (parent)
{

}

RaporttiScene::RaporttiScene(RaportinKirjoittaja raportti, QObject *parent) :
    RaporttiScene( parent )
{
    nayta( raportti );
}

void RaporttiScene::nayta(RaportinKirjoittaja raportti)
{
    raportti_ = raportti;
//    naytaPdf( raportti_.pdf(raidat_) );
}

QString RaporttiScene::otsikko() const
{
    return raportti_.otsikko();
}

QByteArray RaporttiScene::csv()
{
    return  raportti_.csv();
}

QString RaporttiScene::html()
{
    return raportti_.html();
}

bool RaporttiScene::sivunAsetuksetMuuttuneet()
{
//    naytaPdf( raportti_.pdf(raidat_) );
    return true;
}

bool RaporttiScene::raidoita(bool raidat)
{
    raidat_ = raidat;
//    naytaPdf( raportti_.pdf(raidat_));
    return true;
}

void RaporttiScene::tulosta(QPrinter *printer)
{
    QPainter painter(printer);
    raportti_.tulosta( printer, &painter, raidat_);
    painter.end();
}

bool RaporttiScene::csvMuoto()
{
    return raportti_.csvKaytossa();
}
