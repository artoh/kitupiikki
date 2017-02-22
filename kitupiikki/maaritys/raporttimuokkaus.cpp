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

#include "raporttimuokkaus.h"
#include "db/kirjanpito.h"
#include "raportinkorostin.h"

#include <QDebug>
#include <QMessageBox>

RaporttiMuokkaus::RaporttiMuokkaus(QWidget *parent) :
    MaaritysWidget(parent)
{
    ui = new Ui::RaporttiMuokkain;
    ui->setupUi(this);

    new RaportinKorostin( ui->editori->document() );

    connect( ui->valintaCombo, SIGNAL(currentIndexChanged(QString)), this, SLOT(avaaRaportti(QString)));
    connect( ui->editori, SIGNAL(textChanged()), this, SLOT(merkkaaMuokattu()));
    connect( ui->uusiNappi, SIGNAL(clicked(bool)), this, SLOT(uusi()));
    connect( ui->kopioiNappi, SIGNAL(clicked(bool)), this, SLOT(kopio()));

    muokattu = false;
}

RaporttiMuokkaus::~RaporttiMuokkaus()
{
    delete ui;
}

bool RaporttiMuokkaus::nollaa()
{
    // Haetaan comboon käytettävissä olevat raportit
    ui->valintaCombo->clear();
    foreach (QString nimi, kp()->asetukset()->avaimet("Raportti/") )
    {
        ui->valintaCombo->addItem( nimi.mid(9) );
    }

    return true;
}

bool RaporttiMuokkaus::tallenna()
{
    QString uusinimi = ui->valintaCombo->currentText();
    if( uusinimi.length() < 4)
    {
        if( uusinimi.isEmpty())
            QMessageBox::critical(this, tr("Tallennettavalla raportilla ei nimeä"),
                              tr("Kirjoita tallennettavalle raportille nimi Uusi-napin vasemmalla "
                                 "puolella olevaan tilaan ja yritä tallentamista uudelleen"));
        else
            QMessageBox::critical(this, tr("Tallennettavan raportin nimi liian lyhyt"),
                              tr("Raportin nimen oltava vähintään neljä kirjainta."
                                 "Kirjoita tallennettavalle raportille nimi Uusi-napin vasemmalla "
                                 "puolella olevaan tilaan ja yritä tallentamista uudelleen"));
        return false;
    }

    QString tallennettava;

    if( ui->taseRadio->isChecked())
        tallennettava = ":tase\n";
    else
        tallennettava = ":tulos\n";

    tallennettava.append( ui->editori->toPlainText());

    kp()->asetukset()->aseta( "Raportti/" + uusinimi, tallennettava);
    if( uusinimi != vanhanimi)
        kp()->asetukset()->poista("Raportti/" + vanhanimi);

    muokattu = false;
    emit tallennaKaytossa(false);

    return true;
}

bool RaporttiMuokkaus::onkoMuokattu()
{
    return muokattu;
}

void RaporttiMuokkaus::avaaRaportti(const QString &raportti)
{
    if( onkoMuokattu() && raportti != vanhanimi && ui->editori->toPlainText().length() > 10)
        if( !kysyTallennus())
        {
            ui->valintaCombo->setCurrentIndex( ui->valintaCombo->findText( vanhanimi) );
            return;
        }

    if( !raportti.isEmpty() && vanhanimi != raportti)
    {

        QString teksti = kp()->asetukset()->asetus( "Raportti/" + raportti );
        int rivinvaihto = teksti.indexOf('\n');

        QString ekarivi = teksti.left( rivinvaihto );
        ui->taseRadio->setChecked( ekarivi.startsWith(":tase"));
        ui->tulosRadio->setChecked( ekarivi.startsWith(":tulos"));


        ui->editori->setPlainText(teksti.mid(rivinvaihto + 1));
        muokattu = false;
        vanhanimi = raportti;
        emit tallennaKaytossa(false);

    }
}

void RaporttiMuokkaus::merkkaaMuokattu()
{
    muokattu = true;
    emit tallennaKaytossa(true);
}

void RaporttiMuokkaus::uusi()
{
    if( onkoMuokattu()  && ui->editori->toPlainText().length() > 10 )
        if( !kysyTallennus())
            return;

    // Aloittaa muokkauksen tyhjästä
    ui->editori->clear();
    vanhanimi.clear();
    ui->valintaCombo->addItem( QString() );
    ui->valintaCombo->setCurrentIndex( ui->valintaCombo->count() - 1 );
    muokattu = false;
    emit tallennaKaytossa(false);
}

void RaporttiMuokkaus::kopio()
{
    if( onkoMuokattu()  && ui->editori->toPlainText().length() > 10 )
        if( !kysyTallennus())
            return;

    // Aloittaa uuden muokkauksen tämän pohjalta
    vanhanimi.clear();
    ui->valintaCombo->addItem( QString() );
    ui->valintaCombo->setCurrentIndex( ui->valintaCombo->count() - 1 );
    muokattu = false;
    emit tallennaKaytossa(false);
}

bool RaporttiMuokkaus::kysyTallennus()
{
    int tallennetaanko =  QMessageBox::question(this, tr("Raportin muokkaus"),
                                                tr("Raporttia on muokattu.\n"
                                                   "Tallennetaanko muokattu raportti?"),
                                                QMessageBox::Save | QMessageBox::No | QMessageBox::Cancel);
    if( tallennetaanko == QMessageBox::Save)
        tallenna();
    return tallennetaanko == QMessageBox::Save || tallennetaanko == QMessageBox::No;
}
