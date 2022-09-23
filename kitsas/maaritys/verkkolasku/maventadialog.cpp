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
#include "maventadialog.h"
#include "model/toiminimimodel.h"
#include "ui_maventa.h"

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"
#include "pilvi/pilvikysely.h"

#include "rekisteri/asiakastoimittajadlg.h"

#include <QMessageBox>
#include <QPushButton>
#include <QBuffer>
#include <QRegularExpression>

#include <QDebug>
#include <QJsonDocument>
#include <QUuid>

MaventaDialog::MaventaDialog(QWidget *parent) :
    QDialog(parent), ui( new Ui::MaventaDialog)
{
    ui->setupUi(this);        

    connect( ui->api, &QLineEdit::textEdited, this, &MaventaDialog::muokattu);
    connect( ui->uuid, &QLineEdit::textEdited, this, &MaventaDialog::muokattu);
    connect( ui->buttonBox, &QDialogButtonBox::helpRequested, [] { kp()->ohje("asetukset/verkkolaskut"); });

    muokattu();
}

void MaventaDialog::accept()
{
    bool pilvessa = qobject_cast<PilviModel*>(kp()->yhteysModel());

    QVariantMap map;
    map.insert("apikey", ui->api->text().trimmed());
    map.insert("clientid", ui->uuid->text().trimmed());
    map.insert("kitsasbilling", ui->kitsasLaskuButton->isChecked() && pilvessa);
    map.insert("active", ui->activeBox->isChecked());
    map.insert("name", kp()->asetukset()->nimi());

    QVariantMap settings = settingsAsetuksista();
    QVariantMap notifications;
    QVariantMap receiving;
    if( ui->ilmoitusEmail->text().length() > 6 ) {
        receiving.insert("enabled", true);
        receiving.insert("how_to_send","OTHER_EMAIL");
        receiving.insert("other_email", ui->ilmoitusEmail->text());
    } else {
        receiving.insert("enabled", false);
    }
    notifications.insert("on_receiving", receiving);
    QVariantMap onErrors;
    onErrors.insert("to_user", ui->emailKayttajalleBox->isChecked() ? true : false );
    QStringList emails = ui->virheEmail->text().trimmed().split(QRegularExpression("(;|,|\\s)"));
    QVariantList errorEmails;
    QRegularExpression emailRe(R"(^.*@.*\.\w+$)");

    for(auto email: emails) {
        if( emailRe.match(email).hasMatch() ) {
            errorEmails.append(email);
        }
    }

    onErrors.insert("to_emails", errorEmails.isEmpty() ? QVariant() : errorEmails );
    notifications.insert("on_send_errors", onErrors);
    settings.insert("invoice_notifications", notifications);

    bool tulostuspalvelu = ui->tulostusCheck->isChecked();
    QVariantMap print;
    print.insert("enabled", tulostuspalvelu);
    if(tulostuspalvelu) {
        print.insert("letter_class", ui->priorityRadio->isChecked() ? "PRIORITY" : "ECONOMY");
        print.insert("color_scheme", ui->variRadio->isChecked() ? "COLORED" : "BLACK_AND_WHITE");
        print.insert("attachment_print", false);
        print.insert("marketing_page", false);
        print.insert("use_own_pdf", ui->omaCheck->isChecked());
    }
    settings.insert("send_invoice_print", print);

    map.insert("settings", settings);

    qDebug() << QJsonDocument::fromVariant(settings).toJson(QJsonDocument::Compact);

    const QString osoite = QString("%1/maventa/%2").arg(kp()->pilvi()->finvoiceOsoite(),kp()->asetukset()->ytunnus());
    PilviKysely *pk= new PilviKysely( kp()->pilvi(), KpKysely::POST, osoite);
    connect( pk, &PilviKysely::vastaus, this, &MaventaDialog::vastaus);
    pk->kysy(map);

    kp()->asetukset()->aseta("MaventaPostitus", tulostuspalvelu && ui->kaytaPostitustaCheck->isChecked());
}

void MaventaDialog::lataa(const QVariantMap &data)
{
    QVariantMap book = data.value("book").toMap();

    ui->api->setText( book.value("apikey").toString() );
    ui->api->setReadOnly( book.value("kitsasuser").toBool() );

    ui->uuid->setText( book.value("company").toString());
    ui->uuid->setReadOnly( book.value("companybykitsas").toBool() );

    bool pilvessa = qobject_cast<PilviModel*>(kp()->yhteysModel());
    bool kitsaslasku = book.value("kitsasbilling").toBool() && pilvessa;

    ui->kitsasLaskuButton->setEnabled(pilvessa);
    ui->kitsasLaskuButton->setChecked( kitsaslasku );
    ui->maventaLaskuButton->setChecked( !kitsaslasku );
    ui->maventaLaskuButton->setVisible( !kitsaslasku || !book.value("companybykitsas").toBool() );

    ui->activeBox->setChecked( book.value("active").toBool() );

    QVariantMap notifications = data.value("invoice_notifications").toMap();
    QVariantMap receiving = notifications.value("on_receiving").toMap();
    ui->ilmoitusEmail->setText( receiving.value("other_email").toString());
    QVariantMap errorEmail = notifications.value("on_send_errors").toMap();
    ui->virheEmail->setText( errorEmail.value("to_emails").toStringList().join(',') );
    ui->emailKayttajalleBox->setChecked( errorEmail.value("to_user").toBool() );
    ui->emailKayttajalleBox->setVisible( !book.value("kitsasuser").toBool() );

    QVariantMap print = data.value("send_invoice_print").toMap();

    bool tulostusPalveluKaytossa = print.value("enabled").toBool();

    ui->tulostusCheck->setChecked( tulostusPalveluKaytossa );
    ui->kaytaPostitustaCheck->setEnabled( tulostusPalveluKaytossa );
    ui->kaytaPostitustaCheck->setChecked( tulostusPalveluKaytossa && kp()->asetukset()->onko("MaventaPostitus") );

    bool saastoluokka = print.value("letter_class").toString() == "ECONOMY";
    bool mustavalko = print.value("color_scheme").toString() == "BLACK_AND_WHITE";

    ui->priorityRadio->setChecked( !saastoluokka );
    ui->economyRadio->setChecked( saastoluokka );
    ui->mvRadio->setChecked( mustavalko );
    ui->variRadio->setChecked( !mustavalko );
    ui->omaCheck->setChecked( print.value("use_own_pdf").toBool() );

    muokattu();
}

QVariantMap MaventaDialog::settingsAsetuksista()
{
    QVariantMap settings;

    QVariantMap details;
    const ToiminimiModel* tnimi = kp()->toiminimet();
    details.insert("name", tnimi->tieto(ToiminimiModel::Nimi));
    details.insert("email", tnimi->tieto(ToiminimiModel::Sahkoposti));
    details.insert("website", tnimi->tieto(ToiminimiModel::Kotisivu));
    settings.insert("details", details);

    QVariantMap address;
    address.insert("street_address", tnimi->tieto(ToiminimiModel::Katuosoite));
    address.insert("post_code", tnimi->tieto(ToiminimiModel::Postinumero));
    address.insert("post_office", tnimi->tieto(ToiminimiModel::Kaupunki));
    address.insert("city", kp()->asetukset()->asetus(AsetusModel::Kotipaikka));
    address.insert("country", "FI");
    settings.insert("address", address);

    QImage logo = tnimi->logo();
    if( !logo.isNull()) {
        QByteArray ba;
        QBuffer buffer(&ba);
        logo.save(&buffer,"JPEG",80);
        buffer.close();
        QByteArray base64 = ba.toBase64();
        QVariantMap pdf;
        pdf.insert("content", base64);
        QVariantMap logos;
        logos.insert("pdf", pdf);
        settings.insert("logos", logos);
    }

    return settings;
}

void MaventaDialog::vastaus(QVariant *data)
{
    QVariantMap map = data->toMap();
    if( map.isEmpty()) {
         QMessageBox::critical(this, tr("Maventa asetukset"), tr("Maventan asetusten muuttaminen epäonnistui.\n\n"
                                                                       "Tarkasta kirjoittamasi avaimet."));
    } else {
        emit liitetty(data);
        QDialog::accept();
    }
}

void MaventaDialog::muokattu()
{
    QUuid apiUuid(  ui->api->text().trimmed());
    QUuid companyUuid( ui->uuid->text().trimmed());


    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                !apiUuid.isNull() &&
                !companyUuid.isNull());
}

