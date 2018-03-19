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

#include <QSqlQuery>
#include <QDebug>
#include <QPrinter>
#include <QPainter>
#include <QSqlError>

#include <QTemporaryFile>
#include <QTextDocument>
#include <QMessageBox>
#include "alvilmoitusdialog.h"
#include "ui_alvilmoitusdialog.h"

#include "kirjaus/ehdotusmodel.h"
#include "db/kirjanpito.h"



AlvIlmoitusDialog::AlvIlmoitusDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AlvIlmoitusDialog)
{
    ui->setupUi(this);
}

AlvIlmoitusDialog::~AlvIlmoitusDialog()
{
    delete ui;
}

QDate AlvIlmoitusDialog::teeAlvIlmoitus(QDate alkupvm, QDate loppupvm)
{
    // Tarkistetaan, että tarvittavat tilit löytyy

    if( !kp()->tilit()->tiliTyypilla(TiliLaji::ALVSAATAVA).onkoValidi() ||
        !kp()->tilit()->tiliTyypilla(TiliLaji::ALVSAATAVA).onkoValidi() ||
            !kp()->tilit()->tiliTyypilla(TiliLaji::ALVVELKA).onkoValidi() )
    {
        QMessageBox::critical(0, tr("Kitupiikin virhe"),
                              tr("Alv-tilitystä ei voi laatia, koska tilikartta on puutteellinen."));
        return QDate();
    }

    // TODO: Maksuperusteisessa alvissa:
    // Maksuperusteisen alv:n lopettaminen
    // Erääntynyt (12kk) maksuperusteinen alv

    AlvIlmoitusDialog dlg;
    if( dlg.alvIlmoitus(alkupvm, loppupvm))
        return loppupvm;
    else
        return QDate();
}

bool AlvIlmoitusDialog::alvIlmoitus(QDate alkupvm, QDate loppupvm)
{
    QMap<int,int> verotKannoittainSnt;  // verokanta - maksettava vero

    // Lisätään kotimaiset verokannat, jotta ilmoitus näyttää paremmalta
    verotKannoittainSnt.insert(24, 0);
    verotKannoittainSnt.insert(14, 0);
    verotKannoittainSnt.insert(10, 0);


    int bruttoveroayhtSnt = 0;
    int bruttovahennettavaaSnt = 0;

    EhdotusModel ehdotus;
    QSqlQuery query( *kp()->tietokanta() );

    // 1) Bruttojen oikaisut
    // Korjattu 6.3.2018 #81 since 0.6

    query.exec(  QString("select alvkoodi,alvprosentti,sum(debetsnt) as debetit, sum(kreditsnt) as kreditit, tili from vienti where pvm between \"%3\" and \"%4\" and (alvkoodi=%1 or alvkoodi=%2) group by alvkoodi,tili,alvprosentti")
                 .arg(AlvKoodi::MYYNNIT_BRUTTO).arg(AlvKoodi::OSTOT_BRUTTO)
                 .arg(alkupvm.toString(Qt::ISODate)).arg(loppupvm.toString(Qt::ISODate)));

    qDebug() << query.lastQuery();

    while( query.next() && query.value("alvprosentti").toInt())
    {

        Tili tili = kp()->tilit()->tiliIdlla( query.value("tili").toInt() );
        int alvprosentti = query.value("alvprosentti").toInt();
        qlonglong saldoSnt =  query.value("kreditit").toInt() - query.value("debetit").toLongLong();


        VientiRivi rivi;        // Rivi, jolla tiliä oikaistaan
        VientiRivi verorivi;    // Rivi, jolla kirjataan alv-tilille

        rivi.pvm = loppupvm;
        rivi.tili = tili;
        rivi.alvprosentti = alvprosentti;

        verorivi.pvm = loppupvm;
        verorivi.alvprosentti = alvprosentti;

        // Brutosta erotetaan verot
        int veroSnt = ( alvprosentti * saldoSnt ) / ( 100 + alvprosentti) ;
        int nettoSnt = saldoSnt - veroSnt;

        if( nettoSnt > 0)
        {
            rivi.debetSnt = veroSnt;
            verorivi.kreditSnt = veroSnt;
        } else {
            rivi.kreditSnt = 0 - veroSnt;
            verorivi.debetSnt = 0 - veroSnt;
        }

        if( query.value("alvkoodi").toInt() == AlvKoodi::MYYNNIT_BRUTTO )
        {
            verotKannoittainSnt[ alvprosentti ] = verotKannoittainSnt.value(alvprosentti, 0) + veroSnt;
            bruttoveroayhtSnt += veroSnt;

            rivi.selite = tr("Alv-kirjaus %1 - %2 %3 % vero (NETTO %L4 €, BRUTTO %L5€)").arg(alkupvm.toString(Qt::SystemLocaleShortDate))
                    .arg(loppupvm.toString(Qt::SystemLocaleShortDate))
                    .arg(alvprosentti)
                    .arg(nettoSnt / 100.0,0, 'f',2)
                    .arg(saldoSnt / 100.0,0, 'f', 2);            
            rivi.debetSnt = veroSnt;

            verorivi.tili = kp()->tilit()->tiliTyypilla(TiliLaji::ALVVELKA);
            verorivi.alvkoodi = query.value("alvkoodi").toInt() | AlvKoodi::ALVKIRJAUS;

        }
        else
        {
            bruttovahennettavaaSnt += qAbs(veroSnt);

            rivi.selite = tr("Alv-kirjaus %1 - %2 %3 % vähennys (NETTO %L4 €, BRUTTO %L5€) ").arg(alkupvm.toString(Qt::SystemLocaleShortDate))
                    .arg(loppupvm.toString(Qt::SystemLocaleShortDate))
                    .arg(alvprosentti)
                    .arg(qAbs(nettoSnt) / 100.0,0, 'f',2)
                    .arg(qAbs(saldoSnt) / 100.0,0, 'f', 2);

            verorivi.tili = kp()->tilit()->tiliTyypilla(TiliLaji::ALVSAATAVA);
            verorivi.alvkoodi = query.value("alvkoodi").toInt() | AlvKoodi::ALVVAHENNYS;
        }

        verorivi.selite = tr("%1 tilillä %2 %3")
                .arg(rivi.selite)
                .arg( tili.numero() )
                .arg( tili.nimi() );

        ehdotus.lisaaVienti(rivi);
        ehdotus.lisaaVienti(verorivi);
    }

    // 2) Nettokirjausten koonti
    query.exec( QString("select alvprosentti, sum(debetsnt) as debetit, sum(kreditsnt) as kreditit from vienti where pvm between \"%1\" and \"%2\" and alvkoodi=%3 group by alvprosentti")
                .arg(alkupvm.toString(Qt::ISODate)).arg(loppupvm.toString(Qt::ISODate)).arg(AlvKoodi::ALVKIRJAUS + AlvKoodi::MYYNNIT_NETTO) );

    qDebug() << query.lastQuery();
    while( query.next())
    {
        int alvprosentti = query.value("alvprosentti").toInt();
        int saldo = query.value("kreditit").toInt() - query.value("debetit").toInt();
        verotKannoittainSnt[ alvprosentti ] = verotKannoittainSnt.value(alvprosentti) + saldo;
    }


    // Muut kirjaukset tauluihin
    query.exec( QString("select alvkoodi, sum(debetsnt) as debetit, sum(kreditsnt) as kreditit from vienti where pvm between \"%1\" and \"%2\" group by alvkoodi")
                .arg(alkupvm.toString(Qt::ISODate)).arg(loppupvm.toString(Qt::ISODate)) );

    qDebug() << query.lastQuery();
    QMap<int,int> kooditaulu;

    int nettoverosnt = 0;
    int nettovahennyssnt = 0;

    while( query.next())
    {
        int saldo = query.value("kreditit").toInt() - query.value("debetit").toInt();
        int koodi = query.value("alvkoodi").toInt();

        if( koodi  > AlvKoodi::ALVVAHENNYS)
        {
            nettovahennyssnt += 0 - saldo;
            kooditaulu.insert(koodi, 0-saldo);
        }
        else if( koodi > AlvKoodi::ALVKIRJAUS)
        {
            nettoverosnt += saldo;
            kooditaulu.insert(koodi, saldo);
        }
        else
        {
            if( koodi / 10 % 2 )  // 10 myynti, 20 osto jne.
                kooditaulu.insert(koodi, saldo);
            else
                kooditaulu.insert(koodi, 0 - saldo);
        }
    }
    // Kirjaus alv-saamistililtä ja alv-velkatililtä verovelkatilille
    if( nettoverosnt + bruttoveroayhtSnt)
    {
        VientiRivi rivi;
        rivi.pvm = loppupvm;
        rivi.tili = kp()->tilit()->tiliTyypilla(TiliLaji::ALVVELKA);
        rivi.selite = tr("Alv-kirjaus %1 - %2 ").arg(alkupvm.toString(Qt::SystemLocaleShortDate)).arg(loppupvm.toString(Qt::SystemLocaleShortDate));
        rivi.debetSnt = nettoverosnt + bruttoveroayhtSnt;
        ehdotus.lisaaVienti(rivi);
    }
    if( nettovahennyssnt + bruttovahennettavaaSnt)
    {
        VientiRivi rivi;
        rivi.pvm = loppupvm;
        rivi.tili = kp()->tilit()->tiliTyypilla(TiliLaji::ALVSAATAVA);
        rivi.selite = tr("Alv-kirjaus %1 - %2 ").arg(alkupvm.toString(Qt::SystemLocaleShortDate)).arg(loppupvm.toString(Qt::SystemLocaleShortDate));
        rivi.kreditSnt = nettovahennyssnt + bruttovahennettavaaSnt;
        ehdotus.lisaaVienti(rivi);
    }
    // Ja lopuksi kirjataan verot verotilille
    int maksettavavero = bruttoveroayhtSnt + nettoverosnt - bruttovahennettavaaSnt - nettovahennyssnt;
    if( maksettavavero )
    {
        VientiRivi rivi;
        rivi.pvm = loppupvm;
        rivi.tili = kp()->tilit()->tiliTyypilla(TiliLaji::VEROVELKA);
        rivi.selite = tr("Alv-ilmoituksella tilitettävä vero kaudelta %1 - %2").arg(alkupvm.toString(Qt::SystemLocaleShortDate)).arg(loppupvm.toString(Qt::SystemLocaleShortDate));
        if( maksettavavero > 0 )
            rivi.kreditSnt = maksettavavero;
        else
            rivi.debetSnt = 0 - maksettavavero;
        rivi.alvkoodi = AlvKoodi::MAKSETTAVAALV;
        ehdotus.lisaaVienti(rivi);
    }


    // Laskelman tulostus
    QMapIterator<int,int> iter(verotKannoittainSnt);
    iter.toBack();

    kirjoittaja = new RaportinKirjoittaja();
    kirjoittaja->asetaOtsikko("ARVONLISÄVEROLASKELMA");
    kirjoittaja->asetaKausiteksti( QString("%1 - %2").arg(alkupvm.toString(Qt::SystemLocaleShortDate)).arg(loppupvm.toString(Qt::SystemLocaleShortDate) ) );
    kirjoittaja->lisaaVenyvaSarake();
    kirjoittaja->lisaaEurosarake();

    if( kp()->onkoMaksuperusteinenAlv(loppupvm) )
        otsikko("Kotimaan arvonlisävero laskettu maksuperusteisesti.");

    otsikko("Vero kotimaan myynnistä verokannoittain");



    while( iter.hasPrevious())
    {
        iter.previous();
        luku(tr("%1 %:n vero").arg(iter.key()), iter.value() );
    }


    otsikko("Vero ostoista ja maahantuonneista");
    luku(tr("Vero tavaraostoista muista EU-maista"), kooditaulu.value(AlvKoodi::ALVKIRJAUS + AlvKoodi::YHTEISOHANKINNAT_TAVARAT)  );
    luku(tr("Vero palveluostoista muista EU-maista"), kooditaulu.value(AlvKoodi::ALVKIRJAUS + AlvKoodi::YHTEISOHANKINNAT_PALVELUT)  );
    luku(tr("Vero tavaroiden maahantuonnista EU:n ulkopuolelta"), kooditaulu.value(AlvKoodi::ALVKIRJAUS + AlvKoodi::RAKENNUSPALVELU_OSTO) );
    luku(tr("Vero rakentamispalvelun ja metalliromun ostoista"), kooditaulu.value(AlvKoodi::ALVKIRJAUS + AlvKoodi::RAKENNUSPALVELU_OSTO) );


    otsikko("Vähennettävä vero");
    luku(tr("Verokauden vähennettävä vero"), nettovahennyssnt + bruttovahennettavaaSnt );

    otsikko( tr("Myynnit ja ostot"));
    luku(tr("0-verokannan alainen liikevaihto"), kooditaulu.value(AlvKoodi::ALV0) );
    luku(tr("Tavaroiden myynti muihin EU-maihin"), kooditaulu.value(AlvKoodi::YHTEISOMYYNTI_TAVARAT) );
    luku(tr("Palveluiden myynti muihin EU-maihin"), kooditaulu.value(AlvKoodi::YHTEISOMYYNTI_PALVELUT) );
    luku(tr("Tavaraostot muista EU-maista"),kooditaulu.value(AlvKoodi::YHTEISOHANKINNAT_TAVARAT)  );
    luku(tr("Palveluostot muista EU-maista"), kooditaulu.value(AlvKoodi::YHTEISOHANKINNAT_PALVELUT) );
    luku(tr("Tavaroiden maahantuonnit EU:n ulkopuolelta"), kooditaulu.value(AlvKoodi::MAAHANTUONTI)  );
    luku(tr("Rakentamispalveluiden ja metalliromun myynnit"), kooditaulu.value(AlvKoodi::RAKENNUSPALVELU_OSTO) );
    luku(tr("Rakentamispalveluiden ja metalliromun ostot"), kooditaulu.value(AlvKoodi::RAKENNUSPALVELU_OSTO) );


    otsikko(tr("Maksettava vero"));
    luku(tr("Vero yhteensä"), bruttoveroayhtSnt + nettoverosnt );
    luku(tr("Vähenettävä vero yhteensä"), bruttovahennettavaaSnt + nettovahennyssnt);
    luku(tr("Maksettava vero"), maksettavavero  , true);

    // Alarajahuojennus
    int alvKaudenPituus = kp()->asetukset()->luku("AlvKausi");
    if( ( alvKaudenPituus > 1 && loppupvm.month() == 12) ||
         ( alvKaudenPituus == 1 && loppupvm == kp()->tilikaudet()->tilikausiPaivalle(loppupvm).paattyy() )   )
    {
        long liikevaihto = 0;
        long vero = maksettavavero;
        QDate laskelmaMista;

        if( alvKaudenPituus == 1)
        {
            // Lasketaan tältä tilikaudelta
            Tilikausi kausi = kp()->tilikaudet()->tilikausiPaivalle( loppupvm);
            laskelmaMista = kausi.alkaa();
        }
        else
        {
            laskelmaMista = loppupvm.addYears(-1);
            if( kp()->tilikaudet()->kirjanpitoAlkaa().daysTo( laskelmaMista ) < 0 )
                laskelmaMista = kp()->tilikaudet()->kirjanpitoAlkaa();
        }
        int kuukausiaLaskelmassa = laskelmaMista.daysTo(loppupvm) / 30;

        QSqlQuery kysely;
        kysely.exec(  QString("SELECT SUM(kreditsnt), SUM(debetsnt) "
                                   "FROM vienti, tili WHERE "
                                   "pvm BETWEEN \"%1\" AND \"%2\" "
                                   "AND vienti.tili=tili.id AND "
                                   "tili.tyyppi = \"CL\" AND vienti.alvkoodi > 0")
                           .arg(laskelmaMista.toString(Qt::ISODate))
                           .arg(loppupvm.toString(Qt::ISODate)));
        if( kysely.next())
            liikevaihto = kysely.value(0).toInt() - kysely.value(1).toInt();

        kysely.exec(  QString("SELECT SUM(kreditsnt), SUM(debetsnt) "
                                   "FROM vienti WHERE "
                                   "pvm BETWEEN \"%1\" AND \"%2\" "
                                   "AND alvkoodi = %3 ")
                           .arg(laskelmaMista.toString(Qt::ISODate))
                           .arg(loppupvm.toString(Qt::ISODate))
                           .arg( AlvKoodi::MAKSETTAVAALV ));
        if( kysely.next())
            vero += kysely.value(1).toInt() - kysely.value(0).toInt();

        int suhteutettu = liikevaihto;

        if( kuukausiaLaskelmassa )
            suhteutettu = liikevaihto *  12 / kuukausiaLaskelmassa;


        long huojennus = 0;
        if( suhteutettu <= 1000000)
            huojennus = vero;
        else if( suhteutettu <= 3000000)
        {
            huojennus = vero - ((( suhteutettu - 1000000) * vero  ) / 2000000 );
        }
        otsikko(tr("Arvonlisäveron alarajahuojennus"));
        luku(tr("Liikevaihto"), liikevaihto );
        luku(tr("Maksettu vero"), vero );
        luku(tr("Arvio alarajahuojennuksesta"), huojennus);

        RaporttiRivi rivi;
        rivi.lisaa(tr("Yllä oleva laskelma on tehty koko verollisella liikevaihdolla ja "
                      "maksetulla arvonlisäverolla. Alarajahuojennusta laskettaessa "
                      "on otettava huomioon useita poikkeuksia ja huojennus "
                      "on laskettava erikseen verohallinnon ohjeiden mukaisesti."),2);
        kirjoittaja->lisaaRivi(rivi);


    }


    ui->ilmoitusBrowser->setHtml( kirjoittaja->html());
    if( exec() )
    {
        // Laskelma vahvistettu, tallennetaan tositteeksi
        TositeModel model( kp()->tietokanta());
        model.asetaPvm( loppupvm );
        model.asetaTositelaji(0);
        model.asetaOtsikko( tr("Arvonlisävero %1 - %2").arg(alkupvm.toString(Qt::SystemLocaleShortDate)).arg(loppupvm.toString(Qt::SystemLocaleShortDate)) );

        model.json()->set("AlvTilitysAlkaa", alkupvm);
        model.json()->set("AlvTilitysPaattyy", loppupvm);
        model.json()->set("MaksettavaAlv", maksettavavero);
        ehdotus.tallenna( model.vientiModel() );

        // Liitetään laskelma
        QTemporaryFile file( QDir::tempPath() + "/alv-XXXXXX.pdf");
        file.open();
        file.close();
        QPrinter printer;

        printer.setPageSize(QPrinter::A4);
        printer.setOutputFileName( file.fileName() );

        QPainter painter(&printer);

        kirjoittaja->tulosta(&printer, &painter);
        painter.end();

        model.liiteModel()->lisaaTiedosto( file.fileName(), tr("Alv-laskelma"));

        if( !model.tallenna() )
        {
            QMessageBox::critical(this, tr("Virhe alv-tilityksen tallentamisessa"),
                                  tr("Alv-tilityksen tallentuminen epäonnistui seuraavan "
                                     "tietokantavirheen takia: %1").arg( kp()->tietokanta()->lastError().text() ));
            return false;
        }

        // Laskelman erittely liitetään myös ...
        QTemporaryFile eriFile(QDir::tempPath() + "/alv-XXXXXX.pdf" );
        eriFile.open();
        eriFile.close();

        printer.setOutputFileName(eriFile.fileName());
        painter.begin(&printer);
        erittely(alkupvm,loppupvm).tulosta(&printer, &painter);
        painter.end();

        model.liiteModel()->lisaaTiedosto( eriFile.fileName(), tr("Alv-erittely"));
        model.tallenna();

        return true;
    }

    return false;
}

void AlvIlmoitusDialog::otsikko(const QString &teksti)
{
    RaporttiRivi rivi;
    kirjoittaja->lisaaRivi();
    rivi.lisaa(teksti);
    rivi.lihavoi();
    kirjoittaja->lisaaRivi(rivi);
}

void AlvIlmoitusDialog::luku(const QString &nimike, int senttia, bool viiva)
{
    RaporttiRivi rivi;
    rivi.lisaa(nimike);
    rivi.lisaa( senttia ,true);
    if( viiva )
        rivi.viivaYlle(true);
    kirjoittaja->lisaaRivi(rivi);
}

RaportinKirjoittaja AlvIlmoitusDialog::erittely(QDate alkupvm, QDate loppupvm)
{
    RaportinKirjoittaja kirjoittaja;
    kirjoittaja.asetaOtsikko("ARVONLISÄVEROLASKELMAN ERITTELY");
    kirjoittaja.asetaKausiteksti( QString("%1 - %2").arg(alkupvm.toString(Qt::SystemLocaleShortDate)).arg(loppupvm.toString(Qt::SystemLocaleShortDate) ) );

    kirjoittaja.lisaaPvmSarake();
    kirjoittaja.lisaaSarake("TOSITE12345");
    kirjoittaja.lisaaVenyvaSarake();
    kirjoittaja.lisaaSarake("24");
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
    int tilisumma = 0;
    int yhtsumma = 0;

    kysely.exec(kysymys);
    while(kysely.next())
    {
        int alvkoodi = kysely.value("alvkoodi").toInt();
        int alvprosentti = kysely.value("alvprosentti").toInt();
        int tilinro = kysely.value("nro").toInt();

        if( tilinro != nTili || alvkoodi != nAlvkoodi || alvprosentti != nProsentti)
        {
            if( tilisumma)
            {
                RaporttiRivi tiliSummaRivi;
                tiliSummaRivi.lisaa(" ", 3);
                tiliSummaRivi.lisaa( QString::number(nProsentti));
                tiliSummaRivi.lisaa( tilisumma);
                tiliSummaRivi.viivaYlle();
                kirjoittaja.lisaaRivi(tiliSummaRivi);

                if( (alvkoodi != nAlvkoodi || alvprosentti != nProsentti) && yhtsumma != tilisumma )
                {
                    // Lopuksi vielä lihavoituna kokonaissumma
                    RaporttiRivi summaRivi;
                    summaRivi.lisaa(" ", 3);
                    summaRivi.lisaa( QString::number(nProsentti));
                    summaRivi.lisaa( yhtsumma );
                    summaRivi.lihavoi();
                    kirjoittaja.lisaaRivi(summaRivi);
                }


                kirjoittaja.lisaaRivi();
                tilisumma = 0;
            }
        }


        if( alvkoodi != nAlvkoodi || alvprosentti != nProsentti)
        {

            RaporttiRivi koodiOtsikko;
            if( alvkoodi == AlvKoodi::MAKSETTAVAALV)
                koodiOtsikko.lisaa(tr("MAKSETTAVA VERO"), 3);
            else if( alvkoodi > AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON)
                koodiOtsikko.lisaa(tr("KOHDENTAMATON MAKSUPERUSTEINEN ARVONLISÄVERO"));
            else if( alvkoodi > AlvKoodi::ALVVAHENNYS)
                koodiOtsikko.lisaa( tr("VÄHENNYS: %1").arg( kp()->alvTyypit()->seliteKoodilla(alvkoodi - AlvKoodi::ALVVAHENNYS) ), 3 );
            else if( alvkoodi > AlvKoodi::ALVKIRJAUS)
                koodiOtsikko.lisaa( tr("VERO: %1").arg( kp()->alvTyypit()->seliteKoodilla(alvkoodi - AlvKoodi::ALVKIRJAUS) ), 3 );
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
            tiliOtsikko.lisaa( QString::number(alvprosentti));
            kirjoittaja.lisaaRivi(tiliOtsikko);
            nTili = tilinro;
        }

        RaporttiRivi rivi;
        rivi.lisaa( kysely.value("paiva").toDate() );
        rivi.lisaa( QString("%1%2").arg( kp()->tositelajit()->tositelaji( kysely.value("laji").toInt() ).tunnus() )
                    .arg( kysely.value("tunniste").toInt() ));
        rivi.lisaa( kysely.value("selite").toString());
        rivi.lisaa( QString::number(alvprosentti));

        int debetsnt = kysely.value("debetsnt").toInt();
        int kreditsnt = kysely.value("kreditsnt").toInt();
        int summa = kreditsnt - debetsnt;
        if( alvkoodi % 100 / 10 == 2)
            summa = debetsnt - kreditsnt;
        rivi.lisaa( summa );
        kirjoittaja.lisaaRivi( rivi );

        tilisumma += summa;
        yhtsumma += summa;

    }
    return kirjoittaja;

}
