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

    connect( ui->nimiEdit, &QLineEdit::textChanged, this, &PilviLoginDlg::validoi);
    connect( ui->emailEdit, &QLineEdit::textChanged, this, &PilviLoginDlg::validoi);
}

PilviLoginDlg::~PilviLoginDlg()
{
    delete ui;
}

void PilviLoginDlg::accept()
{
    QVariantMap map;
    map.insert("nimi", ui->nimiEdit->text());
    map.insert("email", ui->emailEdit->text());

    QNetworkAccessManager *mng = kp()->networkManager();

    // Tähän pilviosoite!
    QNetworkRequest request(QUrl( kp()->pilvi()->pilviLoginOsoite() + "/users") );

    request.setRawHeader("Content-Type","application/json");
    request.setRawHeader("User-Agent", QString(qApp->applicationName() + " " + qApp->applicationVersion()).toUtf8());

    QNetworkReply *reply = mng->post( request, QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact) );
    connect( reply, &QNetworkReply::finished, this, &PilviLoginDlg::lahetetty);

    QDialog::accept();

}

void PilviLoginDlg::validoi()
{
    QRegularExpression emailRe(R"(^(\w*(\.\w+)?)+@(\w+\.\w+)(\.\w+)*$)");
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled( emailRe.match( ui->emailEdit->text()).hasMatch()
                                                             && ui->nimiEdit->text().length() > 3);
}

void PilviLoginDlg::lahetetty()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>( sender());
    if( reply->error()) {
        QMessageBox::critical(this, tr("Rekisteröityminen epäonnistui"),
            tr("Rekisteröinnin lähettäminen palvelimelle epäonnistui "
               "tietoliikennevirheen %1 takia.\n\n"
               "Yritä myöhemmin uudelleen").arg( reply->error() ));
        deleteLater();
        return;
    }

    QByteArray vastaus = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson( vastaus );

    QString email = doc.object().value("email").toString();
    QString avain = doc.object().value("avain").toString();

    kp()->settings()->setValue("CloudEmail", email);
    kp()->settings()->setValue("CloudKey", avain);

    QMessageBox::information(this, tr("Vahvistusviesti lähetetty"),
        tr("Vahvistusviesti on lähetetty sähköpostiisi.\n"
           "Napsauta saamassasi viestissä olevaa linkkiä ja käynnistä sen jälkeen "
           "Kitupiikki uudelleen."));

    deleteLater();
}
