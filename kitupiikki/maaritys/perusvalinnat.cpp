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
#include "uusikp/skripti.h"


Perusvalinnat::Perusvalinnat() :
    MaaritysWidget(),
    ui(new Ui::Perusvalinnat)
{
    ui->setupUi(this);

    connect(ui->vaihdaLogoNappi, SIGNAL(clicked(bool)), this, SLOT(vaihdaLogo()));

    connect( ui->organisaatioEdit, SIGNAL(textChanged(QString)), this, SLOT(ilmoitaMuokattu()));
    connect( ui->ytunnusEdit, SIGNAL(textChanged(QString)), this, SLOT(ilmoitaMuokattu()));
    connect( ui->alvCheck, SIGNAL(clicked(bool)), this, SLOT(ilmoitaMuokattu()));
    connect( ui->edistyneetCheck, SIGNAL(clicked(bool)), this, SLOT(ilmoitaMuokattu()));
    connect( ui->osoiteEdit, SIGNAL(textChanged()), this, SLOT(ilmoitaMuokattu()));
    connect( ui->kotipaikkaEdit, SIGNAL(textChanged(QString)), this, SLOT(ilmoitaMuokattu()));
    connect( ui->puhelinEdit, SIGNAL(textChanged(QString)), this, SLOT(ilmoitaMuokattu()));
    connect( ui->paivitysCheck, SIGNAL(clicked(bool)), this, SLOT(ilmoitaMuokattu()));
    connect( ui->muotoCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(ilmoitaMuokattu()));

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
    ui->edistyneetCheck->setChecked( kp()->asetukset()->onko("NaytaEdistyneet"));
    ui->osoiteEdit->setText( kp()->asetukset()->asetus("Osoite"));
    ui->kotipaikkaEdit->setText( kp()->asetukset()->asetus("Kotipaikka"));
    ui->puhelinEdit->setText( kp()->asetus("Puhelin"));

    // Haetaan muodot

    QStringList muodot = kp()->asetukset()->avaimet("MuotoOn/");
    if( muodot.count())
    {
        for(QString muoto : muodot)
        {
            ui->muotoCombo->addItem( muoto.mid(8));
        }
        ui->muotoCombo->setCurrentIndex( ui->muotoCombo->findText( kp()->asetukset()->asetus("Muoto") ));
        ui->muotoLabel->setText( kp()->asetukset()->asetus("MuotoTeksti"));
    }
    else
    {
        ui->muotoLabel->hide();
        ui->muotoCombo->hide();
    }

    QSettings asetukset;
    ui->paivitysCheck->setChecked( asetukset.value("NaytaPaivitykset", true).toBool() );

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
    QSettings asetukset;

    return  ui->organisaatioEdit->text() != kp()->asetus("Nimi")  ||
            ui->ytunnusEdit->text() != kp()->asetus("Ytunnus") ||
            ui->alvCheck->isChecked() != kp()->asetukset()->onko("AlvVelvollinen") ||
            ui->edistyneetCheck->isChecked() != kp()->asetukset()->onko("NaytaEdistyneet") ||
            !uusilogo.isNull() ||
            ui->osoiteEdit->toPlainText() != kp()->asetukset()->asetus("Osoite") ||
            ui->kotipaikkaEdit->text() != kp()->asetukset()->asetus("Kotipaikka") ||
            ui->puhelinEdit->text() != kp()->asetukset()->asetus("Puhelin") ||
            ui->paivitysCheck->isChecked() != asetukset.value("NaytaPaivitykset",true).toBool() ||
            ( ui->muotoCombo->currentText() != kp()->asetukset()->asetus("Muoto"));
}

bool Perusvalinnat::tallenna()
{
    QSettings asetukset;
    asetukset.setValue("NaytaPaivitykset", ui->paivitysCheck->isChecked());

    kp()->asetukset()->aseta("Nimi", ui->organisaatioEdit->text());
    kp()->asetukset()->aseta("Ytunnus", ui->ytunnusEdit->text());
    kp()->asetukset()->aseta("AlvVelvollinen", ui->alvCheck->isChecked());
    kp()->asetukset()->aseta("NaytaEdistyneet", ui->edistyneetCheck->isChecked());
    kp()->asetukset()->aseta("Osoite", ui->osoiteEdit->toPlainText());
    kp()->asetukset()->aseta("Kotipaikka", ui->kotipaikkaEdit->text());
    kp()->asetukset()->aseta("Puhelin", ui->puhelinEdit->text());

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

        uusilogo.save(Kirjanpito::db()->hakemisto().absoluteFilePath("logo.png")  );    // Skaalaamaton logo
        uusilogo.scaled(64, 64, Qt::KeepAspectRatio).save( Kirjanpito::db()->hakemisto().absoluteFilePath("logo64.png")  );
        uusilogo.scaled(128, 128, Qt::KeepAspectRatio).save( Kirjanpito::db()->hakemisto().absoluteFilePath("logo128.png")  );
    }
    uusilogo = QImage();

    if( ui->muotoCombo->currentText() != kp()->asetukset()->asetus("Muoto"))
    {
        // Muodon vaihto pitää vielä varmistaa
        if( QMessageBox::question(0, tr("Vahvista muutos"),
                                  tr("Haluatko todella tehdä muutoksen\n"
                                     "%1: %2").arg( kp()->asetukset()->asetus("MuotoTeksti") )
                                              .arg( ui->muotoCombo->currentText()),
                                  QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Yes)
        {
            // Suorittaa muodon vaihtoon liittyvät skriptit
            Skripti::suorita("MuotoPois/" + kp()->asetukset()->asetus("Muoto") );
            kp()->asetukset()->aseta("Muoto", ui->muotoCombo->currentText());
            Skripti::suorita("MuotoOn/" + ui->muotoCombo->currentText());
            nollaa();
        }

    }
    else
        emit kp()->onni("Asetukset tallennettu");

    emit kp()->perusAsetusMuuttui();     // Uusi lataus, koska nimi tai kuva saattoi vaihtua!

    return true;
}
