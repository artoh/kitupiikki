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
#include "ryhmalaskutab.h"

#include "../asiakkaatmodel.h"
#include "rekisteri/ryhmatmodel.h"
#include "db/kirjanpito.h"
#include "laskutettavatmodel.h"

#include <QVBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QTableView>
#include <QSortFilterProxyModel>

RyhmalaskuTab::RyhmalaskuTab(QWidget *parent) :
    QSplitter(parent)
{
    luoUi();
}

void RyhmalaskuTab::luoUi()
{
    setOrientation(Qt::Horizontal);

    QComboBox *ryhmaCombo = new QComboBox;
    ryhmaCombo->setModel(kp()->ryhmat());

    QLineEdit *suodatusEdit = new QLineEdit;
    suodatusEdit->setPlaceholderText(tr("Suodata nimellä"));

    AsiakkaatModel *asiakkaat = new AsiakkaatModel(this);
    asiakkaat->paivita(AsiakkaatModel::REKISTERI);
    connect( ryhmaCombo, &QComboBox::currentTextChanged,
             [ryhmaCombo, asiakkaat] { asiakkaat->suodataRyhma(ryhmaCombo->currentData(AsiakkaatModel::IdRooli).toInt()); }  );

    QSortFilterProxyModel *suodatusProxy = new QSortFilterProxyModel(this);
    suodatusProxy->setSourceModel(asiakkaat);
    connect( suodatusEdit, &QLineEdit::textChanged,
             suodatusProxy, &QSortFilterProxyModel::setFilterFixedString);

    QTableView *asiakasView = new QTableView();
    asiakasView->setModel(suodatusProxy);

    QVBoxLayout *vleiska = new QVBoxLayout;
    vleiska->addWidget(ryhmaCombo);
    vleiska->addWidget(suodatusEdit);
    vleiska->addWidget(asiakasView);

    QWidget *vwg = new QWidget;
    vwg->setLayout(vleiska);
    addWidget(vwg);

    LaskutettavatModel *laskutettavat = new LaskutettavatModel(this);
    QTableView *view = new QTableView;
    view->setModel(laskutettavat);
    addWidget(view);

    connect( asiakasView, &QTableView::clicked,
             [laskutettavat] (const QModelIndex& index)
            {laskutettavat->lisaa(index.data(AsiakkaatModel::MapRooli).toMap());});

}
