/*
   Copyright (C) 2019 Arto Hyvättinen

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
#include "uudelleennumerointi.h"
#include "ui_uudelleennumerointi.h"

#include "db/kirjanpito.h"

Uudelleennumerointi::Uudelleennumerointi(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Uudelleennumerointi)
{
    ui->setupUi(this);
}

Uudelleennumerointi::~Uudelleennumerointi()
{
    delete ui;
}

void Uudelleennumerointi::numeroiUudelleen(const Tilikausi &kausi)
{
    Uudelleennumerointi numerointi;
    numerointi.ui->alkuPvm->setDate(kausi.alkaa());
    numerointi.ui->loppuPvm->setDate(kausi.paattyy());

    numerointi.ui->alkuPvm->setDateRange(kausi.alkaa(), kausi.paattyy());
    numerointi.ui->loppuPvm->setDateRange(kausi.alkaa(), kausi.paattyy());

    numerointi.exec();

}

void Uudelleennumerointi::accept()
{
    QVariantMap map;
    map.insert("alkaa", ui->alkuPvm->date());
    map.insert("loppuu", ui->loppuPvm->date());

    KpKysely *kysely = kpk("/tilikaudet/numerointi", KpKysely::POST);
    connect( kysely, &KpKysely::vastaus, this, &Uudelleennumerointi::valmis);
    kysely->kysy(map);
}

void Uudelleennumerointi::valmis()
{
    QDialog::accept();
    emit kp()->onni(tr("Tositteet numeroitu uudelleen"));
}
