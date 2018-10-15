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
#include "budjettivertailu.h"
#include "db/kirjanpito.h"
#include "db/tilikausimodel.h"

Budjettivertailu::Budjettivertailu() :
    Raportti(nullptr)
{
    ui_ = new Ui::Budjettivertailu;
    ui_->setupUi(raporttiWidget);

    ui_->kausiCombo->setModel(kp()->tilikaudet());
    ui_->kausiCombo->setModelColumn(TilikausiModel::KAUSI);
    ui_->kausiCombo->setCurrentIndex( kp()->tilikaudet()->indeksiPaivalle( kp()->paivamaara() ) );


    // Lisätään muokattavat raportit
    QStringList raporttilista;

    for (QString rnimi : kp()->asetukset()->avaimet("Raportti/") )
    {
        // Kohdennusraportit kuitenkin vain, jos kohdennuksia käytettävissä
        if( kp()->asetukset()->asetus(rnimi).startsWith(":kohdennus") && !kp()->kohdennukset()->kohdennuksia() )
            continue;
        // Ei tasetita
        if( kp()->asetukset()->asetus(rnimi).startsWith(":tase") )
            continue;

        // Raporttilajit: Jos lajillinen raportti (esim. Tase/PMA, tulee listalle kuitenkin vain Tase yhteen kertaan
        if( rnimi.count(QChar('/')) > 1)
            rnimi = rnimi.left( rnimi.lastIndexOf(QChar('/')) );

        if( !raporttilista.contains(rnimi))
            raporttilista.append(rnimi);
    }
    raporttilista.sort(Qt::CaseInsensitive);
    for( const QString& nimi : raporttilista)
    {
        ui_->raporttiCombo->addItem( QIcon(":/pic/tekstisivu.png"),nimi.mid(9), nimi );
    }

}

Budjettivertailu::~Budjettivertailu()
{
    delete ui_;
}

RaportinKirjoittaja Budjettivertailu::raportti()
{
    RaportinKirjoittaja rk;
    return rk;
}
