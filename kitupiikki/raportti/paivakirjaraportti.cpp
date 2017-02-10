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

PaivakirjaRaportti::PaivakirjaRaportti()
{
    ui = new Ui::Paivakirja;
    ui->setupUi(this);
}

PaivakirjaRaportti::~PaivakirjaRaportti()
{
    delete ui;
}

void PaivakirjaRaportti::alustaLomake()
{
    Tilikausi nykykausi = Kirjanpito::db()->tilikausiPaivalle( Kirjanpito::db()->paivamaara() );
    ui->alkupvm->setDate(nykykausi.alkaa());
    ui->loppupvm->setDate(nykykausi.paattyy());
}

RaportinKirjoittaja PaivakirjaRaportti::raportti()
{

    RaportinKirjoittaja kirjoittaja;
    kirjoittaja.asetaOtsikko("PÄIVÄKIRJA");
    kirjoittaja.asetaKausiteksti(QString("%1 - %2").arg( ui->alkupvm->date().toString(Qt::SystemLocaleShortDate) )
                                             .arg( ui->loppupvm->date().toString(Qt::SystemLocaleShortDate) ) );

    kirjoittaja.lisaaPvmSarake();
    kirjoittaja.lisaaSarake("ABC12345 ");
    kirjoittaja.lisaaSarake("999999 Tilinnimi tarkentimineen");
    kirjoittaja.lisaaVenyvaSarake();
    kirjoittaja.lisaaEurosarake();
    kirjoittaja.lisaaEurosarake();

    RaporttiRivi otsikko;
    otsikko.lisaa("Pvm");
    otsikko.lisaa("Tosite");
    otsikko.lisaa("Tili");
    otsikko.lisaa("Selite");
    otsikko.lisaa("Debet €", 1, true);
    otsikko.lisaa("Kredit €", 1, true);
    kirjoittaja.lisaaOtsake(otsikko);

    QSqlQuery kysely;
    QString kysymys = QString("SELECT pvm, nro, debetsnt, kreditsnt, selite, tunniste, nimi, tyyppi, tositelaji from vientivw "
                              "WHERE pvm BETWEEN \"%1\" AND \"%2\" ORDER BY pvm")
                              .arg( ui->alkupvm->date().toString(Qt::ISODate) )
                              .arg( ui->loppupvm->date().toString(Qt::ISODate));
    kysely.exec(kysymys);

    while( kysely.next())
    {
        RaporttiRivi rivi;
        rivi.lisaa( kysely.value("pvm").toDate());
        rivi.lisaa( kysely.value("tositelaji").toString() + kysely.value("tunniste").toString());
        rivi.lisaa( tr("%1 %2").arg(kysely.value("nro").toString()).arg(kysely.value("nimi").toString()));
        rivi.lisaa( kysely.value("selite").toString());
        rivi.lisaa( kysely.value("debetsnt").toInt());
        rivi.lisaa( kysely.value("kreditsnt").toInt());
        kirjoittaja.lisaaRivi( rivi );
    }

    return kirjoittaja;

}
