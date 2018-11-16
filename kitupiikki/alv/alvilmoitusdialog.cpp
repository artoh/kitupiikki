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

#include <QTextDocument>
#include <QMessageBox>
#include "alvilmoitusdialog.h"
#include "ui_alvilmoitusdialog.h"

#include "kirjaus/ehdotusmodel.h"
#include "db/kirjanpito.h"

#include "raportti/alverittely.h"


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
        QMessageBox::critical(nullptr, tr("Kitupiikin virhe"),
                              tr("Alv-tilitystä ei voi laatia, koska tilikartta on puutteellinen."));
        return QDate();
    }

    AlvIlmoitusDialog dlg;

    // Maksuperusteisen alv:n lopettaminen

    if( alkupvm == kp()->asetukset()->pvm("MaksuAlvLoppuu"))
    {
        if( !dlg.maksuperusteisenTilitys(kp()->asetukset()->pvm("MaksuAlvLoppuu"), alkupvm ) )
            return QDate();
    }

    // Erääntynyt (12kk) maksuperusteinen alv
    else if( kp()->onkoMaksuperusteinenAlv( loppupvm ) )
    {
        if( !dlg.maksuperusteisenTilitys( alkupvm.addYears(-1), alkupvm ) )
            return QDate();
    }


    if( dlg.alvIlmoitus(alkupvm, loppupvm))
        return loppupvm;
    else
        return QDate();
}

bool AlvIlmoitusDialog::alvIlmoitus(QDate alkupvm, QDate loppupvm)
{
    QMap<int,qlonglong> verotKannoittainSnt;  // verokanta - maksettava vero

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
        qlonglong veroSnt = qRound( ( alvprosentti * (double) saldoSnt ) / ( 100 + alvprosentti) );
        qlonglong nettoSnt = saldoSnt - veroSnt;

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

            rivi.selite = tr("Alv-kirjaus %1 - %2 %3 % vero (NETTO %L4 €, BRUTTO %L5€)").arg(alkupvm.toString("dd.MM.yyyy"))
                    .arg(loppupvm.toString("dd.MM.yyyy"))
                    .arg(alvprosentti)
                    .arg(nettoSnt / 100.0,0, 'f',2)
                    .arg(saldoSnt / 100.0,0, 'f', 2);            
            rivi.debetSnt = veroSnt;

            verorivi.tili = kp()->tilit()->tiliTyypilla(TiliLaji::ALVVELKA);
            verorivi.alvkoodi = AlvKoodi::MYYNNIT_BRUTTO + AlvKoodi::ALVKIRJAUS;

        }
        else
        {
            bruttovahennettavaaSnt += qAbs(veroSnt);

            rivi.selite = tr("Alv-kirjaus %1 - %2 %3 % vähennys (NETTO %L4 €, BRUTTO %L5€) ").arg(alkupvm.toString("dd.MM.yyyy"))
                    .arg(loppupvm.toString("dd.MM.yyyy"))
                    .arg(alvprosentti)
                    .arg(qAbs(nettoSnt) / 100.0,0, 'f',2)
                    .arg(qAbs(saldoSnt) / 100.0,0, 'f', 2);

            verorivi.tili = kp()->tilit()->tiliTyypilla(TiliLaji::ALVSAATAVA);
            verorivi.alvkoodi = AlvKoodi::OSTOT_BRUTTO + AlvKoodi::ALVVAHENNYS;
        }

        verorivi.selite = tr("%1 tilillä %2 %3")
                .arg(rivi.selite)
                .arg( tili.numero() )
                .arg( tili.nimi() );

        ehdotus.lisaaVienti(rivi);
        ehdotus.lisaaVienti(verorivi);
    }

    // 2) Nettokirjausten koonti
    query.exec( QString("select alvprosentti, sum(debetsnt) as debetit, sum(kreditsnt) as kreditit from vienti where pvm between \"%1\" and \"%2\" and (alvkoodi=%3 or alvkoodi=%4) group by alvprosentti")
                .arg(alkupvm.toString(Qt::ISODate)).arg(loppupvm.toString(Qt::ISODate))
                .arg(AlvKoodi::ALVKIRJAUS + AlvKoodi::MYYNNIT_NETTO).arg(AlvKoodi::ALVKIRJAUS + AlvKoodi::MAKSUPERUSTEINEN_MYYNTI) );

    while( query.next())
    {
        int alvprosentti = query.value("alvprosentti").toInt();
        int saldo = query.value("kreditit").toInt() - query.value("debetit").toInt();
        verotKannoittainSnt[ alvprosentti ] = verotKannoittainSnt.value(alvprosentti) + saldo;
    }


    // Muut kirjaukset tauluihin
    query.exec( QString("select alvkoodi, sum(debetsnt) as debetit, sum(kreditsnt) as kreditit from vienti where pvm between \"%1\" and \"%2\" group by alvkoodi")
                .arg(alkupvm.toString(Qt::ISODate)).arg(loppupvm.toString(Qt::ISODate)) );

    QMap<int,qlonglong> kooditaulu;

    int nettoverosnt = 0;
    int nettovahennyssnt = 0;

    while( query.next())
    {
        qlonglong saldo = query.value("kreditit").toLongLong() - query.value("debetit").toLongLong();
        int koodi = query.value("alvkoodi").toInt();

        if( koodi > AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON)
            continue;   // Ei kirjaus eikä vähennys
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
        rivi.selite = tr("Alv-kirjaus %1 - %2 ").arg(alkupvm.toString("dd.MM.yyyy")).arg(loppupvm.toString("dd.MM.yyyy"));
        rivi.debetSnt = nettoverosnt + bruttoveroayhtSnt;
        rivi.alvkoodi = AlvKoodi::TILITYS;
        ehdotus.lisaaVienti(rivi);
    }
    if( nettovahennyssnt + bruttovahennettavaaSnt)
    {
        VientiRivi rivi;
        rivi.pvm = loppupvm;
        rivi.tili = kp()->tilit()->tiliTyypilla(TiliLaji::ALVSAATAVA);
        rivi.selite = tr("Alv-kirjaus %1 - %2 ").arg(alkupvm.toString("dd.MM.yyyy")).arg(loppupvm.toString("dd.MM.yyyy"));
        rivi.kreditSnt = nettovahennyssnt + bruttovahennettavaaSnt;
        rivi.alvkoodi = AlvKoodi::TILITYS;
        ehdotus.lisaaVienti(rivi);
    }
    // Ja lopuksi kirjataan verot verotilille
    int maksettavavero = bruttoveroayhtSnt + nettoverosnt - bruttovahennettavaaSnt - nettovahennyssnt;
    if( maksettavavero )
    {
        VientiRivi rivi;
        rivi.pvm = loppupvm;
        rivi.tili = kp()->tilit()->tiliTyypilla(TiliLaji::VEROVELKA);
        if( maksettavavero > 0 )
        {
            rivi.selite = tr("Alv-ilmoituksella tilitettävä vero kaudelta %1 - %2").arg(alkupvm.toString("dd.MM.yyyy")).arg(loppupvm.toString("dd.MM.yyyy"));
            rivi.kreditSnt = maksettavavero;
        }
        else
        {
            // #219 Jos kokonaisuudessa palautettavaa, kirjataan se verosaataviin
            if( kp()->tilit()->tiliTyypilla(TiliLaji::VEROSAATAVA).onkoValidi() )
                rivi.tili = kp()->tilit()->tiliTyypilla(TiliLaji::VEROSAATAVA);

            rivi.selite = tr("Alv-palautus kaudelta %1 - %2").arg(alkupvm.toString("dd.MM.yyyy")).arg(loppupvm.toString("dd.MM.yyyy"));
            rivi.debetSnt = 0 - maksettavavero;
        }
        rivi.alvkoodi = AlvKoodi::MAKSETTAVAALV;
        ehdotus.lisaaVienti(rivi);
    }


    // Laskelman tulostus


    kirjoittaja = new RaportinKirjoittaja();
    kirjoittaja->asetaOtsikko("ARVONLISÄVEROLASKELMA");
    kirjoittaja->asetaKausiteksti( QString("%1 - %2").arg(alkupvm.toString("dd.MM.yyyy")).arg(loppupvm.toString("dd.MM.yyyy") ) );
    kirjoittaja->lisaaVenyvaSarake();
    kirjoittaja->lisaaEurosarake();

    if( kp()->onkoMaksuperusteinenAlv(loppupvm) )
        otsikko("Kotimaan arvonlisävero laskettu maksuperusteisesti.");

    otsikko("Vero kotimaan myynnistä verokannoittain");

    QMapIterator<int,qlonglong> iter(verotKannoittainSnt);
    iter.toBack();

    while( iter.hasPrevious())
    {
        iter.previous();
        luku(tr("%1 %:n vero").arg(iter.key()), iter.value() );
    }


    otsikko("Vero ostoista ja maahantuonneista");
    luku(tr("Vero tavaraostoista muista EU-maista"), kooditaulu.value(AlvKoodi::ALVKIRJAUS + AlvKoodi::YHTEISOHANKINNAT_TAVARAT)  );
    luku(tr("Vero palveluostoista muista EU-maista"), kooditaulu.value(AlvKoodi::ALVKIRJAUS + AlvKoodi::YHTEISOHANKINNAT_PALVELUT)  );
    luku(tr("Vero tavaroiden maahantuonnista EU:n ulkopuolelta"), kooditaulu.value(AlvKoodi::ALVKIRJAUS + AlvKoodi::MAAHANTUONTI) );
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
    luku(tr("Rakentamispalveluiden ja metalliromun myynnit"), kooditaulu.value(AlvKoodi::RAKENNUSPALVELU_MYYNTI) );
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
        qlonglong kuukausiaLaskelmassa = laskelmaMista.daysTo(loppupvm) / 30;

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

        qlonglong suhteutettu = liikevaihto;

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


    ui->ilmoitusBrowser->setHtml( kirjoittaja->html() + "<hr>" + AlvErittely::kirjoitaRaporti(alkupvm, loppupvm).html());
    if( exec() )
    {
        // Laskelma vahvistettu, tallennetaan tositteeksi
        TositeModel model( kp()->tietokanta());
        model.asetaPvm( loppupvm );
        model.asetaTositelaji(0);
        model.asetaOtsikko( tr("Arvonlisävero %1 - %2").arg(alkupvm.toString("dd.MM.yyyy")).arg(loppupvm.toString("dd.MM.yyyy")) );

        model.json()->set("AlvTilitysAlkaa", alkupvm);
        model.json()->set("AlvTilitysPaattyy", loppupvm);
        model.json()->set("MaksettavaAlv", maksettavavero);
        ehdotus.tallenna( model.vientiModel() );

        // Liitetään laskelma
        model.liiteModel()->lisaaLiite( kirjoittaja->pdf(false, false), tr("Alv-laskelma") );

        // Tallennetaan laskelma, jotta erittelyssä olisi myös bruttokirjaukset
        model.tallenna();

        // Laskelman erittely liitetään myös ...
        model.liiteModel()->lisaaLiite( AlvErittely::kirjoitaRaporti(alkupvm, loppupvm).pdf(false, false), tr("Alv-erittely") );


        if( !model.tallenna() )
        {
            QMessageBox::critical(this, tr("Virhe alv-tilityksen tallentamisessa"),
                                  tr("Alv-tilityksen tallentuminen epäonnistui seuraavan "
                                     "tietokantavirheen takia: %1").arg( kp()->tietokanta()->lastError().text() ));
            return false;
        }

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

void AlvIlmoitusDialog::luku(const QString &nimike, qlonglong senttia, bool viiva)
{
    RaporttiRivi rivi;
    rivi.lisaa(nimike);
    rivi.lisaa( senttia ,true);
    if( viiva )
        rivi.viivaYlle(true);
    kirjoittaja->lisaaRivi(rivi);
}

bool AlvIlmoitusDialog::maksuperusteisenTilitys(const QDate &paivayksesta, const QDate &tilityspvm)
{
    // Hakee kaikki sanottua vanhemmat erät ja jos niillä saldoa, niin lävähtävät maksuun
    QSqlQuery kysely( QString("SELECT id, alvkoodi, alvprosentti FROM vienti WHERE (tili=%1 OR tili=%2) "
                              "AND pvm <='%3'")
                      .arg( kp()->tilit()->tiliTyypilla(TiliLaji::KOHDENTAMATONALVVELKA).id() )
                      .arg( kp()->tilit()->tiliTyypilla(TiliLaji::KOHDENTAMATONALVSAATAVA).id())
                      .arg( paivayksesta.toString(Qt::ISODate)));

    EhdotusModel ehdotus;

    while( kysely.next())
    {
        int alvkoodi = kysely.value("alvkoodi").toInt();
        if( alvkoodi != AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON + AlvKoodi::MAKSUPERUSTEINEN_MYYNTI &&
            alvkoodi != AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON + AlvKoodi::MAKSUPERUSTEINEN_OSTO )
            continue;

        TaseEra veroEra( kysely.value("id").toInt() );
        qlonglong saldo = veroEra.saldoSnt;

        if( !saldo )
            continue;

        // Kirjataan kohdentamattomasta alv-velasta (saatavasta) alv-velkaan (saatavaan)

        VientiRivi kohdentamaton;
        kohdentamaton.pvm = tilityspvm;
        kohdentamaton.tili = alvkoodi == AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON + AlvKoodi::MAKSUPERUSTEINEN_OSTO ?
                    kp()->tilit()->tiliTyypilla(TiliLaji::KOHDENTAMATONALVSAATAVA) :
                    kp()->tilit()->tiliTyypilla(TiliLaji::KOHDENTAMATONALVVELKA);
        kohdentamaton.kreditSnt = saldo > 0 ? saldo : 0;
        kohdentamaton.debetSnt = saldo < 0 ? 0 - saldo : 0;
        kohdentamaton.alvkoodi = AlvKoodi::TILITYS;
        kohdentamaton.eraId = kysely.value("id").toInt();
        kohdentamaton.selite = tr("Maksuperusteinen %1 % alv %2 / %3 [%4]").arg( kysely.value("alvprosentti").toInt() )
                .arg(veroEra.tositteenTunniste()).arg(veroEra.pvm.toString("dd.MM.yyyy"))
                .arg(veroEra.selite);

        VientiRivi verorivi;
        verorivi.pvm = tilityspvm;
        verorivi.tili = alvkoodi == AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON + AlvKoodi::MAKSUPERUSTEINEN_OSTO ?
                    kp()->tilit()->tiliTyypilla(TiliLaji::ALVSAATAVA) :
                    kp()->tilit()->tiliTyypilla(TiliLaji::ALVVELKA);
        verorivi.debetSnt = kohdentamaton.kreditSnt;
        verorivi.kreditSnt = kohdentamaton.debetSnt;
        verorivi.selite = kohdentamaton.selite;
        verorivi.alvkoodi = alvkoodi == AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON + AlvKoodi::MAKSUPERUSTEINEN_OSTO ?
                    AlvKoodi::ALVVAHENNYS + AlvKoodi::MAKSUPERUSTEINEN_OSTO :
                    AlvKoodi::ALVKIRJAUS + AlvKoodi::MAKSUPERUSTEINEN_MYYNTI;
        verorivi.alvprosentti = kysely.value("alvprosentti").toInt();

        ehdotus.lisaaVienti(kohdentamaton);
        ehdotus.lisaaVienti(verorivi);
    }

    // Jos erääntyneitä on, niin pyydetään lupa niiden kirjaamiseen
    if( ehdotus.rowCount(QModelIndex()))
    {
        if( QMessageBox::question(nullptr, tr("Arvonlisäveron kausi-ilmoitus"),
                                 tr("Tähän verotusjaksoon on kohdistettava %1 kpl erääntynyttä maksuperusteisena kirjattua "
                                    "arvonlisäveron suoritusta.\n"
                                    "Tehdäänkö kohdistuskirjaukset ja jatketaan arvonlisäverotukseen?")
                                        .arg( ehdotus.rowCount(QModelIndex()) / 2),
                              QMessageBox::Yes | QMessageBox::Cancel ) != QMessageBox::Yes )
            return false;

        // Tehdään kirjaaja
        TositeModel tosite( kp()->tietokanta() );
        tosite.asetaPvm(tilityspvm);
        tosite.asetaOtsikko(tr("Erääntynyt maksuperusteinen arvonlisävero"));
        tosite.asetaTositelaji(0);

        ehdotus.tallenna( tosite.vientiModel() );
        return tosite.tallenna();
    }

    return true;

}

