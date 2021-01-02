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

#include "emailmaaritys.h"
#include "db/kirjanpito.h"
#include "db/yhteysmodel.h"

#include "smtpclient/SmtpMime"
#include <QMessageBox>


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
    connect( ui->kopioEdit, &QLineEdit::textChanged, this, &EmailMaaritys::ilmoitaMuokattu);

    connect( ui->tkAsetusRadio, &QRadioButton::toggled, this, &EmailMaaritys::ilmoitaMuokattu);
    connect( ui->maksutiedotRadio, &QRadioButton::toggled, this, &EmailMaaritys::ilmoitaMuokattu);
    connect( ui->saateEdit, &QPlainTextEdit::textChanged , this, &EmailMaaritys::ilmoitaMuokattu);

    connect( ui->kokeileNappi, SIGNAL(clicked(bool)), this, SLOT(kokeile()));
    connect( ui->porttiSpin, SIGNAL(valueChanged(int)), this, SLOT(porttiVaihtui(int)));

    ui->testiLabel->hide();
}

EmailMaaritys::~EmailMaaritys()
{
    delete ui;
}

bool EmailMaaritys::nollaa()
{
    // SSL-varoitus siirretty sähköpostin asetuksiin

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
    ui->tyyppiCombo->setEnabled( ssltuki );
    if( !ssltuki)
        ui->tyyppiCombo->setCurrentIndex(0);

    paikallinen_ = !kp()->yhteysModel() || !kp()->yhteysModel()->onkoOikeutta(YhteysModel::ASETUKSET);

    if( kp()->asetukset()->asetus("SmtpServer").isEmpty() || paikallinen_) {
        ui->palvelinEdit->setText( kp()->settings()->value("SmtpServer").toString() );
        ui->kayttajaEdit->setText( kp()->settings()->value("SmtpUser").toString());
        ui->salasanaEdit->setText( kp()->settings()->value("SmtpPassword").toString());
        ui->nimiEdit->setText( kp()->settings()->value("EmailNimi").toString());
        ui->emailEdit->setText( kp()->settings()->value("EmailOsoite").toString());
        ui->porttiSpin->setValue( kp()->settings()->value("SmtpPort",QSslSocket::supportsSsl() ? 465 : 25).toInt());
        ui->tkAsetusRadio->setChecked(true);
        ui->tyyppiCombo->setCurrentIndex(  sslIndeksi( kp()->settings()->value("EmailSSL").toString() ));
        ui->kopioEdit->setText(kp()->settings()->value("EmailKopio").toString());
    } else {
        ui->palvelinEdit->setText( kp()->asetukset()->asetus("SmtpServer") );
        ui->porttiSpin->setValue( kp()->asetukset()->luku("SmtpPort", QSslSocket::supportsSsl() ? 465 : 25));
        ui->kayttajaEdit->setText( kp()->asetukset()->asetus("SmtpUser"));
        ui->salasanaEdit->setText( kp()->asetukset()->asetus("SmtpPassword"));
        ui->nimiEdit->setText( kp()->asetukset()->asetus("EmailNimi"));
        ui->emailEdit->setText( kp()->asetukset()->asetus("EmailOsoite"));
        ui->tyyppiCombo->setCurrentIndex(  sslIndeksi( kp()->asetukset()->asetus("EmailSSL") ));
        ui->kopioEdit->setText(kp()->asetukset()->asetus("EmailKopio"));
        ui->kpAsetusRadio->setChecked(true);
    }


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
        kp()->settings()->setValue("EmailSSL", sslAsetus(ui->tyyppiCombo->currentIndex()));
        kp()->settings()->setValue("EmailKopio", ui->kopioEdit->text());
        if( !paikallinen_ )
            kp()->asetukset()->poista("SmtpServer");
    } else {
        kp()->asetukset()->aseta("SmtpServer", ui->palvelinEdit->text());
        kp()->asetukset()->aseta("SmtpPort", ui->porttiSpin->value());
        kp()->asetukset()->aseta("SmtpUser", ui->kayttajaEdit->text());
        kp()->asetukset()->aseta("SmtpPassword", ui->salasanaEdit->text());
        kp()->asetukset()->aseta("EmailNimi", ui->nimiEdit->text());
        kp()->asetukset()->aseta("EmailOsoite", ui->emailEdit->text());
        kp()->asetukset()->aseta("EmailKopio", ui->kopioEdit->text());
        kp()->asetukset()->aseta("EmailSSL", sslAsetus(ui->tyyppiCombo->currentIndex()));
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
            kp()->settings()->value("EmailOsoite").toString() != ui->emailEdit->text() ||
            kp()->settings()->value("EmailKopio").toString() != ui->kopioEdit->text() ||
            kp()->settings()->value("EmailSSL").toString() != sslAsetus(ui->tyyppiCombo->currentIndex()) :

            kp()->asetukset()->asetus("SmtpServer") != ui->palvelinEdit->text() ||
            kp()->asetukset()->luku("SmtpPort",465) != ui->porttiSpin->value() ||
            kp()->asetukset()->asetus("SmtpUser") != ui->kayttajaEdit->text() ||
            kp()->asetukset()->asetus("SmtpPassword") != ui->salasanaEdit->text() ||
            kp()->asetukset()->asetus("EmailNimi") != ui->nimiEdit->text() ||
            kp()->asetukset()->asetus("EmailOsoite") != ui->emailEdit->text() ||
            kp()->asetukset()->asetus("EmailKopio") != ui->kopioEdit->text() ||
            kp()->asetukset()->asetus("EmailSSL") != sslAsetus(ui->tyyppiCombo->currentIndex());


    int emailmuoto = ui->maksutiedotRadio->isChecked() ? 1 : 0;

    return muokattu || emailmuoto != kp()->asetukset()->luku("EmailMuoto") ||
            ui->saateEdit->toPlainText() != kp()->asetukset()->asetus("EmailSaate");

}

int EmailMaaritys::sslIndeksi(const QString &asetus)
{
    if( asetus == "STARTTLS")
        return SmtpClient::TlsConnection;
    else if(asetus == "ON")
        return SmtpClient::SslConnection;
    else
        return SmtpClient::TcpConnection;
}

QString EmailMaaritys::sslAsetus(int indeksi)
{
    switch (indeksi) {
        case SmtpClient::TcpConnection: return "EI";
        case SmtpClient::SslConnection: return "ON";
        case SmtpClient::TlsConnection: return "STARTTLS";
    }
    return QString();
}

void EmailMaaritys::ilmoitaMuokattu()
{
    emit tallennaKaytossa( onkoMuokattu());
}

void EmailMaaritys::kokeile()
{
    ui->testiLabel->show();
    ui->kokeileNappi->setEnabled(false);

    SmtpClient smtp( ui->palvelinEdit->text(), ui->porttiSpin->value(), (SmtpClient::ConnectionType) ui->tyyppiCombo->currentIndex());
    if( !ui->salasanaEdit->text().isEmpty()) {
        smtp.setUser(ui->kayttajaEdit->text());
        smtp.setPassword(ui->salasanaEdit->text());
    }

    MimeMessage message;
    message.setSender(new EmailAddress(ui->emailEdit->text(), ui->nimiEdit->text()));
    message.addRecipient(new EmailAddress(ui->emailEdit->text(), ui->nimiEdit->text()));

    message.setSubject(tr("Kitsaan sähköpostikokeilu"));

    MimeText text;
    text.setText(tr("Sähköpostin lähettäminen Kitsas-ohjelmasta onnistui %1").arg(QDateTime::currentDateTime().toString("dd.MM.yyyy hh.mm")));
    message.addPart(&text);

    MimeAttachment attachment(new QFile(":/pic/kitsas350.png"));
    attachment.setContentType("image/jpeg");
    message.addPart(&attachment);

    if(!smtp.connectToHost()) {
        QMessageBox::critical(this, tr("Sähköpostin lähettäminen epäonnistui"), tr("Sähköpostipalvelimeen %1 yhdistäminen epäonnistui").arg(ui->palvelinEdit->text()));
    } else if( !ui->salasanaEdit->text().isEmpty() && !smtp.login()) {
        QMessageBox::critical(this, tr("Sähköpostin lähettäminen epäonnistui"), tr("Sähköpostipalvelimeen kirjautuminen epäonnistui").arg(ui->palvelinEdit->text()));
    } else if( !smtp.sendMail(message)) {
        QMessageBox::critical(this, tr("Sähköpostin lähettäminen epäonnistui"), tr("Virhe sähköpostia lähetettäessä").arg(ui->palvelinEdit->text()));
    } else {
        QMessageBox::information(this, tr("Sähköposti lähetetty"), tr("Sähköpostin lähettäminen onnistui"));
    }

    ui->testiLabel->hide();
    ui->kokeileNappi->setEnabled(true);

}

void EmailMaaritys::porttiVaihtui(int portti)
{
    if( portti == 25)
        ui->tyyppiCombo->setCurrentIndex(SmtpClient::TcpConnection);
    else if( portti == 465)
        ui->tyyppiCombo->setCurrentIndex(SmtpClient::SslConnection);
    else if(portti == 587)
        ui->tyyppiCombo->setCurrentIndex(SmtpClient::TlsConnection);
}

