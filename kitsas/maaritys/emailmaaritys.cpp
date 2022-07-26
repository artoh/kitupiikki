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
#include <QJsonDocument>

#include "emailmaaritys.h"
#include "db/kirjanpito.h"
#include "db/yhteysmodel.h"
#include "pilvi/pilvimodel.h"

#include "smtpclient/SmtpMime"
#include "pilvi/pilvikysely.h"
#include <QMessageBox>

#include <QFontMetrics>


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

    connect( ui->tyyppiCombo, &QComboBox::currentTextChanged, this, &EmailMaaritys::ilmoitaMuokattu);

    connect( ui->kokeileNappi, SIGNAL(clicked(bool)), this, SLOT(kokeile()));
    connect( ui->porttiSpin, SIGNAL(valueChanged(int)), this, SLOT(porttiVaihtui(int)));

    connect( ui->kitsasRadio, &QRadioButton::toggled, this, &EmailMaaritys::paivitaKitsasVaihto);

    ui->testiLabel->hide();
}

EmailMaaritys::~EmailMaaritys()
{
    delete ui;
}

bool EmailMaaritys::nollaa()
{
    // SSL-varoitus siirretty sähköpostin asetuksiin

    bool kitsasEmail = kp()->asetukset()->onko("KitsasEmail");
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
    ui->minaRadio->setVisible(false);

    if( !kitsasEmail && (kp()->asetukset()->asetus(AsetusModel::SmtpServer).isEmpty() || paikallinen_)) {
        ui->palvelinEdit->setText( kp()->settings()->value("SmtpServer").toString() );
        ui->kayttajaEdit->setText( kp()->settings()->value("SmtpUser").toString());
        ui->salasanaEdit->setText( kp()->settings()->value("SmtpPassword").toString());
        ui->nimiEdit->setText( kp()->settings()->value("EmailNimi").toString());
        ui->emailEdit->setText( kp()->settings()->value("EmailOsoite").toString());
        ui->porttiSpin->setValue( kp()->settings()->value("SmtpPort",QSslSocket::supportsSsl() ? 465 : 25).toInt());
        ui->tkAsetusRadio->setChecked(!kitsasEmail);
        ui->tyyppiCombo->setCurrentIndex(  sslIndeksi( kp()->settings()->value("EmailSSL").toString() ));
        ui->kopioEdit->setText(kp()->settings()->value("EmailKopio").toString());
    } else {
        ui->palvelinEdit->setText( kp()->asetukset()->asetus(AsetusModel::SmtpServer) );
        ui->porttiSpin->setValue( kp()->asetukset()->luku(AsetusModel::SmtpPort, QSslSocket::supportsSsl() ? 465 : 25));
        ui->kayttajaEdit->setText( kp()->asetukset()->asetus(AsetusModel::SmtpUser));
        ui->salasanaEdit->setText( kp()->asetukset()->asetus(AsetusModel::SmtpPassword));        
        ui->tyyppiCombo->setCurrentIndex(  sslIndeksi( kp()->asetukset()->asetus(AsetusModel::EmailSSL) ));
        ui->kopioEdit->setText(kp()->asetukset()->asetus(AsetusModel::EmailKopio));
        ui->kpAsetusRadio->setChecked(true);

        ui->minaRadio->setVisible( kp()->pilvi()->kayttajaPilvessa() );
        const QString omaEmail = kp()->asetukset()->asetus( QString("OmaEmail/%1").arg(kp()->pilvi()->kayttajaPilvessa()) );
        if( omaEmail.isEmpty() || !kp()->pilvi()->kayttajaPilvessa() ) {
            ui->nimiEdit->setText( kp()->asetukset()->asetus(AsetusModel::EmailNimi));
            ui->emailEdit->setText( kp()->asetukset()->asetus(AsetusModel::EmailOsoite));
        } else {
            ui->minaRadio->setChecked(true);
            const int vali = omaEmail.indexOf(' ');
            ui->emailEdit->setText( omaEmail.left(vali));
            ui->nimiEdit->setText( omaEmail.mid(vali + 1) );
        }

    }

    ui->kitsasRadio->setChecked(kitsasEmail);
    ui->kpAsetusRadio->setDisabled( paikallinen_ );
    ui->kitsasRadio->setEnabled( qobject_cast<PilviModel*>( kp()->yhteysModel() ));
    paivitaKitsasVaihto();

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
        kp()->asetukset()->aseta(AsetusModel::SmtpServer, ui->palvelinEdit->text());
        kp()->asetukset()->aseta(AsetusModel::SmtpPort, ui->porttiSpin->value());
        kp()->asetukset()->aseta(AsetusModel::SmtpUser, ui->kayttajaEdit->text());
        kp()->asetukset()->aseta(AsetusModel::SmtpPassword, ui->salasanaEdit->text());
        kp()->asetukset()->aseta(AsetusModel::EmailNimi, ui->nimiEdit->text());
        kp()->asetukset()->aseta(AsetusModel::EmailSSL, sslAsetus(ui->tyyppiCombo->currentIndex()));

        if( ui->minaRadio->isChecked()) {
            QString mina = ui->emailEdit->text();
            mina.remove(' ');
            mina.append(' ');
            mina.append(ui->nimiEdit->text());
            kp()->asetukset()->aseta(QString("OmaEmail/%1").arg(kp()->pilvi()->kayttajaPilvessa()), mina);
        } else {
            kp()->asetukset()->aseta(AsetusModel::EmailOsoite, ui->emailEdit->text());
            kp()->asetukset()->aseta(AsetusModel::EmailKopio, ui->kopioEdit->text());
            kp()->asetukset()->poista(QString("OmaEmail/%1").arg(kp()->pilvi()->kayttajaPilvessa()));
        }
    }

    kp()->asetukset()->aseta("KitsasEmail", ui->kitsasRadio->isChecked());

    return true;
}

bool EmailMaaritys::onkoMuokattu()
{
    bool tietokoneAsetukset = ui->tkAsetusRadio->isChecked();

    bool muokattu = tietokoneAsetukset ?
            kp()->settings()->value("SmtpServer").toString() != ui->palvelinEdit->text() ||
            kp()->settings()->value("SmtpPort",465).toInt() != ui->porttiSpin->value() ||
            kp()->settings()->value("SmtpUser").toString() != ui->kayttajaEdit->text() ||
            kp()->settings()->value("SmtpPassword").toString() != ui->salasanaEdit->text() ||            
            kp()->settings()->value("EmailNimi").toString() != ui->nimiEdit->text() ||
            kp()->settings()->value("EmailOsoite").toString() != ui->emailEdit->text() ||
            kp()->settings()->value("EmailKopio").toString() != ui->kopioEdit->text() ||
            kp()->settings()->value("EmailSSL").toString() != sslAsetus(ui->tyyppiCombo->currentIndex()) :

            kp()->asetukset()->asetus(AsetusModel::SmtpServer) != ui->palvelinEdit->text() ||
            kp()->asetukset()->luku(AsetusModel::SmtpPort,465) != ui->porttiSpin->value() ||
            kp()->asetukset()->asetus(AsetusModel::SmtpUser) != ui->kayttajaEdit->text() ||
            kp()->asetukset()->asetus(AsetusModel::SmtpPassword) != ui->salasanaEdit->text() ||
            kp()->asetukset()->asetus(AsetusModel::EmailKopio) != ui->kopioEdit->text() ||
            kp()->asetukset()->asetus(AsetusModel::EmailSSL) != sslAsetus(ui->tyyppiCombo->currentIndex());

    if( !tietokoneAsetukset ) {
        const QString omaEmail = kp()->asetukset()->asetus( QString("OmaEmail/%1").arg(kp()->pilvi()->kayttajaPilvessa()) );
        if( omaEmail.isEmpty() || !kp()->pilvi()->kayttajaPilvessa()) {
            muokattu |= kp()->asetukset()->asetus(AsetusModel::EmailNimi) != ui->nimiEdit->text() ||
                        kp()->asetukset()->asetus(AsetusModel::EmailOsoite) != ui->emailEdit->text() ||
                        ui->minaRadio->isChecked();
        } else {
            QString mina = ui->emailEdit->text();
            mina.remove(' ');
            mina.append(' ');
            mina.append(ui->nimiEdit->text());
            muokattu |= mina != omaEmail || !ui->minaRadio->isChecked();
        }
    }


    return muokattu || ui->kitsasRadio->isChecked() != kp()->asetukset()->onko("KitsasEmail");

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

    if( ui->kitsasRadio->isChecked()) {
        kokeileKitsas();
    } else {
        kokeileSmtp();
    }

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

void EmailMaaritys::paivitaKitsasVaihto()
{
    bool kitsas = ui->kitsasRadio->isChecked();
    ui->smtpGroup->setVisible( !kitsas );
    ui->kokeileNappi->setVisible( true );
    ilmoitaMuokattu();
}

void EmailMaaritys::kokeileSmtp()
{
    MimeMessage message;
    message.setSender(EmailAddress(ui->emailEdit->text(), ui->nimiEdit->text()));
    message.addRecipient(EmailAddress(ui->emailEdit->text(), ui->nimiEdit->text()));

    message.setSubject(tr("Kitsaan sähköpostikokeilu"));
    MimeText text;
    text.setText(tr("Sähköpostin lähettäminen Kitsas-ohjelmasta onnistui %1").arg(QDateTime::currentDateTime().toString("dd.MM.yyyy hh.mm")));
    message.addPart(&text);

    QFile image(":/pic/kitsas350.png");
    MimeAttachment attachment(&image);
    attachment.setContentType("image/jpeg");
    message.addPart(&attachment);

    SmtpClient smtp( ui->palvelinEdit->text(), ui->porttiSpin->value(), (SmtpClient::ConnectionType) ui->tyyppiCombo->currentIndex());
    smtp.connectToHost();

    if( !smtp.waitForReadyConnected()) {
        QMessageBox::critical(this, tr("Sähköpostin lähettäminen epäonnistui"), tr("Sähköpostipalvelimeen %1 yhdistäminen epäonnistui").arg(ui->palvelinEdit->text()));
        ui->testiLabel->hide();
        ui->kokeileNappi->setEnabled(true);
        return;
    }

    if( !ui->salasanaEdit->text().isEmpty()) {
        smtp.login(ui->kayttajaEdit->text(), ui->salasanaEdit->text());
        if( !smtp.waitForAuthenticated()) {
            QMessageBox::critical(this, tr("Sähköpostin lähettäminen epäonnistui"), tr("Sähköpostipalvelimeen kirjautuminen epäonnistui"));
            ui->testiLabel->hide();
            ui->kokeileNappi->setEnabled(true);
            return;
        }
    }
    smtp.sendMail(message);
    if( !smtp.waitForMailSent()) {
        QMessageBox::critical(this, tr("Sähköpostin lähettäminen epäonnistui"), tr("Virhe sähköpostia lähetettäessä"));
    } else {
        QMessageBox::information(this, tr("Sähköposti lähetetty"), tr("Sähköpostin lähettäminen onnistui"));
    }
    ui->testiLabel->hide();
    ui->kokeileNappi->setEnabled(true);
}

void EmailMaaritys::kokeileKitsas()
{
    QString url = kp()->pilvi()->finvoiceOsoite() + "/attachment" ;
    PilviKysely *pk = new PilviKysely( kp()->pilvi(), KpKysely::POST,
                url );
    connect( pk, &PilviKysely::vastaus, this, &EmailMaaritys::liiteLahetetty);

    QFile kuvaTiedosto(":/pic/kitsas350.png");
    kuvaTiedosto.open(QFile::ReadOnly);
    pk->lahetaTiedosto(kuvaTiedosto.readAll());
}

void EmailMaaritys::liiteLahetetty(QVariant *data)
{
    QVariantMap map = data->toMap();
    map.insert("filename", "kitsas.png");
    map.insert("contentType","image/png");
    QVariantList liitteet;
    liitteet.append(map);

    QVariantMap viesti;
    QString osoite = QString("\"%1\" %2")
            .arg(ui->nimiEdit->text())
            .arg(ui->emailEdit->text());

    viesti.insert("from", osoite);
    viesti.insert("to", osoite);
    viesti.insert("subject", tr("Kitsaan sähköpostikokeilu"));
    viesti.insert("text", tr("Sähköpostin lähettäminen Kitsas-ohjelmasta onnistui %1").arg(QDateTime::currentDateTime().toString("dd.MM.yyyy hh.mm")));
    viesti.insert("attachments", liitteet);

    QString url = kp()->pilvi()->finvoiceOsoite() + "/email" ;
    PilviKysely *pk = new PilviKysely( kp()->pilvi(), KpKysely::POST,
                url );
    connect( pk, &PilviKysely::vastaus, this, &EmailMaaritys::kitsasOnnistui);
    connect( pk, &KpKysely::virhe, this, &EmailMaaritys::kitsasEpaonnistui );

    pk->kysy(viesti);

}

void EmailMaaritys::kitsasOnnistui()
{
    QMessageBox::information(this, tr("Sähköposti lähetetty"), tr("Sähköpostin lähettäminen onnistui"));
    ui->testiLabel->hide();
    ui->kokeileNappi->setEnabled(true);
}

void EmailMaaritys::kitsasEpaonnistui(int virhe, const QString &selitys)
{
    QMessageBox::critical(this, tr("Sähköpostin lähettäminen epäonnistui"), tr("Virhe sähköpostia lähetettäessä") + QString("\n %1 %2").arg(virhe).arg(selitys));
    ui->testiLabel->hide();
    ui->kokeileNappi->setEnabled(true);

}

