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
#include "tulomenoapuri.h"
#include "ui_tulomenoapuri.h"
#include "tmrivit.h"

#include "db/kirjanpito.h"
#include "model/tosite.h"
#include "db/tositetyyppimodel.h"
#include "kirjaus/kohdennusproxymodel.h"

#include <QSortFilterProxyModel>


TuloMenoApuri::TuloMenoApuri(QWidget *parent, Tosite *tosite) :
    ApuriWidget (parent, tosite),
    ui(new Ui::TuloMenoApuri),
    rivit_(new TmRivit)
{
    ui->setupUi(this);

    veroFiltteri_ = new QSortFilterProxyModel(this);
    veroFiltteri_->setFilterRole( VerotyyppiModel::KoodiTekstiRooli);
    veroFiltteri_->setSourceModel( kp()->alvTyypit());
    ui->alvCombo->setModel(veroFiltteri_);

    ui->kohdennusCombo->setModel(new KohdennusProxyModel(this));
    ui->kohdennusCombo->setModelColumn( KohdennusModel::NIMI);
    ui->kohdennusCombo->setCurrentIndex( ui->kohdennusCombo->findData(0, KohdennusModel::IdRooli ));


    ui->alkuEdit->setNull();
    ui->loppuEdit->setNull();
    ui->erapaivaEdit->setNull();

    ui->vastatiliEdit->suodataTyypilla("[AB].*");

    ui->tilellaView->setModel( rivit_);
    ui->tilellaView->horizontalHeader()->setSectionResizeMode(TmRivit::TILI, QHeaderView::Stretch);


    if( tosite )
        reset();

    connect( ui->tiliEdit, &TilinvalintaLine::textChanged, this, &TuloMenoApuri::tiliMuuttui );
    connect( ui->maaraEdit, &KpEuroEdit::textChanged, this, &TuloMenoApuri::maaraMuuttui);
    connect( ui->lisaaRiviNappi, &QPushButton::clicked, this, &TuloMenoApuri::lisaaRivi);
    connect( ui->tilellaView->selectionModel(), &QItemSelectionModel::currentRowChanged , this, &TuloMenoApuri::haeRivi);
}

TuloMenoApuri::~TuloMenoApuri()
{
    delete ui;
}

void TuloMenoApuri::reset()
{
    // Haetaan tietoja mallista ;)

    alusta( tosite()->data(Tosite::TYYPPI).toInt() == TositeTyyppi::MENO );

    // Haetaan rivien tiedot
    ui->tilellaView->setVisible( rivit_->rowCount() > 1 );
    ui->tilellaView->selectRow(0);

}

void TuloMenoApuri::lisaaRivi()
{
    ui->tilellaView->setVisible(true);
    ui->tilellaView->selectRow( rivit_->lisaaRivi() );
}

void TuloMenoApuri::tiliMuuttui()
{
    Tili tili = ui->tiliEdit->valittuTili();
    rivit_->setTili( rivilla(), tili );

    bool tasapoisto = tili.onko(TiliLaji::TASAERAPOISTO);
    ui->poistoLabel->setVisible(tasapoisto);
    ui->poistoSpin->setVisible(tasapoisto);

    // TODO: Vero-oletusten hakeminen
}

void TuloMenoApuri::maaraMuuttui()
{
    rivit_->setMaara( rivilla(), ui->maaraEdit->asCents());
}

void TuloMenoApuri::haeRivi(const QModelIndex &index)
{
    int rivi = index.row();
    ui->tiliEdit->valitseTili( rivit_->tili(rivi));
    ui->maaraEdit->setCents( rivit_->maara(rivi) );
}

void TuloMenoApuri::alusta(bool meno)
{
    if(meno) {
        ui->tiliLabel->setText( tr("Menotili") );
        ui->tiliEdit->suodataTyypilla("(AP|D).*");
        veroFiltteri_->setFilterRegExp("^(0|2[1-79]|927)");
        ui->toimittajaLabel->setText( tr("Toimittaja"));
    } else {
        ui->tiliLabel->setText( tr("Tulotili"));
        ui->tiliEdit->suodataTyypilla("(AP|C).*");
        veroFiltteri_->setFilterRegExp("^(0|1[1-79])");
        ui->toimittajaLabel->setText( tr("Asiakas"));
    }


    bool alv = kp()->asetukset()->onko( AsetusModel::ALV );
    ui->alvLabel->setVisible(alv);
    ui->alvCombo->setVisible(alv);
    ui->alvSpin->setVisible(alv);
    ui->verotonLabel->setVisible(alv);
    ui->verotonEdit->setVisible(alv);
    ui->vahennysCheck->setVisible(alv);

    bool kohdennuksia = kp()->kohdennukset()->kohdennuksia();
    ui->kohdennusLabel->setVisible(kohdennuksia);
    ui->kohdennusCombo->setVisible(kohdennuksia);

    bool merkkauksia = kp()->kohdennukset()->merkkauksia();
    ui->merkkauksetLabel->setVisible(merkkauksia);
    ui->merkkauksetEdit->setVisible(merkkauksia);
}

int TuloMenoApuri::rivilla() const
{
    return ui->tilellaView->currentIndex().row();
}
