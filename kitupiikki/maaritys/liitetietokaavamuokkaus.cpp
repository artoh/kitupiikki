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

#include "liitetietokaavamuokkaus.h"
#include "db/kirjanpito.h"
#include "kaavankorostin.h"

#include "ui_lisaaraporttidialogi.h"

#include <QDialog>

LiitetietokaavaMuokkaus::LiitetietokaavaMuokkaus() :
    ui( new Ui::Kaavaeditori)
{
    ui->setupUi(this);
    new KaavanKorostin(  ui->editori->document());
    connect( ui->editori, SIGNAL(textChanged()), this, SLOT(ilmoitaOnkoMuokattu()));
    connect( ui->raporttiNappi, SIGNAL(clicked(bool)), this, SLOT(lisaaRaportti()));
    connect( ui->kieliCombo, &QComboBox::currentTextChanged, this, &LiitetietokaavaMuokkaus::lataa);
}

LiitetietokaavaMuokkaus::~LiitetietokaavaMuokkaus()
{
    delete ui;
}

bool LiitetietokaavaMuokkaus::nollaa()
{
    ui->kieliCombo->clear();
    QStringList kaavat = kp()->asetukset()->avaimet("tppohja/");
    for(auto kaava: kaavat) {
        QString kieli = kaava.mid(8);
        ui->kieliCombo->addItem(QIcon(":/liput/" + kieli), kp()->asetukset()->kieli(kieli), kieli);
    }
    return true;
}

bool LiitetietokaavaMuokkaus::tallenna()
{
    QString kieli = ui->kieliCombo->currentData().toString();
    kp()->asetukset()->aseta("tppohja/" + kieli, ui->editori->toPlainText() );
    ui->editori->document()->setModified(false);
    ilmoitaOnkoMuokattu();
    return true;
}

bool LiitetietokaavaMuokkaus::onkoMuokattu()
{
    return ui->editori->document()->isModified();
}

void LiitetietokaavaMuokkaus::lataa()
{
    QString kieli = ui->kieliCombo->currentData().toString();
    ui->editori->setPlainText( kp()->asetukset()->asetus("tppohja/" + kieli) );
}

void LiitetietokaavaMuokkaus::ilmoitaOnkoMuokattu()
{
    emit tallennaKaytossa(onkoMuokattu());
}

void LiitetietokaavaMuokkaus::lisaaRaportti()
{
    QDialog dlg;
    Ui::LisaaRaportti dlgUi;
    dlgUi.setupUi(&dlg);

    // Lisätään raportit dialogiin
    foreach (QString nimi, kp()->asetukset()->avaimet("Raportti/") )
    {
        dlgUi.rapottiCombo->addItem( nimi.mid(9) );
    }

    if( dlg.exec() )
    {
        // Since 1.1 Mahdollisuus budjettivertailuun (vain tuloslaskelmassa)
        QString budjettivertailu = dlgUi.vertailuBox->isChecked() ? "$" : QString();

        if( dlgUi.erittelyCheck->isChecked() )
            ui->editori->insertPlainText( QString("@%1%3*%2@")
                                          .arg( dlgUi.rapottiCombo->currentText() )
                                          .arg( dlgUi.otsikkoLine->text())
                                          .arg(budjettivertailu));
        else
            ui->editori->insertPlainText( QString("@%1%3!%2@")
                                          .arg( dlgUi.rapottiCombo->currentText() )
                                          .arg( dlgUi.otsikkoLine->text())
                                          .arg(budjettivertailu));

    }

}
