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
#include "ryhmaanasiakkaatproxy.h"
#include "kielidelegaatti.h"
#include "toimitustapadelegaatti.h"
#include "rekisteri/asiakastoimittajadlg.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QTableView>
#include <QListView>
#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QPushButton>

RyhmalaskuTab::RyhmalaskuTab(QWidget *parent) :
    QSplitter(parent)
{
    luoUi();
}

void RyhmalaskuTab::lisaaKaikki()
{
    QList<int> idt;
    for(int i=0; i < asiakasView_->model()->rowCount(); i++) {
        idt.append( asiakasView_->model()->index(i,0).data(AsiakkaatModel::IdRooli).toInt() );
    }
    for(int id: idt) {
        laskutettavat_->lisaa(id);
    }

}

void RyhmalaskuTab::uusiAsiakas()
{
    AsiakasToimittajaDlg* dlg = new AsiakasToimittajaDlg(this);
    connect( dlg, &AsiakasToimittajaDlg::tallennettu, [this] (int id) { this->laskutettavat_->lisaa(id); });
    if( ryhmaCombo_->currentData().toInt())
        dlg->lisaaRyhmaan(ryhmaCombo_->currentData().toInt());
    dlg->uusi();
}

void RyhmalaskuTab::luoUi()
{
    laskutettavat_ = new LaskutettavatModel(this);

    setOrientation(Qt::Horizontal);

    ryhmaCombo_ = new QComboBox;
    ryhmaCombo_->setModel(kp()->ryhmat());

    QLineEdit *suodatusEdit = new QLineEdit;
    suodatusEdit->setPlaceholderText(tr("Suodata nimellä"));

    AsiakkaatModel *asiakkaat = new AsiakkaatModel(this);
    asiakkaat->paivita(AsiakkaatModel::REKISTERI);
    connect( ryhmaCombo_, &QComboBox::currentTextChanged,
             [this, asiakkaat] { asiakkaat->suodataRyhma(this->ryhmaCombo_->currentData(AsiakkaatModel::IdRooli).toInt()); }  );

    RyhmaanAsiakkaatProxy *suodatusProxy = new RyhmaanAsiakkaatProxy(this);
    suodatusProxy->asetaLaskutettavatModel(laskutettavat_);
    suodatusProxy->setSourceModel(asiakkaat);
    suodatusProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    connect( suodatusEdit, &QLineEdit::textChanged,
             suodatusProxy, &QSortFilterProxyModel::setFilterFixedString);
    connect( laskutettavat_, &LaskutettavatModel::rowsInserted, suodatusProxy, &RyhmaanAsiakkaatProxy::invalidate);

    asiakasView_ = new QListView();
    asiakasView_->setModel(suodatusProxy);

    QVBoxLayout *vleiska = new QVBoxLayout;
    vleiska->addWidget(ryhmaCombo_);
    vleiska->addWidget(suodatusEdit);
    vleiska->addWidget(asiakasView_);


    QPushButton *lisaaAsiakasNappi = new QPushButton(QIcon(":/pic/lisaa.png"), tr("Uusi asiakas"));
    connect( lisaaAsiakasNappi, &QPushButton::clicked, this, &RyhmalaskuTab::uusiAsiakas);

    QPushButton *kaikkiNappi = new QPushButton(QIcon(":/pic/asiakkaat.png"), tr("Lisää kaikki"));
    connect( kaikkiNappi, &QPushButton::clicked, this, &RyhmalaskuTab::lisaaKaikki);

    QHBoxLayout *vnleiska = new QHBoxLayout;
    vnleiska->addWidget(lisaaAsiakasNappi);
    vnleiska->addWidget(kaikkiNappi);
    vleiska->addLayout(vnleiska);

    QWidget *vwg = new QWidget;
    vwg->setLayout(vleiska);
    addWidget(vwg);

    QTableView *view = new QTableView;
    view->setModel(laskutettavat_);
    view->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    view->setItemDelegateForColumn(LaskutettavatModel::KIELI, new KieliDelegaatti(this));
    view->setItemDelegateForColumn(LaskutettavatModel::LAHETYSTAPA, new ToimitustapaDelegaatti(this));


    addWidget(view);

    setStretchFactor(0,1);
    setStretchFactor(1,3);

    connect( asiakasView_, &QTableView::clicked,
             [this] (const QModelIndex& index)
            { this->laskutettavat_->lisaa(index.data(AsiakkaatModel::IdRooli).toInt());});

}
