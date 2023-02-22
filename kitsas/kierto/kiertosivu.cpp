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
#include "kiertosivu.h"
#include "ui_kiertosivu.h"

#include "kiertoselausmodel.h"
#include "db/kirjanpito.h"
#include "db/yhteysmodel.h"

KiertoSivu::KiertoSivu(QWidget *parent) :
    KitupiikkiSivu(parent),
    ui(new Ui::KiertoSivu),
    model(new KiertoSelausModel(this))
{
    ui->setupUi(this);
    ui->view->setModel(model);
    ui->view->horizontalHeader()->setSectionResizeMode(KiertoSelausModel::OTSIKKO, QHeaderView::Stretch);

    ui->tab->addTab(QIcon(":/pic/inbox.png"),tr("Työlista"));
    ui->tab->addTab(QIcon(":/pic/kierto-harmaa.svg"),tr("Kaikki"));

    connect( ui->view, &QTableView::clicked, this, &KiertoSivu::naytaTosite);
    connect( ui->tab, &QTabBar::currentChanged, this, &KiertoSivu::vaihdaTab);
}

KiertoSivu::~KiertoSivu()
{
    delete ui;
}

void KiertoSivu::siirrySivulle()
{
    if( !kp()->yhteysModel())
        return;
    if( !kp()->yhteysModel()->onkoOikeutta(YhteysModel::KIERTO_SELAAMINEN)) {
        ui->tab->setCurrentIndex(TYOLISTA);
        ui->tab->setTabEnabled(KAIKKI, false);
        model->naytaKaikki(false);
    } else {
        ui->tab->setTabEnabled(KAIKKI, true);       
    }
    model->lataa();
}

void KiertoSivu::naytaTosite(const QModelIndex &index)
{
    if( index.isValid()) {
        QList<int> lista;
        for(int i=0; i < index.model()->rowCount(); i++) {
            lista.append( index.sibling(i,0).data(KiertoSelausModel::IdRooli).toInt() );
        }

        emit tositeValittu( index.data(KiertoSelausModel::IdRooli).toInt(), lista, KirjausSivu::PALATAAN_AINA);
    }
}

void KiertoSivu::vaihdaTab(int tab)
{
    model->naytaKaikki(tab == KAIKKI);
}
