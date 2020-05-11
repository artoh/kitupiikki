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
#include "paakirja.h"

#include "db/kirjanpito.h"

#include <QDebug>

Paakirja::Paakirja(QObject *parent, const QString &kielikoodi)
    : Raportteri(parent, kielikoodi)
{

}

void Paakirja::kirjoita(const QDate &mista, const QDate &mihin, int optiot, int kohdennuksella,
                        int tililta)
{
    saapuneet_ = 0;
    optiot_ = optiot;

    KpKysely *saldokysely = kpk("/saldot");
    saldokysely->lisaaAttribuutti("pvm",mista);
    saldokysely->lisaaAttribuutti("alkusaldot");
    if( kohdennuksella > -1)
        saldokysely->lisaaAttribuutti("kohdennus", kohdennuksella);
    if( tililta )
        saldokysely->lisaaAttribuutti("tili", tililta);

    connect( saldokysely, &KpKysely::vastaus, this, &Paakirja::saldotSaapuu);


    KpKysely *vientikysely = kpk("/viennit");
    vientikysely->lisaaAttribuutti("alkupvm", mista);
    vientikysely->lisaaAttribuutti("loppupvm", mihin);
    vientikysely->lisaaAttribuutti("jarjestys","tili");

    if( kohdennuksella > -1)
        vientikysely->lisaaAttribuutti("kohdennus", kohdennuksella);
    if( tililta )
        vientikysely->lisaaAttribuutti("tili", tililta);

    connect( vientikysely, &KpKysely::vastaus, this, &Paakirja::viennitSaapuu);


    if( kohdennuksella > -1 )
        // Tulostetaan vain yhdestä kohdennuksesta
        rk.asetaOtsikko( QString(kaanna("PÄÄKIRJAN OTE") + "\n%1").arg( kp()->kohdennukset()->kohdennus(kohdennuksella).nimi()) ) ;
    else if( tililta)
        rk.asetaOtsikko( kaanna("PÄÄKIRJAN OTE"));
    else
        rk.asetaOtsikko( kaanna("PÄÄKIRJA"));

    rk.asetaKausiteksti(QString("%1 - %2").arg( mista.toString("dd.MM.yyyy") )
                                             .arg( mihin.toString("dd.MM.yyyy") ) );

    rk.lisaaPvmSarake();        // Pvm
    rk.lisaaSarake("ABC1234 "); // Tosite
    if( optiot & AsiakasToimittaja)
        rk.lisaaVenyvaSarake();
    rk.lisaaVenyvaSarake();     // Selite
    if( optiot & TulostaKohdennukset)
        rk.lisaaSarake("Kohdennusnimi"); // Kohdennus
    rk.lisaaEurosarake();   // Debet
    rk.lisaaEurosarake();   // Kredit
    rk.lisaaEurosarake();   // Saldo

    RaporttiRivi otsikko;
    otsikko.lisaa(kaanna("Pvm"));
    otsikko.lisaa(kaanna("Tosite"));
    if( optiot & AsiakasToimittaja)
        otsikko.lisaa(kaanna("Asiakas/Toimittaja"));
    otsikko.lisaa(kaanna("Selite"));
    if( optiot & TulostaKohdennukset )
        otsikko.lisaa(kaanna("Kohdennus"));
    otsikko.lisaa(kaanna("Debet €"),1,true);
    otsikko.lisaa(kaanna("Kredit €"),1,true);
    otsikko.lisaa(kaanna("Saldo €"),1, true);
    rk.lisaaOtsake(otsikko);

    saldokysely->kysy();
    vientikysely->kysy();

}

void Paakirja::saldotSaapuu(QVariant *data)
{
    QVariantMap saldot = data->toMap();
    QMapIterator<QString,QVariant> iter(saldot);
    while(iter.hasNext()) {
        iter.next();
        int tili = iter.key().toInt();
        saldot_.insert(tili, qRound64(iter.value().toDouble() * 100.0));
        if( !data_.contains(tili))
            data_.insert(tili, QList<QVariantMap>());
    }
    if( ++saapuneet_ > 1)
        kirjoitaDatasta();
}

void Paakirja::viennitSaapuu(QVariant *data)
{

    for(auto vienti : data->toList()) {
        QVariantMap map = vienti.toMap();
        int tili = map.value("tili").toInt();
        data_[tili].append(map);
    }

    if( ++saapuneet_ > 1)
        kirjoitaDatasta();
}

void Paakirja::kirjoitaDatasta()
{
    QMapIterator<int, QList<QVariantMap>> iter(data_);

    qlonglong kaikkiDebet = 0;
    qlonglong kaikkiKredit = 0;

    while( iter.hasNext()) {
        iter.next();

        Tili tili = kp()->tilit()->tiliNumerolla(iter.key());
        if( tili.onkoValidi())
        {

            RaporttiRivi rivi;
            rivi.lihavoi();
            rivi.lisaaLinkilla( RaporttiRiviSarake::TILI_LINKKI, tili.numero(),
                                tili.nimiNumero(), 5);

            qlonglong saldo =  saldot_.value( tili.numero() );
            rivi.lisaa( saldo );
            rk.lisaaRivi(rivi);

            qlonglong debetSumma = 0l;
            qlonglong kreditSumma = 0l;

            for(const QVariantMap& vienti : iter.value()) {

                RaporttiRivi rr;
                rr.lisaa( vienti.value("pvm").toDate() );

                QVariantMap tositeMap = vienti.value("tosite").toMap();

                rr.lisaaTositeTunnus( tositeMap.value("pvm").toDate(), tositeMap.value("sarja").toString(), tositeMap.value("tunniste").toInt(),
                                      optiot_ & SamaTilikausi);
                if( optiot_ & AsiakasToimittaja)
                    rr.lisaa( vienti.value("kumppani").toMap().value("nimi").toString() );
                rr.lisaa( vienti.value("selite").toString());

                if( optiot_ & TulostaKohdennukset)
                    rr.lisaa(kp()->kohdennukset()->kohdennus( vienti.value("kohdennus").toInt() ).nimi() );

                rr.lisaa(  vienti.value("debet").toDouble()  );
                rr.lisaa(  vienti.value("kredit").toDouble()  );

                debetSumma += qRound64( vienti.value("debet").toDouble() * 100 );
                kreditSumma += qRound64( vienti.value("kredit").toDouble() * 100 );

                if( tili.onko(TiliLaji::VASTAAVAA))
                {
                    saldo += qRound64( vienti.value("debet").toDouble() * 100 );
                    saldo -= qRound64( vienti.value("kredit").toDouble() * 100 );
                } else {
                    saldo -= qRound64( vienti.value("debet").toDouble() * 100 );
                    saldo += qRound64( vienti.value("kredit").toDouble() * 100 );
                }

                rr.lisaa( saldo,true);
                rk.lisaaRivi(rr);

            }
            if( (debetSumma || kreditSumma) && optiot_ & TulostaSummat  ) {
                RaporttiRivi summa(RaporttiRivi::EICSV);
                summa.viivaYlle();
                summa.lihavoi();
                summa.lisaa("",2);

                if( optiot_ & TulostaKohdennukset)
                    summa.lisaa("");

                qlonglong muutos = tili.onko(TiliLaji::VASTAAVAA) ?
                        debetSumma - kreditSumma : kreditSumma - debetSumma;
                summa.lisaa(muutos,false, true);

                summa.lisaa(debetSumma);
                summa.lisaa(kreditSumma);
                summa.lisaa(saldo);

                rk.lisaaRivi(summa);

                kaikkiDebet += debetSumma;
                kaikkiKredit += kreditSumma;
            }
            rk.lisaaRivi();

        }

    }
    if( optiot_ & TulostaSummat ) {
        RaporttiRivi summa(RaporttiRivi::EICSV);
        summa.viivaYlle();
        summa.lihavoi();
        summa.lisaa(kaanna("Yhteensä"),3);

        summa.lisaa(kaikkiDebet);
        summa.lisaa(kaikkiKredit);
        summa.lisaa("");

        rk.lisaaRivi(summa);
    }

    emit valmis(rk);
}

