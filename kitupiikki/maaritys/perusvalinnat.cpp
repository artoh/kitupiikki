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

#include <QFileDialog>
#include <QFile>
#include <QImage>
#include <QPixmap>
#include <QSettings>
#include <QMessageBox>

#include "perusvalinnat.h"
#include "ui_perusvalinnat.h"

#include "db/kirjanpito.h"
#include "sqlite/sqlitemodel.h"
#include "validator/ytunnusvalidator.h"

Perusvalinnat::Perusvalinnat() :
    TallentavaMaaritysWidget(nullptr),
    ui(new Ui::Perusvalinnat)
{
    ui->setupUi(this);

    connect(ui->vaihdaLogoNappi, SIGNAL(clicked(bool)), this, SLOT(vaihdaLogo()));
    connect( ui->hakemistoNappi, &QPushButton::clicked, [this] { kp()->avaaUrl( this->ui->sijaintiLabel->text() );  } );
    connect( ui->poistaLogoNappi, &QPushButton::clicked, [this] { poistalogo=true; ui->logoLabel->clear(); ilmoitaMuokattu(); });

    ui->ytunnusEdit->setValidator(new YTunnusValidator());

}

Perusvalinnat::~Perusvalinnat()
{
    delete ui;
}

bool Perusvalinnat::nollaa()
{
    TallentavaMaaritysWidget::nollaa();
    naytaLogo();
    connect( ui->laajuusCombo, &QComboBox::currentTextChanged, this, &Perusvalinnat::alvilaajuudesta);

    SQLiteModel *sqlite = qobject_cast<SQLiteModel*>( kp()->yhteysModel() );
    if( sqlite ) {
        ui->sijaintiLabel->setText( sqlite->tiedostopolku() );
    } else {
        ui->sijaintiLabel->hide();
        ui->hakemistoNappi->hide();
        ui->tsLabel->hide();
    }

    return true;
}

void Perusvalinnat::vaihdaLogo()
{
    QString tiedostopolku = QFileDialog::getOpenFileName(this,"Valitse logo", QDir::homePath(),"Kuvatiedostot (*.png *.jpg)" );
    if( !tiedostopolku.isEmpty())
    {
        QImage uusilogo;
        uusilogo.load( tiedostopolku );
        kp()->asetaLogo(uusilogo);
        naytaLogo();
    }

}

void Perusvalinnat::poistaLogo()
{
    kp()->asetaLogo(QImage());
    ui->poistaLogoNappi->setEnabled( false );
    naytaLogo();
}


bool Perusvalinnat::tallenna()
{
    // Jos muoto tai laajuus vaihtuu, vaikuttaa se tilikarttaan ja ehkä myös alviin
    TallentavaMaaritysWidget::tallenna();
    emit kp()->perusAsetusMuuttui();     // Uusi lataus, koska nimi tai kuva saattoi vaihtua!
    ui->poistaLogoNappi->setEnabled( !kp()->logo().isNull() );

    return true;
}

void Perusvalinnat::alvilaajuudesta()
{
    int laajuus = ui->laajuusCombo->currentData().toInt();
    int alvlaajuus = kp()->asetukset()->luku("alvlaajuus");
    ui->alvCheck->setChecked( laajuus >= alvlaajuus );
}

void Perusvalinnat::naytaLogo()
{
    QImage logo = kp()->logo().scaledToHeight(128);
    if( logo.isNull()) {
        ui->logoLabel->clear();
    } else {
        ui->logoLabel->setPixmap( QPixmap::fromImage(logo));
    }

    ui->poistaLogoNappi->setEnabled( logo.isNull() );
}
