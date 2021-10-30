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

#include <QDebug>

#include "paakirjaraportti.h"
#include "db/kirjanpito.h"
#include "paakirja.h"
#include "ui_paivakirja.h"
#include "raporttivalinnat.h"

PaakirjaRaportti::PaakirjaRaportti()
    : PaakirjaPaivakirjaKantaRaporttiWidget()
{
    ui->jarjestysRyhma->hide();
    ui->ryhmittelelajeittainCheck->hide();    
    ui->eriPaivatCheck->hide();
    ui->laatuLabel->hide();
    ui->laatuSlider->hide();

    connect( ui->alkupvm, &QDateEdit::dateChanged, this, &PaakirjaRaportti::haeTilitComboon);
    connect( ui->loppupvm, &QDateEdit::dateChanged, this, &PaakirjaRaportti::haeTilitComboon);    
    haeTilitComboon();

}


void PaakirjaRaportti::haeTilitComboon()
{
    ui->kohdennusCombo->suodataValilla( ui->alkupvm->date(), ui->loppupvm->date() );

    // Tässä pitäisi ehkä sittenkin käyttää summia ;)
    KpKysely *kysely = kpk("/saldot");
    kysely->lisaaAttribuutti("alkupvm", ui->alkupvm->date());
    kysely->lisaaAttribuutti("pvm", ui->loppupvm->date());
    connect(kysely, &KpKysely::vastaus, this, &PaakirjaRaportti::tiliListaSaapuu);
    kysely->kysy();

}

void PaakirjaRaportti::tiliListaSaapuu(QVariant *data)
{
    QVariantMap map = data->toMap();

    ui->tiliCombo->clear();
    for( const QString& tiliStr : map.keys()) {
        const Tili& tili = kp()->tilit()->tiliNumerolla( tiliStr.toInt() );
        ui->tiliCombo->addItem( tili.nimiNumero(), tili.numero() );
    }
    ui->tiliCombo->model()->sort(0);
}

void PaakirjaRaportti::tallennaValinnat()
{

    kp()->raporttiValinnat()->aseta(RaporttiValinnat::Tyyppi, "paakirja");
}
