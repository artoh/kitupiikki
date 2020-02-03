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
#include "kiertomaaritys.h"
#include "ui_kiertomaaritys.h"
#include "db/kirjanpito.h"
#include "kiertomodel.h"
#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"
#include "kiertomuokkausdlg.h"

#include <QSortFilterProxyModel>

KiertoMaaritys::KiertoMaaritys(QWidget *parent) :
    MaaritysWidget(parent),
    ui(new Ui::KiertoMaaritys)
{
    ui->setupUi(this);

    QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(kp()->kierrot());
    proxy->setSortRole(KiertoModel::NimiRooli);
    ui->view->setModel(proxy);

    QString osoite = kp()->pilvi()->pilviLoginOsoite();
    osoite = osoite.left(osoite.lastIndexOf('/'));


    ui->osoiteEdit->setText( QString("%1/portaali/%2")
                              .arg(osoite)
                              .arg(kp()->pilvi()->pilviId()));

    connect(ui->uusiNappi, &QPushButton::clicked, this, &KiertoMaaritys::uusi);
}

KiertoMaaritys::~KiertoMaaritys()
{
    delete ui;
}

void KiertoMaaritys::uusi()
{
    KiertoMuokkausDlg dlg(0, this, ui->portaaliRyhma->isChecked());
    dlg.exec();
}
