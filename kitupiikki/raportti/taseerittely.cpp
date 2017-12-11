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

TaseErittely::TaseErittely() :
    Raportti()
{
    ui = new Ui::TaseErittely;
    ui->setupUi( raporttiWidget);

    ui->tilikausiCombo->setModel( kp()->tilikaudet());
    ui->tilikausiCombo->setModelColumn( TilikausiModel::KAUSI);
    ui->tilikausiCombo->setCurrentIndex( ui->tilikausiCombo->count() -1  );
}

TaseErittely::~TaseErittely()
{
    delete ui;
}

RaportinKirjoittaja TaseErittely::raportti()
{
    return kirjoitaRaportti( kp()->tilikaudet()->tilikausiIndeksilla( ui->tilikausiCombo->currentIndex()) );
}

RaportinKirjoittaja TaseErittely::kirjoitaRaportti(Tilikausi tilikaudelta)
{
    RaportinKirjoittaja rk;
    rk.asetaOtsikko("TASE-ERITTELY");
    rk.asetaKausiteksti( tilikaudelta.kausivaliTekstina());

    rk.lisaaSarake("12345678"); // Tosite
    rk.lisaaPvmSarake();        // Päivämäärä
    rk.lisaaVenyvaSarake();     // Selite
    rk.lisaaSarake("Alkusaldo12"); // Saldo/Loppusaldo
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

    kysely.exec( QString("select tili.id from tili,vienti where vienti.tili=tili.id and tili.ysiluku < 300000000 "
                         "and pvm <= \"%1\" group by tili.ysiluku order by tili.ysiluku").arg(tilikaudelta.paattyy().toString(Qt::ISODate)) );
    while(kysely.next())
        tiliIdt.append( kysely.value(0).toInt());

    int otsikkoId = 0;
    long edYsiluku = 0;

    foreach (int tiliId, tiliIdt)
    {
        Tili tili = kp()->tilit()->tiliIdlla(tiliId);

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

        if( otsikkoId != tili.ylaotsikkoId())
        {
            // Tulostetaan yläotsikko
            otsikkoId = tili.ylaotsikkoId();
            RaporttiRivi valiOtsikko;
            valiOtsikko.lisaa( kp()->tilit()->tiliIdlla( otsikkoId ).nimi().toUpper(), 3 );
            valiOtsikko.lihavoi();
            rk.lisaaRivi(valiOtsikko);
            rk.lisaaRivi();
        }
        // Ja sitten toimitaan erittelyvalinnan mukaisesti

        if( tili.taseErittelyTapa() == Tili::TASEERITTELY_SALDOT)
        {
            RaporttiRivi rr;
            rr.lisaa( QString("%1 %2").arg(tili.numero()).arg(tili.nimi()), 3 );
            rr.lisaa( tili.saldoPaivalle( tilikaudelta.paattyy() ), true);
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
            tilinnimi.lisaa( QString("%1 %2").arg(tili.numero()).arg(tili.nimi()), 3 );
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
                RaporttiRivi ekaRivi;
                ekaRivi.lisaa( "", 1);
                ekaRivi.lisaa( tilikaudelta.alkaa());
                ekaRivi.lisaa("Alkutase");
                ekaRivi.lisaa( tili.saldoPaivalle( tilikaudelta.alkaa().addDays(-1) ), true);
                rk.lisaaRivi( ekaRivi);

                // Muutokset
                kysely.exec(QString("SELECT tositelaji,tunniste,pvm,selite,debetsnt,kreditsnt from vientivw where tilinro=%1 and "
                            "pvm between \"%2\" and \"%3\" order by pvm")
                            .arg(tili.numero()).arg(tilikaudelta.alkaa().toString(Qt::ISODate)).arg(tilikaudelta.paattyy().toString(Qt::ISODate)) );
                while( kysely.next() )
                {
                    RaporttiRivi rr;
                    rr.lisaa( QString("%1%2").arg( kysely.value("tositelaji").toString()).arg(kysely.value("tunniste").toInt()));
                    rr.lisaa( kysely.value("pvm").toDate());
                    rr.lisaa(kysely.value("selite").toString());
                    if( tili.onko(TiliLaji::VASTAAVAA))
                        rr.lisaa( kysely.value("debetsnt").toInt() - kysely.value("kreditsnt").toInt());
                    else
                        rr.lisaa( kysely.value("kreditsnt").toInt() - kysely.value("debetsnt").toInt());
                    rk.lisaaRivi(rr);
                }

            }
            else if( tili.taseErittelyTapa() == Tili::TASEERITTELY_LISTA)
            {
                // Tase-erät saldoineen löytyvät kätevästi EranValintaModel:ista
                EranValintaModel erat;
                erat.lataa(tili, false, tilikaudelta.paattyy());
                // HUOM! Tase-erät alkavat riviltä 1, rivillä 0 "Muodosta uusi tase-erä"
                for(int i=1; i < erat.rowCount(QModelIndex()); i++)
                {
                    RaporttiRivi rr;
                    QModelIndex ind = erat.index(i, 0);
                    rr.lisaa( ind.data(EranValintaModel::TositteenTunnisteRooli).toString()  );
                    rr.lisaa( ind.data(EranValintaModel::PvmRooli).toDate());
                    rr.lisaa( ind.data(EranValintaModel::SeliteRooli).toString());
                    rr.lisaa( ind.data(EranValintaModel::SaldoRooli).toInt());
                    rk.lisaaRivi(rr);
                }
            }
            else if( tili.taseErittelyTapa() == Tili::TASEERITTELY_TAYSI)
            {
                // Tulostetaan tase-erät, joilla saldoa alkupäivällä tai tapahtumia tilikauden aikana

                // EraId,SaldoSnt
                QHash<int,int> alkusaldot;

                // Sijoitetaan ensin kaikki tämän tilikauden tase-erät
                kysely.exec( QString("SELECT DISTINCT id, eraid FROM viennit WHERE tili=%1 "
                                     "AND pvm BETWEEN \"%2\" AND \"%3\" ")
                             .arg(tili.id()).arg(tilikaudelta.alkaa().toString(Qt::ISODate))
                             .arg(tilikaudelta.paattyy().toString(Qt::ISODate)));
                while( kysely.next() )
                {
                    if( kysely.value("eraid").toInt())
                        alkusaldot.insert( kysely.value("eraid").toInt(), 0);
                    else
                        alkusaldot.insert( kysely.value("id").toInt(),0);
                }

                // Sitten alkusaldot
                kysely.exec(QString("SELECT eraid, sum(debetsnt) as debetit, sum(kreditsnt) as kreditit from vienti "
                                   "where tili=%1 and eraid is not null and pvm < \"%2\" group by eraid")
                           .arg(tili.id()).arg(tilikaudelta.paattyy().toString(Qt::ISODate)));

                // Tallennetaan saldotaulukkoon tilien eräsaldot
                while( kysely.next() )
                {
                    alkusaldot.insert( kysely.value("eraid").toInt(), kysely.value("debetit").toInt() - kysely.value("kreditit").toInt() );
                }

                // ja lisätään vielä aloittava vienti
                kysely.exec(QString("SELECT id, pvm, selite, debetsnt, kreditsnt from vienti "
                           "where tili=%1 and eraid is NULL and pvm < \"%2\" order by pvm")
                           .arg(tili.id()).arg( tilikaudelta.alkaa().toString(Qt::ISODate) ));


                while( kysely.next())
                {

                    int id = kysely.value("id").toInt();
                    alkusaldot[id] = alkusaldot.value(id, 0) + kysely.value("debetsnt").toInt() - kysely.value("kreditsnt").toInt();
                }

                // Sitten haetaan tase-erän tiedot
                QList<TaseEra> erat;
                kysely.exec(QString("SELECT id, pvm, selite "
                           "where tili=%1 and eraid is NULL order by pvm")
                           .arg(tili.id()));

                while( kysely.next())
                {
                    if( alkusaldot.contains(kysely.value("id").toInt()))
                    {
                        TaseEra era;
                        era.eraId = kysely.value("id").toInt();
                        era.pvm = kysely.value("pvm").toDate();
                        era.selite = kysely.value("selite").toString();
                        era.saldoSnt = alkusaldot.value(era.eraId);
                        erat.append(era);
                    }
                }

                // Nyt voidaan käydä tase-erät läpi
                foreach (TaseEra era, erat)
                {
                    RaporttiRivi nimirivi;
                    nimirivi.lisaa("TOSITE");
                    nimirivi.lisaa(era.pvm);
                    rk.lisaaRivi(nimirivi);
                }


            }

            // Loppusaldo
            RaporttiRivi vikaRivi;
            vikaRivi.lisaa("", 2);
            vikaRivi.lisaa(tr("Tilin %1 lopputase").arg(tili.numero()));
            vikaRivi.lisaa( tili.saldoPaivalle(tilikaudelta.paattyy()), true);
            vikaRivi.lihavoi();
            vikaRivi.viivaYlle();
            rk.lisaaRivi( vikaRivi );

        }

        rk.lisaaRivi();

    }

    return rk;
}
