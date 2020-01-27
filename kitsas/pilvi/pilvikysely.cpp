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
#include <QJsonObject>
#include <QJsonValue>
#include <QDebug>

#include "tuonti/csvtuonti.h"

#include <iostream>

PilviKysely::PilviKysely(PilviModel *parent, KpKysely::Metodi metodi, QString polku)
    : KpKysely (parent, metodi, polku)
{
    Q_ASSERT(polku != "/viennit/-1");
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

    std::cerr << "===========" << url.toString().toStdString() << "================\n";
    std::cerr << QString::fromUtf8(QJsonDocument::fromVariant(data).toJson(QJsonDocument::Indented)).toStdString();


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
    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
        [this](QNetworkReply::NetworkError code){ this->verkkovirhe(code); });



}

void PilviKysely::lahetaTiedosto(const QByteArray &ba, const QMap<QString,QString>& meta)
{
    PilviModel *model = qobject_cast<PilviModel*>( parent() );
    QNetworkRequest request( QUrl( model->pilviosoite() + polku() ) );

    request.setRawHeader("Authorization", QString("bearer %1").arg( model->token() ).toLatin1());
    request.setRawHeader("User-Agent", QString(qApp->applicationName() + " " + qApp->applicationVersion() ).toLatin1()  );

    if( !meta.contains("Content-type")) {
        QString tunnistettutyyppi = tiedostotyyppi(ba);
        if( !tunnistettutyyppi.isNull())
            request.setRawHeader("Content-type", tunnistettutyyppi.toLatin1());
    }
    QMapIterator<QString,QString> iter(meta);
    while( iter.hasNext()) {
        iter.next();
        request.setRawHeader(iter.key().toLatin1(), iter.value().toLatin1());
    }

    QNetworkReply *reply = metodi()==KpKysely::POST ?
                kp()->networkManager()->post(request, ba) :
                kp()->networkManager()->put(request, ba);
    connect( reply, &QNetworkReply::finished, this, &PilviKysely::vastausSaapuu);
    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
        [this](QNetworkReply::NetworkError code){ this->verkkovirhe(code); });
}

void PilviKysely::vastausSaapuu()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>( sender());
    if( reply->error()) {
        QByteArray vastaus = reply->readAll();

        qDebug() << " (VIRHE!) " << reply->error() << " " << reply->request().url().toString() ;
        std::cerr <<  vastaus.toStdString();


        QString selite = QJsonDocument::fromJson(vastaus).object().value("virhe").toString();
        emit virhe( reply->error(), selite);

        return;
    } else {
        QByteArray luettu = reply->readAll();
        std::cout << luettu.toStdString();
        std::cout.flush();

        if( reply->header(QNetworkRequest::ContentTypeHeader).toString().startsWith("application/json") ) {
            vastaus_ = QJsonDocument::fromJson(luettu).toVariant();
        } else {
            vastaus_ = luettu;
        }        
        emit vastaus( &vastaus_ );
        if( metodi() == KpKysely::POST) {
            QString location = QString::fromLatin1(reply->rawHeader("Location"));
            qDebug() << "Location: " << location;
            qDebug() << reply->rawHeaderList();
            int lisattyid = location.mid( location.lastIndexOf('/') + 1 ).toInt();
            emit lisaysVastaus(vastaus_, lisattyid);
        }
    }

    this->deleteLater();
}

void PilviKysely::verkkovirhe(QNetworkReply::NetworkError koodi)
{
    if( koodi == QNetworkReply::ConnectionRefusedError)
        emit kp()->onni(tr("<b>Palvelimeen ei saada yhteyttä</b><br>Palvelin on ehkä tilapäisesti poissa käytöstä"), Kirjanpito::Verkkovirhe);
    else if( koodi == 8 || koodi == 9)
        emit kp()->onni(tr("<b>Häiriö verkkoyhteydessä</b>"), Kirjanpito::Verkkovirhe);
    else if( koodi < QNetworkReply::ContentAccessDenied)
        emit kp()->onni(tr("<b>Verkkovirhe %1</b>").arg(koodi), Kirjanpito::Verkkovirhe);
    else if( koodi == QNetworkReply::ContentAccessDenied)
        emit kp()->onni(tr("<b>Oikeutesi eivät riitä tähän toimintoon</b>").arg(koodi), Kirjanpito::Stop);
    else if( koodi == QNetworkReply::InternalServerError)
        emit kp()->onni(tr("<b>Palvelinvirhe %1</b>").arg(koodi), Kirjanpito::Stop);
}
