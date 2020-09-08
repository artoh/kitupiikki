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
#include "alvraporttiwidget.h"
#include "alv/alvlaskelma.h"

AlvRaporttiWidget::AlvRaporttiWidget() :
    RaporttiWidget(nullptr),
    ui( new Ui::PvmVali)
{
    ui->setupUi(raporttiWidget);

    QDate pvm = QDate::currentDate().addMonths(-1);
    QDate alku = QDate( pvm.year(), pvm.month(), 1 );

    ui->alkaa->setDate(alku);
    ui->paattyy->setDate( alku.addMonths(1).addDays(-1)  );
}

AlvRaporttiWidget::~AlvRaporttiWidget()
{
    delete ui;
}

void AlvRaporttiWidget::esikatsele()
{
    AlvLaskelma *laskelma = new AlvLaskelma(this);
    connect( laskelma, &AlvLaskelma::valmis, this, &RaporttiWidget::nayta);
    laskelma->laske( ui->alkaa->date(), ui->paattyy->date());
}
