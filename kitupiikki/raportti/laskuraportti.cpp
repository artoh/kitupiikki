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

#include "laskuraportti.h"

#include "db/kirjanpito.h"

LaskuRaportti::LaskuRaportti()
{
    ui = new Ui::Laskuraportti;
    ui->setupUi( raporttiWidget );

    ui->alkaenPvm->setDate( kp()->tilikaudet()->kirjanpitoAlkaa() );
    ui->paattyenPvm->setDate( kp()->tilikaudet()->kirjanpitoLoppuu() );
    ui->saldoPvm->setDate( kp()->paivamaara() );

}

LaskuRaportti::~LaskuRaportti()
{
    delete ui;
}

RaportinKirjoittaja LaskuRaportti::raportti()
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

    return kirjoitaRaportti( ui->saldoPvm->date(), ui->avoimet->isChecked(),
                             lajittelu, rajaus, ui->alkaenPvm->date(), ui->paattyenPvm->date());

}

RaportinKirjoittaja LaskuRaportti::kirjoitaRaportti(QDate saldopvm, bool avoimet, LaskuRaportti::Lajittelu lajittelu, LaskuRaportti::PvmRajaus rajaus, QDate mista, QDate mihin)
{
    RaportinKirjoittaja rk;

    rk.asetaOtsikko("LASKUT");
    rk.asetaKausiteksti( saldopvm.toString(Qt::SystemLocaleShortDate) );

    rk.lisaaSarake("XXXXXXXXXX");  // Viite
    rk.lisaaPvmSarake();         // Laskupvm
    rk.lisaaPvmSarake();         // Eräpäivä
    rk.lisaaEurosarake();       // Summa
    rk.lisaaEurosarake();       // Avoinna
    rk.lisaaVenyvaSarake();     // Asiakas

    RaporttiRivi otsikko;
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

    QString ehto;
    if( rajaus == RajaaErapaiva )
            ehto = QString(" WHERE erapvm BETWEEN %1 and %2")
                    .arg( mista.toString(Qt::ISODate) ).arg( mihin.toString(Qt::ISODate));
    else if( rajaus == RajaaLaskupaiva )
            ehto = QString(" WHERE laskupvm BETWEEN %1 and %2")
                    .arg( mista.toString(Qt::ISODate) ).arg( mihin.toString(Qt::ISODate));

    QString kysymys = QString("SELECT id, tosite, laskupvm, erapvm, summaSnt, avoinSnt, asiakas, kirjausperuste, json from lasku "
                             " %1 order by %2").arg(ehto).arg(jarjestys);

    QSqlQuery kysely(kysymys);

    while( kysely.next() )
    {
        // Tässä vaiheessa haasteena on saada avoin summa ;)
        qlonglong avoinna = kysely.value("summaSnt").toLongLong();

        // TODO TODO TODO

        // Lopuksi tulostus
        if( avoimet && !avoinna)
            continue;

        RaporttiRivi rivi;
        rivi.lisaa( kysely.value("id").toString() );
        rivi.lisaa( kysely.value("laskupvm").toDate());
        rivi.lisaa( kysely.value("erapvm").toDate());
        rivi.lisaa( kysely.value("summaSnt").toLongLong());
        rivi.lisaa( avoinna);
        rivi.lisaa( kysely.value("asiakas").toString());
        rk.lisaaRivi(rivi);

    }

    return rk;
}
