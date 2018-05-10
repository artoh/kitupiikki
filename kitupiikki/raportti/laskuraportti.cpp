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
#include <QDebug>

#include "laskuraportti.h"

#include "db/kirjanpito.h"

LaskuRaportti::LaskuRaportti()
{
    ui = new Ui::Laskuraportti;
    ui->setupUi( raporttiWidget );

    ui->alkaenPvm->setDate( kp()->tilikaudet()->kirjanpitoAlkaa() );
    ui->paattyenPvm->setDate( kp()->tilikaudet()->kirjanpitoLoppuu() );
    ui->saldoPvm->setDate( kp()->paivamaara() );

    connect( ui->myyntiRadio, SIGNAL(toggled(bool)), this, SLOT(tyyppivaihtuu()));

}

LaskuRaportti::~LaskuRaportti()
{
    delete ui;
}

RaportinKirjoittaja LaskuRaportti::raportti(bool csvmuoto)
{
    PvmRajaus rajaus = KaikkiLaskut;
    if( ui->rajaaEra->isChecked())
        rajaus = RajaaErapaiva;
    else if( ui->rajaaPvm->isChecked())
        rajaus = RajaaLaskupaiva;

    Lajittelu lajittelu = Laskupaiva;

    if( ui->lajitteleViite->isChecked())
        lajittelu = Viitenumero;
    else if( ui->lajitteleErapvm->isChecked())
        lajittelu = Erapaiva;
    else if( ui->lajitteleSumma->isChecked())
        lajittelu = Summa;
    else if( ui->lajitteleAsiakas->isChecked())
        lajittelu = Asiakas;

    return kirjoitaRaportti( ui->saldoPvm->date(), ui->myyntiRadio->isChecked(),
                             ui->avoimet->isChecked(),
                             lajittelu, ui->summaBox->isChecked() && !csvmuoto,  ui->viiteBox->isChecked(), rajaus, ui->alkaenPvm->date(), ui->paattyenPvm->date());

}

RaportinKirjoittaja LaskuRaportti::kirjoitaRaportti(QDate saldopvm, bool myyntilaskuja, bool avoimet, LaskuRaportti::Lajittelu lajittelu, bool summat, bool viitteet, LaskuRaportti::PvmRajaus rajaus, QDate mista, QDate mihin)
{
    if( myyntilaskuja)
        return myyntilaskut(saldopvm, avoimet, lajittelu, summat, viitteet, rajaus, mista, mihin);
    else
        return ostolaskut(saldopvm, avoimet, lajittelu, summat, viitteet, rajaus, mista, mihin);
}


RaportinKirjoittaja LaskuRaportti::myyntilaskut(QDate saldopvm, bool avoimet, LaskuRaportti::Lajittelu lajittelu, bool summat, bool viitteet, LaskuRaportti::PvmRajaus rajaus, QDate mista, QDate mihin)
{
    RaportinKirjoittaja rk;

    rk.asetaOtsikko("MYYNTILASKUT");

    rk.asetaKausiteksti( saldopvm.toString("dd.MM.yyyy") );

    if( viitteet)
        rk.lisaaSarake("XXXXXXXXXX");  // Viite

    rk.lisaaPvmSarake();         // Laskupvm
    rk.lisaaPvmSarake();         // Eräpäivä
    rk.lisaaEurosarake();       // Summa
    rk.lisaaEurosarake();       // Avoinna
    rk.lisaaVenyvaSarake();     // Asiakas

    RaporttiRivi otsikko;
    if( viitteet )
        otsikko.lisaa("Viitenro");

    otsikko.lisaa("Laskupvm");
    otsikko.lisaa("Eräpvm");
    otsikko.lisaa("Summa", 1, true);
    otsikko.lisaa("Maksamatta", 1, true);
    otsikko.lisaa("Asiakas");

    rk.lisaaOtsake(otsikko);


    QString jarjestys = "laskupvm";
    if( lajittelu == Viitenumero)
        jarjestys = "id";
    else if( lajittelu == Erapaiva)
        jarjestys = "erapvm";
    else if( lajittelu == Summa)
        jarjestys = "summaSnt";
    else if( lajittelu == Asiakas)
        jarjestys = "asiakas";


    qlonglong laskusumma = 0;
    qlonglong avoinsumma = 0;

    QString kysymys = QString("SELECT pvm, debetsnt, kreditsnt, eraid, viite, erapvm, asiakas, laskupvm FROM vienti LEFT OUTER JOIN tili ON vienti.tili=tili.id "
                     "WHERE ((viite IS NOT NULL AND iban IS NULL) OR (tyyppi='AO' and vienti.id=vienti.eraid) ) ");

    if( rajaus == RajaaErapaiva)
        kysymys.append( QString(" AND erapvm BETWEEN '%1' AND '%2' ") .arg(mista.toString(Qt::ISODate)).arg(mihin.toString(Qt::ISODate)) );
    else if( rajaus == RajaaLaskupaiva)
        kysymys.append( QString(" AND laskupvm BETWEEN '%1' AND '%2' ") .arg(mista.toString(Qt::ISODate)).arg(mihin.toString(Qt::ISODate)) );

    kysymys.append(" ORDER BY " + jarjestys);


    QSqlQuery kysely(kysymys);

    while( kysely.next() )
    {
        qlonglong avoinna = 0;

        if( kysely.value("eraid").toInt() )
        {
            QString erakysymys = QString("SELECT sum(debetsnt) as debet, sum(kreditsnt) as kredit FROM vienti WHERE eraid=%1 AND pvm<='%2'")
                    .arg( kysely.value("eraid").toInt())
                    .arg( saldopvm.toString(Qt::ISODate));

            QSqlQuery erakysely( erakysymys);
            if( erakysely.next())
            {
                avoinna -= erakysely.value("kredit").toLongLong();
                avoinna += erakysely.value("debet").toLongLong();
            }

        }

        // Lopuksi tulostus
        if( avoimet && !avoinna)
            continue;

        RaporttiRivi rivi;
        if( viitteet)
            rivi.lisaa( kysely.value("viite").toString() );

        rivi.lisaa( kysely.value("laskupvm").toDate());
        rivi.lisaa( kysely.value("erapvm").toDate());
        rivi.lisaa( kysely.value("debetsnt").toLongLong());
        rivi.lisaa( avoinna);
        rivi.lisaa( kysely.value("asiakas").toString());
        rk.lisaaRivi(rivi);

        laskusumma += kysely.value("debetsnt").toLongLong();
        avoinsumma += avoinna;

    }

    if( summat )
    {
        RaporttiRivi summarivi;
        if( viitteet)
            summarivi.lisaa("Yhteensä",3);
        else
            summarivi.lisaa("Yhteensä",2);

        summarivi.lisaa( laskusumma );
        summarivi.lisaa( avoinsumma );
        summarivi.lisaa("");
        summarivi.viivaYlle();
        rk.lisaaRivi(summarivi);
    }

    return rk;
}

RaportinKirjoittaja LaskuRaportti::ostolaskut(QDate saldopvm, bool avoimet, LaskuRaportti::Lajittelu lajittelu, bool summat, bool viitteet, LaskuRaportti::PvmRajaus rajaus, QDate mista, QDate mihin)
{
    RaportinKirjoittaja rk;

    rk.asetaOtsikko("OSTOLASKUT");

    rk.asetaKausiteksti( saldopvm.toString("dd.MM.yyyy") );

    if( viitteet )
    {
        rk.lisaaSarake("FI00000000000000000000"); // Tili
        rk.lisaaSarake("XXXXXXXXXX");  // Viite
    }
    rk.lisaaPvmSarake();         // Laskupvm
    rk.lisaaPvmSarake();         // Eräpäivä
    rk.lisaaEurosarake();       // Summa
    rk.lisaaEurosarake();       // Avoinna
    rk.lisaaVenyvaSarake();     // Myyjä

    RaporttiRivi otsikko;
    if( viitteet )
    {
        otsikko.lisaa("Tilille");
        otsikko.lisaa("Viitenro");
    }
    otsikko.lisaa("Laskupvm");
    otsikko.lisaa("Eräpvm");
    otsikko.lisaa("Summa", 1, true);
    otsikko.lisaa("Maksamatta", 1, true);
    otsikko.lisaa("Myyjä/Selite");

    rk.lisaaOtsake(otsikko);

    // Rivit laitetaan QMapiin ja alkuun niiden lajitteluavain
    QMultiMap<QString, RaporttiRivi> rivit;


    qlonglong laskusumma = 0;
    qlonglong avoinsumma = 0;


    QString ehto;
    if( rajaus == RajaaErapaiva )
            ehto = QString(" erapvm BETWEEN %1 and %2 AND")
                    .arg( mista.toString(Qt::ISODate) ).arg( mihin.toString(Qt::ISODate));
    else if( rajaus == RajaaLaskupaiva )
            ehto = QString(" pvm BETWEEN %1 and %2 AND")
                    .arg( mista.toString(Qt::ISODate) ).arg( mihin.toString(Qt::ISODate));

    QString kysymys = QString("SELECT vienti.id, pvm, kreditsnt, viite, iban, erapvm, vienti.json, selite FROM vienti,tili WHERE "
                             " %1 vienti.tili=tili.id AND tili.tyyppi='BO' AND kreditsnt > 0  ").arg(ehto);

    QSqlQuery kysely(kysymys);

    while( kysely.next() )
    {
        qlonglong avoinna = kysely.value("kreditsnt").toLongLong();
        JsonKentta json( kysely.value("vienti.json").toByteArray() );

        // Nyt pitää hakea tähän tase-erään tulevat muutokset ko. päivään asti
        QSqlQuery erakysely( QString("SELECT debetsnt, kreditsnt FROM vienti "
                         "WHERE eraid=%1 AND pvm <= '%2'")
                             .arg( kysely.value("vienti.id").toInt()  ).arg(saldopvm.toString(Qt::ISODate)));

        qDebug() << erakysely.lastQuery();

        while( erakysely.next())
        {
            avoinna += erakysely.value("kreditsnt").toLongLong();
            avoinna -= erakysely.value("debetsnt").toLongLong();
        }

        // Lopuksi tulostus
        if( avoimet && !avoinna)
            continue;

        QString saaja = json.str("SaajanNimi");
        if( saaja.isEmpty())
            saaja = kysely.value("selite").toString();

        RaporttiRivi rivi;
        if(viitteet)
        {
            rivi.lisaa( kysely.value("iban").toString());
            rivi.lisaa( kysely.value("viite").toString() );
        }
        rivi.lisaa( kysely.value("pvm").toDate());
        rivi.lisaa( kysely.value("erapvm").toDate());
        rivi.lisaa( kysely.value("kreditsnt").toLongLong());
        rivi.lisaa( avoinna);
        rivi.lisaa( saaja );

        laskusumma += kysely.value("kreditsnt").toLongLong();
        avoinsumma += avoinna;

        QString avain = kysely.value("pvm").toDate().toString(Qt::ISODate);
        if( lajittelu == Erapaiva)
            avain = kysely.value("erapvm").toDate().toString(Qt::ISODate);
        else if( lajittelu == Summa)
            avain = QString("%1").arg(kysely.value("kreditSnt").toLongLong(), 10, 10, QChar('0'));
        else if( lajittelu == Asiakas)
            avain = json.str(saaja);

        rivit.insert( avain, rivi);
    }

    // Laitetaan nyt rivit järjestyksessä
    for( RaporttiRivi rivi : rivit.values())
        rk.lisaaRivi(rivi);

    if( summat )
    {
        RaporttiRivi summarivi;
        if( viitteet )
            summarivi.lisaa("Yhteensä",4);
        else
            summarivi.lisaa("Yhteensä",2);
        summarivi.lisaa( laskusumma );
        summarivi.lisaa( avoinsumma );
        summarivi.lisaa(" ");
        summarivi.viivaYlle();
        rk.lisaaRivi(summarivi);
    }

    return rk;
}


void LaskuRaportti::tyyppivaihtuu()
{
    ui->lajitteleViite->setEnabled( ui->myyntiRadio->isChecked() );
    if( ui->myyntiRadio->isChecked())
        ui->lajitteleAsiakas->setText( tr("Asiakas"));
    else
        ui->lajitteleAsiakas->setText(tr("Myyjä"));
}
