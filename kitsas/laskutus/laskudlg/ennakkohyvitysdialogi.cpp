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
#include "ennakkohyvitysdialogi.h"
#include "ui_ennakkohyvitysdialogi.h"
#include "ennakkohyvitysmodel.h"

#include <QPushButton>

EnnakkoHyvitysDialogi::EnnakkoHyvitysDialogi(EnnakkoHyvitysModel *model, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EnnakkoHyvitysDialogi)
{
    ui->setupUi(this);
    ui->view->setModel(model);
    ui->view->horizontalHeader()->setSectionResizeMode(EnnakkoHyvitysModel::SELITE, QHeaderView::Stretch);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    connect( ui->view->selectionModel(), &QItemSelectionModel::selectionChanged,
             this, &EnnakkoHyvitysDialogi::riviValittu);
}

EnnakkoHyvitysDialogi::~EnnakkoHyvitysDialogi()
{
    delete ui;
}

int EnnakkoHyvitysDialogi::eraId() const
{
    const QModelIndex &index = ui->view->selectionModel()->selection().indexes().value(0);
    return index.data(EnnakkoHyvitysModel::EraIdRooli).toInt();
}

Euro EnnakkoHyvitysDialogi::euro() const
{
    return ui->euroEdit->euro();
}

void EnnakkoHyvitysDialogi::riviValittu(const QItemSelection &valinta)
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled( !valinta.indexes().isEmpty() );
    ui->euroEdit->setValue( valinta.indexes().value(0).data(EnnakkoHyvitysModel::EuroRooli).toDouble() );
}
