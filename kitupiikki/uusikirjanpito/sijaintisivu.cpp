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
#include <QFileInfo>
#include <QRegularExpression>
#include <QMessageBox>

#include "sijaintisivu.h"

SijaintiSivu::SijaintiSivu()
{
    setTitle("Kirjanpidon sijainti");

    ui = new Ui::SijaintiSivu;
    ui->setupUi(this);

    registerField("tiedosto",ui->tiedostoEdit);
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
    {
        QFileInfo info(sijainti);
        if( !info.isWritable())
        {
            QMessageBox::critical(this, tr("Hakemisto ei voi kelpaa"), tr("Valitsemaasi hakemistoon ei voida luoda uutta tiedostoa. "
                                                                          "Sinulla ei ehkä ole kirjoitusoikeuksia tähän hakemistoon. \n\n"
                                                                          "Ole hyvä ja valitse toinen hakemisto."));
        }
        else
        {
            ui->sijaintiEdit->setText(sijainti);
            ui->tiedostoEdit->setText( estaTuplaTiedosto(ui->tiedostoEdit->text()) );
        }
    }
}

void SijaintiSivu::initializePage()
{
    QString nimi = field("nimi").toString();
    nimi.replace("ä","a").replace("ö","o")
            .replace("Ä","A").replace("Ö","o")
            .replace("å","a").replace("Å","A")
            .replace(QRegularExpression("[^a-zA-Z0-9]"),"");

    if( field("harjoitus").toBool() )
        nimi += "-harjoitus";
    nimi += ".kitsas";

    ui->tiedostoEdit->setText(estaTuplaTiedosto( nimi ));
}

QString SijaintiSivu::estaTuplaTiedosto(QString tiedosto)
{
    // Poistetaan lopussa mahdollisesti jo oleva lisäys
    tiedosto.replace(QRegularExpression("-?\\d*.kitsas$"),"");

    QString lisake = "";
    int lisanumero = 0;
    while( QFile(ui->sijaintiEdit->text() + "/" + tiedosto + lisake + ".kitsas").exists() )
    {
        lisanumero++;
        lisake = QString("-%1").arg(lisanumero);
    }

    return tiedosto + lisake + ".kitupiikki";
}

bool SijaintiSivu::validatePage()
{
    QFileInfo sijainti(ui->sijaintiEdit->text());
    if( sijainti.isWritable())
        return true;
    QMessageBox::critical(this, tr("Hakemisto ei voi kelpaa"), tr("Valitsemaasi hakemistoon ei voida luoda uutta tiedostoa. "
                                                                  "Sinulla ei ehkä ole kirjoitusoikeuksia tähän hakemistoon. \n\n"
                                                                  "Ole hyvä ja valitse toinen hakemisto."));
    return false;
}

