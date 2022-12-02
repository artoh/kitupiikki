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

#include "finvoicevelho.h"
#include "model/toiminimimodel.h"
#include "rekisteri/asiakastoimittajadlg.h"

#include "maventadialog.h"
#include "tools/finvoicehaku.h"

#include <QDebug>
#include <QJsonDocument>

VerkkolaskuMaaritys::VerkkolaskuMaaritys() :
    MaaritysWidget(),
    ui(new Ui::Finvoicevalinnat)
{
    ui->setupUi(this);

    connect( ui->eiKaytossa, &QPushButton::clicked, this, &VerkkolaskuMaaritys::valintaMuuttui);
    connect( ui->paikallinen, &QPushButton::clicked, this, &VerkkolaskuMaaritys::valintaMuuttui);
    connect( ui->integroitu, &QPushButton::clicked, this, &VerkkolaskuMaaritys::valintaMuuttui);
    connect( ui->hakemistoNappi, &QPushButton::clicked, this, &VerkkolaskuMaaritys::valitseKansio);
    connect( ui->soapCheck, &QCheckBox::clicked, [this] { emit this->tallennaKaytossa(this->onkoMuokattu());});
    connect( ui->ovtEdit, &QLineEdit::textEdited, [this] { emit this->tallennaKaytossa(this->onkoMuokattu());});
    connect( ui->operaattoriEdit, &QLineEdit::textEdited, [this] { emit this->tallennaKaytossa(this->onkoMuokattu());});
    connect(ui->ivAsetusNappi, &QPushButton::clicked, this, &VerkkolaskuMaaritys::maaritaMaventa);
    connect( ui->noudaNappi, &QPushButton::clicked, this, &VerkkolaskuMaaritys::noudaNyt);
    connect( ui->suosiCheck, &QPushButton::clicked, this, &VerkkolaskuMaaritys::valintaMuuttui);
    connect( ui->velhoNappi, &QPushButton::clicked, this, &VerkkolaskuMaaritys::velho);
}

VerkkolaskuMaaritys::~VerkkolaskuMaaritys()
{
    delete ui;
}

void VerkkolaskuMaaritys::paivitaMaventa()
{        
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
        inits.insert("FinvoiceSuosi", ui->suosiCheck->isChecked() ? "ON" : "EI");
        inits.insert("FinvoiceSOAP", ui->soapCheck->isChecked() ? "ON" : "EI");
        kp()->asetukset()->aseta(inits);
    }

    kp()->settings()->setValue( QString("FinvoiceHakemisto/%1").arg(kp()->asetukset()->asetus(AsetusModel::UID)), ui->hakemistoEdit->text() );
    return true;
}


bool VerkkolaskuMaaritys::nollaa()
{
    maventaInfo_.clear();
    paivitaMaventa();    

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
    ui->soapCheck->setChecked( kp()->asetukset()->onko("FinvoiceSOAP") );

    ui->suosiCheck->setChecked( kp()->asetukset()->onko("FinvoiceSuosi") );
    ui->suosiCheck->setEnabled( kp()->yhteysModel()->onkoOikeutta(YhteysModel::ASETUKSET) );


    valintaMuuttui();

    QStringList vaaditutYhteystiedot;
    vaaditutYhteystiedot << "Kotipaikka" << "Kaupunki" << "Postinumero" << "Katuosoite" << "Email";
    bool puuttuu = false;
    for(const auto& tieto : vaaditutYhteystiedot)
        if( kp()->asetukset()->asetus(tieto).length() < 2 )
            puuttuu = true;
    ui->yhteystietovirheLabel->setVisible(puuttuu);

    ui->kirjauduLabel->setVisible( !kp()->pilvi()->kayttajaPilvessa() );

    if( !kp()->pilvi()->kayttajaPilvessa() || puuttuu)
    {
        ui->velhoGroup->setEnabled(false);
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
           ui->suosiCheck->isChecked() != kp()->asetukset()->onko("FinvoiceSuosi") ||
           ui->soapCheck->isChecked() != kp()->asetukset()->onko("FinvoiceSOAP");

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
    ui->odotaLabel->setVisible( maventaInfo_.isEmpty() );
    ui->velhoGroup->setVisible( ui->eiKaytossa->isChecked() || maventaInfo_.value("company").toMap().value("company_state").toString() == "UNVERIFIED" );
    ui->kaytossaGroup->setVisible( !ui->eiKaytossa->isChecked() && !maventaInfo_.isEmpty() && !maventaInfo_.value("book").toMap().value("active").toBool() );
    ui->hakemistoGroup->setVisible( ui->paikallinen->isChecked());    
    ui->osoiteGroup->setVisible( !ui->eiKaytossa->isChecked() );
    ui->maventaGroup->setVisible( (ui->eiKaytossa->isChecked() && maventaInfo_.value("company").toMap().value("company_state").toString() == "UNVERIFIED" )
                                  || (ui->integroitu->isChecked() && !maventaInfo_.isEmpty())  );
    ui->suosiCheck->setVisible(!ui->eiKaytossa->isChecked() && kp()->yhteysModel()->onkoOikeutta(YhteysModel::ASETUKSET));

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
    dlg->lataa(maventaInfo_);
    if( dlg->exec() == QDialog::Accepted) {
        ui->integroitu->setChecked(true);
        valintaMuuttui();
    }
    dlg->deleteLater();

}

void VerkkolaskuMaaritys::maventaTiedot(QVariant *data)
{

    maventaInfo_ = data ? data->toMap() : QVariantMap();
    QVariantMap book = maventaInfo_.value("book").toMap();
    QVariantMap company = maventaInfo_.value("company").toMap();
    QVariantMap user = maventaInfo_.value("user").toMap();
    QVariantList profiles = maventaInfo_.value("profiles").toList();


    qDebug() << QJsonDocument::fromVariant(maventaInfo_).toJson(QJsonDocument::Compact);

    QString vismaStatus;
    QString bankStatus;
    QString peppolStatus;

    for(const auto& profile : profiles) {
        QVariantMap map = profile.toMap();
        const QString network = map.value("network").toString();
        const QString status = map.value("status").toString();

        if( network == "VISMA") {
            vismaStatus = status;
            // Verkkolaskuosoite löytyy tästä!
            ui->ovtEdit->setText( map.value("endpoint_id").toString() );
            ui->ovtEdit->setReadOnly(true);
            ui->operaattoriEdit->setText("003721291126");
            ui->operaattoriEdit->setReadOnly(true);
            if( onkoMuokattu() ) {
                tallenna();
                emit tallennaKaytossa(false);
            }
        } else if( network == "BANK") {
            bankStatus = status;
        } else if( network == "PEPPOL") {
            peppolStatus = status;
        }
    }

    const bool profileOk =
            vismaStatus == "active" &&
            bankStatus == "active" &&
            peppolStatus == "active";

    ui->maventaOk->setVisible( book.value("active").toBool() && profileOk);
    ui->virheLabel->setVisible( !profileOk );
    ui->noudaNappi->setEnabled(  profileOk );

    valintaMuuttui();
    ui->odotaLabel->setVisible(false);

    if( maventaInfo_.contains("availability")) {
        if( maventaInfo_.value("availability").toString() == "FREE") {
            ui->maventaInfo->setText(tr("Yritykselläsi ei ole vielä Maventan verkkolaskutiliä."));
        } else {
            ui->maventaInfo->setText(tr("Yritykselläsi on jo Maventan verkkolaskutili"));
        }
        return;
    } else if( !profileOk ) {
        QVariantMap status = maventaInfo_.value("status").toMap();
        const QString authState = status.value("auth_state").toString();
        const QString authEmail = status.value("auth_email").toString();

        if( authState == "PENDING") {
            ui->maventaInfo->setText( tr("Käyttöönottoon tarvittava allekirjoituspyyntö lähetetään pian sähköpostiosoitteeseen %1").arg(authEmail));
        } else if( authState == "SENT") {
            ui->maventaInfo->setText( tr("Käyttöönottoon tarvittava allekirjoituspyyntö on lähetetty sähköpostiosoitteeseen %1").arg(authEmail));
        } else if( authState == "CANCELED") {
            ui->maventaInfo->setText( tr("Sähköinen allekirjoituspyyntö on peruttu") );
        } else if( authState == "NONE") {
            ui->maventaInfo->setText( tr("Käyttöönottoa ei ole vahvistettu sähköisellä allekirjoituksella"));
        } else if( vismaStatus == "error") {
            ui->maventaInfo->setText( tr("Verkkolaskuosoitteen rekisteröinnissä on tapahtunut virhe."));
        } else if( vismaStatus == "pending" ) {
            ui->maventaInfo->setText( tr("Verkkolaskuosoitteesi rekisteröinti on kesken."));
        } else if( vismaStatus.isEmpty()) {
            ui->maventaInfo->setText( tr("Verkkolaskujen vastaanottoa ei ole otettu käyttöön."));
        } else if( bankStatus == "error" ) {
            ui->maventaInfo->setText( tr("Pankkiyhteyden käyttöönotossa on tapahtunut virhe."));
        } else if( bankStatus == "pending" ) {
            ui->maventaInfo->setText( tr("Pankkiyhteyden käyttöönotto on kesken. Tämä voi kestää muutaman arkipäivän."));
        } else if( bankStatus.isEmpty()) {
            ui->maventaInfo->setText( tr("Pankkiyhteyttä ei ole otettu käyttöön."));
        } else if( peppolStatus == "error" ) {
            ui->maventaInfo->setText( tr("Kansainvälisen PEPPOL-yhteyden käyttöönotossa on tapahtunut virhe.") );
        } else if( peppolStatus == "pendign") {
            ui->maventaInfo->setText( tr("Kansainvälisen PEPPOL-yhteyden käyttöönotto on kesken.") );
        } else if( peppolStatus.isEmpty()) {
            ui->maventaInfo->setText( tr("Kansainvälistä PEPPOL-yhteyttä ei ole otettu käyttöön.") );
        }
        return;
    }

    QVariantMap print = maventaInfo_.value("send_invoice_print").toMap();    

    ui->maventaInfo->setText(company.value("name").toString() + "\n" +
            ( book.value("kitsasuser").toBool() ? "" : user.value("first_name").toString() + " " + user.value("last_name").toString() + "\n") + "\n" +
            ( book.value("kitsasbilling").toBool() ? tr("Verkkolaskujen hinnoittelu Kitsaan hinnaston mukaan.") : tr("Verkkolaskujen hinnoittelu Maventan hinnaston mukaan.") ) + "\n" +
            ( ( book.value("active").toBool() ? tr("Verkkolaskujen nouto on käytössä") : tr("Verkkolaskujen automaattista noutoa ei ole otettu käyttöön") ) + "\n" ) +
            ( (print.value("enabled").toBool() && kp()->asetukset()->onko("MaventaPostitus")) ? tr("Postitettavien laskujen tulostus ja postitus Maventan kautta käytössä") : tr("Tulostuspalvelu ei ole käytössä") ) );

    if( !qobject_cast<PilviModel*>(kp()->yhteysModel()) ) {
        // Paikallisessa kirjanpidossa päivitetään aktiivisuustieto
        // kirjanpidon asetuksiin, mikä määrittelee, noudetaanko
        // verkkolaskut ohjelman käynnistyessä
        kp()->asetukset()->aseta("PaikallinenMaventaHaku", book.value("active").toBool());
    }
}

void VerkkolaskuMaaritys::eiTietoja()
{
    maventaTiedot(nullptr);
}

void VerkkolaskuMaaritys::noudaNyt()
{
    if( qobject_cast<PilviModel*>(kp()->yhteysModel())) {
        QString osoite = QString("%1/maventa/fetch").arg(kp()->pilvi()->finvoiceOsoite()).arg(kp()->asetukset()->asetus(AsetusModel::Ytunnus));
        PilviKysely *pk = new PilviKysely( kp()->pilvi(), KpKysely::POST, osoite);
        connect(pk, &PilviKysely::vastaus, [] { emit kp()->onni("Verkkolaskut päivitetty kiertoon"); });
        pk->kysy();
    } else {
        FinvoiceHaku::instanssi()->haeUudetKayttajanPyynnosta();
    }
}

void VerkkolaskuMaaritys::velho()
{
    FinvoiceVelho velho(this);

    velho.kitsasKaytossa( maventaInfo_.value("availability").toString() == "FREE" ||
                          maventaInfo_.value("company").toMap().value("company_state").toString() == "UNVERIFIED" );

    if( velho.exec() != QWizard::Accepted ) {
        return;
    }

    if( velho.field("paikallinen").toBool()) {
        ui->paikallinen->setChecked(true);
        valintaMuuttui();
    } else if( velho.field("maventa").toBool()) {        
        maaritaMaventa();
    } else if( velho.field("kitsas").toBool()) {
        ToiminimiModel* tnimi = kp()->toiminimet();
        const QString authEmail = velho.field("auth").toString();
        const QString notifEmail = tnimi->tieto(ToiminimiModel::Sahkoposti);

        QVariantMap data;
        data.insert("name", tnimi->tieto(ToiminimiModel::Nimi));
        data.insert("businessid", kp()->asetukset()->ytunnus());
        if( kp()->asetukset()->onko(AsetusModel::AlvVelvollinen)) {
            data.insert("vat", AsiakasToimittajaDlg::yToAlv(kp()->asetukset()->ytunnus()) );
        }
        data.insert("address", tnimi->tieto(ToiminimiModel::Katuosoite));
        data.insert("postcode", tnimi->tieto(ToiminimiModel::Postinumero));
        data.insert("postoffice", tnimi->tieto(ToiminimiModel::Kaupunki));
        data.insert("homecity", kp()->asetukset()->asetus(AsetusModel::Kotipaikka));
        data.insert("email", tnimi->tieto(ToiminimiModel::Sahkoposti));
        data.insert("kitsasbilling", true);
        data.insert("authemail", authEmail);
        data.insert("active", true);


        QVariantMap settings = MaventaDialog::settingsAsetuksista();
        QVariantMap notifications;
        QVariantMap receiving;
        if( velho.field("saapunut").toBool() ) {
            receiving.insert("enabled", true);
            receiving.insert("how_to_send","OTHER_EMAIL");
            receiving.insert("other_email", notifEmail  );
        } else {
            receiving.insert("enabled", false);
        }
        notifications.insert("on_receiving", receiving);
        QVariantMap onErrors;
        onErrors.insert("to_user", false );
        QVariantList errorEmails;
        if( velho.field("virhe").toBool()) {
            errorEmails << notifEmail;
        }
        onErrors.insert("to_emails", errorEmails.isEmpty() ? QVariant() : errorEmails );
        notifications.insert("on_send_errors", onErrors);
        settings.insert("invoice_notifications", notifications);

        bool tulostuspalvelu = velho.field("postitus").toBool();
        QVariantMap print;
        print.insert("enabled", tulostuspalvelu);
        if(tulostuspalvelu) {
            print.insert("letter_class", "ECONOMY");
            print.insert("color_scheme",  "BLACK_AND_WHITE");
            print.insert("attachment_print", false);
            print.insert("marketing_page", false);
            print.insert("use_own_pdf", false);
        }
        settings.insert("send_invoice_print", print);

        data.insert("settings", settings);
        kp()->asetukset()->aseta("MaventaPostitus", tulostuspalvelu);
        ui->integroitu->setChecked(true);
        valintaMuuttui();
        tallenna();
        emit this->tallennaKaytossa(false);

        qDebug() << QJsonDocument::fromVariant(settings).toJson(QJsonDocument::Compact);
        const QString osoite = QString("%1/maventa/%2").arg(kp()->pilvi()->finvoiceOsoite(),kp()->asetukset()->ytunnus());

        PilviKysely *pk= new PilviKysely( kp()->pilvi(), KpKysely::POST, osoite);
        connect( pk, &PilviKysely::vastaus, this, &VerkkolaskuMaaritys::maventaTiedot);
        pk->kysy(data);

    }

}
