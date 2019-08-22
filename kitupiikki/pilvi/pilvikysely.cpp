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
#include "pilvikysely.h"
#include "db/kirjanpito.h"
#include "pilvimodel.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QApplication>

#include <QDebug>

PilviKysely::PilviKysely(PilviModel *parent, KpKysely::Metodi metodi, QString polku)
    : KpKysely (parent, metodi, polku)
{

}

void PilviKysely::kysy(const QVariant &data)
{
    PilviModel *model = qobject_cast<PilviModel*>( parent() );
    QUrl url( polku().contains("//") ? polku() :  model->pilviosoite() + polku());
    url.setQuery( urlKysely() );

    QNetworkRequest request( url );

    request.setRawHeader("Authorization", QString("bearer %1").arg( model->token() ).toLatin1());
    request.setRawHeader("User-Agent", QString(qApp->applicationName() + " " + qApp->applicationVersion() ).toLatin1()  );

    QNetworkReply *reply = nullptr;

    if( metodi() == GET)    {
        reply = kp()->networkManager()->get( request );
    } else if(metodi() == DELETE) {
        reply = kp()->networkManager()->deleteResource( request );
    } else  {
        request.setRawHeader("Content-Type","application/json");
        QByteArray ba = QJsonDocument::fromVariant(data).toJson(QJsonDocument::Compact);

        if( metodi() == POST )
            reply = kp()->networkManager()->post(request, ba);
        else if( metodi() == PATCH)
            reply = kp()->networkManager()->sendCustomRequest(request, "PATCH", ba);
        else if( metodi() == PUT)
            reply = kp()->networkManager()->put(request, ba);
    }

    connect( reply, &QNetworkReply::finished, this, &PilviKysely::vastausSaapuu );

}

void PilviKysely::lahetaTiedosto(const QByteArray &ba, const QString &tiedostonimi)
{
    PilviModel *model = qobject_cast<PilviModel*>( parent() );
    QNetworkRequest request( QUrl( model->pilviosoite() + polku() ) );

    request.setRawHeader("Authorization", QString("bearer %1").arg( model->token() ).toLatin1());
    request.setRawHeader("User-Agent", QString(qApp->applicationName() + " " + qApp->applicationVersion() ).toLatin1()  );

    // Tässä vaiheessa tiedostotyyppi nimestä ...
    // TODO: Tiedostotyyppi sisällön perusteella

    QString tyyppi = "image/jpeg";
    if( tiedostonimi.toLower().endsWith(".pdf"))
        tyyppi = "application/pdf";

    request.setRawHeader("Content-type", tyyppi.toLatin1());
    request.setRawHeader("Content-Disposition", "attachment; filename=" + tiedostonimi.toUtf8());

    QNetworkReply *reply = kp()->networkManager()->post(request, ba);
    connect( reply, &QNetworkReply::finished, this, &PilviKysely::vastausSaapuu);
}

void PilviKysely::vastausSaapuu()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>( sender());
    if( reply->error()) {
        QByteArray vastaus = reply->readAll();
        qDebug() << " (VIRHE!) " << reply->error() << " " << reply->request().url().toString() ;
        qDebug() << vastaus;

        emit virhe( reply->error());

        return;
    } else {
        QByteArray luettu = reply->readAll();

        if( reply->header(QNetworkRequest::ContentTypeHeader).toString().startsWith("application/json") ) {
            vastaus_ = QJsonDocument::fromJson(luettu).toVariant();
        } else {
            vastaus_ = luettu;
        }

        emit vastaus( &vastaus_ );
        qDebug() << " (OK) " << luettu.left(35) << " " << reply->header(QNetworkRequest::ContentTypeHeader).toString();
    }
    this->deleteLater();
}
