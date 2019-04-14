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
        QVariantMap map;

        if( ui->salaEdit->isVisible() &&  !ui->salaEdit->text().isEmpty() ) {
            map.insert("salasana", ui->salaEdit->text());
            if( !ui->muistaCheck->isChecked()) {
                kp()->pilvi()->kirjaudu( ui->emailEdit->text(), ui->salaEdit->text() );
                QDialog::accept();
                return;
            }
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
    QString avain = doc.object().value("avain").toString();
    bool rekisteroity = doc.object().value("rekisteroity").toBool();
    bool vahvistettu = doc.object().value("vahvistettu").toBool();

    kp()->settings()->setValue("CloudEmail", email);
    kp()->settings()->setValue("CloudKey", avain);

    ui->buttonBox->button( QDialogButtonBox::Ok )->setEnabled(true);
    ui->buttonBox->button( QDialogButtonBox::Cancel )->setEnabled(false);
    if( vahvistettu )
        accept();
    else if( rekisteroity )
        ui->pino->setCurrentIndex( VAHVISTUS );
    else
        ui->pino->setCurrentIndex( REKISTEROINTI );

}

void PilviLoginDlg::salatietosaapuu()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>( sender());
    QByteArray vastaus = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson( vastaus );

qDebug() << doc.toJson();

    bool salasanattu = doc.object().value("salasanalla").toBool();

    ui->salaLabel->setVisible( salasanattu );
    ui->salaEdit->setVisible( salasanattu );
    ui->muistaCheck->setVisible( salasanattu );

}
