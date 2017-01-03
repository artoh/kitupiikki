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

#include <QFile>
#include <QFileDialog>
#include <QRegularExpression>

#include "sijaintisivu.h"

SijaintiSivu::SijaintiSivu()
{
    setTitle("Tiedostojen sijainti");

    ui = new Ui::SijaintiSivu;
    ui->setupUi(this);

    registerField("hakemisto",ui->hakemistoEdit);
    registerField("sijainti",ui->sijaintiEdit);

    ui->sijaintiEdit->setText( QDir::homePath());

    connect( ui->vaihdaSijaintiNappi, SIGNAL(clicked(bool)),
             this, SLOT(vaihdaSijainti()));
}

SijaintiSivu::~SijaintiSivu()
{
    delete ui;
}

void SijaintiSivu::vaihdaSijainti()
{
    QString sijainti = QFileDialog::getExistingDirectory(this,"Valitse sijainti",
                                                         ui->sijaintiEdit->text());
    if( !sijainti.isEmpty())
        ui->sijaintiEdit->setText(sijainti);
    estaTuplaHakemistot();
}

void SijaintiSivu::initializePage()
{
    QString nimi = field("nimi").toString();
    QString siistitty = nimi.replace("ä","a").replace("ö","o")
            .replace("Ä","A").replace("Ö","o")
            .replace("å","a").replace("Å","å")
            .replace(QRegularExpression("[^a-zA-Z0-9]"),"");

    if( field("todellinen").toBool() )
        siistitty += "-kirjanpito";
    else
        siistitty += "-kokeilu";

    ui->hakemistoEdit->setText(siistitty);
    estaTuplaHakemistot();
}

void SijaintiSivu::estaTuplaHakemistot()
{
    QString hakemisto = ui->hakemistoEdit->text();
    // Poistetaan lopussa mahdollisesti jo oleva numerolisäys
    QString nimi = hakemisto.replace(QRegularExpression("-\\d*$"),"");

    QString lisake = "";
    int lisanumero = 0;
    while( QFile(ui->sijaintiEdit->text() + "/" + nimi + lisake).exists() )
    {
        lisanumero++;
        lisake = QString("-%1").arg(lisanumero);
    }

    ui->hakemistoEdit->setText(nimi + lisake);
}

