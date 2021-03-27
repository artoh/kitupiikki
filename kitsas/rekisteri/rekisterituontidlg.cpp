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
#include "rekisterituontidlg.h"
#include "ui_rekisterituontidlg.h"

#include "rekisterituontimodel.h"
#include "rekisterituontidelegaatti.h"
#include "laskutus/ryhmalasku/kielidelegaatti.h"
#include "model/lasku.h"
#include "maamodel.h"
#include "db/kirjanpito.h"
#include <QDebug>
#include <QPushButton>

RekisteriTuontiDlg::RekisteriTuontiDlg(const QString &tiedosto, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RekisteriTuontiDlg),
    model_(new RekisteriTuontiModel(this))
{
    ui->setupUi(this);
    connect( model_, &RekisteriTuontiModel::otsikkorivit, ui->otsikotCheck, &QCheckBox::setChecked);
    connect( ui->otsikotCheck, &QCheckBox::clicked, model_, &RekisteriTuontiModel::asetaOtsikkorivi);

    model_->lataaCsv(tiedosto);

    ui->tuontiView->setModel(model_);
    ui->tuontiView->horizontalHeader()->setSectionResizeMode(RekisteriTuontiModel::MALLI, QHeaderView::Stretch);
    ui->tuontiView->setItemDelegateForColumn(RekisteriTuontiModel::MUOTO, new RekisteriTuontiDelegaatti(this));
    ui->ryhmaWidget->valitseRyhmat(QVariantList());

    KieliDelegaatti::alustaKieliCombo(ui->kieliCombo);
    alustaMaksutavat( ui->laskutusCombo );
    alustaMaksutavat( ui->varaLaskuCombo );
    ui->maaCombo->setModel( new MaaModel(this));
    ui->maaCombo->setCurrentIndex( ui->maaCombo->findData("fi", MaaModel::KoodiRooli));

    connect( ui->buttonBox, &QDialogButtonBox::helpRequested, [] { kp()->ohje("/laskutus/rekisteri"); });
}

RekisteriTuontiDlg::~RekisteriTuontiDlg()
{
    delete ui;
}

void RekisteriTuontiDlg::accept()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    tallennusLista_ = model_->lista();
    tallennaSeuraava();
}

void RekisteriTuontiDlg::alustaMaksutavat(QComboBox *combo)
{
    combo->addItem(QIcon(":/pic/tulosta.png"), tr("Tulostus"), Lasku::TULOSTETTAVA);
    combo->addItem(QIcon(":/pic/kirje.png"),tr("Postitus"), Lasku::POSTITUS);
    combo->addItem(QIcon(":/pic/email.png"), tr("Sähköposti"), Lasku::SAHKOPOSTI);
    combo->addItem(QIcon(":/pic/verkkolasku.png"), tr("Verkkolasku"), Lasku::VERKKOLASKU);
}

void RekisteriTuontiDlg::tallennaSeuraava()
{
    if( tallennusLista_.isEmpty()) {
        QDialog::accept();
        emit kp()->kirjanpitoaMuokattu();
        return;
    }
    QVariantMap map = tallennusLista_.takeFirst().toMap();
    if( !map.contains("maa"))
        map.insert("maa", ui->maaCombo->currentData(MaaModel::KoodiRooli).toString());
    if( !map.contains("kieli"))
        map.insert("kieli", ui->kieliCombo->currentData().toString());
    QVariantList ryhmat = ui->ryhmaWidget->valitutRyhmat();
    if( !ryhmat.isEmpty())
        map.insert("ryhmat", ryhmat);
    map.insert("laskutapa", maksutapa(map));

    KpKysely *kysely = kpk( "/kumppanit", KpKysely::POST);
    connect(kysely, &KpKysely::vastaus, this, &RekisteriTuontiDlg::tallennaSeuraava);
    kysely->kysy(map);
}

int RekisteriTuontiDlg::maksutapa(const QVariantMap &map)
{
    if( ui->laskutusCombo->currentData().toInt() == Lasku::SAHKOPOSTI && map.contains("email"))
        return Lasku::SAHKOPOSTI;
    else if( ui->laskutusCombo->currentData().toInt() == Lasku::VERKKOLASKU && map.contains("ovt"))
        return Lasku::VERKKOLASKU;
    else if( ui->laskutusCombo->currentData().toInt() == Lasku::POSTITUS && map.contains("osoite"))
        return Lasku::POSTITUS;

    if( ui->varaLaskuCombo->currentData().toInt() == Lasku::SAHKOPOSTI && map.contains("email"))
        return Lasku::SAHKOPOSTI;
    else if( ui->varaLaskuCombo->currentData().toInt() == Lasku::VERKKOLASKU && map.contains("ovt"))
        return Lasku::VERKKOLASKU;
    else if( ui->varaLaskuCombo->currentData().toInt() == Lasku::POSTITUS && map.contains("osoite"))
        return Lasku::POSTITUS;

    return Lasku::TULOSTETTAVA;
}
