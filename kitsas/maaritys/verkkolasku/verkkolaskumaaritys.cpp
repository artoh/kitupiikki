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
#include "verkkolaskumaaritys.h"

#include "ui_verkkolasku.h"
#include "db/kirjanpito.h"
#include "db/yhteysmodel.h"
#include "pilvi/pilvimodel.h"
#include <QSettings>
#include <QButtonGroup>
#include <QFileDialog>
#include <QMessageBox>

#include "kierto/kiertomodel.h"

VerkkolaskuMaaritys::VerkkolaskuMaaritys() :
    MaaritysWidget(),
    ui(new Ui::Finvoicevalinnat)
{
    ui->setupUi(this);

    connect( ui->eiKaytossa, &QPushButton::clicked, this, &VerkkolaskuMaaritys::valintaMuuttui);
    connect( ui->paikallinen, &QPushButton::clicked, this, &VerkkolaskuMaaritys::valintaMuuttui);
    connect( ui->integroitu, &QPushButton::clicked, this, &VerkkolaskuMaaritys::valintaMuuttui);
    connect( ui->hakemistoNappi, &QPushButton::clicked, this, &VerkkolaskuMaaritys::valitseKansio);
    connect( ui->ovtEdit, &QLineEdit::textEdited, [this] { emit this->tallennaKaytossa(this->onkoMuokattu());});
    connect( ui->operaattoriEdit, &QLineEdit::textEdited, [this] { emit this->tallennaKaytossa(this->onkoMuokattu());});
}

VerkkolaskuMaaritys::~VerkkolaskuMaaritys()
{
    delete ui;
}

bool VerkkolaskuMaaritys::tallenna()
{
    int lasku = ui->paikallinen->isChecked() ? PAIKALLINEN :
                                               ui->integroitu->isChecked() ? INTEGROITU :
                                                                             EIKAYTOSSA;
    if( lasku == PAIKALLINEN && ( ui->ovtEdit->text().length() < 12 ||
                                  ui->operaattoriEdit->text().length() < 4 ||
                                  ui->hakemistoEdit->text().isEmpty())) {

        QMessageBox::critical(this, tr("Puutteelliset määritykset"),
                              tr("Verkkolaskutuksen määritykset ovat puutteelliset."));
        return false;
    }
    QVariantMap inits;
    inits.insert("FinvoiceKaytossa", lasku);
    inits.insert("OvtTunnus", ui->ovtEdit->text());
    inits.insert("Operaattori", ui->operaattoriEdit->text());
    kp()->asetukset()->aseta(inits);

    ui->kiertoCombo->setModel(kp()->kierrot());

    kp()->settings()->setValue( QString("FinvoiceHakemisto/%1").arg(kp()->asetus("UID")), ui->hakemistoEdit->text() );
    return true;
}


bool VerkkolaskuMaaritys::nollaa()
{
    int verkkolasku = kp()->asetukset()->luku("FinvoiceKaytossa");

    ui->integroitu->setEnabled( qobject_cast<PilviModel*>(kp()->yhteysModel()) ||
                                kp()->pilvi()->plan() > 0);

    ui->kaytossaGroup->setEnabled( kp()->yhteysModel()->onkoOikeutta(YhteysModel::ASETUKSET) );

    ui->eiKaytossa->setChecked( verkkolasku == EIKAYTOSSA);
    ui->paikallinen->setChecked( verkkolasku == PAIKALLINEN);
    ui->integroitu->setChecked( verkkolasku == INTEGROITU);

    ui->osoiteGroup->setEnabled( kp()->yhteysModel()->onkoOikeutta(YhteysModel::ASETUKSET) );
    ui->noutoGroup->setEnabled( kp()->yhteysModel()->onkoOikeutta(YhteysModel::ASETUKSET) );

    ui->ovtEdit->setText( kp()->asetus("OvtTunnus") );
    ui->operaattoriEdit->setText( kp()->asetus("Operaattori"));

    ui->hakemistoEdit->setText( kp()->settings()->value( QString("FinvoiceHakemisto/%1").arg(kp()->asetus("UID"))).toString());

    ui->eiVastaanottoa->setChecked(true);

    valintaMuuttui();

    return true;
}

bool VerkkolaskuMaaritys::onkoMuokattu()
{
    int lasku = ui->paikallinen->isChecked() ? PAIKALLINEN :
                                               ui->integroitu->isChecked() ? INTEGROITU :
                                                                             EIKAYTOSSA;
    return lasku != kp()->asetukset()->luku("FinvoiceKaytossa") ||
           ui->ovtEdit->text() != kp()->asetus("OvtTunnus") ||
           ui->operaattoriEdit->text() != kp()->asetus("Operaattori") ||
           ui->hakemistoEdit->text() != kp()->settings()->value( QString("FinvoiceHakemisto/%1").arg(kp()->asetus("UID"))).toString();

}

void VerkkolaskuMaaritys::valitseKansio()
{
    QString hakemisto = QFileDialog::getExistingDirectory(this, tr("Valitse laskujen tallennushakemisto"));
    if( !hakemisto.isEmpty())
        ui->hakemistoEdit->setText(hakemisto);
    emit tallennaKaytossa(onkoMuokattu());
}

void VerkkolaskuMaaritys::valintaMuuttui()
{
    ui->ivAsetusNappi->setEnabled( ui->integroitu->isChecked() );
    ui->hakemistoGroup->setVisible( ui->paikallinen->isChecked());

    ui->saapuneetKansio->setEnabled( ui->integroitu->isChecked());
    ui->kiertoRadio->setEnabled( ui->integroitu->isChecked());
    ui->kiertoCombo->setEnabled( ui->integroitu->isChecked() && ui->kiertoCombo->count());
    ui->postitusCheck->setEnabled( ui->integroitu->isChecked());
    emit tallennaKaytossa(onkoMuokattu());
}
