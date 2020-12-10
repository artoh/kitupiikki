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
#include "tositeluettelo.h"
#include "db/kirjanpito.h"
#include "db/tositetyyppimodel.h"

TositeLuettelo::TositeLuettelo(QObject *parent, QString kieli)
    : Raportteri (parent, kieli)
{

}

void TositeLuettelo::kirjoita(const QDate &mista, const QDate &mihin, int optiot)
{
    KpKysely *kysely = kpk("/tositteet");
    kysely->lisaaAttribuutti("alkupvm", mista);
    kysely->lisaaAttribuutti("loppupvm", mihin);

    rk.asetaOtsikko(kaanna("TOSITELUETTELO"));
    rk.asetaKausiteksti(QString("%1 - %2").arg( mista.toString("dd.MM.yyyy") )
                                             .arg( mihin.toString("dd.MM.yyyy") ) );

    rk.lisaaSarake("ABC1234/99 ");
    rk.lisaaPvmSarake();
    if( optiot & AsiakasToimittaja)
        rk.lisaaVenyvaSarake(50);
    rk.lisaaVenyvaSarake();
    rk.lisaaSarake("XXX kpl ");
    rk.lisaaEurosarake();

    {
        RaporttiRivi otsikko;
        otsikko.lisaa(kaanna("Tosite"));
        otsikko.lisaa(kaanna("Pvm"));
        if( optiot & AsiakasToimittaja)
            otsikko.lisaa(kaanna("Asiakas/Toimittaja"));
        otsikko.lisaa(kaanna("Otsikko"));
        otsikko.lisaa(kaanna("Liitteitä"));
        otsikko.lisaa(kaanna("Summa €"),1,true);
        rk.lisaaOtsake(otsikko);
    }


    if( optiot & RyhmitteleLajeittain) {
        kysely->lisaaAttribuutti("jarjestys", optiot & TositeJarjestyksessa ? "tyyppi,tosite" : "tyyppi,pvm");
    } else {
        kysely->lisaaAttribuutti("jarjestys", optiot & TositeJarjestyksessa ? "tosite" : "pvm");
    }
    optiot_ = optiot;
    if( kp()->tilikausiPaivalle(mista).alkaa() == kp()->tilikausiPaivalle(mihin).alkaa())
        optiot_ |= SamaTilikausi;

    connect( kysely, &KpKysely::vastaus, this, &TositeLuettelo::dataSaapuu);
    kysely->kysy();
}

void TositeLuettelo::dataSaapuu(QVariant *data)
{
    qlonglong lajisumma = 0l;
    qlonglong summa = 0l;
    int edellinentyyppi = -1;
    QDate edpvm;

    QVariantList lista = data->toList();
    for( auto item: lista) {
        QVariantMap map = item.toMap();
        int tamatyyppi = map.value("tyyppi").toInt();

        if( optiot_ & RyhmitteleLajeittain && edellinentyyppi != tamatyyppi) {
            if( optiot_ & TulostaSummat && edellinentyyppi >= 0) {
                RaporttiRivi valisumma(RaporttiRivi::EICSV);
                valisumma.lisaa( kaanna("Yhteensä"), 5);
                valisumma.lisaa( lajisumma );
                valisumma.viivaYlle();
                rk.lisaaRivi(valisumma);
                lajisumma = 0l;
            }
            if( edellinentyyppi >= 0)
                rk.lisaaTyhjaRivi();

            RaporttiRivi laji(RaporttiRivi::EICSV);
            laji.lisaa( kaanna(kp()->tositeTyypit()->nimi(tamatyyppi)), 5 );
            laji.lihavoi();
            rk.lisaaRivi(laji);
            edellinentyyppi = tamatyyppi;
        }

        RaporttiRivi rivi;

        if( (optiot_ & ErittelePaivat) && edpvm.isValid() &&
                map.value("pvm").toDate() != edpvm)
            rk.lisaaTyhjaRivi();
        edpvm = map.value("pvm").toDate();

        rivi.lisaaTositeTunnus( map.value("pvm").toDate(), map.value("sarja").toString(),
                                map.value("tunniste").toInt(), optiot_ & SamaTilikausi);

        rivi.lisaa( map.value("pvm").toDate() );
        if( optiot_ & AsiakasToimittaja)
            rivi.lisaa(map.value("kumppani").toString());
        rivi.lisaa( map.value("otsikko").toString());        
        if( map.value("liitteita").toInt())
            rivi.lisaa( tr("%1 kpl").arg( map.value("liitteita").toInt() ));
        else
            rivi.lisaa("");
        qlonglong euro = qRound64( map.value("summa").toDouble() * 100.0 );
        rivi.lisaa( euro );

        lajisumma += euro;
        summa += euro;

        rk.lisaaRivi( rivi );

    }
    if( optiot_ & TulostaSummat && edellinentyyppi) {
        RaporttiRivi valisumma(RaporttiRivi::EICSV);
        valisumma.lisaa(kaanna("Yhteensä"), 5);
        valisumma.lisaa( lajisumma );
        valisumma.viivaYlle();
        rk.lisaaRivi(valisumma);
        lajisumma = 0l;
    }
    if( optiot_ & TulostaSummat ) {
        rk.lisaaTyhjaRivi();
        RaporttiRivi summarivi(RaporttiRivi::EICSV);
        summarivi.lisaa(kaanna("Yhteensä"), 5);
        summarivi.lisaa( summa );
        summarivi.viivaYlle();
        summarivi.lihavoi();
        rk.lisaaRivi(summarivi);
        lajisumma = 0l;
    }

    emit valmis(rk);
}
