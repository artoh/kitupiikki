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

InboxMaaritys::InboxMaaritys() :
    ui_(new Ui::InboxMaaritys)
{
    ui_->setupUi(this);
    rekisteroi( ui_->kansioEdit, "KirjattavienKansio" );
    connect( ui_->valitseNappi, &QPushButton::clicked, this, &InboxMaaritys::valitseKansio );
    rekisteroi( ui_->kopioEdit, "KirjattavienSiirtokansio");
    connect( ui_->kopioNappi, &QPushButton::clicked, this, &InboxMaaritys::valitseKopioKansio);
    rekisteroi( ui_->kopioRadio, "KirjattavienKansioSiirto");

}

InboxMaaritys::~InboxMaaritys()
{
    delete ui_;
}

bool InboxMaaritys::tallenna()
{
    TallentavaMaaritysWidget::tallenna();
    emit kp()->inboxMuuttui();
    return true;
}

bool InboxMaaritys::nollaa()
{
    ui_->poistaRadio->setChecked( !kp()->asetukset()->onko("KirjattavienKansioSiirto") );
    ui_->kopioRadio->setEnabled(!kp()->asetukset()->asetus("KirjattavienKansio").isEmpty() );
    return TallentavaMaaritysWidget::nollaa();
}

void InboxMaaritys::valitseKansio()
{
    QString kansio = QFileDialog::getExistingDirectory(this, tr("Valitse kirjattavien tositteiden kansio"),
                                                       QDir::homePath());
    if( !kansio.isEmpty())
        ui_->kansioEdit->setText(kansio);
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
}
