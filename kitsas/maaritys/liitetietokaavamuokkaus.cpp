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
    for(auto kieli: kp()->asetukset()->kielet()) {
        ui->kieliCombo->addItem(lippu(kieli) , kp()->asetukset()->kieli(kieli), kieli);
    }
    ui->kieliCombo->setCurrentIndex( ui->kieliCombo->findData( kp()->asetus("kieli") ) );

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
    connect(dlgUi.buttonBox, &QDialogButtonBox::helpRequested, [] { kp()->ohje("maaritykset/tilinpaatos");});

    // Lisätään raportit dialogiin
    foreach (QString nimi, kp()->asetukset()->avaimet("tase/") )
    {
        dlgUi.rapottiCombo->addItem( nimi );
    }
    for( auto nimi : kp()->asetukset()->avaimet("tulos/"))
        dlgUi.rapottiCombo->addItem( nimi );

    connect( dlgUi.rapottiCombo, &QComboBox::currentTextChanged, [dlgUi] (const QString& raportti) { dlgUi.groupBox->setEnabled(raportti.startsWith("tulos")); } );

    if( dlg.exec() )        
    {
        QString optiot;
        if( dlgUi.erittelyCheck->isChecked())
            optiot.append("E");
        if( dlgUi.rapottiCombo->currentText().startsWith("tulos")) {
            if( dlgUi.kustannuspaikatRadio->isChecked())
                optiot.append("K");
            else if( dlgUi.projektitRadio->isChecked())
                optiot.append("P");
            if( dlgUi.budjettiCheck->isChecked())
                optiot.append("B");
        }
        ui->editori->insertPlainText(QString("@%1:%2!%3@")
                                     .arg(dlgUi.rapottiCombo->currentText())
                                     .arg(optiot)
                                     .arg(dlgUi.otsikkoLine->text()));
    }
}
