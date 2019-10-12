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

#include "validator/ytunnusvalidator.h"

Perusvalinnat::Perusvalinnat() :
    TallentavaMaaritysWidget(nullptr),
    ui(new Ui::Perusvalinnat)
{
    ui->setupUi(this);

    connect(ui->vaihdaLogoNappi, SIGNAL(clicked(bool)), this, SLOT(vaihdaLogo()));
    connect( ui->hakemistoNappi, SIGNAL(clicked(bool)), this, SLOT(avaaHakemisto()));
    connect( ui->avaaArkistoNappi, &QPushButton::clicked, [] { kp()->avaaUrl( kp()->arkistopolku() ); } );    
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
}

void Perusvalinnat::vaihdaLogo()
{

}


void Perusvalinnat::avaaHakemisto()
{
    QFileInfo info( kp()->tiedostopolku());
    kp()->avaaUrl( QUrl::fromLocalFile( info.absoluteDir().absolutePath() ) );
}


bool Perusvalinnat::tallenna()
{


    if( ui->muotoCombo->currentText() != kp()->asetukset()->asetus("Muoto"))
    {
        // Muodon vaihto pitää vielä varmistaa
        if( QMessageBox::question(nullptr, tr("Vahvista muutos"),
                                  tr("Haluatko todella tehdä muutoksen\n"
                                     "%1: %2").arg( kp()->asetukset()->asetus("MuotoTeksti") )
                                              .arg( ui->muotoCombo->currentText()),
                                  QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Yes)
        {
            // Suorittaa muodon vaihtoon liittyvät skriptit
            kp()->asetukset()->aseta("Muoto", ui->muotoCombo->currentText());
            nollaa();
        }

    }
    else
        emit kp()->onni("Asetukset tallennettu");

    emit kp()->perusAsetusMuuttui();     // Uusi lataus, koska nimi tai kuva saattoi vaihtua!
    ui->poistaLogoNappi->setEnabled( !kp()->logo().isNull() );

    return true;
}
