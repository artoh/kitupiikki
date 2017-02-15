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

#include <QSortFilterProxyModel>

#include "tilinvalintadialogi.h"
#include "ui_tilinvalintadialogi.h"

#include "kirjanpito.h"

TilinValintaDialogi::TilinValintaDialogi(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TilinValintaDialogi)
{
    ui->setupUi(this);

    QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel( kp()->tilit());
    proxy->setFilterRole( TiliModel::NimiRooli);
    proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxy->setSortRole(TiliModel::YsiRooli);


    ui->view->setModel(proxy);
    ui->view->setColumnHidden(TiliModel::NRONIMI, true);

    connect(ui->suodatusEdit, SIGNAL(textChanged(QString)),
            proxy, SLOT(setFilterFixedString(QString)));

    ui->view->resizeColumnsToContents();

}

TilinValintaDialogi::~TilinValintaDialogi()
{
    delete ui;
}


Tili TilinValintaDialogi::valitseTili(const QString &alku)
{
    TilinValintaDialogi dlg;
    dlg.ui->suodatusEdit->setText(alku);

    if( dlg.exec())
        return kp()->tilit()->tiliNumerolla(1911);
    return Tili();
}
