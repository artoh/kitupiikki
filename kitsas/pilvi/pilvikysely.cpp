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
    QString osoite = polku().contains("//") ? polku() : model->pilvi().url() + polku();
    QUrl url( osoite );
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
    connect(reply, &QNetworkReply::errorOccurred, this,
        [this](QNetworkReply::NetworkError code){ this->verkkovirhe(code); });



}

void PilviKysely::lahetaTiedosto(const QByteArray &ba, const QMap<QString,QString>& meta)
{
    PilviModel *model = qobject_cast<PilviModel*>( parent() );
    QString osoite = polku().contains("//") ? polku() : model->pilvi().url() + polku();

    QUrl url( osoite );
    url.setQuery( urlKysely() );
    QNetworkRequest request( url );

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
        const QString name = iter.key();
        QString value = iter.value();

        if( name == "Filename") {
            // Turvataan tiedostonnimi skandeilta
            request.setRawHeader("Filename-Base64", value.toUtf8().toBase64());
            value.remove(turvaRe__);
        }
        request.setRawHeader(name.toLatin1(), value.toLatin1());
    }

    QNetworkReply *reply = metodi()==KpKysely::POST ?
                kp()->networkManager()->post(request, ba) :
                kp()->networkManager()->put(request, ba);
    connect( reply, &QNetworkReply::finished, this, &PilviKysely::vastausSaapuu);
    connect(reply, &QNetworkReply::errorOccurred, this,
        [this](QNetworkReply::NetworkError code){ this->verkkovirhe(code); });
}

void PilviKysely::vastausSaapuu()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>( sender());
    if( reply->error()) {
        QByteArray vastaus = reply->readAll();

        qCritical() << " (VIRHE!) " << reply->error() << " " << reply->request().url().toString() ;
        qCritical() <<  QString::fromUtf8(vastaus);


        QString selite = QJsonDocument::fromJson(vastaus).object().value("virhe").toString();
        QVariant variantti = QJsonDocument::fromJson(vastaus).toVariant();
        emit virhe( reply->error(), selite, variantti);

        return;
    } else {
        QByteArray luettu = reply->readAll();

        if( reply->header(QNetworkRequest::ContentTypeHeader).toString().startsWith("application/json") ) {
            vastaus_ = QJsonDocument::fromJson(luettu).toVariant();
        } else {
            vastaus_ = luettu;
        }        
        emit vastaus( &vastaus_ );
        if( metodi() == KpKysely::POST) {
            QString location = QString::fromLatin1(reply->rawHeader("Location"));

            int lisattyid = location.mid( location.lastIndexOf('/') + 1 ).toInt();
            if(lisattyid)
                emit lisaysVastaus(vastaus_, lisattyid);
        }
    }

    this->deleteLater();
    reply->deleteLater();
}

void PilviKysely::verkkovirhe(QNetworkReply::NetworkError koodi)
{
    if( koodi == QNetworkReply::ConnectionRefusedError)
        emit kp()->onni(tr("<b>Palvelimeen ei saada yhteyttä</b><br>Palvelin on ehkä tilapäisesti poissa käytöstä"), Kirjanpito::Verkkovirhe);
    else if( koodi == QNetworkReply::RemoteHostClosedError)
        emit kp()->onni(tr("<b>Yhteys katkesi</b><br>Syynä voi olla hetkellinen suuri pyyntöjen määrä palvelimelle. <br>Yritä uudelleen pienen hetken jälkeen."), Kirjanpito::Verkkovirhe);
    else if( koodi == 8 || koodi == 9)
        emit kp()->onni(tr("<b>Häiriö verkkoyhteydessä</b>"), Kirjanpito::Verkkovirhe);
    else if( koodi < QNetworkReply::ContentAccessDenied)
        emit kp()->onni(tr("<b>Verkkovirhe %1</b>").arg(koodi), Kirjanpito::Verkkovirhe);
    else if( koodi == QNetworkReply::ContentAccessDenied)
        emit kp()->onni(tr("<b>Oikeutesi eivät riitä tähän toimintoon</b>"), Kirjanpito::Stop);
    else if( koodi == QNetworkReply::InternalServerError)
        emit kp()->onni(tr("<b>Palvelinvirhe</b>"), Kirjanpito::Stop);
    else if( koodi == QNetworkReply::ProtocolInvalidOperationError)
        emit kp()->onni(tr("<b>Virhe palvelimella</b>"), Kirjanpito::Stop);
    else if( koodi == QNetworkReply::UnknownServerError)
        emit kp()->onni(tr("<b>Palvelinvirhe</b><br>Palvelu on ehkä tilapäisesti poissa käytöstä"), Kirjanpito::Verkkovirhe);
    kp()->odotusKursori(false);
}

QRegularExpression PilviKysely::turvaRe__ = QRegularExpression("[^A-Za-z0-9/.-]");
