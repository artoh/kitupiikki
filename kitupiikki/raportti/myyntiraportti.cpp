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

#include "myyntiraportti.h"

#include "ui_taseerittely.h"
#include "db/kirjanpito.h"

#include <QSqlQuery>
#include <QMap>
#include <QJsonDocument>
#include <QVariant>


MyyntiRaportti::MyyntiRaportti()
{
    ui = new Ui::TaseErittely;
    ui->setupUi( raporttiWidget );

    Tilikausi kausi = kp()->tilikausiPaivalle( kp()->paivamaara() );

    ui->alkaa->setDate(kausi.alkaa());
    ui->paattyy->setDate(kausi.paattyy());
}

MyyntiRaportti::~MyyntiRaportti()
{
    delete ui;
}

RaportinKirjoittaja MyyntiRaportti::raportti(bool /* csvmuoto */)
{
    return kirjoitaRaportti(ui->alkaa->date(), ui->paattyy->date());
}

RaportinKirjoittaja MyyntiRaportti::kirjoitaRaportti(QDate mista, QDate mihin)
{

    RaportinKirjoittaja rk;
    rk.asetaOtsikko("MYYNTI");
    rk.asetaKausiteksti(QString("%1 - %2").arg(mista.toString("dd.MM.yyyy")).arg(mihin.toString("dd.MM.yyyy")));

    //   Tuote    kpl     á hinta     hinta yht
    //
    rk.lisaaVenyvaSarake();
    rk.lisaaSarake("999999.99");
    rk.lisaaEurosarake();
    rk.lisaaEurosarake();

    // Otsikot
    {
        RaporttiRivi otsikko;
        otsikko.lisaa("Tuote");
        otsikko.lisaa("Kpl");
        otsikko.lisaa("á netto");
        otsikko.lisaa("Yht netto");
        rk.lisaaOtsake(otsikko);
    }

    QMap<QString,double> myyntiKpl;
    QMap<QString,qlonglong> myyntiSnt;

    QSqlQuery kysely;
    kysely.exec(QString("pvm, json from vienti where viite is not null and pvm between '%1' and '%2'")
                .arg(mista.toString(Qt::ISODate)).arg(mihin.toString(Qt::ISODate)));

    while( kysely.next())
    {
        QDate pvm = kysely.value(0).toDate();
        QJsonDocument json = QJsonDocument::fromJson( kysely.value(1).toByteArray() );
        QVariantMap vmap = json.toVariant().toMap();
        if( vmap.contains("Erittely"))
        {
            for( QVariant var : vmap.value("Erittely").toList())
            {
                QString nimike = var.toMap().value("Selite").toString();

                double maara = var.toMap().value("Maara").toDouble();
                qlonglong sentit = var.toMap().value("Nettoyht").toLongLong();

                myyntiKpl[nimike] = myyntiKpl.value(nimike,0.0) + maara;
                myyntiSnt[nimike] = myyntiSnt.value(nimike, sentit) + maara;
            }
        }
    }

    QMapIterator iter(myyntiKpl);
    while( iter.next() )
    {
        RaporttiRivi rivi;
        rivi.lisaa( iter.key());

        double kpl = iter.value();
        qlonglong snt = myyntiSnt.value(iter.key());

        rivi.lisaa( QString("%L1").arg(kpl,0,'f',2), 1, true );
        rivi.lisaa( snt / kpl);
        rivi.lisaa( snt );
        rk.lisaaRivi(rivi);
    }

    return rk;
}


