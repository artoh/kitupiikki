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

#include "kirjauswg.h"
#include "vientimodel.h"
#include "tilidelegaatti.h"

#include "db/kirjanpito.h"

KirjausWg::KirjausWg(Kirjanpito *kp) : QWidget(), kirjanpito(kp)
{
    viennitModel = new VientiModel(kp, this);

    ui = new Ui::KirjausWg();
    ui->setupUi(this);

    ui->viennitView->setModel(viennitModel);
    ui->viennitView->setItemDelegateForColumn( VientiModel::TILI, new TiliDelegaatti(kp) );

    connect( ui->lisaaRiviNappi, SIGNAL(clicked(bool)), this, SLOT(lisaaRivi()));

    tyhjenna();
}

KirjausWg::~KirjausWg()
{
    delete ui;
    delete viennitModel;
}

QDate KirjausWg::tositePvm() const
{
    return ui->tositePvmEdit->date();
}

void KirjausWg::lisaaRivi()
{
    viennitModel->lisaaRivi();
    ui->viennitView->setFocus();
    ui->viennitView->setCurrentIndex( viennitModel->index( viennitModel->rowCount(QModelIndex()), VientiModel::TILI ) );
}

void KirjausWg::tyhjenna()
{
    ui->tositePvmEdit->setDate( kirjanpito->paivamaara() );
}
