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
    asetukset_.lataa();

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

    ui->tkAsetusRadio->setChecked( asetukset_.asetustapa() == EmailAsetukset::ASETUKSET_TIETOKONE );
    ui->kpAsetusRadio->setChecked( asetukset_.asetustapa() == EmailAsetukset::ASETUKSET_KIRJANPITO );
    ui->kitsasRadio->setChecked( asetukset_.kitsasPalvelin() );


    ui->palvelinEdit->setText( asetukset_.palvelin() );
    ui->kayttajaEdit->setText( asetukset_.kayttaja() );
    ui->salasanaEdit->setText( asetukset_.salasana() );
    ui->porttiSpin->setValue( asetukset_.portti() );
    ui->tyyppiCombo->setCurrentIndex( asetukset_.suojaus() );
    ui->kopioEdit->setText( asetukset_.kopioOsoite() );

    ui->minaRadio->setVisible( kp()->pilvi()->kayttajaPilvessa() );
    ui->minaRadio->setChecked( asetukset_.kayttajakohtainen() );
    ui->nimiEdit->setText( asetukset_.lahettajaNimi() );
    ui->emailEdit->setText( asetukset_.lahettajaOsoite());

    ui->tkAsetusRadio->setEnabled( kp()->yhteysModel() && kp()->yhteysModel()->onkoOikeutta(YhteysModel::ASETUKSET) );
    ui->kitsasRadio->setEnabled( qobject_cast<PilviModel*>( kp()->yhteysModel() ));
    paivitaKitsasVaihto();

    return true;
}

bool EmailMaaritys::tallenna()
{
    if( ui->kpAsetusRadio->isChecked()) {
        asetukset_.setAsetusTapa(EmailAsetukset::ASETUKSET_KIRJANPITO);
    } else if( ui->kitsasRadio->isChecked()) {
        asetukset_.setAsetusTapa(EmailAsetukset::KITSAS_PALVELIN);
    } else {
        asetukset_.setAsetusTapa(EmailAsetukset::ASETUKSET_TIETOKONE);
    }

    asetukset_.setPalvelin( ui->palvelinEdit->text() );
    asetukset_.setKayttaja( ui->kayttajaEdit->text() );
    asetukset_.setSalasana( ui->salasanaEdit->text());
    asetukset_.setPortti( ui->porttiSpin->value());
    asetukset_.setSuojaus( ui->tyyppiCombo->currentIndex() );

    asetukset_.setKopioOsoite( ui->kopioEdit->text() );
    asetukset_.setKayttajaKohtainen( ui->minaRadio->isChecked() );
    asetukset_.setLahettajaOsoite( ui->emailEdit->text() );
    asetukset_.setLahettajaNimi( ui->nimiEdit->text() );

    asetukset_.tallenna();

    return true;
}

bool EmailMaaritys::onkoMuokattu()
{
    return
        ui->tkAsetusRadio->isChecked() != (asetukset_.asetustapa() == EmailAsetukset::ASETUKSET_TIETOKONE) ||
        ui->kpAsetusRadio->isChecked() != (asetukset_.asetustapa() == EmailAsetukset::ASETUKSET_KIRJANPITO) ||
        ui->kitsasRadio->isChecked() != (asetukset_.asetustapa() == EmailAsetukset::KITSAS_PALVELIN) ||
        ui->palvelinEdit->text() != asetukset_.palvelin() ||
        ui->porttiSpin->value() != asetukset_.portti() ||
        ui->kayttajaEdit->text() != asetukset_.kayttaja() ||
        ui->salasanaEdit->text() != asetukset_.salasana() ||
        ui->tyyppiCombo->currentIndex() != asetukset_.suojaus() ||
        ui->minaRadio->isChecked() != asetukset_.kayttajakohtainen() ||
        ui->emailEdit->text() != asetukset_.lahettajaOsoite() ||
        ui->nimiEdit->text() != asetukset_.lahettajaNimi() ||
        ui->kopioEdit->text() != asetukset_.kopioOsoite();

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

    ui->lahettajaOsoiteLabel->setText( kitsas ? tr("Sähköpostiosoite") : tr("Vastausosoite"));

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
            .arg(ui->nimiEdit->text(),ui->emailEdit->text());

    viesti.insert("senderName",  ui->nimiEdit->text() );
    viesti.insert("senderAddress", ui->emailEdit->text());
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

