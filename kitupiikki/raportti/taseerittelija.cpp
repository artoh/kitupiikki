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
#include "taseerittelija.h"
#include "db/kirjanpito.h"

TaseErittelija::TaseErittelija(QObject *parent) : QObject(parent)
{

}

void TaseErittelija::kirjoita(const QDate& mista, const QDate &mihin)
{
    KpKysely* kysely = kpk("/erat");
    kysely->lisaaAttribuutti("erittely");
    kysely->lisaaAttribuutti("alkaa", mista);
    kysely->lisaaAttribuutti("loppuu", mihin);
    connect( kysely, &KpKysely::vastaus, this, &TaseErittelija::dataSaapuu );
    kysely->kysy();
}

void TaseErittelija::dataSaapuu(QVariant *data)
{
    RaportinKirjoittaja rk(false);
    rk.asetaOtsikko("TASE-ERITTELY");       // Näiden käännöksiin vielä mekanismi ????
    rk.asetaKausiteksti(QString("%1 - %2").arg(mista_.toString("dd.MM.yyyy")).arg(mihin_.toString("dd.MM.yyyy")));

    rk.lisaaSarake("12345678"); // Tosite
    rk.lisaaPvmSarake();        // Päivämäärä
    rk.lisaaVenyvaSarake();     // Selite
    rk.lisaaEurosarake();

    // Lisätään otsikot
    {
        RaporttiRivi yotsikko;
        yotsikko.lisaa("Tili",3);
        rk.lisaaOtsake(yotsikko);

        RaporttiRivi otsikko;
        otsikko.lisaa("Tosite");
        otsikko.lisaa("Päivämäärä");
        otsikko.lisaa("Selite");
        otsikko.lisaa("€",1,true);
        rk.lisaaOtsake(otsikko);
        rk.lisaaRivi();
    }

    qlonglong summa = 0;
    bool vastattavaa = false;
    RaporttiRivi otsikkoRivi;
    otsikkoRivi.lisaa("VASTAAVAA",3);
    otsikkoRivi.asetaKoko(14);
    rk.lisaaRivi(otsikkoRivi);

    QVariantMap map = data->toMap();
    QMapIterator<QString,QVariant> iter(map);

    while( iter.hasNext()) {
        iter.next();

        if( iter.key().startsWith('2') && !vastattavaa) {
            RaporttiRivi otsikkoRivi;
            otsikkoRivi.lisaa("VASTATTAVAA",3);
            otsikkoRivi.asetaKoko(14);
            rk.lisaaRivi(otsikkoRivi);
            vastattavaa = true;
            summa = 0;
        }

        QChar tyyppi = iter.key().at( iter.key().length()-1 );
        int tilinumero = iter.key().left(iter.key().length()-1).toInt();
        Tili* tili = kp()->tilit()->tili(tilinumero);
        if( !tili)
            continue;

        if( tyyppi == 'S') {    // VAIN SALDO
            RaporttiRivi rr;
            rr.lisaaLinkilla( RaporttiRiviSarake::TILI_NRO, tili->numero(), tili->nimiNumero(), 3 );
            rr.lisaa( qRound64(iter.value().toDouble() * 100), true);
            rr.lihavoi();
            rk.lisaaRivi(rr);
        } else {
            qlonglong loppusaldo = 0l;

            // Nimike
            RaporttiRivi tilinnimi;
            tilinnimi.lisaaLinkilla( RaporttiRiviSarake::TILI_NRO, tili->numero(), tili->nimiNumero(), 3 );
            tilinnimi.lihavoi();
            rk.lisaaRivi(tilinnimi);

            if( tyyppi == 'T') {
                // Täysi tase-erittely
                rk.lisaaRivi();
                for(QVariant era : iter.value().toList() ) {
                    QVariantMap map = era.toMap();

                    RaporttiRivi nimirivi;
                    QVariantMap eramap = map.value("era").toMap();
                    nimirivi.lisaaLinkilla( RaporttiRiviSarake::TOSITE_ID,
                                            eramap.value("id").toInt(),
                                            kp()->tositeTunnus( eramap.value("tunniste").toInt(),
                                            eramap.value("pvm").toDate(),
                                            eramap.value("sarja").toString()) );
                    nimirivi.lisaa( eramap.value("pvm").toDate() );
                    nimirivi.lisaa( eramap.value("selite").toString());
                    nimirivi.lisaa( qRound64( eramap.value("eur").toDouble() * 100));
                    rk.lisaaRivi(nimirivi);

                    if( eramap.value("pvm").toDate() < mista_ && map.value("ennen") != eramap.value("eur") ) {
                        RaporttiRivi poistettuRivi;
                        poistettuRivi.lisaa(" ",2);
                        poistettuRivi.lisaa( tr("Lisäykset/vähennykset %1 saakka").arg( mista_.addDays(-1).toString("dd.MM.yyyy")));
                        poistettuRivi.lisaa( qRound64( map.value("ennnen").toDouble() * 100 ) -
                                             qRound64( eramap.value("eur").toDouble() * 100));
                        rk.lisaaRivi(poistettuRivi);

                        RaporttiRivi saldorivi;
                        saldorivi.lisaa(" ", 2);
                        saldorivi.lisaa( tr("Jäljellä %1").arg( mista_.toString("dd.MM.yyyy")));
                        saldorivi.lisaa( qRound64( map.value("ennnen").toDouble() * 100 ) );
                        saldorivi.viivaYlle();
                        rk.lisaaRivi( saldorivi);
                    }

                    // Muutokset
                    for( QVariant muutos : map.value("kausi").toList()) {
                        QVariantMap mmap = muutos.toMap();
                        RaporttiRivi rr;

                        rr.lisaaLinkilla(RaporttiRiviSarake::TOSITE_ID,
                                         mmap.value("id").toInt(),
                                         kp()->tositeTunnus(mmap.value("tunniste").toInt(),
                                                            mmap.value("pvm").toDate(),
                                                            mmap.value("sarja").toString()) );
                        rr.lisaa( mmap.value("pvm").toDate());
                        rr.lisaa( mmap.value("selite").toString());
                        rr.lisaa(qRound64( mmap.value("eur").toDouble() * 100.0 ));
                        rk.lisaaRivi(rr);
                    }
                    // Loppusaldo
                    RaporttiRivi loppuRivi;
                    loppuRivi.lisaa(" ", 2);
                    loppuRivi.lisaa( tr("Loppusaldo %2").arg(mihin_.toString("dd.MM.yyyy")));
                    loppuRivi.lisaa( qRound64(map.value("saldo").toDouble() * 100.0),true);
                    loppuRivi.viivaYlle();
                    rk.lisaaRivi(loppuRivi);

                    rk.lisaaRivi();
                    loppusaldo += qRound64(map.value("saldo").toDouble() * 100.0);
                }

            } else if ( tyyppi == 'M') {
                QVariantMap map = iter.value().toMap();
                loppusaldo = qRound64( map.value("saldo").toDouble() * 100);

                RaporttiRivi saldorivi;
                saldorivi.lisaa(" ", 2);
                saldorivi.lisaa( tr("Alkusaldo %1").arg( mista_.toString("dd.MM.yyyy")));
                saldorivi.lisaa( qRound64( map.value("ennnen").toDouble() * 100 ) );
                rk.lisaaRivi( saldorivi);

                // Muutokset
                for( QVariant muutos : map.value("kausi").toList()) {
                    QVariantMap mmap = muutos.toMap();
                    RaporttiRivi rr;

                    rr.lisaaLinkilla(RaporttiRiviSarake::TOSITE_ID,
                                     mmap.value("id").toInt(),
                                     kp()->tositeTunnus(mmap.value("tunniste").toInt(),
                                                        mmap.value("pvm").toDate(),
                                                        mmap.value("sarja").toString()) );
                    rr.lisaa( mmap.value("pvm").toDate());
                    rr.lisaa( mmap.value("selite").toString());
                    rr.lisaa(qRound64( mmap.value("eur").toDouble() * 100.0 ));
                    rk.lisaaRivi(rr);
                }

            } else if( tyyppi == 'E') {
                QVariantList erat = iter.value().toList();

                for( QVariant era : erat ) {
                    QVariantMap emap = era.toMap();
                    RaporttiRivi rr;

                    rr.lisaaLinkilla(RaporttiRiviSarake::TOSITE_ID,
                                     emap.value("id").toInt(),
                                     kp()->tositeTunnus(emap.value("tunniste").toInt(),
                                                        emap.value("pvm").toDate(),
                                                        emap.value("sarja").toString()) );
                    rr.lisaa( emap.value("pvm").toDate());
                    rr.lisaa( emap.value("selite").toString());
                    rr.lisaa(qRound64( emap.value("eur").toDouble() * 100.0 ));
                    rk.lisaaRivi(rr);
                    loppusaldo += qRound64( emap.value("eur").toDouble() * 100.0 );
                }

            }

            RaporttiRivi vikaRivi;
            vikaRivi.lisaa("", 2);
            vikaRivi.lisaa(tr("Tilin %1 loppusaldo").arg(tili->numero()));
            vikaRivi.lisaa( loppusaldo, true);
            vikaRivi.lihavoi();
            vikaRivi.viivaYlle();
            rk.lisaaRivi( vikaRivi );
        }
        rk.lisaaRivi();
    }
    emit valmis(rk);
}
