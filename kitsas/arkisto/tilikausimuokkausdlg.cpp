/*
   Copyright (C) 2019 Arto Hyvättinen

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
#include "tilikausimuokkausdlg.h"
#include "ui_tilikausimuokkausdlg.h"

#include "db/kirjanpito.h"
#include <QPushButton>
#include <QMessageBox>

TilikausiMuokkausDlg::TilikausiMuokkausDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TilikausiMuokkausDlg)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    connect( ui->buttonBox, &QDialogButtonBox::helpRequested, [] { kp()->ohje("tilikaudet/"); });
}

TilikausiMuokkausDlg::~TilikausiMuokkausDlg()
{
    delete ui;
}

void TilikausiMuokkausDlg::muokkaa(Tilikausi kausi)
{
    kausi_ = kausi;
    alkuperainenAlkupaiva_ = kausi.alkaa();

    ui->alkaaEdit->setDate(kausi.alkaa());
    ui->loppuuEdit->setDate(kausi.paattyy());
    ui->virheLabel->hide();

    ui->henkilostoSpin->setValue(kausi.henkilosto());

    QDate lukituspaiva = kp()->tilitpaatetty();

    ui->lukittuCheck->setChecked( lukituspaiva >= kausi.alkaa() );
    ui->lukittuCheck->setEnabled( true );

    ui->avausCheck->setChecked( kausi.paattyy() == kp()->asetukset()->pvm("TilinavausPvm"));
    ui->avausCheck->setEnabled( kp()->tilikaudet()->tilikausiIndeksilla(0).paattyy() == kausi.paattyy() );

    pbtn = new QPushButton(QIcon(":/pic/poista.png"), tr("Poista tilikausi"));
    ui->buttonBox->addButton(pbtn, QDialogButtonBox::ActionRole);
    connect( pbtn, &QPushButton::clicked, this, &TilikausiMuokkausDlg::poista);
    connect( ui->alkaaEdit, &QDateEdit::dateChanged, this, &TilikausiMuokkausDlg::tarkastaKausi);
    connect( ui->loppuuEdit, &QDateEdit::dateChanged, this, &TilikausiMuokkausDlg::tarkastaKausi);
    connect( ui->lukittuCheck, &QCheckBox::clicked, this, &TilikausiMuokkausDlg::puralukko);
    connect( ui->avausCheck, &QCheckBox::clicked, this, &TilikausiMuokkausDlg::muutaAvaus);

    pbtn->setEnabled( !ui->lukittuCheck->isChecked() );

    exec();
}

void TilikausiMuokkausDlg::accept()
{
    if( ui->alkaaEdit->date() != kausi_.alkaa() ||
        ui->loppuuEdit->date() != kausi_.paattyy()) {
        if( QMessageBox::warning(this, tr("Tilikauden muuttaminen"),
                                 tr("Oletko varma, että haluat muuttaa tilikautta?\n\n"
                                    "Tilikauden muuttaminen saattaa aiheuttaa tositteiden jäämistä "
                                    "virheellisesti tilikausien ulkopuolelle, tositteiden numeroinnin vioittumista "
                                    "ja muitakin ongelmia kirjanpitoon."), QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel ) != QMessageBox::Yes)
            return;
    }
    Tilikausi uusi = kausi_;
    uusi.asetaAlkaa(ui->alkaaEdit->date());
    uusi.asetaPaattyy(ui->loppuuEdit->date());
    uusi.set("henkilosto", ui->henkilostoSpin->value());
    if( !ui->lukittuCheck->isChecked())
        uusi.unset("vahvistettu");

    uusi.tallenna(kausi_.alkaa());

    // Lukitseminen/lukituksen poisto
    if( ui->lukittuCheck->isChecked()) {
        if( kp()->tilitpaatetty() < kausi_.paattyy() )
            kp()->asetukset()->aseta("TilitPaatetty", uusi.paattyy());
    } else {
        if( kp()->tilitpaatetty() >= kausi_.paattyy() )
            kp()->asetukset()->aseta("TilitPaatetty", uusi.alkaa().addDays(-1));
    }

    // Tilinavaus
    if( ui->avausCheck->isChecked()) {
        kp()->asetukset()->aseta("TilinavausPvm", uusi.paattyy());        
        kp()->asetukset()->aseta("Tilinavaus", uusi.paattyy() < kp()->tilitpaatetty() ? 1 : 2);
    } else if( kp()->asetukset()->pvm("TilinavausPvm") == kausi_.paattyy()) {
        poistaTilinavaus();
    }
    QDialog::accept();

}

void TilikausiMuokkausDlg::tarkastaKausi()
{
    QDate alkaa = ui->alkaaEdit->date();
    QDate loppuu = ui->loppuuEdit->date();

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->virheLabel->show();

    if( loppuu < alkaa) {
        ui->virheLabel->setText(tr("Tilikauden loppu on ennen alkua"));
    } else if( loppuu > alkaa.addMonths(18)) {
        ui->virheLabel->setText("Tilikauden enimmäiskesto on 18 kuukautta");

    } else if( (kp()->tilikaudet()->tilikausiPaivalle(alkaa).alkaa().isValid() &&
               kp()->tilikaudet()->tilikausiPaivalle(alkaa).alkaa() != kausi_.alkaa()) ||
               (kp()->tilikaudet()->tilikausiPaivalle(loppuu).alkaa().isValid() &&
                              kp()->tilikaudet()->tilikausiPaivalle(loppuu).alkaa() != kausi_.alkaa())) {
        ui->virheLabel->setText(tr("Tilikausi menee päällekkäin toisen tilikauden kanssa"));
    }

    else {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        if( loppuu < kausi_.pvm("viimeinen")) {
            ui->virheLabel->show();
            ui->virheLabel->setText(tr("Kirjauksia jää tilikauden ulkopuolelle"));
        } else {
            ui->virheLabel->hide();
        }
    }

}

void TilikausiMuokkausDlg::puralukko()
{
    if( !ui->lukittuCheck->isChecked() && ui->alkaaEdit->date() < kp()->tilitpaatetty()) {
        if( QMessageBox::warning(this, tr("Tilikauden lukitsemisen peruminen"),
                                 tr("Oletko varma, että haluat perua tilikauden lukitsemisen?\n\n"
                                    "Kaikki tilinpäätökseen liittyvät toimet on tehtävä uudelleen ja tilinpäätös on mahdollisesti "
                                    "myös vahvistettava uudelleen.\n\n"
                                    "Kirjanpitolaki 2. luku 7§ 2 mom:\n"
                                    "Tositteen, kirjanpidon tai muun kirjanpitoaineiston sisältöä ei saa muuttaa tai poistaa "
                                    "tilinpäätöksen laatimisen jälkeen."), QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel ) != QMessageBox::Yes)
            ui->lukittuCheck->setChecked(true);

    }
    pbtn->setEnabled( !ui->lukittuCheck->isChecked() );
}

void TilikausiMuokkausDlg::poistaTilinavaus()
{
    kp()->asetukset()->poista("TilinavausPvm");
    kp()->asetukset()->poista("Tilinavaus");
    if(tilinavausId_) {
        KpKysely *poisto = kpk(QString("/tositteet/%1").arg(tilinavausId_), KpKysely::DELETE);
        connect(poisto, &KpKysely::vastaus, kp(), &Kirjanpito::kirjanpitoaMuokattu);
        poisto->kysy();
    }
}

void TilikausiMuokkausDlg::muutaAvaus()
{
    if( ui->avausCheck->isChecked()) {
        if( kp()->asetukset()->pvm("TilinavausPvm") != kausi_.paattyy()) {
            if( QMessageBox::warning(this, tr("Tilinavauksen lisääminen"),
                                     tr("Haluatko tehdä tästä tilikaudesta tilinavauksen?\nSinun syötettävä tilinavaus vielä erikseen."), QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel ) != QMessageBox::Yes)
                ui->avausCheck->setChecked(false);
        }
    } else if(kp()->asetukset()->pvm("TilinavausPvm") == kausi_.paattyy()) {
        if( QMessageBox::warning(this, tr("Tilinavauksen poistaminen"),
                                 tr("Haluatko merkitä, että tämä tilikausi ei ole tilinavaus?\nValinta poistaa myös tilinavauksen."), QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel ) != QMessageBox::Yes)
            ui->avausCheck->setChecked(true);
    }
}

void TilikausiMuokkausDlg::poista()
{
    if( kausi_.pvm("viimeinen").isNull()) {
        if( QMessageBox::warning(this, tr("Tilikauden poistaminen"),
                                 tr("Oletko varma, että haluat poistaa tilikauden?"), QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel ) != QMessageBox::Yes)
            return;
    } else {
        if( QMessageBox::warning(this, tr("Tilikauden poistaminen"),
                                 tr("Oletko varma, että haluat poistaa tilikauden?\n\n"
                                    "Tilikauden tositteita ei poisteta, mutta ne jäävät "
                                    "virheellisesti tilikausien ulkopuolelle eivätkä näy käyttäjälle "
                                    "ennen kuin niille on lisätty tilikausi."), QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel ) != QMessageBox::Yes)
            return;
    }
    if( kausi_.paattyy() == kp()->asetukset()->pvm("TilinavausPvm")) {
        poistaTilinavaus();
    }
    kausi_.poista();
    QDialog::accept();
}

void TilikausiMuokkausDlg::tilinavausId(QVariant *data)
{
    QVariantList lista = data->toList();
    if( !lista.isEmpty()) {
        QVariantMap map = lista.first().toMap();
        tilinavausId_ = map.value("id").toInt();
    }
}
