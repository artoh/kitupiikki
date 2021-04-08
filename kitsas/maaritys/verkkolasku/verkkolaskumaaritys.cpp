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
#include "pilvi/pilvikysely.h"
#include <QSettings>
#include <QButtonGroup>
#include <QFileDialog>
#include <QMessageBox>

#include "kierto/kiertomodel.h"
#include "maventadialog.h"
#include "tools/finvoicehaku.h"

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
    connect( ui->noutoCheck, &QCheckBox::clicked, this, &VerkkolaskuMaaritys::setFlow);
    connect(ui->ivAsetusNappi, &QPushButton::clicked, this, &VerkkolaskuMaaritys::maaritaMaventa);
    connect( ui->postitusCheck, &QCheckBox::clicked, [this] { emit this->tallennaKaytossa(this->onkoMuokattu());});
    connect( ui->noudaNappi, &QPushButton::clicked, this, &VerkkolaskuMaaritys::noudaNyt);
    connect( ui->suosiCheck, &QPushButton::clicked, this, &VerkkolaskuMaaritys::valintaMuuttui);
}

VerkkolaskuMaaritys::~VerkkolaskuMaaritys()
{
    delete ui;
}

void VerkkolaskuMaaritys::paivitaMaventa()
{
    ui->kayttajaLabel->setText(tr("Haetaan käyttäjätietoja..."));
    QString osoite = QString("%1/maventa/%2").arg(kp()->pilvi()->finvoiceOsoite()).arg(kp()->asetukset()->asetus(AsetusModel::Ytunnus));
    PilviKysely *pk = new PilviKysely( kp()->pilvi(), KpKysely::GET, osoite);
    connect(pk, &PilviKysely::vastaus, this, &VerkkolaskuMaaritys::maventaTiedot);
    connect(pk, &PilviKysely::virhe, this, &VerkkolaskuMaaritys::eiTietoja);

    pk->kysy();
}

bool VerkkolaskuMaaritys::tallenna()
{
    int lasku = ui->paikallinen->isChecked() ? PAIKALLINEN :
                                               ui->integroitu->isChecked() ? MAVENTA :
                                                                             EIKAYTOSSA;
    if( lasku == PAIKALLINEN && ( ui->ovtEdit->text().length() < 12 ||
                                  ui->operaattoriEdit->text().length() < 4 ||
                                  ui->hakemistoEdit->text().isEmpty())) {

        QMessageBox::critical(this, tr("Puutteelliset määritykset"),
                              tr("Verkkolaskutuksen määritykset ovat puutteelliset."));
        return false;
    }

    if( kp()->yhteysModel()->onkoOikeutta(YhteysModel::ASETUKSET)) {
        QVariantMap inits;
        inits.insert("FinvoiceKaytossa", lasku);
        inits.insert("OvtTunnus", ui->ovtEdit->text());
        inits.insert("Operaattori", ui->operaattoriEdit->text());
        inits.insert("MaventaPostitus", ui->postitusCheck->isChecked() ? "ON" : "EI");
        inits.insert("FinvoiceSuosi", ui->suosiCheck->isChecked() ? "ON" : "EI");
        kp()->asetukset()->aseta(inits);
    }

    kp()->settings()->setValue( QString("FinvoiceHakemisto/%1").arg(kp()->asetukset()->asetus(AsetusModel::UID)), ui->hakemistoEdit->text() );
    return true;
}


bool VerkkolaskuMaaritys::nollaa()
{
    int verkkolasku = kp()->asetukset()->luku("FinvoiceKaytossa");

    ui->integroitu->setEnabled( qobject_cast<PilviModel*>(kp()->yhteysModel()) ||
                                kp()->pilvi()->tilausvoimassa());

    ui->kaytossaGroup->setEnabled( kp()->yhteysModel()->onkoOikeutta(YhteysModel::ASETUKSET) );

    ui->eiKaytossa->setChecked( verkkolasku == EIKAYTOSSA);
    ui->paikallinen->setChecked( verkkolasku == PAIKALLINEN);
    ui->integroitu->setChecked( verkkolasku == MAVENTA);

    ui->osoiteGroup->setEnabled( kp()->yhteysModel()->onkoOikeutta(YhteysModel::ASETUKSET) );

    ui->ovtEdit->setText( kp()->asetukset()->asetus(AsetusModel::OvtTunnnus) );
    ui->operaattoriEdit->setText( kp()->asetukset()->asetus(AsetusModel::Operaattori));

    ui->hakemistoEdit->setText( kp()->settings()->value( QString("FinvoiceHakemisto/%1").arg(kp()->asetukset()->asetus(AsetusModel::UID))).toString());
    ui->postitusCheck->setChecked( kp()->asetukset()->onko("MaventaPostitus") );

    ui->noutoCheck->setDisabled(true);
    ui->suosiCheck->setChecked( kp()->asetukset()->onko("FinvoiceSuosi") );


    valintaMuuttui();

    QStringList vaaditutYhteystiedot;
    vaaditutYhteystiedot << "Kotipaikka" << "Kaupunki" << "Postinumero" << "Katuosoite";
    bool puuttuu = false;
    for(const auto& tieto : vaaditutYhteystiedot)
        if( kp()->asetukset()->asetus(tieto).length() < 2 )
            puuttuu = true;
    ui->yhteystietovirheLabel->setVisible(puuttuu);


    if( kp()->pilvi()->kayttajaPilvessa()) {
        ui->kirjauduLabel->hide();
    }

    if( !kp()->pilvi()->kayttajaPilvessa() || puuttuu)
    {
        ui->kirjauduLabel->show();
        ui->kaytossaGroup->setEnabled(false);
        ui->hakemistoGroup->setEnabled(false);
        ui->maventaGroup->setEnabled(false);
    }    

    return true;
}

bool VerkkolaskuMaaritys::onkoMuokattu()
{
    int lasku = ui->paikallinen->isChecked() ? PAIKALLINEN :
                                               ui->integroitu->isChecked() ? MAVENTA :
                                                                             EIKAYTOSSA;
    return lasku != kp()->asetukset()->luku(AsetusModel::FinvoiceKaytossa) ||
           ui->ovtEdit->text() != kp()->asetukset()->asetus(AsetusModel::OvtTunnnus) ||
           ui->operaattoriEdit->text() != kp()->asetukset()->asetus(AsetusModel::Operaattori) ||
           ui->hakemistoEdit->text() != kp()->settings()->value( QString("FinvoiceHakemisto/%1").arg(kp()->asetukset()->asetus(AsetusModel::UID))).toString() ||
           ui->postitusCheck->isChecked() != kp()->asetukset()->onko("MaventaPostitus") ||
           ui->suosiCheck->isChecked() != kp()->asetukset()->onko("FinvoiceSuosi");

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
    ui->hakemistoGroup->setVisible( ui->paikallinen->isChecked());
    ui->maventaGroup->setVisible( ui->integroitu->isChecked());
    ui->suosiCheck->setEnabled( !ui->eiKaytossa->isChecked());

    if( ui->integroitu->isChecked()) {
        paivitaMaventa();
    }

    if( !ui->eiKaytossa->isChecked() && ui->ovtEdit->text().isEmpty())
        ui->ovtEdit->setText("0037" + kp()->asetukset()->asetus(AsetusModel::Ytunnus).remove('-'));
    if( ui->integroitu->isChecked() && ui->operaattoriEdit->text().isEmpty())
        ui->operaattoriEdit->setText("003721291126");

    emit tallennaKaytossa(onkoMuokattu());
}

void VerkkolaskuMaaritys::maaritaMaventa()
{
    MaventaDialog *dlg = new MaventaDialog(this);
    connect( dlg, &MaventaDialog::liitetty, this, &VerkkolaskuMaaritys::maventaTiedot);
    dlg->exec();
    dlg->deleteLater();

}

void VerkkolaskuMaaritys::maventaTiedot(QVariant *data)
{
    QVariantMap info = data ? data->toMap() : QVariantMap();
    qDebug() << info;

    if( info.isEmpty()) {
        ui->noutoCheck->setEnabled(false);
        ui->noudaNappi->setEnabled(false);
        ui->kayttajaLabel->setText(tr("Käyttäjää ei määritelty"));
    } else {
        QString txt = QString("%1 %2 \n%3")
                .arg(info.value("user").toMap().value("first_name").toString())
                .arg(info.value("user").toMap().value("last_name").toString())
                .arg(info.value("company").toMap().value("name").toString());

        if( info.value("send_invoice_print").toMap().value("enabled").toBool()) {
            txt.append("\n\nTulostuspalvelu on käytössä.");
        } else {
            txt.append("\n\nTulostuspalvelu ei ole käytössä. Tulostat itse postitettavaksi merkittävät laskut.");
        }

        QVariantMap virheviestit = info.value("invoice_notifications").toMap().value("on_send_errors").toMap();
        qDebug() << virheviestit;
        if( !virheviestit.value("to_user").toBool() && virheviestit.value("to_emails").isNull())
            txt.append(tr("\n\nEt saa ilmoitusta, jos laskun lähettäminen epäonnistuu. "
                          "Ota Maventan asetuksissa käyttöön Vastaanota ilmoituksia laskujen lähetysvirheistä."));

        ui->kayttajaLabel->setText(txt);
        ui->noutoCheck->setChecked( !info.value("flow").toMap().isEmpty() );
        ui->noutoCheck->setEnabled( kp()->yhteysModel()->onkoOikeutta(YhteysModel::ASETUKSET) );
        ui->noutoCheck->setEnabled( kp()->yhteysModel()->onkoOikeutta(YhteysModel::TOSITE_LUONNOS) ||
                                    kp()->yhteysModel()->onkoOikeutta(YhteysModel::KIERTO_LISAAMINEN));
        ui->postitusCheck->setEnabled( kp()->yhteysModel()->onkoOikeutta(YhteysModel::ASETUKSET) );

    }
}

void VerkkolaskuMaaritys::eiTietoja()
{
    maventaTiedot(nullptr);
}

void VerkkolaskuMaaritys::setFlow(bool on)
{
    QString osoite = QString("%1/maventa/%2/flow").arg(kp()->pilvi()->finvoiceOsoite()).arg(kp()->asetukset()->asetus(AsetusModel::Ytunnus).simplified());
    PilviKysely *pk = new PilviKysely( kp()->pilvi(), KpKysely::POST, osoite);
    if( !on)
        pk->lisaaAttribuutti("off","");
    connect(pk, &PilviKysely::vastaus, this, &VerkkolaskuMaaritys::maventaTiedot);
    pk->kysy();
}

void VerkkolaskuMaaritys::noudaNyt()
{
    if( qobject_cast<PilviModel*>(kp()->yhteysModel())) {
        QString osoite = QString("%1/maventa/fetch").arg(kp()->pilvi()->finvoiceOsoite()).arg(kp()->asetukset()->asetus(AsetusModel::Ytunnus));
        PilviKysely *pk = new PilviKysely( kp()->pilvi(), KpKysely::POST, osoite);
        connect(pk, &PilviKysely::vastaus, [] { emit kp()->onni("Verkkolaskut päivitetty kiertoon"); });
        pk->kysy();
    } else {
        FinvoiceHaku::instanssi()->haeUudet();
    }
}
