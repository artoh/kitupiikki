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
#include "paivakirja.h"

#include "db/kirjanpito.h"

Paivakirja::Paivakirja(QObject *parent) : QObject(parent)
{

}

void Paivakirja::kirjoita(QDate mista, QDate mihin)
{
    kirjoittaja.asetaOtsikko(tr("PÄIVÄKIRJA"));


    kirjoittaja.asetaKausiteksti(QString("%1 - %2").arg( mista.toString("dd.MM.yyyy") )
                                             .arg( mihin.toString("dd.MM.yyyy") ) );

    kirjoittaja.lisaaPvmSarake();
    kirjoittaja.lisaaSarake("ABC1234/99 ");
    kirjoittaja.lisaaSarake("999999 Tilinimi tarkeinteilla");
    kirjoittaja.lisaaVenyvaSarake();
    kirjoittaja.lisaaEurosarake();
    kirjoittaja.lisaaEurosarake();

    {
        RaporttiRivi otsikko(RaporttiRivi::EICSV);
        otsikko.lisaa("Pvm");
        otsikko.lisaa("Tosite");
        otsikko.lisaa("Tili");
        otsikko.lisaa("Selite");
        otsikko.lisaa("Debet €", 1, true);
        otsikko.lisaa("Kredit €", 1, true);
        kirjoittaja.lisaaOtsake(otsikko);
    }

    KpKysely *kysely = kpk("/viennit");
    kysely->lisaaAttribuutti("alkupvm", mista);
    kysely->lisaaAttribuutti("loppupvm", mihin);

    connect( kysely, &KpKysely::vastaus, this, &Paivakirja::dataSaapuu);

    kysely->kysy();
}

void Paivakirja::dataSaapuu(QVariant *data)
{
    QVariantList lista = data->toList();


    for(auto item : lista) {
        QVariantMap map = item.toMap();

        RaporttiRivi rivi;

        rivi.lisaa( map.value("pvm").toDate() );
        rivi.lisaa( kp()->tositeTunnus(  map.value("tosite").toMap().value("tositelaji").toInt(),
                                         map.value("tosite").toMap().value("tunniste").toInt() ,
                                         map.value("tosite").toMap().value("pvm").toDate() ) );
        Tili* tili = kp()->tilit()->tiliPNumerolla( map.value("tili").toInt() );
        if( tili )
            rivi.lisaa( QString("%1 %2").arg( tili->numero()).arg(tili->nimi()) );
        else
            continue;   // ei kelvollista tiliä!

        rivi.lisaa( map.value("selite").toString() );

        qlonglong debetsnt = qRound( map.value("debet").toDouble() * 100 );
        qlonglong kreditsnt = qRound( map.value("kredit").toDouble() * 100);

        rivi.lisaa( debetsnt );
        rivi.lisaa( kreditsnt );

        kirjoittaja.lisaaRivi(rivi);

    }
    emit valmis( kirjoittaja );
}



