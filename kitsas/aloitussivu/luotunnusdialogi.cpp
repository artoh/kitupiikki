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
#include "luotunnusdialogi.h"
#include "ui_luotunnusdialogi.h"

#include <QPushButton>
#include <QRegularExpression>
#include <QTimer>
#include <QJsonDocument>

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"

LuoTunnusDialogi::LuoTunnusDialogi(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LuoTunnusDialogi)
{
    ui->setupUi(this);

    ui->ehtoBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    connect( ui->ehdotCheck, &QCheckBox::clicked, [this] (bool onko) { this->ui->ehtoBox->button(QDialogButtonBox::Ok)->setEnabled(onko);});
    connect( ui->ehtoBox, &QDialogButtonBox::helpRequested, [] { kp()->ohje("aloittaminen/tilaus/tunnus/"); });
    connect( ui->ehtoBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, [this] { this->ui->stackedWidget->setCurrentIndex(OSOITE); });

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    connect( ui->buttonBox, &QDialogButtonBox::helpRequested, [] { kp()->ohje("aloittaminen/tilaus/tunnus/"); });
    connect( ui->osoiteEdit, &QLineEdit::textEdited, this, &LuoTunnusDialogi::tarkastaEmail);
    connect( ui->nimiEdit, &QLineEdit::textEdited, [this] { ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled( ui->nimiEdit->text().length() > 3 && !ui->kaytossaLabel->isVisible() );} );

    ui->kaytossaLabel->hide();
    ui->verkkovirheLabel->hide();
}

LuoTunnusDialogi::~LuoTunnusDialogi()
{
    delete ui;
}

void LuoTunnusDialogi::accept()
{
    QVariantMap map;

    map.insert("email", ui->osoiteEdit->text());
    map.insert("name", ui->nimiEdit->text());
    QNetworkAccessManager *mng = kp()->networkManager();

    QNetworkRequest request(QUrl( kp()->pilvi()->pilviLoginOsoite() + "/users") );

    request.setRawHeader("Content-Type","application/json");
    request.setRawHeader("User-Agent", QString(qApp->applicationName() + " " + qApp->applicationVersion()).toUtf8());

    QNetworkReply *reply = mng->post( request, QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact) );
    connect( reply, &QNetworkReply::finished, this, &LuoTunnusDialogi::rekisterointiLahti);

    ui->stackedWidget->setCurrentIndex(ODOTA);
}

void LuoTunnusDialogi::tarkastaEmail()
{
    ui->kaytossaLabel->hide();
    ui->verkkovirheLabel->hide();
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    QRegularExpression emailRe(R"(^[A-Z0-9a-z._%+-]+@[A-Za-z0-9.-]+[.][A-Za-z]{2,64}$)");
    if( emailRe.match( ui->osoiteEdit->text()).hasMatch() ) {
        // Tarkistetaan sähköposti ja toimitaan sen mukaan
        QNetworkRequest request(QUrl( kp()->pilvi()->pilviLoginOsoite() + "/users/" + ui->osoiteEdit->text() ));
        request.setRawHeader("User-Agent", QString(qApp->applicationName() + " " + qApp->applicationVersion()).toUtf8());
        QNetworkReply *reply =  kp()->networkManager()->get(request);
        connect( reply, &QNetworkReply::finished, this, &LuoTunnusDialogi::emailTarkistettu);
        connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            [this](QNetworkReply::NetworkError code){ this->verkkovirhe(code); });
        connect( reply, &QNetworkReply::sslErrors, [] (const QList<QSslError>& errors) { for(auto virhe : errors) qDebug() << virhe.errorString();  });
    }
}

void LuoTunnusDialogi::emailTarkistettu()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>( sender());
    bool kaytossa =  (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200) ;
    ui->kaytossaLabel->setVisible(kaytossa);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!kaytossa && ui->nimiEdit->text().length() > 2);
}

void LuoTunnusDialogi::verkkovirhe(QNetworkReply::NetworkError virhe)
{
    if( virhe == QNetworkReply::ConnectionRefusedError)
        ui->verkkovirheLabel->setText(tr("<p><b>Palvelin ei juuri nyt ole käytettävissä. Yritä myöhemmin uudelleen.</b>"));
    else if( virhe == QNetworkReply::TemporaryNetworkFailureError || virhe == QNetworkReply::NetworkSessionFailedError)
        ui->verkkovirheLabel->setText(tr("<p><b>Verkkoon ei saada yhteyttä</b>"));
    else if(virhe < QNetworkReply::ContentAccessDenied )
        ui->verkkovirheLabel->setText(tr("<p><b>Palvelinyhteydessä on virhe (%1)</b>").arg(virhe));
    if( virhe < QNetworkReply::ContentAccessDenied) {
        ui->verkkovirheLabel->show();
        QTimer::singleShot(30000, this, &LuoTunnusDialogi::tarkastaEmail);
    }
}

void LuoTunnusDialogi::rekisterointiLahti()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>( sender());
    if( reply->error()) {
        ui->onniViesti->setText(
            tr("Rekisteröinnin lähettäminen palvelimelle epäonnistui "
               "tietoliikennevirheen %1 takia.\n\n"
               "Yritä myöhemmin uudelleen").arg( reply->error() ));
    } else {
        ui->onniViesti->setText( tr("Sinulle on lähetetty osoitteeseen %1 sähköpostiviesti, "
                                    "jossa olevalla linkillä pääset viimeistelemään käyttäjätunnuksen luomisen ja "
                                    "asettamaan itsellesi salasanan.\n\n"
                                    ).arg(ui->osoiteEdit->text()) );

    }
    ui->stackedWidget->setCurrentIndex(VALMIS);
}
