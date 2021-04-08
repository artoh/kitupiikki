/*
   Copyright (C) 2018 Arto Hyvättinen

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
#include "inboxmaaritys.h"
#include "ui_inboxmaaritys.h"
#include "db/kirjanpito.h"

#include <QFileDialog>
#include <QSettings>

InboxMaaritys::InboxMaaritys() :
    ui_(new Ui::InboxMaaritys)
{
    ui_->setupUi(this);
    connect( ui_->valitseNappi, &QPushButton::clicked, this, &InboxMaaritys::valitseKansio);
    connect( ui_->poistaNappi, &QPushButton::clicked, [this] { emit this->tallennaKaytossa(this->onkoMuokattu()); });
    connect( ui_->kopioNappi, &QPushButton::clicked, this, &InboxMaaritys::valitseKopioKansio);
    connect( ui_->kopioRadio, &QPushButton::clicked, [this] { emit this->tallennaKaytossa(this->onkoMuokattu()); });
    connect( ui_->poistaRadio , &QPushButton::clicked, [this] { this->ui_->kopioEdit->clear(); emit this->tallennaKaytossa(this->onkoMuokattu());});
}

InboxMaaritys::~InboxMaaritys()
{
    delete ui_;
}

bool InboxMaaritys::tallenna()
{
    QString uid = kp()->asetukset()->asetus(AsetusModel::UID);
    kp()->settings()->beginGroup(uid);

    kp()->settings()->setValue("KirjattavienKansio", ui_->kansioEdit->text());
    kp()->settings()->setValue("KirjattavienSiirtoKansio", ui_->kopioEdit->text());
    kp()->settings()->setValue("KirjattavienSiirto", !ui_->poistaRadio->isChecked() );
    kp()->settings()->endGroup();


    emit kp()->inboxMuuttui();
    return true;
}

bool InboxMaaritys::nollaa()
{
    QString uid = kp()->asetukset()->asetus(AsetusModel::UID);
    kp()->settings()->beginGroup(uid);
    ui_->kansioEdit->setText( kp()->settings()->value("KirjattavienKansio").toString() );
    ui_->kopioEdit->setText( kp()->settings()->value("KirjattavienSiirtoKansio").toString());
    bool poista = !kp()->settings()->value("KirjattavienSiirto").toBool();
    ui_->poistaRadio->setChecked( poista );
    ui_->kopioRadio->setChecked( !poista );
    kp()->settings()->endGroup();
    return true;
}

bool InboxMaaritys::onkoMuokattu()
{
    QString uid = kp()->asetukset()->asetus(AsetusModel::UID);
    kp()->settings()->beginGroup(uid);
    QString kirjattavat = ( kp()->settings()->value("KirjattavienKansio").toString() );
    QString siirto = ( kp()->settings()->value("KirjattavienSiirtoKansio").toString());
    bool poista = !kp()->settings()->value("KirjattavienSiirto").toBool();
    kp()->settings()->endGroup();


    return
        ui_->kansioEdit->text() != kirjattavat ||
        ui_->kopioEdit->text() != siirto ||
        ui_->poistaRadio->isChecked() != poista;

}

void InboxMaaritys::valitseKansio()
{
    QString kansio = QFileDialog::getExistingDirectory(this, tr("Valitse kirjattavien tositteiden kansio"),
                                                       QDir::homePath());
    if( !kansio.isEmpty())
        ui_->kansioEdit->setText(kansio);

    ui_->kopioRadio->setEnabled(!kp()->asetukset()->asetus("KirjattavienKansio").isEmpty() );
    emit tallennaKaytossa(onkoMuokattu());
}

void InboxMaaritys::valitseKopioKansio()
{
    QString kansio = QFileDialog::getExistingDirectory(this, tr("Valitse kansio, jonne kirjatut siirretään"),
                                                       QDir::homePath());
    if( !kansio.isEmpty())
    {
        ui_->kopioEdit->setText(kansio);
        ui_->kopioRadio->setEnabled(true);
        ui_->kopioRadio->setChecked(true);
    }
    else {
        ui_->kopioRadio->setEnabled(false);
        ui_->poistaRadio->setChecked(true);
    }
    emit tallennaKaytossa(onkoMuokattu());
}
