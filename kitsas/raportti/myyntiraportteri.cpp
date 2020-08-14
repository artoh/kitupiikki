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
#include "myyntiraportteri.h"
#include "db/kirjanpito.h"

MyyntiRaportteri::MyyntiRaportteri(QObject *parent, const QString kielikoodi)
    : Raportteri(parent, kielikoodi)
{

}

void MyyntiRaportteri::kirjoita(const QDate &mista, const QDate &mihin)
{
    rk.asetaOtsikko(kaanna("MYYNTI"));
    rk.asetaKausiteksti(QString("%1 - %2")
                        .arg(mista.toString("dd.MM.yyyy"))
                        .arg(mihin.toString("dd.MM.yyyy")));

    rk.lisaaVenyvaSarake();
    rk.lisaaEurosarake();
    rk.lisaaEurosarake();
    rk.lisaaEurosarake();

    RaporttiRivi otsikko;
    otsikko.lisaa(kaanna("Tuote"));
    otsikko.lisaa(kaanna("myynti kpl"),1,true);
    otsikko.lisaa(kaanna("keskim. á netto"),1,true);
    otsikko.lisaa(kaanna("netto yht"),1,true);
    rk.lisaaOtsake(otsikko);

    KpKysely *kysely = kpk("/tuotteet/myynti");
    kysely->lisaaAttribuutti("alkupvm", mista);
    kysely->lisaaAttribuutti("loppupvm", mihin);

    connect( kysely, &KpKysely::vastaus, this, &MyyntiRaportteri::dataSaapuu);
    kysely->kysy();

}

void MyyntiRaportteri::dataSaapuu(QVariant *data)
{
    QVariantList lista = data->toList();
    QVariantMap epatuotteet;
    double yhteensa = 0;

    for(const auto& item : lista) {
        const QVariantMap& map = item.toMap();
        if( !map.contains("tuote")  || map.value("tuote").toInt() == 0) {
            epatuotteet = map;
            continue;
        }
        RaporttiRivi rivi;
        rivi.lisaa( map.value("nimike").toString() );
        double kpl = map.value("kpl").toDouble();
        rivi.lisaa( kpl );
        double myynti = map.value("myynti").toDouble();
        if( qAbs(kpl) > 1e-5)
            rivi.lisaa( myynti / kpl );
        else
            rivi.lisaa("");
        rivi.lisaa(myynti);
        yhteensa += myynti;

        rk.lisaaRivi(rivi);
    }

    if(!epatuotteet.isEmpty()) {
        RaporttiRivi lisarivi(RaporttiRivi::EICSV);
        lisarivi.lisaa(kaanna("Muu myynti"),3);
        lisarivi.lisaa(epatuotteet.value("myynti").toDouble());
        yhteensa += epatuotteet.value("myynti").toDouble();
        rk.lisaaRivi(lisarivi);
    }

    RaporttiRivi summarivi(RaporttiRivi::EICSV);
    summarivi.lisaa(kaanna("Yhteensä"),3);
    summarivi.lisaa(yhteensa);
    summarivi.viivaYlle();
    rk.lisaaRivi(summarivi);


    emit valmis(rk);
}
