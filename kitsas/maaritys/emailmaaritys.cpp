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

#include <QSettings>
#include <QSslSocket>

#include "laskutus/smtp.h"

#include "emailmaaritys.h"
#include "db/kirjanpito.h"
#include "db/yhteysmodel.h"


EmailMaaritys::EmailMaaritys() :
    ui(new Ui::EMailMaaritys)
{
    ui->setupUi(this);

    connect( ui->palvelinEdit, SIGNAL(textChanged(QString)), this, SLOT(ilmoitaMuokattu()));
    connect( ui->porttiSpin, SIGNAL(valueChanged(int)), this, SLOT(ilmoitaMuokattu()));
    connect( ui->kayttajaEdit, SIGNAL(textChanged(QString)), this, SLOT(ilmoitaMuokattu()));
    connect(ui->salasanaEdit, SIGNAL(textChanged(QString)), this, SLOT(ilmoitaMuokattu()));
    connect( ui->emailEdit, SIGNAL(textChanged(QString)), this, SLOT(ilmoitaMuokattu()));
    connect( ui->nimiEdit, SIGNAL(textChanged(QString)), this, SLOT(ilmoitaMuokattu()));

    connect( ui->tkAsetusRadio, &QRadioButton::toggled, this, &EmailMaaritys::ilmoitaMuokattu);
    connect( ui->maksutiedotRadio, &QRadioButton::toggled, this, &EmailMaaritys::ilmoitaMuokattu);
    connect( ui->saateEdit, &QPlainTextEdit::textChanged , this, &EmailMaaritys::ilmoitaMuokattu);

    connect( ui->kokeileNappi, SIGNAL(clicked(bool)), this, SLOT(kokeile()));
}

EmailMaaritys::~EmailMaaritys()
{
    delete ui;
}

bool EmailMaaritys::nollaa()
{
    paikallinen_ = !kp()->yhteysModel() || !kp()->yhteysModel()->onkoOikeutta(YhteysModel::ASETUKSET);

    if( kp()->asetukset()->asetus("SmtpServer").isEmpty() || paikallinen_) {
        ui->palvelinEdit->setText( kp()->settings()->value("SmtpServer").toString() );
        ui->kayttajaEdit->setText( kp()->settings()->value("SmtpUser").toString());
        ui->salasanaEdit->setText( kp()->settings()->value("SmtpPassword").toString());
        ui->nimiEdit->setText( kp()->settings()->value("EmailNimi").toString());
        ui->emailEdit->setText( kp()->settings()->value("EmailOsoite").toString());
        ui->tkAsetusRadio->setChecked(true);
    } else {
        ui->palvelinEdit->setText( kp()->asetukset()->asetus("SmtpServer") );
        ui->kayttajaEdit->setText( kp()->asetukset()->asetus("SmtpUser"));
        ui->salasanaEdit->setText( kp()->asetukset()->asetus("SmtpPassword"));
        ui->nimiEdit->setText( kp()->asetukset()->asetus("EmailNimi"));
        ui->emailEdit->setText( kp()->asetukset()->asetus("EmailOsoite"));
        ui->kpAsetusRadio->setChecked(true);
    }

    // SSL-varoitus siirretty sähköpostin asetuksiin, koska
    // päivitykset tarkastetaan ilman suojaa

    bool ssltuki = QSslSocket::supportsSsl();

    if( !ssltuki)
    {
        ui->sslTeksti->setText(tr("<b>SSL-suojattu verkkoliikenne ei käytössä</b><p>"
                          "Laskujen lähettäminen suojatulla sähköpostilla edellyttää "
                          "OpenSSL-kirjaston versiota %1"
                          "<p>Voidaksesi lähettää suojattua sähköpostia lataa Internetistä "
                          "ja asenna OpenSSL-kirjasto %1.").arg(QSslSocket::sslLibraryBuildVersionString()));
    }
    ui->sslTeksti->setVisible( !ssltuki );
    ui->sslVaro->setVisible( !ssltuki );
    ui->kayttajaEdit->setEnabled( ssltuki);
    ui->salasanaEdit->setEnabled( ssltuki );
    ui->porttiSpin->setValue( kp()->settings()->value("SmtpPort", QSslSocket::supportsSsl() ? 465 : 25 ).toInt());

    ui->maksutiedotRadio->setChecked( kp()->asetukset()->luku("EmailMuoto") > 0 );
    ui->saateRadio->setChecked( kp()->asetukset()->luku("EmailMuoto") == 0);
    ui->saateEdit->setPlainText( kp()->asetukset()->asetus("EmailSaate") );

    ui->kpAsetusRadio->setDisabled( paikallinen_ );
    ui->saateEdit->setDisabled( paikallinen_ );
    ui->maksutiedotRadio->setDisabled( paikallinen_ );
    ui->saateEdit->setDisabled( paikallinen_ );

    return true;
}

bool EmailMaaritys::tallenna()
{
    if( ui->tkAsetusRadio->isChecked()) {
        kp()->settings()->setValue("SmtpServer", ui->palvelinEdit->text());
        kp()->settings()->setValue("SmtpPort", ui->porttiSpin->value());
        kp()->settings()->setValue("SmtpUser", ui->kayttajaEdit->text());
        kp()->settings()->setValue("SmtpPassword", ui->salasanaEdit->text());
        kp()->settings()->setValue("EmailNimi", ui->nimiEdit->text());
        kp()->settings()->setValue("EmailOsoite", ui->emailEdit->text());
        if( !paikallinen_ )
            kp()->asetukset()->poista("SmtpServer");
    } else {
        kp()->asetukset()->aseta("SmtpServer", ui->palvelinEdit->text());
        kp()->asetukset()->aseta("SmtpPort", ui->porttiSpin->value());
        kp()->asetukset()->aseta("SmtpUser", ui->kayttajaEdit->text());
        kp()->asetukset()->aseta("SmtpPassword", ui->salasanaEdit->text());
        kp()->asetukset()->aseta("EmailNimi", ui->nimiEdit->text());
        kp()->asetukset()->aseta("EmailOsoite", ui->emailEdit->text());
    }

    if( !paikallinen_ ) {
        kp()->asetukset()->aseta("EmailMuoto", ui->maksutiedotRadio->isChecked() ? 1 : 0);
        kp()->asetukset()->aseta("EmailSaate", ui->saateEdit->toPlainText());
    }

    return true;
}

bool EmailMaaritys::onkoMuokattu()
{
    bool muokattu = ui->tkAsetusRadio->isChecked() ?
            kp()->settings()->value("SmtpServer").toString() != ui->palvelinEdit->text() ||
            kp()->settings()->value("SmtpPort",465).toInt() != ui->porttiSpin->value() ||
            kp()->settings()->value("SmtpUser").toString() != ui->kayttajaEdit->text() ||
            kp()->settings()->value("SmtpPassword").toString() != ui->salasanaEdit->text() ||            
            kp()->settings()->value("EmailNimi").toString() != ui->nimiEdit->text() ||
            kp()->settings()->value("EmailOsoite").toString() != ui->emailEdit->text()  :

            kp()->asetukset()->asetus("SmtpServer") != ui->palvelinEdit->text() ||
            kp()->asetukset()->luku("SmtpPort",465) != ui->porttiSpin->value() ||
            kp()->asetukset()->asetus("SmtpUser") != ui->kayttajaEdit->text() ||
            kp()->asetukset()->asetus("SmtpPassword") != ui->salasanaEdit->text() ||
            kp()->asetukset()->asetus("EmailNimi") != ui->nimiEdit->text() ||
            kp()->asetukset()->asetus("EmailOsoite") != ui->emailEdit->text();

    int emailmuoto = ui->maksutiedotRadio->isChecked() ? 1 : 0;

    return muokattu || emailmuoto != kp()->asetukset()->luku("EmailMuoto") ||
            ui->saateEdit->toPlainText() != kp()->asetukset()->asetus("EmailSaate");

}

void EmailMaaritys::ilmoitaMuokattu()
{
    emit tallennaKaytossa( onkoMuokattu());
}

void EmailMaaritys::kokeile()
{
    ui->tulosLabel->clear();
    QString osoite = QString("=?utf-8?B?%1?= <%2>").arg( QString(ui->nimiEdit->text().toUtf8().toBase64()) ).arg(ui->emailEdit->text());

    Smtp *smtp = new Smtp( ui->kayttajaEdit->text(), ui->salasanaEdit->text(), ui->palvelinEdit->text(), ui->porttiSpin->value());
    connect( smtp, &Smtp::status, this, &EmailMaaritys::statusmuuttui);


    QFile kuva(":/pic/kitsas350.png");
    kuva.open(QIODevice::ReadOnly);

    smtp->lahetaLiitteella(osoite, osoite, tr("Kitsaan sähköpostikokeilu"),
                           tr("Sähköpostin lähettäminen Kitsas-ohjelmasta onnistui %1").arg(QDateTime::currentDateTime().toString("dd.MM.yyyy hh.mm")),
                       "possu.png", kuva.readAll());

}

void EmailMaaritys::statusmuuttui(int status)
{
    switch (status) {
    case Smtp::Connecting:
        ui->tulosLabel->setText( tr("Yhdistetään sähköpostipalvelimeen..."));
        break;
    case Smtp::Sending:
        ui->tulosLabel->setText(tr("Lähetetään sähköpostia..."));
        break;
    case Smtp::Send:
        ui->tulosLabel->setText(tr("Sähköposti lähetetty."));
        break;
    case Smtp::Failed:
        ui->tulosLabel->setText(tr("Sähköpostin lähettäminen epäonnistui."));
        break;
    }
}

