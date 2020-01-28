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

#include "paakirjaraportti.h"

#include "db/kirjanpito.h"
#include "db/tilikausi.h"

#include "raportinkirjoittaja.h"

#include "paakirja.h"
#include "tools/tulkki.h"

PaakirjaRaportti::PaakirjaRaportti()
    : RaporttiWidget(nullptr)
{
    ui = new Ui::Paivakirja;
    ui->setupUi( raporttiWidget );

    Tilikausi nykykausi = Kirjanpito::db()->tilikausiPaivalle( Kirjanpito::db()->paivamaara() );
    if( !nykykausi.alkaa().isValid())
        nykykausi = kp()->tilikaudet()->tilikausiIndeksilla( kp()->tilikaudet()->rowCount(QModelIndex()) - 1 );

    ui->alkupvm->setDate(nykykausi.alkaa());
    ui->loppupvm->setDate(nykykausi.paattyy());
    ui->kohdennusCombo->valitseNaytettavat(KohdennusProxyModel::KAIKKI);

    if( kp()->kohdennukset()->rowCount() == 0) {
        ui->kohdennusCheck->setVisible(false);
        ui->kohdennusCombo->setVisible(false);
    }


    ui->jarjestysRyhma->hide();
    ui->ryhmittelelajeittainCheck->hide();

    connect( ui->alkupvm, &QDateEdit::dateChanged, this, &PaakirjaRaportti::haeTilitComboon);
    connect( ui->loppupvm, &QDateEdit::dateChanged, this, &PaakirjaRaportti::haeTilitComboon);
    haeTilitComboon();

    Tulkki::alustaKieliCombo(ui->kieliCombo);
}


void PaakirjaRaportti::haeTilitComboon()
{

    // Tässä pitäisi ehkä sittenkin käyttää summia ;)
    KpKysely *kysely = kpk("/saldot");
    kysely->lisaaAttribuutti("alkupvm", ui->alkupvm->date());
    kysely->lisaaAttribuutti("pvm", ui->loppupvm->date());
    connect(kysely, &KpKysely::vastaus, this, &PaakirjaRaportti::tiliListaSaapuu);
    kysely->kysy();

}

void PaakirjaRaportti::esikatsele()
{
    Paakirja *kirja = new Paakirja(this);
    connect( kirja, &Paakirja::valmis, this, &RaporttiWidget::nayta);

    int kohdennuksella = -1;
    if( ui->kohdennusCheck->isChecked())
        kohdennuksella = ui->kohdennusCombo->kohdennus();
    int tililta = 0;
    if( ui->tiliBox->isChecked())
        tililta = ui->tiliCombo->currentData().toInt();

    int optiot = 0;
    if( ui->tulostakohdennuksetCheck->isChecked() )
        optiot |= Paakirja::TulostaKohdennukset;
    if( ui->tulostasummat->isChecked() )
        optiot |= Paakirja::TulostaSummat;


    kirja->kirjoita(ui->alkupvm->date(), ui->loppupvm->date(), optiot,
                    kohdennuksella, tililta);

}

void PaakirjaRaportti::tiliListaSaapuu(QVariant *data)
{
    QVariantMap map = data->toMap();

    ui->tiliCombo->clear();
    for( QString tiliStr : map.keys()) {
        const Tili& tili = kp()->tilit()->tiliNumerolla( tiliStr.toInt() );
        ui->tiliCombo->addItem( tili.nimiNumero(), tili.numero() );
    }
    ui->tiliCombo->model()->sort(0);
}
