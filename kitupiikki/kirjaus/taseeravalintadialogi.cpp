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

#include "taseeravalintadialogi.h"
#include "ui_taseeravalintadialogi.h"

#include <QDebug>

TaseEraValintaDialogi::TaseEraValintaDialogi(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TaseEraValintaDialogi)
{
    ui->setupUi(this);

    proxy_ = new QSortFilterProxyModel(this);
    proxy_->setSourceModel(&model_);

    ui->view->setModel( proxy_);
    ui->view->setSelectionMode(QListView::SingleSelection);

    connect( ui->suodatusEdit, SIGNAL(textChanged(QString)), proxy_, SLOT(setFilterFixedString(QString)));
    connect( ui->view->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(eraValintaVaihtuu()) );
}

TaseEraValintaDialogi::~TaseEraValintaDialogi()
{
    delete ui;
}

int TaseEraValintaDialogi::nayta(Tili tili, int taseEra, int poistoKk)
{
    model_.lataa(tili, true);

    ui->view->setCurrentIndex( proxy_->index(0,0));
    for(int i=0; i < proxy_->rowCount(); i++)
    {
        if( proxy_->data( proxy_->index(i,0), EranValintaModel::EraIdRooli ) == taseEra)
        {
            ui->view->setCurrentIndex( proxy_->index(i,0) );
            break;
        }
    }

    poistotililla_ = poistoKk > -1;
    eraValintaVaihtuu();    // Jotta tarpeettomat poistot poistetaan ;)

    return exec();
}

int TaseEraValintaDialogi::eraId()
{
    return ui->view->currentIndex().data(EranValintaModel::EraIdRooli).toInt();
}

int TaseEraValintaDialogi::poistoKk()
{
    if( poistotililla_ && eraId() == 0)
        return ui->poistoSpin->value();
    else
        return 0;
}

void TaseEraValintaDialogi::eraValintaVaihtuu()
{
    qDebug() << ui->view->currentIndex().row() << " " <<  eraId() << " " << ui->view->currentIndex().data(EranValintaModel::SeliteRooli).toString();

    ui->poistoLabel->setVisible( eraId() == 0 && poistotililla_);
    ui->poistoSpin->setVisible( eraId() == 0 && poistotililla_);
}
