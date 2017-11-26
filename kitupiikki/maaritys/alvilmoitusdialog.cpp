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

#include <QTemporaryFile>
#include <QTextDocument>

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

    query.exec(  QString("select alvkoodi,alvprosentti,sum(debetsnt) as debetit, sum(kreditsnt) as kreditit, tili from vienti where pvm between \"%3\" and \"%4\" and (alvkoodi=%1 or alvkoodi=%2) group by alvkoodi,tili,alvprosentti")
                 .arg(AlvKoodi::MYYNNIT_BRUTTO).arg(AlvKoodi::OSTOT_BRUTTO)
                 .arg(alkupvm.toString(Qt::ISODate)).arg(loppupvm.toString(Qt::ISODate)));

    qDebug() << query.lastQuery();

    while( query.next() && query.value("alvprosentti").toInt())
    {

        Tili tili = kp()->tilit()->tiliIndeksilla( query.value("tili").toInt() );
        int alvprosentti = query.value("alvprosentti").toInt();
        int saldoSnt = query.value("kreditit").toInt() - query.value("debetit").toInt();

        VientiRivi rivi;
        rivi.pvm = loppupvm;
        rivi.tili = tili;
        rivi.alvprosentti = alvprosentti;
        // Brutosta erotetaan verot
        int veroSnt = ( alvprosentti * saldoSnt ) / ( 100 + alvprosentti) ;
        int nettoSnt = saldoSnt - veroSnt;


        if( query.value("alvkoodi").toInt() == AlvKoodi::MYYNNIT_BRUTTO )
        {
            verotKannoittainSnt[ alvprosentti ] = verotKannoittainSnt.value(alvprosentti, 0) + veroSnt;
            bruttoveroayhtSnt += veroSnt;

            rivi.selite = tr("Alv-kirjaus %1 - %2 %3 % vero (NETTO %L4 €, BRUTTO %L5€) ").arg(alkupvm.toString(Qt::SystemLocaleShortDate))
                    .arg(loppupvm.toString(Qt::SystemLocaleShortDate))
                    .arg(alvprosentti)
                    .arg(nettoSnt / 100.0,0, 'f',2)
                    .arg(saldoSnt / 100.0,0, 'f', 2);
            rivi.debetSnt = veroSnt;

        }
        else
        {
            bruttovahennettavaaSnt += qAbs(veroSnt);

            rivi.selite = tr("Alv-kirjaus %1 - %2 %3 % vähennys (NETTO %L4 €, BRUTTO %L5€) ").arg(alkupvm.toString(Qt::SystemLocaleShortDate))
                    .arg(loppupvm.toString(Qt::SystemLocaleShortDate))
                    .arg(alvprosentti)
                    .arg(qAbs(nettoSnt) / 100.0,0, 'f',2)
                    .arg(qAbs(saldoSnt) / 100.0,0, 'f', 2);
            rivi.kreditSnt = veroSnt;
        }
    qDebug() << rivi.selite;
        ehdotus.lisaaVienti(rivi);
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
    if( nettoverosnt )
    {
        VientiRivi rivi;
        rivi.pvm = loppupvm;
        rivi.tili = kp()->tilit()->tiliTyypilla(TiliLaji::ALVVELKA);
        rivi.selite = tr("Alv-kirjaus %1 - %2 ").arg(alkupvm.toString(Qt::SystemLocaleShortDate)).arg(loppupvm.toString(Qt::SystemLocaleShortDate));
        rivi.debetSnt = nettoverosnt;
        ehdotus.lisaaVienti(rivi);
    }
    if( nettovahennyssnt )
    {
        VientiRivi rivi;
        rivi.pvm = loppupvm;
        rivi.tili = kp()->tilit()->tiliTyypilla(TiliLaji::ALVSAATAVA);
        rivi.selite = tr("Alv-kirjaus %1 - %2 ").arg(alkupvm.toString(Qt::SystemLocaleShortDate)).arg(loppupvm.toString(Qt::SystemLocaleShortDate));
        rivi.kreditSnt = nettovahennyssnt;
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
        rivi.alvkoodi = AlvKoodi::ALVKIRJAUS;
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
