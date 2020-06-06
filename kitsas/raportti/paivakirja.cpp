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
#include "db/tositetyyppimodel.h"

Paivakirja::Paivakirja(QObject *parent, const QString &kielikoodi) : Raportteri(parent, kielikoodi)
{

}

void Paivakirja::kirjoita(const QDate &mista, const QDate &mihin, int optiot, int kohdennuksella)
{
    optiot_ = optiot;

    if( kp()->tilikausiPaivalle(mista).alkaa() == kp()->tilikausiPaivalle(mihin).alkaa())
        optiot_ |= SamaTilikausi;



    if( kohdennuksella > -1 ) {
        // Tulostetaan vain yhdestä kohdennuksesta
        optiot_ |= Kohdennuksella;
        rk.asetaOtsikko( QString("%1 (%2)").arg(kaanna("PÄIVÄKIRJA")).arg( kp()->kohdennukset()->kohdennus(kohdennuksella).nimi() ) );
    } else
    rk.asetaOtsikko(kaanna("PÄIVÄKIRJA"));


    rk.asetaKausiteksti(QString("%1 - %2").arg( mista.toString("dd.MM.yyyy") )
                                             .arg( mihin.toString("dd.MM.yyyy") ) );

    rk.lisaaPvmSarake();
    if( kp()->asetukset()->onko("erisarjaan") )
        rk.lisaaSarake("ABC1234/99 ");
    else
        rk.lisaaSarake("12345");

    rk.lisaaSarake("1234 Arvonlisäverosaamiset");
    if( optiot_ & TulostaKohdennukset)
        rk.lisaaSarake("Kohdennusnimi");
    if( optiot_ & AsiakasToimittaja)
        rk.lisaaVenyvaSarake(75);
    rk.lisaaVenyvaSarake();
    rk.lisaaEurosarake();
    rk.lisaaEurosarake();

    {
        RaporttiRivi otsikko;
        otsikko.lisaa(kaanna("Pvm"));
        otsikko.lisaa(kaanna("Tosite"));
        otsikko.lisaa(kaanna("Tili"));
        if( optiot_ & TulostaKohdennukset)
            otsikko.lisaa(kaanna("Kohdennus"));
        if( optiot_ & AsiakasToimittaja)
            otsikko.lisaa(kaanna("Asiakas/Toimittaja"));
        otsikko.lisaa(kaanna("Selite"));
        otsikko.lisaa(kaanna("Debet €"), 1, true);
        otsikko.lisaa(kaanna("Kredit €"), 1, true);
        rk.lisaaOtsake(otsikko);
    }

    KpKysely *kysely = kpk("/viennit");
    kysely->lisaaAttribuutti("alkupvm", mista);
    kysely->lisaaAttribuutti("loppupvm", mihin);

    if( optiot_ & TositeJarjestyksessa ) {
        if( optiot_ & RyhmitteleLajeittain)
            kysely->lisaaAttribuutti("jarjestys","laji,tosite");
        else
            kysely->lisaaAttribuutti("jarjestys","tosite");
    } else if( optiot_ & RyhmitteleLajeittain)
        kysely->lisaaAttribuutti("jarjestys","laji");

    if( kohdennuksella > -1)
        kysely->lisaaAttribuutti("kohdennus", kohdennuksella);

    connect( kysely, &KpKysely::vastaus, this, &Paivakirja::dataSaapuu);

    kysely->kysy();
}

void Paivakirja::dataSaapuu(QVariant *data)
{
    QVariantList lista = data->toList();
    QDate edpaiva;

    int edellinentyyppi = 0;

    qlonglong debetsumma = 0;
    qlonglong kreditsumma = 0;

    qlonglong debetvalisumma = 0;
    qlonglong kreditvalisumma = 0;

    for(auto item : lista) {
        QVariantMap map = item.toMap();

        int tositetyyppi = map.value("tosite").toMap().value("tyyppi").toInt();
        if( optiot_ & RyhmitteleLajeittain && edellinentyyppi != tositetyyppi ) {
            if( optiot_ & TulostaSummat && edellinentyyppi) {
                RaporttiRivi valisumma(RaporttiRivi::EICSV);
                valisumma.lisaa(kaanna("Yhteensä"), optiot_ & TulostaKohdennukset ? 5 : 4  );
                valisumma.lisaa( debetvalisumma);
                valisumma.lisaa(kreditvalisumma);
                valisumma.viivaYlle();
                rk.lisaaRivi(valisumma);

                debetvalisumma = 0l;
                kreditvalisumma = 0l;
            }

            if( edellinentyyppi)
                rk.lisaaTyhjaRivi();

            RaporttiRivi ryhma(RaporttiRivi::EICSV);
            ryhma.lisaa( kp()->tositeTyypit()->nimi(tositetyyppi),4 );
            ryhma.lihavoi();
            rk.lisaaRivi(ryhma);

            edellinentyyppi = tositetyyppi;
        }


        RaporttiRivi rivi;

        QDate pvm = map.value("pvm").toDate();

        if( (optiot_ & ErittelePaivat) && pvm != edpaiva && edpaiva.isValid())
            rk.lisaaTyhjaRivi();
        edpaiva = pvm;

        rivi.lisaa( pvm );

        // Ei toisteta turhaan tilikauden tunnusta
        QVariantMap tositemap = map.value("tosite").toMap();
        rivi.lisaaTositeTunnus( tositemap.value("pvm").toDate(), tositemap.value("sarja").toString(), tositemap.value("tunniste").toInt(), optiot_ & SamaTilikausi );

        Tili* tili = kp()->tilit()->tili( map.value("tili").toInt() );
        if( tili )
            rivi.lisaaLinkilla(RaporttiRiviSarake::TILI_NRO, tili->numero(), tili->nimiNumero());
        else
            continue;   // ei kelvollista tiliä!

        if( optiot_ & TulostaKohdennukset )
            rivi.lisaa( kp()->kohdennukset()->kohdennus( map.value("kohdennus").toInt() ).nimi() );

        QString kumppani = map.value("kumppani").toMap().value("nimi").toString();
        QString selite = map.value("selite").toString();

        if( optiot_ & AsiakasToimittaja)
            rivi.lisaa( kumppani );
        rivi.lisaa( optiot_ & AsiakasToimittaja && selite == kumppani ? "" : selite );

        qlonglong debetsnt = qRound64( map.value("debet").toDouble() * 100.0 );
        qlonglong kreditsnt = qRound64( map.value("kredit").toDouble() * 100.0 );

        debetsumma += debetsnt;
        debetvalisumma += debetsnt;
        kreditsumma += kreditsnt;
        kreditvalisumma += kreditsnt;

        rivi.lisaa( debetsnt );
        rivi.lisaa( kreditsnt );

        rk.lisaaRivi(rivi);

    }

    if( optiot_ & TulostaSummat && edellinentyyppi) {
        RaporttiRivi valisumma(RaporttiRivi::EICSV);
        valisumma.lisaa(kaanna("Yhteensä"), optiot_ & TulostaKohdennukset ? 5 : 4  );
        valisumma.lisaa( debetvalisumma);
        valisumma.lisaa(kreditvalisumma);
        valisumma.viivaYlle();
        rk.lisaaRivi(valisumma);
    }

    if( optiot_ & TulostaSummat) {
        rk.lisaaTyhjaRivi();
        RaporttiRivi summarivi(RaporttiRivi::EICSV);
        summarivi.lisaa(kaanna("Yhteensä"), optiot_ & TulostaKohdennukset ? 5 : 4  );
        summarivi.lisaa( debetsumma);
        summarivi.lisaa(kreditsumma);
        summarivi.viivaYlle();
        summarivi.lihavoi();
        rk.lisaaRivi(summarivi);
    }

    emit valmis( rk );
}



