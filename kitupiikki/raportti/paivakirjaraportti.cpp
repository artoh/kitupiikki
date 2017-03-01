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

#include <QDateEdit>

#include <QSqlQuery>

#include "paivakirjaraportti.h"

#include "db/kirjanpito.h"
#include "db/tilikausi.h"

#include "raportinkirjoittaja.h"

#include <QDebug>
#include <QSqlError>

PaivakirjaRaportti::PaivakirjaRaportti(QPrinter *printer)
    : Raportti(printer)
{
    ui = new Ui::Paivakirja;
    ui->setupUi( raporttiWidget );

    Tilikausi nykykausi = Kirjanpito::db()->tilikausiPaivalle( Kirjanpito::db()->paivamaara() );
    ui->alkupvm->setDate(nykykausi.alkaa());
    ui->loppupvm->setDate(nykykausi.paattyy());
}

PaivakirjaRaportti::~PaivakirjaRaportti()
{
    delete ui;
}


RaportinKirjoittaja PaivakirjaRaportti::raportti()
{

    RaportinKirjoittaja kirjoittaja;
    kirjoittaja.asetaOtsikko("PÄIVÄKIRJA");
    kirjoittaja.asetaKausiteksti(QString("%1 - %2").arg( ui->alkupvm->date().toString(Qt::SystemLocaleShortDate) )
                                             .arg( ui->loppupvm->date().toString(Qt::SystemLocaleShortDate) ) );

    kirjoittaja.lisaaPvmSarake();
    kirjoittaja.lisaaSarake("ABC1234 ");
    kirjoittaja.lisaaSarake("999999 Tilinnimitilinimi");
    kirjoittaja.lisaaSarake("Kohdennusnimi");
    kirjoittaja.lisaaVenyvaSarake();
    kirjoittaja.lisaaEurosarake();
    kirjoittaja.lisaaEurosarake();

    RaporttiRivi otsikko;
    otsikko.lisaa("Pvm");
    otsikko.lisaa("Tosite");
    otsikko.lisaa("Tili");
    otsikko.lisaa("Kohdennus");
    otsikko.lisaa("Selite");
    otsikko.lisaa("Debet €", 1, true);
    otsikko.lisaa("Kredit €", 1, true);
    kirjoittaja.lisaaOtsake(otsikko);

    QSqlQuery kysely;
    QString kysymys = QString("SELECT pvm, tositelaji, tunniste, tilinro, tilinimi, selite, debetsnt, kreditsnt, kohdennus, kohdennusId from vientivw "
                              "WHERE pvm BETWEEN \"%1\" AND \"%2\" ORDER BY pvm, vientiId")
                              .arg( ui->alkupvm->date().toString(Qt::ISODate) )
                              .arg( ui->loppupvm->date().toString(Qt::ISODate));

    kysely.exec(kysymys);


    while( kysely.next())
    {
        RaporttiRivi rivi;
        rivi.lisaa( kysely.value("pvm").toDate());
        rivi.lisaa( kysely.value("tositelaji").toString() + kysely.value("tunniste").toString());
        rivi.lisaa( tr("%1 %2").arg(kysely.value("tilinro").toString()).arg(kysely.value("tilinimi").toString()));

        if( kysely.value("kohdennusId").toInt() )
            rivi.lisaa( kysely.value("kohdennus").toString());
        else
            rivi.lisaa("");

        rivi.lisaa( kysely.value("selite").toString());
        rivi.lisaa( kysely.value("debetsnt").toInt());
        rivi.lisaa( kysely.value("kreditsnt").toInt());
        kirjoittaja.lisaaRivi( rivi );
    }

    return kirjoittaja;

}
