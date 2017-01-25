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
    QWidget(),
    ui(new Ui::Perusvalinnat)
{
    ui->setupUi(this);

    connect(ui->vaihdaLogoNappi, SIGNAL(clicked(bool)), this, SLOT(vaihdaLogo()));
    connect(ui->nollaaNappi, SIGNAL(clicked(bool)), this, SLOT(nollaa()));
    connect(ui->tallennaNappi, SIGNAL(clicked(bool)), this, SLOT(tallenna()));
}

Perusvalinnat::~Perusvalinnat()
{
    delete ui;
}

void Perusvalinnat::nollaa()
{
    ui->organisaatioEdit->setText( Kirjanpito::db()->asetus("nimi") );
    ui->ytunnusEdit->setText( Kirjanpito::db()->asetus("ytunnus"));

    uusilogo = QImage();

    // Jos logotiedosto, merkitään se
    QFile logotiedosto( Kirjanpito::db()->hakemisto().absoluteFilePath("logo64.png"));
    if( logotiedosto.exists())
        ui->logoLabel->setPixmap( QPixmap( Kirjanpito::db()->hakemisto().absoluteFilePath("logo64.png") ));
    else
        ui->logoLabel->clear();

}

void Perusvalinnat::vaihdaLogo()
{
    QString tiedostopolku = QFileDialog::getOpenFileName(this,"Valitse logo", QDir::homePath(),"Kuvatiedostot (*.png *.jpg)" );
    if( !tiedostopolku.isEmpty())
    {
        uusilogo.load( tiedostopolku );
        ui->logoLabel->setPixmap( QPixmap::fromImage( uusilogo.scaled(64, 64, Qt::KeepAspectRatio) ) );
    }
}

void Perusvalinnat::tallenna()
{
    Kirjanpito::db()->aseta("nimi", ui->organisaatioEdit->text());
    Kirjanpito::db()->aseta("ytunnus", ui->ytunnusEdit->text());

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

}
