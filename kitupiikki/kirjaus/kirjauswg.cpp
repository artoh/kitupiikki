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
#include "eurodelegaatti.h"

#include "db/kirjanpito.h"

#include <QDebug>
#include <QSqlQuery>

KirjausWg::KirjausWg(Kirjanpito *kp) : QWidget(), kirjanpito(kp), tositeId(0)
{
    viennitModel = new VientiModel(kp, this);

    ui = new Ui::KirjausWg();
    ui->setupUi(this);

    ui->viennitView->setModel(viennitModel);
    connect( viennitModel, SIGNAL(siirrySarakkeeseen(QModelIndex)), ui->viennitView, SLOT(setCurrentIndex(QModelIndex)));
    connect( viennitModel, SIGNAL(siirrySarakkeeseen(QModelIndex)), ui->viennitView, SLOT(edit(QModelIndex)));

    ui->viennitView->setItemDelegateForColumn( VientiModel::TILI, new TiliDelegaatti(kp) );

    ui->viennitView->setItemDelegateForColumn( VientiModel::DEBET, new EuroDelegaatti);
    ui->viennitView->setItemDelegateForColumn( VientiModel::KREDIT, new EuroDelegaatti);

    ui->viennitView->hideColumn(VientiModel::PROJEKTI);
    ui->viennitView->hideColumn(VientiModel::KUSTANNUSPAIKKA);
    ui->viennitView->horizontalHeader()->setStretchLastSection(true);

    connect( ui->lisaaRiviNappi, SIGNAL(clicked(bool)), this, SLOT(lisaaRivi()));
    connect( ui->tallennaButton, SIGNAL(clicked(bool)), this, SLOT(tallenna()));

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

    QModelIndex indeksi = viennitModel->index( viennitModel->rowCount(QModelIndex()) - 1, VientiModel::TILI );

    ui->viennitView->setCurrentIndex( indeksi  );
    ui->viennitView->edit( indeksi );
}

void KirjausWg::tyhjenna()
{
    tositeId = 0;
    ui->tositePvmEdit->setDate( kirjanpito->paivamaara() );
}

void KirjausWg::tallenna()
{
    if( tositeId )
    {
        // update
    }
    else
    {
        QSqlQuery query;
        query.prepare("INSERT INTO tosite(pvm,otsikko,kommentti) values(:pvm,:otsikko,:kommentti)");
        query.bindValue(":pvm", ui->tositePvmEdit->date());
        query.bindValue(":otsikko", ui->otsikkoEdit->text());
        query.bindValue(":kommentti", ui->kommentitEdit->document()->toPlainText());
        query.exec();

        tositeId = query.lastInsertId().toInt();
        qDebug() << tositeId;
        viennitModel->tallenna(tositeId);
    }
}
