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

#include "perusvalinnat.h"
#include "ui_perusvalinnat.h"

#include "db/kirjanpito.h"


Perusvalinnat::Perusvalinnat() :
    MaaritysWidget(),
    ui(new Ui::Perusvalinnat)
{
    ui->setupUi(this);

    connect(ui->vaihdaLogoNappi, SIGNAL(clicked(bool)), this, SLOT(vaihdaLogo()));

    connect( ui->organisaatioEdit, SIGNAL(textChanged(QString)), this, SLOT(ilmoitaMuokattu()));
    connect( ui->ytunnusEdit, SIGNAL(textChanged(QString)), this, SLOT(ilmoitaMuokattu()));
    connect( ui->alvCheck, SIGNAL(clicked(bool)), this, SLOT(ilmoitaMuokattu()));

}

Perusvalinnat::~Perusvalinnat()
{
    delete ui;
}

bool Perusvalinnat::nollaa()
{
    ui->organisaatioEdit->setText( Kirjanpito::db()->asetus("Nimi") );
    ui->ytunnusEdit->setText( Kirjanpito::db()->asetus("Ytunnus"));
    ui->alvCheck->setChecked( kp()->asetukset()->onko("AlvVelvollinen"));

    uusilogo = QImage();

    // Jos logotiedosto, merkitään se
    QFile logotiedosto( Kirjanpito::db()->hakemisto().absoluteFilePath("logo64.png"));
    if( logotiedosto.exists())
        ui->logoLabel->setPixmap( QPixmap( Kirjanpito::db()->hakemisto().absoluteFilePath("logo64.png") ));
    else
        ui->logoLabel->clear();
    return true;
}

void Perusvalinnat::vaihdaLogo()
{
    QString tiedostopolku = QFileDialog::getOpenFileName(this,"Valitse logo", QDir::homePath(),"Kuvatiedostot (*.png *.jpg)" );
    if( !tiedostopolku.isEmpty())
    {
        uusilogo.load( tiedostopolku );
        ui->logoLabel->setPixmap( QPixmap::fromImage( uusilogo.scaled(64, 64, Qt::KeepAspectRatio) ) );
    }
    ilmoitaMuokattu();
}

void Perusvalinnat::ilmoitaMuokattu()
{
    emit tallennaKaytossa( onkoMuokattu());
}

bool Perusvalinnat::onkoMuokattu()
{
    return  ui->organisaatioEdit->text() != kp()->asetus("Nimi")  ||
            ui->ytunnusEdit->text() != kp()->asetus("Ytunnus") ||
            ui->alvCheck->isChecked() != kp()->asetukset()->onko("AlvVelvollinen") ||
            !uusilogo.isNull();
}

bool Perusvalinnat::tallenna()
{
    kp()->asetukset()->aseta("Nimi", ui->organisaatioEdit->text());
    kp()->asetukset()->aseta("Ytunnus", ui->ytunnusEdit->text());
    kp()->asetukset()->aseta("AlvVelvollinen", ui->alvCheck->isChecked());

    // Logosta tallennetaan logo64.png ja logo128.png -versiot
    if( !uusilogo.isNull())
    {

        QFile tiedosto64( Kirjanpito::db()->hakemisto().absoluteFilePath("logo64.png") );
        if( tiedosto64.exists())
            tiedosto64.remove();
        QFile tiedosto128( Kirjanpito::db()->hakemisto().absoluteFilePath("logo128.png"));
        if( tiedosto128.exists())
            tiedosto128.remove();
        // Sitten tallennetaan

        uusilogo.scaled(64, 64, Qt::KeepAspectRatio).save( Kirjanpito::db()->hakemisto().absoluteFilePath("logo64.png")  );
        uusilogo.scaled(128, 128, Qt::KeepAspectRatio).save( Kirjanpito::db()->hakemisto().absoluteFilePath("logo128.png")  );
    }
    uusilogo = QImage();

    return true;
}
