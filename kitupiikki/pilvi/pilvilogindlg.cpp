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
#include "pilvilogindlg.h"
#include "ui_pilvilogindlg.h"
#include "db/kirjanpito.h"
#include "pilvimodel.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QRegularExpression>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QApplication>
#include <QMessageBox>

#include <QSettings>


PilviLoginDlg::PilviLoginDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PilviLoginDlg)
{
    ui->setupUi(this);
    ui->buttonBox->button( QDialogButtonBox::Ok )->setEnabled(false);
    ui->salaLabel->setVisible(false);
    ui->salaEdit->setVisible(false);
    ui->muistaCheck->setVisible(false);

    ui->pino->setCurrentIndex(VALINNAT);
    ui->unohtuiNappi->setVisible(false);

    connect( ui->emailEdit, &QLineEdit::textChanged, this, &PilviLoginDlg::validoi);
}

PilviLoginDlg::~PilviLoginDlg()
{
    delete ui;
}

void PilviLoginDlg::accept()
{
    if( ui->pino->currentIndex() == VALINNAT)
    {
        ui->unohtuiNappi->setVisible(false);
        QVariantMap map;

        if( ui->salaEdit->isVisible() &&  !ui->salaEdit->text().isEmpty() ) {
            kp()->pilvi()->kirjaudu( ui->emailEdit->text(), ui->salaEdit->text(), ui->muistaCheck->isChecked() );
            QDialog::accept();
            return;
        }

        map.insert("email", ui->emailEdit->text());
        QNetworkAccessManager *mng = kp()->networkManager();

        // Tähän pilviosoite!
        QNetworkRequest request(QUrl( kp()->pilvi()->pilviLoginOsoite() + "/users") );

        request.setRawHeader("Content-Type","application/json");
        request.setRawHeader("User-Agent", QString(qApp->applicationName() + " " + qApp->applicationVersion()).toUtf8());

        QNetworkReply *reply = mng->post( request, QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact) );
        connect( reply, &QNetworkReply::finished, this, &PilviLoginDlg::lahetetty);

        ui->pino->setCurrentIndex( ODOTA);
        ui->buttonBox->button( QDialogButtonBox::Ok )->setEnabled(false);

    } else {
        QDialog::accept();
        kp()->pilvi()->kirjaudu();
    }
}

void PilviLoginDlg::validoi()
{
    QRegularExpression emailRe(R"(^([\w-]*(\.[\w-]+)?)+@(\w+\.\w+)(\.\w+)*$)");
    bool kelpo = emailRe.match( ui->emailEdit->text()).hasMatch();
    QString email = ui->emailEdit->text().toLower();

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled( kelpo );
    if( kelpo ) {
        QNetworkRequest request(QUrl( kp()->pilvi()->pilviLoginOsoite() + "/users/" + email ));
        request.setRawHeader("User-Agent", QString(qApp->applicationName() + " " + qApp->applicationVersion()).toUtf8());
        QNetworkReply *reply =  kp()->networkManager()->get(request);
        connect( reply, &QNetworkReply::finished, this, &PilviLoginDlg::salatietosaapuu);
    }
}

void PilviLoginDlg::lahetetty()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>( sender());
    if( reply->error()) {
        QMessageBox::critical(this, tr("Rekisteröityminen epäonnistui"),
            tr("Rekisteröinnin lähettäminen palvelimelle epäonnistui "
               "tietoliikennevirheen %1 takia.\n\n"
               "Yritä myöhemmin uudelleen").arg( reply->error() ));
        reject();
        return;
    }

    QByteArray vastaus = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson( vastaus );

    QString email = doc.object().value("email").toString();
    QString avain = doc.object().value("key").toString();

    kp()->settings()->setValue("CloudEmail", email);
    kp()->settings()->setValue("CloudKey", avain);

    ui->buttonBox->button( QDialogButtonBox::Ok )->setEnabled(true);
    ui->buttonBox->button( QDialogButtonBox::Cancel )->setEnabled(false);

    if( ui->salaLabel->isVisible() )
        ui->pino->setCurrentIndex( SALASANA );
    else
        ui->pino->setCurrentIndex( REKISTEROINTI );

}

void PilviLoginDlg::salatietosaapuu()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>( sender());
    bool olemassa =  reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200 ;

    ui->salaLabel->setVisible( olemassa );
    ui->salaEdit->setVisible( olemassa );
    ui->muistaCheck->setVisible( olemassa );
    ui->unohtuiNappi->setVisible( olemassa );

}
