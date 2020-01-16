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
#include "laskuraportteri.h"

#include "db/kirjanpito.h"

LaskuRaportteri::LaskuRaportteri(QObject *parent, const QString kielikoodi)
    : Raportteri(parent, kielikoodi)
{

}

void LaskuRaportteri::kirjoita(int optiot, const QDate &saldopvm, const QDate &mista, const QDate &mihin)
{
    optiot_ = optiot;
    saldopvm_ = saldopvm;

    KpKysely *kysely = optiot & Myyntilaskut ? kpk("/myyntilaskut") : kpk("/ostolaskut");
    if( optiot & RajaaLaskupaivalla) {
        kysely->lisaaAttribuutti("alkupvm", mista);
        kysely->lisaaAttribuutti("loppupvm", mihin);
    } else if( optiot & RajaaErapaivalla) {
        kysely->lisaaAttribuutti("eraalkupvm", mista);
        kysely->lisaaAttribuutti("eraloppupvm", mihin);
    }
    if( optiot & VainAvoimet)
        kysely->lisaaAttribuutti("avoin", QString());
    if( optiot & VainKitsas )
        kysely->lisaaAttribuutti("kitsaslaskut", QString());

    kysely->lisaaAttribuutti("saldopvm", saldopvm);

    connect( kysely, &KpKysely::vastaus, this, &LaskuRaportteri::dataSaapuu);
    kysely->kysy();

}

bool LaskuRaportteri::lajitteluVertailu(const QVariant &eka, const QVariant &toka)
{
    const QVariantMap& ekaMap = eka.toMap();
    const QVariantMap& tokaMap = toka.toMap();

    if( optiot_ & LajitteleNumero)
        return ekaMap.value("numero").toInt() < tokaMap.value("numero").toInt();
    else if( optiot_ & LajitteleViite)
        return ekaMap.value("viite").toString() < tokaMap.value("viite").toString();
    else if( optiot_ & LajitteleErapvm)
        return ekaMap.value("erapvm").toDate() < tokaMap.value("erapvm").toDate();
    else if( optiot_ & LajitteleAsiakas && optiot_ & Myyntilaskut)
        return ekaMap.value("asiakas").toString() < tokaMap.value("asiakas").toString();
    else if( optiot_ & LajitteleAsiakas && optiot_ & Ostolaskut)
        return ekaMap.value("toimittaja").toString() < tokaMap.value("toimittaja").toString();
    else
        return ekaMap.value("pvm").toDate() < tokaMap.value("pvm").toDate();
}

void LaskuRaportteri::dataSaapuu(QVariant *data)
{
    QVariantList list = data->toList();

    if( !(optiot_ & LajittelePvm) )
        std::sort(list.begin(), list.end(), [this] (const QVariant& eka, const QVariant& toka)
            { return this->lajitteluVertailu(eka, toka);});


    rk.asetaOtsikko( optiot_ & Myyntilaskut ? kaanna("MYYNTILASKUT") : kaanna("OSTOLASKUT") );
    rk.asetaKausiteksti( saldopvm_.toString("dd.MM.yyyy"));

    rk.lisaaSarake("XXXXXXXXXXXXXXX");  // Numero / Viite
    rk.lisaaPvmSarake();                // Laskupvm
    rk.lisaaPvmSarake();                // Eräpäivä
    rk.lisaaEurosarake();               // Summa
    rk.lisaaEurosarake();               // Avoinna
    rk.lisaaVenyvaSarake();             // Asiakas

    RaporttiRivi otsikko;
    otsikko.lisaa( optiot_ & NaytaViite ? kaanna("Viite") : kaanna("Numero") );
    otsikko.lisaa( kaanna("Laskupvm"));
    otsikko.lisaa( kaanna("Eräpvm"));
    otsikko.lisaa( kaanna("Summa"));
    otsikko.lisaa( kaanna("Maksamatta"));
    otsikko.lisaa( optiot_ & Myyntilaskut ? kaanna("Asiakas/Selite") : kaanna("Toimittaja / Selite"));
    rk.lisaaOtsake(otsikko);

    qlonglong kokosumma = 0;
    qlonglong avoinsumma = 0;

    for(auto item : list) {
        QVariantMap map = item.toMap();
        RaporttiRivi rivi;

        rivi.lisaa( optiot_ & NaytaViite ? map.value("viite").toString() : map.value("numero").toString());
        rivi.lisaa( map.value("pvm").toDate());
        rivi.lisaa( map.value("erapvm").toDate());

        qlonglong summa = qRound64( map.value("summa").toDouble() * 100.0);
        rivi.lisaa( summa );
        kokosumma += summa;

        qlonglong avoin = qRound64( map.value("avoin").toDouble() * 100.0);
        rivi.lisaa( avoin );
        avoinsumma += avoin;

        QString asiakastoimittaja = optiot_ & Myyntilaskut ? map.value("asiakas").toString() : map.value("toimittaja").toString();
        rivi.lisaa( asiakastoimittaja.isEmpty() ? map.value("selite").toString() : asiakastoimittaja);
        rk.lisaaRivi(rivi);
    }

    if( optiot_ & TulostaSummat) {
        RaporttiRivi summarivi;
        summarivi.lisaa(kaanna("Yhteensä"), 3);
        summarivi.lisaa(kokosumma);
        summarivi.lisaa(avoinsumma);
        summarivi.lisaa("");
        summarivi.viivaYlle();
        rk.lisaaRivi(summarivi);
    }

    emit valmis(rk);
}
