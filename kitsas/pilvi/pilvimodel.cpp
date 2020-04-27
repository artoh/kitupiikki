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
#include "versio.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QMessageBox>
#include <QSslSocket>
#include <QApplication>
#include <QImage>

#include <QDebug>
#include <QTimer>
#include <QFile>
#include <QNetworkReply>

PilviModel::PilviModel(QObject *parent, const QString &token) :
    YhteysModel (parent),
    token_(token)
{
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, [this] {this->paivitaLista(); });
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

    if( role == Qt::DecorationRole) {
        return logot_.value( map.value("id").toInt() );
    }
    if( role == Qt::ForegroundRole) {
        if( map.value("trial").toBool())
            return QColor(Qt::darkGreen);
    }

    return QVariant();
}

int PilviModel::omatPilvet() const
{
    int omia = 0;
    for( auto pilvi : data_.value("clouds").toList())
        if( oikeudet( pilvi.toMap().value("rights").toList() ) & YhteysModel::OMISTAJA )
            omia++;

    return omia;
}


QString PilviModel::pilviLoginOsoite()
{
    return pilviLoginOsoite__;
}

void PilviModel::uusiPilvi(const QVariant &initials)
{
    PilviKysely* kysely = new PilviKysely(this, KpKysely::POST, pilviLoginOsoite() + "/clouds");
    connect( kysely, &PilviKysely::vastaus, this, &PilviModel::pilviLisatty);
    connect( kysely, &PilviKysely::virhe, [] (int koodi, const QString& viesti) {QMessageBox::critical(nullptr, tr("Kirjanpidon luominen epäonnistui"),
                                                                                     tr("Kirjanpitoa luotaessa tapahtui virhe:\n%1 %2").arg(koodi).arg(viesti)); });
    kysely->kysy(initials);
}



bool PilviModel::avaaPilvesta(int pilviId, bool siirrossa)
{

    for( auto var : data_.value("clouds").toList()) {
        QVariantMap map = var.toMap();
        if( map.value("id").toInt() == pilviId) {
            pilviId_ = pilviId;
            osoite_ = map.value("url").toString();
            token_ = map.value("token").toString();
            oikeudet_ = oikeudet( map.value("rights").toList() );
            pilviVat_ = map.value("vat").toBool();
            if( !siirrossa) {
                alusta();
                emit kirjauduttu();
            }
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
    oikeudet_ = 0;
    osoite_.clear();
    token_ = userToken();
}

void PilviModel::poistaNykyinenPilvi()
{
    QString polku = QString("%1/clouds/%2").arg(pilviLoginOsoite()).arg(pilviId());
    PilviKysely *poisto = new PilviKysely(this, KpKysely::DELETE, polku);
    connect( poisto, &KpKysely::vastaus, this, &PilviModel::poistettu);
    poisto->kysy();
}

qlonglong PilviModel::oikeudet(const QVariantList &lista)
{
    qlonglong bittikartta = 0;
    for(auto oikeus : lista) {
        try {
            bittikartta += oikeustunnukset__.at(oikeus.toString());
        } catch( std::out_of_range )
        {
            qDebug() << "Tuntematon oikeus " << oikeus.toString();
        }
    }
    return bittikartta;
}

void PilviModel::asetaPilviLoginOsoite(const QString &osoite)
{
    pilviLoginOsoite__ = osoite;
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
    request.setRawHeader("User-Agent",QString(qApp->applicationName() + " " + qApp->applicationVersion() ).toLatin1());

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
    oikeudet_ = 0;
    timer_->stop();

    endResetModel();

    kp()->yhteysAvattu(nullptr);
}

void PilviModel::paivitaLista(int avaaPilvi)
{
    avaaPilvi_ = avaaPilvi;
    PilviKysely *kysely = new PilviKysely(this, KpKysely::GET,
                                          pilviLoginOsoite() + "/login");
    connect( kysely, &KpKysely::vastaus, this, &PilviModel::paivitysValmis);
    connect( kysely, &KpKysely::virhe, this, &PilviModel::yritaUudelleenKirjautumista);
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

    kayttajaId_ = data_.value("userId").toInt();
    token_ = data_.value("token").toString();   

    if( data_.contains("key"))
    {
        kp()->settings()->setValue("CloudKey", data_.value("key").toString());
        kp()->settings()->setValue("CloudEmail", data_.value("email").toString());
    }

    // Tallennetaan uusi token ja tilataan puuttuvat logot
    for( QVariant variant : data_.value("clouds").toList()) {
    QVariantMap map = variant.toMap();    
        if( map.value("id").toInt() == pilviId_ ) {
            osoite_ = map.value("url").toString();
            token_ = map.value("token").toString();            
            oikeudet_ = oikeudet( map.value("rights").toList());
        }
        // Tilataan puuttuvat logot
        if( !logot_.contains(map.value("id").toInt()))
            tilaaLogo(map);
    }
    // Uuden pilven avaaminen, kun lista on päivittynyt
    if( avaaPilvi_ )
        avaaPilvesta( avaaPilvi_);
    avaaPilvi_ = 0;

    timer_->start(60 * 60 * 1000);  // Tokenin päivitys tunnin välein
    emit kirjauduttu();
}

void PilviModel::pilviLisatty(QVariant *paluu)
{
    QVariantMap map = paluu->toMap();
    avaaPilvi_ = map.value("id").toInt();
    paivitaLista();    

}

void PilviModel::tilaaLogo(const QVariantMap &map)
{
    int id = map.value("id").toInt();
    QUrl url( map.value("url").toString() + "/liitteet/0/logo");
    QNetworkRequest request( url );

    request.setRawHeader("Authorization", QString("bearer %1").arg( map.value("token").toString() ).toLatin1());
    request.setRawHeader("User-Agent", QString(qApp->applicationName() + " " + qApp->applicationVersion() ).toLatin1()  );
    QNetworkReply *reply = kp()->networkManager()->get(request);
    connect( reply, &QNetworkReply::finished, [this, id, reply]
    {
        if( reply->error())
            this->logot_.insert(id, QPixmap());
        else {
            this->logot_.insert(id, QPixmap::fromImage(QImage::fromData(reply->readAll()).scaled(16,16,Qt::KeepAspectRatio)));
            for(int i=0; i<rowCount() ;i++) {
                if( this->data_.value("clouds").toList().value(i).toMap().value("id").toInt() == id ) {
                    emit this->dataChanged(index(i), index(i));
                }
            }
        }
    });
}

void PilviModel::poistettu()
{
    kp()->yhteysAvattu(nullptr);
    paivitaLista();
}

void PilviModel::yritaUudelleenKirjautumista()
{
    qDebug() << "** Yritä kirjautumista uudelleen ** ";

    beginResetModel();
    data_.clear();
    kayttajaId_ = 0;
    oikeudet_ = 0;
    timer_->stop();
    endResetModel();
    kp()->yhteysAvattu(nullptr);

    kirjaudu();
}


std::map<QString,qlonglong> PilviModel::oikeustunnukset__ = {
    {"Ts", TOSITE_SELAUS},
    {"Tl", TOSITE_LUONNOS},
    {"Tt", TOSITE_MUOKKAUS},
    {"Ls", LASKU_SELAUS},
    {"Ll", LASKU_LAATIMINEN},
    {"Lt", LASKU_LAHETTAMINEN},
    {"Kl", KIERTO_LISAAMINEN},
    {"Kt", KIERTO_TARKASTAMINEN},
    {"Kh", KIERTO_HYVAKSYMINEN},
    {"Av", ALV_ILMOITUS},
    {"Bm", BUDJETTI},
    {"Tp", TILINPAATOS},
    {"As", ASETUKSET},
    {"Ko", KAYTTOOIKEUDET},
    {"Om", OMISTAJA},
    {"Xt", TUOTTEET},
    {"Xr", RYHMAT},
    {"Ra", RAPORTIT},
    {"Ks", KIERTO_SELAAMINEN}
};

QString PilviModel::pilviLoginOsoite__;
