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
#include <QDir>
#include <QFileInfo>

#include "perusvalinnat.h"
#include "ui_perusvalinnat.h"
#include "arkistoija/arkistohakemistodialogi.h"

#include "db/kirjanpito.h"
#include "sqlite/sqlitemodel.h"
#include "pilvi/pilvimodel.h"
#include "validator/ytunnusvalidator.h"

Perusvalinnat::Perusvalinnat() :
    TallentavaMaaritysWidget(nullptr),
    ui(new Ui::Perusvalinnat)
{
    ui->setupUi(this);

    connect(ui->vaihdaLogoNappi, SIGNAL(clicked(bool)), this, SLOT(vaihdaLogo()));
    connect( ui->hakemistoNappi, &QPushButton::clicked, this, &Perusvalinnat::avaaHakemisto );
    connect( ui->avaaArkistoNappi, &QPushButton::clicked, [this] { kp()->avaaUrl( QUrl(ui->arkistoEdit->text()) ); });
    connect( ui->vaihdaArkistoNappi, &QPushButton::clicked, this, &Perusvalinnat::vaihdaArkistoHakemisto);
    connect( ui->poistaLogoNappi, &QPushButton::clicked, [this] { poistaLogo(); ui->logoLabel->clear(); });

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

    ui->karttaInfo->setText( QString("%1 %2")
                             .arg(kp()->asetus("Tilikartta"))
                             .arg(kp()->asetukset()->pvm("TilikarttaPvm").toString("dd.MM.yyyy")));
    ui->alvAlkaaEdit->setEnabled( ui->alvCheck->isChecked());

    ui->arkistoEdit->setText(kp()->settings()->value("arkistopolku/" + kp()->asetus("UID")).toString());
    ui->avaaArkistoNappi->setEnabled( !ui->arkistoEdit->text().isEmpty() );

    KpKysely* kokokysely = kpk("/info");
    connect(kokokysely, &KpKysely::vastaus, this, &Perusvalinnat::kokoSaapuu);
    kokokysely->kysy();

    ui->alvCheck->setEnabled(qobject_cast<PilviModel*>(kp()->yhteysModel()) == nullptr || kp()->pilvi()->pilviVat() );


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
    ui->ytunnusEdit->setText(ui->ytunnusEdit->text().simplified());

    // Jos muoto tai laajuus vaihtuu, vaikuttaa se tilikarttaan ja ehkä myös alviin
    TallentavaMaaritysWidget::tallenna();
    emit kp()->perusAsetusMuuttui();     // Uusi lataus, koska nimi tai kuva saattoi vaihtua!    
    kp()->tilit()->paivitaTilat();
    kp()->tilit()->paivitaNimet();

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
    QImage logo = kp()->logo();
    if( logo.isNull()) {
        ui->logoLabel->clear();
    } else {
        ui->logoLabel->setPixmap( QPixmap::fromImage(logo.scaledToHeight(128)));
    }

    ui->poistaLogoNappi->setEnabled( !logo.isNull() );
}

void Perusvalinnat::avaaHakemisto()
{
    QFileInfo info( ui->sijaintiLabel->text() );
    kp()->avaaUrl( QUrl("file://" + info.dir().absolutePath(), QUrl::TolerantMode));
}

void Perusvalinnat::vaihdaArkistoHakemisto()
{
    ui->arkistoEdit->setText( ArkistohakemistoDialogi::valitseArkistoHakemisto(this) );
    ui->avaaArkistoNappi->setEnabled( !ui->arkistoEdit->text().isEmpty());
}

void Perusvalinnat::kokoSaapuu(QVariant *data)
{
    QVariantMap map = data->toMap();
    long koko = map.value("koko").toInt();
    double megaa = koko / 1e6;
    ui->kokoEdit->setText(tr("%L1 Mt").arg(megaa,0,'f',1));

}
