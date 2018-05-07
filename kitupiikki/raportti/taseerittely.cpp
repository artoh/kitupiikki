/*
   Copyright (C) 2017 Arto Hyvättinen

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

#include "taseerittely.h"
#include "db/eranvalintamodel.h"
#include <QSqlQuery>

#include <QDebug>

TaseErittely::TaseErittely() :
    Raportti(false)
{
    ui = new Ui::TaseErittely;
    ui->setupUi( raporttiWidget);

    int tilikausia = kp()->tilikaudet()->rowCount(QModelIndex()) ;
    Tilikausi kausi = tilikausia > 2 ? kp()->tilikaudet()->tilikausiIndeksilla(tilikausia - 2) : kp()->tilikaudet()->tilikausiIndeksilla(tilikausia - 1);

    ui->alkaa->setDate( kausi.alkaa());
    ui->paattyy->setDate( kausi.paattyy());
}

TaseErittely::~TaseErittely()
{
    delete ui;
}

RaportinKirjoittaja TaseErittely::raportti(bool /* csvmuoto */)
{
    return kirjoitaRaportti( ui->alkaa->date(), ui->paattyy->date() );
}

RaportinKirjoittaja TaseErittely::kirjoitaRaportti(QDate mista, QDate mihin)
{
    RaportinKirjoittaja rk;
    rk.asetaOtsikko("TASE-ERITTELY");
    rk.asetaKausiteksti(QString("%1 - %2").arg(mista.toString("dd.MM.yyyy")).arg(mihin.toString("dd.MM.yyyy")));

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

    // Haetaan tilit, joissa kirjauksia
    QSqlQuery kysely;
    QList<int> tiliIdt;

    kysely.exec( QString("select DISTINCT tili.id from tili,vienti where vienti.tili=tili.id and tili.ysiluku < 300000000 "
                         "and pvm <= '%1' order by tili.ysiluku").arg(mihin.toString(Qt::ISODate)) );
    while(kysely.next() )
        tiliIdt.append( kysely.value(0).toInt());

    long edYsiluku = 0;

    foreach (int tiliId, tiliIdt)
    {
        Tili tili = kp()->tilit()->tiliIdlla(tiliId);

        // Ohitetaan tyhjät/tapahtumattomat tilit
        if( !tili.saldoPaivalle(mihin))
        {
             if(tili.taseErittelyTapa() == Tili::TASEERITTELY_SALDOT || tili.taseErittelyTapa() == Tili::TASEERITTELY_LISTA )
             {
                continue;
             }
            else
            {
                // Jos täysi tai muutos-tapahtumaerittely, niin ohitetaan jos ei myöskään tapahtumia
                QSqlQuery tapahtumakysely( QString("SELECT count(id) from vienti where tili=%1 and pvm between '%2' and '%3")
                                           .arg(tiliId).arg(mista.toString(Qt::ISODate)).arg(mihin.toString(Qt::ISODate)));
                if( tapahtumakysely.next())
                    if(!tapahtumakysely.value(0).toInt())
                        continue;
            }
        }

        if( edYsiluku / 100000000 < tili.ysivertailuluku()  / 100000000 )
        {
            RaporttiRivi otsikkoRivi;
            if( tili.ysivertailuluku() / 100000000 == 1)
                otsikkoRivi.lisaa("VASTAAVAA",3);
            else
                otsikkoRivi.lisaa("VASTATTAVAA",3);
            otsikkoRivi.asetaKoko(14);
            rk.lisaaRivi(otsikkoRivi);
            rk.lisaaRivi();
        }
        edYsiluku = tili.ysivertailuluku();

        // Ja sitten toimitaan erittelyvalinnan mukaisesti

        if( tili.taseErittelyTapa() == Tili::TASEERITTELY_SALDOT)
        {

            RaporttiRivi rr;
            rr.lisaaLinkilla( RaporttiRiviSarake::TILI_NRO, tili.numero(), QString("%1 %2").arg(tili.numero()).arg(tili.nimi()), 3 );

            rr.lisaa( tili.saldoPaivalle( mihin ), true);
            rr.lihavoi();
            rk.lisaaRivi(rr);

            // Täydentävä rivi
            if( !tili.json()->str("Taydentava").isEmpty())
            {
                RaporttiRivi lr;
                lr.lisaa( tili.json()->str("Taydentava"), 3);
                rk.lisaaRivi(lr);
            }
        }
        else
        {
            // Nimike
            RaporttiRivi tilinnimi;
            tilinnimi.lisaaLinkilla( RaporttiRiviSarake::TILI_NRO, tili.numero(), QString("%1 %2").arg(tili.numero()).arg(tili.nimi()), 3 );
            tilinnimi.lihavoi();
            rk.lisaaRivi(tilinnimi);
            // Täydentävä rivi
            if( !tili.json()->str("Taydentava").isEmpty())
            {
                RaporttiRivi lr;
                lr.lisaa( tili.json()->str("Taydentava"), 3);
                rk.lisaaRivi(lr);
            }



            if( tili.taseErittelyTapa() == Tili::TASEERITTELY_MUUTOKSET)
            {

                // Alkusaldo
                int alkusaldo = tili.saldoPaivalle( mista.addDays(-1));
                if( alkusaldo )
                {
                    RaporttiRivi ekaRivi;
                    ekaRivi.lisaa( "", 2);
                    ekaRivi.lisaa("Alkusaldo");
                    ekaRivi.lisaa( tili.saldoPaivalle( mista.addDays(-1) ), true);
                    rk.lisaaRivi( ekaRivi);
                }

                // Muutokset
                kysely.exec(QString("SELECT tositelaji,tunniste,pvm,selite,debetsnt,kreditsnt,tositeId from vientivw where tilinro=%1 and "
                            "pvm between \"%2\" and \"%3\" order by pvm")
                            .arg(tili.numero()).arg(mista.toString(Qt::ISODate)).arg(mihin.toString(Qt::ISODate)) );

                qDebug() << kysely.lastQuery();
                while( kysely.next() )
                {
                    RaporttiRivi rr;
                    rr.lisaaLinkilla(RaporttiRiviSarake::TOSITE_ID, kysely.value("tositeId").toInt(),
                                     QString("%1%2/%3").arg( kysely.value("tositelaji").toString()).arg(kysely.value("tunniste").toInt())
                                     .arg( kp()->tilikaudet()->tilikausiPaivalle( kysely.value("pvm").toDate() ).kausitunnus() ) );
                    rr.lisaa( kysely.value("pvm").toDate());
                    rr.lisaa(kysely.value("selite").toString());
                    if( tili.onko(TiliLaji::VASTAAVAA))
                        rr.lisaa( kysely.value("debetsnt").toLongLong() - kysely.value("kreditsnt").toLongLong());
                    else
                        rr.lisaa( kysely.value("kreditsnt").toLongLong() - kysely.value("debetsnt").toLongLong());
                    rk.lisaaRivi(rr);
                }

            }
            else if( tili.taseErittelyTapa() == Tili::TASEERITTELY_LISTA)
            {
                // Tase-erät saldoineen löytyvät kätevästi EranValintaModel:ista
                EranValintaModel erat;
                erat.lataa(tili, false, mihin);
                // HUOM! Tase-erät alkavat riviltä 2, rivillä 0 "Muodosta uusi tase-erä", rivillä 1 "Ei tase-erää"
                for(int i=2; i < erat.rowCount(QModelIndex()); i++)
                {
                    RaporttiRivi rr;
                    QModelIndex ind = erat.index(i, 0);

                    rr.lisaaLinkilla( RaporttiRiviSarake::TOSITE_ID, ind.data(EranValintaModel::TositeIdRooli).toInt(),
                                      ind.data(EranValintaModel::TositteenTunnisteRooli).toString()  );
                    rr.lisaa( ind.data(EranValintaModel::PvmRooli).toDate());
                    rr.lisaa( ind.data(EranValintaModel::SeliteRooli).toString());

                    if( tili.onko(TiliLaji::VASTAAVAA))
                        rr.lisaa( ind.data(EranValintaModel::SaldoRooli).toLongLong());
                    else
                        rr.lisaa( 0 - ind.data(EranValintaModel::SaldoRooli).toLongLong());
                    rk.lisaaRivi(rr);
                }
            }
            else if( tili.taseErittelyTapa() == Tili::TASEERITTELY_TAYSI)
            {
                // Tulostetaan tase-erät, joilla saldoa alkupäivällä tai tapahtumia tilikauden aikana

                // EraId,SaldoSnt
                QHash<int,qlonglong> alkusaldot;

                // Alkusaldot
                kysely.exec(QString("SELECT eraid, sum(debetsnt) as debetit, sum(kreditsnt) as kreditit from vienti "
                                   "where tili=%1 and eraid is not null and pvm < \"%2\" group by eraid")
                           .arg(tili.id()).arg(mista.toString(Qt::ISODate)));


                // Tallennetaan saldotaulukkoon tilien eräsaldot
                while( kysely.next() )
                {
                    if( tili.onko(TiliLaji::VASTATTAVAA))
                         alkusaldot.insert( kysely.value("eraid").toInt(), kysely.value("kreditit").toLongLong() - kysely.value("debetit").toLongLong() );
                    else
                        alkusaldot.insert( kysely.value("eraid").toInt(), kysely.value("debetit").toLongLong() - kysely.value("kreditit").toLongLong() );
                }

                // Sitten haetaan tase-erän tiedot
                kysely.exec(QString("SELECT vienti.id, debetsnt, kreditsnt, vienti.pvm, selite, laji, tunniste from vienti, tosite "
                           "where tili=%1 and vienti.tosite=tosite.id and eraid=vienti.id order by vienti.pvm")
                           .arg(tili.id()));

                while( kysely.next())
                {
                    rk.lisaaRivi();

                    int eraId = kysely.value("id").toInt();

                    qlonglong alkusnt = kysely.value("debetsnt").toLongLong() - kysely.value("kreditsnt").toLongLong();
                    if( tili.onko(TiliLaji::VASTATTAVAA))
                        alkusnt = 0 - alkusnt;

                    qDebug() << kysely.value("id").toInt();

                    RaporttiRivi nimirivi;
                    QString tunniste = QString("%1%2/%3")
                            .arg( kp()->tositelajit()->tositelaji( kysely.value("laji").toInt() ).tunnus() )
                            .arg( kysely.value("tunniste").toInt())
                            .arg( kp()->tilikaudet()->tilikausiPaivalle( kysely.value("vienti.pvm").toDate() ).kausitunnus()  );

                    nimirivi.lisaa(tunniste);
                    nimirivi.lisaa( kysely.value("pvm").toDate());
                    nimirivi.lisaa( kysely.value("selite").toString());
                    nimirivi.lisaa( alkusnt);
                    rk.lisaaRivi(nimirivi);

                    qlonglong saldo = alkusaldot.value(eraId);;

                    if( saldo && saldo != alkusnt )
                    {
                        // Poistot tähän mennessä ja saldo vuoden alkaessa

                        RaporttiRivi poistettuRivi;
                        poistettuRivi.lisaa(" ",2);
                        poistettuRivi.lisaa( tr("Lisäykset/vähennykset %1 saakka").arg( mista.addDays(-1).toString("dd.MM.yyyy")));
                        poistettuRivi.lisaa( saldo - alkusnt);
                        rk.lisaaRivi(poistettuRivi);

                        RaporttiRivi saldorivi;
                        saldorivi.lisaa(" ", 2);
                        saldorivi.lisaa( tr("Jäljellä %1").arg( mista.toString("dd.MM.yyyy")));
                        saldorivi.lisaa( saldo );
                        saldorivi.viivaYlle();
                        rk.lisaaRivi( saldorivi);
                    }
                    else
                        saldo = alkusnt;

                    // Muutokset
                    QSqlQuery muKysely;
                    muKysely.exec(QString("SELECT tositelaji,tunniste,pvm,selite,debetsnt,kreditsnt,tositeId from vientivw where eraid=%1 and "
                                "vientiId<>eraid and  pvm between \"%2\" and \"%3\" order by pvm")
                                .arg( eraId ).arg(mista.toString(Qt::ISODate)).arg(mihin.toString(Qt::ISODate)) );

                    while( muKysely.next() )
                    {
                        RaporttiRivi rr;
                        qlonglong muutos = 0;
                        if( tili.onko(TiliLaji::VASTAAVAA))
                            muutos =  muKysely.value("debetsnt").toLongLong() - muKysely.value("kreditsnt").toLongLong();
                        else
                            muutos =  muKysely.value("kreditsnt").toLongLong() - muKysely.value("debetsnt").toLongLong();
                        saldo += muutos;

                        rr.lisaaLinkilla(RaporttiRiviSarake::TOSITE_ID, muKysely.value("tositeId").toInt(),
                                         QString("%1%2/%3").arg( muKysely.value("tositelaji").toString()).arg(muKysely.value("tunniste").toInt())
                                                            .arg( kp()->tilikaudet()->tilikausiPaivalle( muKysely.value("pvm").toDate() ).kausitunnus() ));

                        rr.lisaa( muKysely.value("pvm").toDate());
                        rr.lisaa( muKysely.value("selite").toString());
                        rr.lisaa( muutos);

                        rk.lisaaRivi(rr);
                    }
                    // Vielä tase-erän loppusaldo

                    RaporttiRivi loppuRivi;
                    loppuRivi.lisaa(" ", 2);
                    loppuRivi.lisaa( tr("Loppusaldo %2").arg(mihin.toString("dd.MM.yyyy")));
                    loppuRivi.lisaa(saldo,true);
                    loppuRivi.viivaYlle();
                    rk.lisaaRivi(loppuRivi);
                }
                rk.lisaaRivi();
            }

            // Loppusaldo
            RaporttiRivi vikaRivi;
            vikaRivi.lisaa("", 2);
            vikaRivi.lisaa(tr("Tilin %1 loppusaldo").arg(tili.numero()));
            vikaRivi.lisaa( tili.saldoPaivalle(mihin), true);
            vikaRivi.lihavoi();
            vikaRivi.viivaYlle();
            rk.lisaaRivi( vikaRivi );

        }

        rk.lisaaRivi();

    }

    return rk;
}
