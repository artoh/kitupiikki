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
#include "rekisteri/asiakastoimittajadlg.h"
#include "toimitustapadelegaatti.h"
#include "model/lasku.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QTableView>
#include <QListView>
#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QPushButton>
#include <QMenu>

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
    connect( dlg, &AsiakasToimittajaDlg::kumppaniTallennettu, this, [this] (const QVariantMap& map) { this->laskutettavat_->lisaa(map.value("id").toInt()); });
    if( ryhmaCombo_->currentData().toInt())
        dlg->lisaaRyhmaan(ryhmaCombo_->currentData().toInt());
    dlg->uusi();
}

void RyhmalaskuTab::poista()
{
    int indeksi = laskutettavatView_->selectionModel()->selectedIndexes().value(0).row();
    if( indeksi > -1)
        laskutettavat_->poista(indeksi);
}

void RyhmalaskuTab::suodataRyhma()
{

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
    connect( ryhmaCombo_, &QComboBox::currentTextChanged, this,
             [this, asiakkaat] { asiakkaat->suodataRyhma(this->ryhmaCombo_->currentData(AsiakkaatModel::IdRooli).toInt()); }  );

    RyhmaanAsiakkaatProxy *suodatusProxy = new RyhmaanAsiakkaatProxy(this);
    suodatusProxy->asetaLaskutettavatModel(laskutettavat_);
    suodatusProxy->setSourceModel(asiakkaat);
    suodatusProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    connect( suodatusEdit, &QLineEdit::textChanged,
             suodatusProxy, &QSortFilterProxyModel::setFilterFixedString);
    connect( laskutettavat_, &LaskutettavatModel::rowsInserted, suodatusProxy, &RyhmaanAsiakkaatProxy::invalidate);
    connect( laskutettavat_, &LaskutettavatModel::rowsRemoved, suodatusProxy, &RyhmaanAsiakkaatProxy::invalidate);

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

    laskutettavatView_ = new QTableView;
    laskutettavatView_->setModel(laskutettavat_);
    laskutettavatView_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    laskutettavatView_->setItemDelegateForColumn(LaskutettavatModel::KIELI, new KieliDelegaatti(this));
    laskutettavatView_->setItemDelegateForColumn(LaskutettavatModel::LAHETYSTAPA, new ToimitustapaDelegaatti(this));
    laskutettavatView_->setEditTriggers(QTableView::AllEditTriggers);

    connect( laskutettavatView_->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]
        {this->poistaNappi_->setEnabled( this->laskutettavatView_->selectionModel()->selectedIndexes().value(0).isValid() );});

    poistaNappi_ = new QPushButton(QIcon(":/pic/poista.png"), tr("Poista"));
    poistaNappi_->setEnabled(false);
    connect( poistaNappi_, &QPushButton::clicked, this, &RyhmalaskuTab::poista);

    tapaMenu_ = new QMenu(this);
    tapaNappi_ = new QPushButton(QIcon(":/pic/mail.png"), tr("Lähetystapa"));
    tapaNappi_->setMenu(tapaMenu_);
    for(int koodi=Lasku::TULOSTETTAVA; koodi < Lasku::TUOTULASKU; koodi++) {
        QAction* action = tapaMenu_->addAction( ToimitustapaDelegaatti::icon(koodi), ToimitustapaDelegaatti::toimitustapa(koodi) );
        connect( action, &QAction::triggered, [this, koodi] { this->laskutettavat_->vaihdaKaikkienTapa(koodi); });
    }

    QVBoxLayout *oleiska = new QVBoxLayout;
    oleiska->addWidget(laskutettavatView_);
    QHBoxLayout *onleiska = new QHBoxLayout;
    onleiska->addStretch();
    onleiska->addWidget(tapaNappi_);
    onleiska->addWidget(poistaNappi_);
    oleiska->addLayout(onleiska);
    QWidget *owidget = new QWidget();
    owidget->setLayout(oleiska);



    addWidget(owidget);

    setStretchFactor(0,1);
    setStretchFactor(1,3);

    connect( asiakasView_, &QTableView::clicked, this,
             [this] (const QModelIndex& index)
            { this->laskutettavat_->lisaa(index.data(AsiakkaatModel::IdRooli).toInt());});

}

