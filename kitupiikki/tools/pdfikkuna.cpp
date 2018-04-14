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

#include "pdfikkuna.h"
#include "kirjaus/naytaliitewg.h"

#include <QSqlQuery>
#include <QMessageBox>

PdfIkkuna::PdfIkkuna(QWidget *parent) : QMainWindow(parent)
{

}

void PdfIkkuna::naytaPdf(const QByteArray &pdfdata)
{
    PdfIkkuna *ikkuna = new PdfIkkuna;

    NaytaliiteWg *wg = new NaytaliiteWg;
    ikkuna->setCentralWidget(wg);

    wg->naytaPdf(pdfdata);
}

void PdfIkkuna::naytaLiite(const int tositeId, const int liiteId)
{
    QSqlQuery kysely( QString("SELECT data FROM liite WHERE tosite=%1 AND liiteno=%2")
                      .arg(tositeId).arg(liiteId));
    if( kysely.next() )
    {
        QByteArray data = kysely.value("data").toByteArray();
        naytaPdf(data);
    }
    else
    {
        QMessageBox::critical(0, tr("Virhe liitteen näyttämisessä"),
                              tr("Liitettä %1-%2 ei löydy").arg(tositeId).arg(liiteId));
    }
}
