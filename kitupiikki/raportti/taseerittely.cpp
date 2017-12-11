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
        yotsikko.lisaa("Saldo €", 1, true);
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

    foreach (int tiliId, tiliIdt)
    {
        Tili tili = kp()->tilit()->tiliIdlla(tiliId);
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
        }
        else
        {
            // Nimike
            RaporttiRivi tilinnimi;
            tilinnimi.lisaa( QString("%1 %2").arg(tili.numero()).arg(tili.nimi()), 3 );
            tilinnimi.lihavoi();
            rk.lisaaRivi(tilinnimi);


            if( tili.taseErittelyTapa() == Tili::TASEERITTELY_MUUTOKSET)
            {

                // Alkusaldo
                RaporttiRivi ekaRivi;
                ekaRivi.lisaa( "", 1);
                ekaRivi.lisaa( tilikaudelta.alkaa());
                ekaRivi.lisaa("Alkutase");
                ekaRivi.lisaa( tili.saldoPaivalle( tilikaudelta.alkaa().addDays(-1) ), true);
                ekaRivi.lihavoi();
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
                    rr.lisaa(kysely.value("selite").toString(), 2);
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

            }

            // Loppusaldo
            RaporttiRivi vikaRivi;
            vikaRivi.lisaa("", 1);
            vikaRivi.lisaa( tilikaudelta.paattyy());
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
