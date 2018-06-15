/*
   Copyright (C) 2018 Arto Hyvättinen

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

#include <QSqlQuery>

#include "alverittely.h"

#include "ui_taseerittely.h"
#include "db/kirjanpito.h"

AlvErittely::AlvErittely()
 : Raportti( false )
{
    ui = new Ui::TaseErittely;
    ui->setupUi( raporttiWidget );

    // Oletuksena haetaan edellisen kuukauden erittely
    QDate taakse = kp()->paivamaara().addMonths(-1);
    QDate pvm = QDate( taakse.year(), taakse.month(), 1 );

    ui->alkaa->setDate( pvm );
    ui->paattyy->setDate( pvm.addMonths(1).addDays(-1));
}

AlvErittely::~AlvErittely()
{
    delete ui;
}

RaportinKirjoittaja AlvErittely::raportti(bool /* csvmuoto */)
{
    return kirjoitaRaporti( ui->alkaa->date(), ui->paattyy->date() );
}

RaportinKirjoittaja AlvErittely::kirjoitaRaporti(QDate alkupvm, QDate loppupvm)
{
    RaportinKirjoittaja kirjoittaja;
    kirjoittaja.asetaOtsikko(tr("ARVONLISÄVEROLASKELMAN ERITTELY"));
    kirjoittaja.asetaKausiteksti( QString("%1 - %2").arg(alkupvm.toString("dd.MM.yyyy")).arg(loppupvm.toString("dd.MM.yyyy") ) );

    kirjoittaja.lisaaPvmSarake();
    kirjoittaja.lisaaSarake("TOSITE12345");
    kirjoittaja.lisaaVenyvaSarake();
    kirjoittaja.lisaaSarake("24 ");
    kirjoittaja.lisaaEurosarake();

    RaporttiRivi otsikko;
    otsikko.lisaa("Pvm");
    otsikko.lisaa("Tosite");
    otsikko.lisaa("Selite");
    otsikko.lisaa("%",1,true);
    otsikko.lisaa("€",1,true);
    kirjoittaja.lisaaOtsake(otsikko);

    QSqlQuery kysely;
    QString kysymys = QString("select vienti.pvm as paiva, debetsnt, kreditsnt, selite, alvkoodi, alvprosentti, nro, tunniste, laji "
                              "from vienti,tili,tosite where vienti.tosite=tosite.id and vienti.tili=tili.id "
                              "and vienti.pvm between \"%1\" and \"%2\" "
                              "and alvkoodi > 0 order by alvkoodi, alvprosentti desc, tili, vienti.pvm")
            .arg(alkupvm.toString(Qt::ISODate))
            .arg(loppupvm.toString(Qt::ISODate));

    int nAlvkoodi = -1; // edellisten alv-prosentti jne...
    int nTili = -1;
    int nProsentti = -1;
    qlonglong tilisumma = 0;
    qlonglong yhtsumma = 0;

    qlonglong veroyhteensa = 0;
    qlonglong vahennysyhteensa = 0;

    int alvkoodi = -1;
    int alvprosentti = -1;
    int tilinro = -1;

    kysely.exec(kysymys);
    while( true )
    {
        // Jotta myös viimeisen rivin jälkeen tulee vielä summat
        // tulee break vasta summatulostuksen jälkeen

        bool jatkuu = kysely.next();

        if( jatkuu )
        {
            alvkoodi = kysely.value("alvkoodi").toInt();
            alvprosentti = kysely.value("alvprosentti").toInt();
            tilinro = kysely.value("nro").toInt();
        }


        if( tilinro != nTili || alvkoodi != nAlvkoodi || alvprosentti != nProsentti || !jatkuu )
        {
            if( tilisumma)
            {
                RaporttiRivi tiliSummaRivi;
                tiliSummaRivi.lisaa(" ", 3);

                if( nAlvkoodi == AlvKoodi::MAKSETTAVAALV)
                    tiliSummaRivi.lisaa("");
                else
                    tiliSummaRivi.lisaa( QString::number(nProsentti));

                tiliSummaRivi.lisaa( tilisumma);
                tiliSummaRivi.viivaYlle();
                kirjoittaja.lisaaRivi(tiliSummaRivi);
                
                if( (alvkoodi != nAlvkoodi || alvprosentti != nProsentti || !jatkuu) && yhtsumma != tilisumma && alvkoodi != AlvKoodi::TILITYS)
                {
                    // Lopuksi vielä lihavoituna alv-koodin ja -prosentin kokonaissumma
                    RaporttiRivi summaRivi;
                    summaRivi.lisaa(" ", 3);

                    if( nAlvkoodi == AlvKoodi::MAKSETTAVAALV)
                        summaRivi.lisaa(" ");
                    else
                        summaRivi.lisaa( QString::number(nProsentti));

                    summaRivi.lisaa( yhtsumma );
                    summaRivi.lihavoi();
                    kirjoittaja.lisaaRivi(summaRivi);
                }

                if( nAlvkoodi == AlvKoodi::MYYNNIT_BRUTTO || nAlvkoodi == AlvKoodi::OSTOT_BRUTTO )
                {
                    // Bruttokirjauksista lasketaan tileittäin veron osuus, ja myös
                    // lisätään se yhteissummiin. Tämä sitä varten, että myös kesken kauden
                    // tulostettavassa alv-erittelyssä näkyisi alv bruttokirjauksista

                    int osuus = qRound( ( nProsentti  * (double) tilisumma ) / ( nProsentti + 100  ) );
                    RaporttiRivi osuusRivi;
                    osuusRivi.lisaa(" ", 2);
                    if( nAlvkoodi == AlvKoodi::MYYNNIT_BRUTTO ) {
                        osuusRivi.lisaa("ALV OSUUS");
                        veroyhteensa += osuus;
                    } else {
                        osuusRivi.lisaa("ALV VÄHENNYS");
                        vahennysyhteensa += osuus;
                    }
                    osuusRivi.lisaa("");
                    osuusRivi.lisaa(osuus);
                    kirjoittaja.lisaaRivi(osuusRivi);

                }


                kirjoittaja.lisaaRivi();

                tilisumma = 0;
            }
        }

        if( !jatkuu )
            break;

        // Teknisiä kirjauksia ei tulosteta erittelyyn
        if( alvkoodi == AlvKoodi::TILITYS)
            continue;


        if( alvkoodi != nAlvkoodi || alvprosentti != nProsentti)
        {

            RaporttiRivi koodiOtsikko;
            if( alvkoodi == AlvKoodi::MAKSETTAVAALV)
                koodiOtsikko.lisaa(tr("MAKSETTAVA VERO"), 3);
            else if( alvkoodi > AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON)
                koodiOtsikko.lisaa(tr("KOHDENTAMATON MAKSUPERUSTEINEN"),3);
            else if( alvkoodi > AlvKoodi::ALVVAHENNYS)
                koodiOtsikko.lisaa( tr("VÄHENNYS %1").arg( kp()->alvTyypit()->seliteKoodilla(alvkoodi - AlvKoodi::ALVVAHENNYS) ), 3 );
            else if( alvkoodi > AlvKoodi::ALVKIRJAUS)
                koodiOtsikko.lisaa( tr("VERO %1").arg( kp()->alvTyypit()->seliteKoodilla(alvkoodi - AlvKoodi::ALVKIRJAUS) ), 3 );
            else
                koodiOtsikko.lisaa( kp()->alvTyypit()->seliteKoodilla(alvkoodi), 3 );

            if( alvprosentti)
                koodiOtsikko.lisaa( QString::number(alvprosentti));
            else
                koodiOtsikko.lisaa("");

            koodiOtsikko.lihavoi();
            kirjoittaja.lisaaRivi(koodiOtsikko);

            nAlvkoodi = alvkoodi;
            nProsentti = alvprosentti;
            nTili = -1;
            yhtsumma = 0;
        }

        if( tilinro != nTili )
        {
            RaporttiRivi tiliOtsikko;
            tiliOtsikko.lisaa( tr("%1 %2").arg(tilinro).arg(kp()->tilit()->tiliNumerolla(tilinro).nimi() ), 3);

            if( alvkoodi == AlvKoodi::MAKSETTAVAALV)
                tiliOtsikko.lisaa("");
            else
                tiliOtsikko.lisaa( QString::number(alvprosentti));

            kirjoittaja.lisaaRivi(tiliOtsikko);
            nTili = tilinro;
        }

        RaporttiRivi rivi;
        rivi.lisaa( kysely.value("paiva").toDate() );
        rivi.lisaa( QString("%1%2").arg( kp()->tositelajit()->tositelaji( kysely.value("laji").toInt() ).tunnus() )
                    .arg( kysely.value("tunniste").toInt() ));
        rivi.lisaa( kysely.value("selite").toString());

        if( alvkoodi == AlvKoodi::MAKSETTAVAALV)
            rivi.lisaa("");
        else
            rivi.lisaa( QString::number(alvprosentti));

        // Rahamäärän etumerkitys riippuu alv-koodista
        int debetsnt = kysely.value("debetsnt").toInt();
        int kreditsnt = kysely.value("kreditsnt").toInt();

        int summa = kreditsnt - debetsnt;   // MYYNTI tai KIRJAUS tai MAKSETTAVA ALV

        if( (( alvkoodi / 100 == 0 || alvkoodi / 100 == 4 ) && alvkoodi % 20 / 10 == 0  ) ||  ( alvkoodi / 100 == 2 ) )
            // 1nx (n parillinen) = OSTO  4nx Maksuperusteinen OSTO , 2xx VÄHENNYS
            summa = debetsnt - kreditsnt;

        rivi.lisaa( summa );
        kirjoittaja.lisaaRivi( rivi );

        tilisumma += summa;
        yhtsumma += summa;

        // Brutto-alv lisätään tileittän, joten sitä ei lisätä enää alv-kirjauksesta
        // Nettokirjausten alv:t lisätään alv-kirjauksista

        if( alvkoodi >= AlvKoodi::ALVVAHENNYS && alvkoodi < AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON && alvkoodi != AlvKoodi::ALVVAHENNYS + AlvKoodi::OSTOT_BRUTTO)
            vahennysyhteensa += summa;
        else if( alvkoodi >= AlvKoodi::ALVKIRJAUS && alvkoodi < AlvKoodi::ALVVAHENNYS && alvkoodi != AlvKoodi::ALVKIRJAUS + AlvKoodi::MYYNNIT_BRUTTO)
            veroyhteensa += summa;

    }

    kirjoittaja.lisaaTyhjaRivi();

    // Lopuksi yhteenveto verosta ja vähennyksestä
    // Tämän pitäisi täsmätä alv-laskelman kanssa ;)

    RaporttiRivi orivi;
    orivi.lisaa("",2);
    orivi.lisaa(tr("Vero yhteensä"), 2);
    orivi.lisaa(veroyhteensa, true);
    kirjoittaja.lisaaRivi( orivi );

    RaporttiRivi vrivi;
    vrivi.lisaa("", 2);
    vrivi.lisaa(tr("Vähennys yhteensä"),2);
    vrivi.lisaa(vahennysyhteensa, true);
    kirjoittaja.lisaaRivi(vrivi);

    RaporttiRivi yrivi;
    yrivi.lisaa("",2);
    yrivi.lisaa(tr("Maksettava vero"), 2);
    yrivi.lisaa( veroyhteensa - vahennysyhteensa, true);
    kirjoittaja.lisaaRivi(yrivi);




    return kirjoittaja;
}
