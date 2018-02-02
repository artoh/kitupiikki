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

    connect( ui->kokeileNappi, SIGNAL(clicked(bool)), this, SLOT(kokeile()));
}

EmailMaaritys::~EmailMaaritys()
{
    delete ui;
}

bool EmailMaaritys::nollaa()
{
    QSettings settings;

    ui->palvelinEdit->setText( settings.value("SmtpServer").toString() );
    ui->kayttajaEdit->setText( settings.value("SmtpUser").toString());
    ui->salasanaEdit->setText( settings.value("SmtpPassword").toString());

    ui->nimiEdit->setText( kp()->asetukset()->asetus("EmailNimi"));
    ui->emailEdit->setText( kp()->asetukset()->asetus("EmailOsoite"));

    // SSL-varoitus siirretty sähköpostin asetuksiin, koska
    // päivitykset tarkastetaan ilman suojaa

    bool ssltuki = QSslSocket::supportsSsl();

    if( !ssltuki)
    {
        ui->sslTeksti->setText(tr("<b>SSL-suojattu verkkoliikenne ei käytössä</b><p>"
                          "Laskujen lähettäminen suojatulla sähköpostilla edellyttää "
                          "OpenSSL-kirjaston versiota %1").arg(QSslSocket::sslLibraryBuildVersionString()));
    }
    ui->sslTeksti->setVisible( !ssltuki );
    ui->sslVaro->setVisible( !ssltuki );
    ui->kayttajaEdit->setEnabled( ssltuki);
    ui->salasanaEdit->setEnabled( ssltuki );

    ui->porttiSpin->setValue( settings.value("SmtpPort", QSslSocket::supportsSsl() ? 465 : 25 ).toInt());

    return true;
}

bool EmailMaaritys::tallenna()
{
    QSettings settings;
    settings.setValue("SmtpServer", ui->palvelinEdit->text());
    settings.setValue("SmtpPort", ui->porttiSpin->value());
    settings.setValue("SmtpUser", ui->kayttajaEdit->text());
    settings.setValue("SmtpPassword", ui->salasanaEdit->text());

    kp()->asetukset()->aseta("EmailNimi", ui->nimiEdit->text());
    kp()->asetukset()->aseta("EmailOsoite", ui->emailEdit->text());

    return true;
}

bool EmailMaaritys::onkoMuokattu()
{
    QSettings settings;

    return settings.value("SmtpServer").toString() != ui->palvelinEdit->text() ||
            settings.value("SmtpPort",465).toInt() != ui->porttiSpin->value() ||
            settings.value("SmtpUser").toString() != ui->kayttajaEdit->text() ||
            settings.value("SmtpPassword").toString() != ui->salasanaEdit->text() ||
            kp()->asetukset()->asetus("EmailNimi") != ui->nimiEdit->text() ||
            kp()->asetukset()->asetus("EmailOsoite") != ui->emailEdit->text();
}

void EmailMaaritys::ilmoitaMuokattu()
{
    emit tallennaKaytossa( onkoMuokattu());
}

void EmailMaaritys::kokeile()
{
    ui->tulosLabel->clear();
    QString osoite = QString("\"%1\" <%2>").arg(ui->nimiEdit->text()).arg(ui->emailEdit->text());

    Smtp *smtp = new Smtp( ui->kayttajaEdit->text(), ui->salasanaEdit->text(), ui->palvelinEdit->text(), ui->porttiSpin->value());
    connect( smtp, SIGNAL(status(QString)), ui->tulosLabel, SLOT(setText(QString)));
    ui->tulosLabel->setText("Lähetetään testisähköpostia...");

    QStringList lista;
    lista << ":/pic/aboutpossu.png";

    smtp->sendMail( osoite, osoite, tr("Kitupiikin kokeilu"),
                    tr("<html><body><h1>Kitupiikki Kirjanpito</h1><p>Sähköpostin lähetys onnistui</p>"
                       "<hr>%1</body></html>").arg(QDateTime::currentDateTime().toString(Qt::SystemLocaleShortDate)), lista);
}

