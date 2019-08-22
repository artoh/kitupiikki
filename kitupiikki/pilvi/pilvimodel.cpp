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
#include "pilvimodel.h"
#include "db/kirjanpito.h"
#include "pilvikysely.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

#include <QDebug>
#include <QTimer>
#include <QFile>

PilviModel::PilviModel(QObject *parent) :
    YhteysModel (parent)
{
    timer_ = new QTimer();
    connect(timer_, &QTimer::timeout, this, &PilviModel::paivitaLista);
}

int PilviModel::rowCount(const QModelIndex & /* parent */) const
{
    return data_.value("clouds").toList().count();
}

QVariant PilviModel::data(const QModelIndex &index, int role) const
{
    QVariantMap map = data_.value("clouds").toList().at(index.row()).toMap();
    if( role == Qt::DisplayRole || role == NimiRooli)
    {        
        return map.value("name").toString();
    } else if( role == IdRooli ) {
        return map.value("id").toInt();
    }

    return QVariant();
}

int PilviModel::omatPilvet() const
{
    int omia = 0;
    for( auto pilvi : data_.value("clouds").toList())
        if( pilvi.toMap().value("right").toString() == "owner")
            omia++;

    return omia;
}


QString PilviModel::pilviLoginOsoite()
{
    return "http://localhost:5665/api";
}

void PilviModel::uusiPilvi(const QVariant &initials)
{
    PilviKysely* kysely = new PilviKysely(this, KpKysely::POST, pilviLoginOsoite() + "/clouds");
    connect( kysely, &PilviKysely::vastaus, this, &PilviModel::pilviLisatty);
    kysely->kysy(initials);

    QJsonDocument doc( QJsonDocument::fromVariant(initials));
    QFile out("/tmp/ulos.json");
    out.open(QIODevice::WriteOnly | QIODevice::Truncate);
    out.write( doc.toJson());
}



bool PilviModel::avaaPilvesta(int pilviId)
{
    for( auto var : data_.value("clouds").toList()) {
        QVariantMap map = var.toMap();
        if( map.value("id").toInt() == pilviId) {
            pilviId_ = pilviId;
            osoite_ = map.value("url").toString();
            token_ = map.value("token").toString();
            oikeudet_ = map.value("right").toString();
            alusta();

            emit kirjauduttu();
            return true;
        }
    }
    return false;
}

KpKysely *PilviModel::kysely(const QString &polku, KpKysely::Metodi metodi)
{
    return new PilviKysely( this, metodi, polku);
}

void PilviModel::sulje()
{
    pilviId_ = 0;
    osoite_.clear();
    token_.clear();
}

void PilviModel::kirjaudu(const QString sahkoposti, const QString &salasana, bool pyydaAvain)
{
    QVariantMap map;
    if( !salasana.isEmpty()) {
        map.insert("email", sahkoposti);
        map.insert("password", salasana);
        if( pyydaAvain )
            map.insert("requestKey",true);
    } else if( kp()->settings()->contains("CloudKey") ) {
        map.insert("email", kp()->settings()->value("CloudEmail") );
        map.insert("key", kp()->settings()->value("CloudKey"));
    }

    QNetworkAccessManager *mng = kp()->networkManager();

    // Tähän pilviosoite!
    QNetworkRequest request(QUrl( pilviLoginOsoite() + "/login") );


    request.setRawHeader("Content-Type","application/json");
    request.setRawHeader("User-Agent","Kitupiikki");

    QNetworkReply *reply = mng->post( request, QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact) );
    connect( reply, &QNetworkReply::finished, this, &PilviModel::kirjautuminenValmis);


}

void PilviModel::kirjauduUlos()
{
    beginResetModel();
    data_.clear();

    kp()->settings()->remove("CloudEmail");
    kp()->settings()->remove("CloudKey");
    kayttajaId_ = 0;
    timer_->stop();

    endResetModel();
}

void PilviModel::paivitaLista()
{
    // Päivitetään lista
    PilviKysely *kysely = new PilviKysely(this, KpKysely::GET,
                                          pilviLoginOsoite() + "/login");
    connect( kysely, &KpKysely::vastaus, this, &PilviModel::paivitysValmis);
    kysely->kysy();
}



void PilviModel::kirjautuminenValmis()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );

    if( reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200) {
        emit loginvirhe();
        return;
    }

    QByteArray vastaus = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson( vastaus );
    QVariant var = doc.toVariant();
    paivitysValmis( &var );

    if( kp()->settings()->value("Viimeisin").toInt() > 0)
        avaaPilvesta( kp()->settings()->value("Viimeisin").toInt() );

}

void PilviModel::paivitysValmis(QVariant *paluu)
{
    beginResetModel();
    data_ = paluu->toMap();
    endResetModel();

    bool ensiKirjaus = kayttajaId_ == 0;
    kayttajaId_ = data_.value("userId").toInt();
    token_ = data_.value("token").toString();

    if( data_.contains("key"))
    {
        kp()->settings()->setValue("CloudKey", data_.value("key").toString());
        kp()->settings()->setValue("CloudEmail", data_.value("email").toString());
    }

    if( ensiKirjaus)
    {
        emit kirjauduttu();
        // Varmuuden vuoksi uusitaan token joka toinen tunti
        timer_->start(1000 * 60 * 60 * 2);
    }
    else {
        // Tallennetaan uusi token
        for( QVariant variant : data_.value("clouds").toList()) {
        QVariantMap map = variant.toMap();
            if( map.value("id").toInt() == pilviId() ) {
                osoite_ = map.value("url").toString();
                token_ = map.value("token").toString();
                oikeudet_ = map.value("right").toString();
            }
        }
    }
    // Uuden pilven avaaminen, kun lista on päivittynyt
    if( avaaPilvi_ )
        avaaPilvesta( avaaPilvi_);
    avaaPilvi_ = 0;
}

void PilviModel::pilviLisatty(QVariant *paluu)
{
    QVariantMap map = paluu->toMap();
    avaaPilvi_ = map.value("id").toInt();
    alusta();
    paivitaLista();

}
