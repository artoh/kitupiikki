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
#include "maaritys/tilitieto/tilitietopalvelu.h"

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
    kayttajaToken_(token),
    tilitietoPalvelu_(new Tilitieto::TilitietoPalvelu(this))
{
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &PilviModel::tarkistaKirjautuminen);    
    connect( kp(), &Kirjanpito::perusAsetusMuuttui, this, [this] { QTimer::singleShot(1500, this, &PilviModel::nimiMuuttui); });
    QTimer::singleShot(150, this, [this] { this->kirjaudu(); });

}

int PilviModel::rowCount(const QModelIndex & /* parent */) const
{
    return pilvet_.count();
}

QVariant PilviModel::data(const QModelIndex &index, int role) const
{
    const ListanPilvi& pilvi = pilvet_.at(index.row());

    if( role == Qt::DisplayRole || role == NimiRooli)
    {        
        return pilvi.nimi();
    } else if( role == IdRooli ) {
        return pilvi.id();
    }

    if( role == Qt::DecorationRole) {
        return pilvi.logo();
    }
    if( role == Qt::ForegroundRole) {
        if( pilvi.kokeilu() )
            return QColor(Qt::darkGreen);
    }

    return QVariant();
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



void PilviModel::avaaPilvesta(int pilviId, bool siirrossa)
{
    if(!siirrossa) kp()->odotusKursori(true);

    // Autentikoidaan ensin
    KpKysely* kysymys = kysely( QString("%1/auth/%2").arg(pilviLoginOsoite()).arg(pilviId));
    connect( kysymys, &KpKysely::vastaus, this, &PilviModel::alustaPilvi);
    kysymys->kysy();
}

KpKysely *PilviModel::kysely(const QString &polku, KpKysely::Metodi metodi)
{
    return new PilviKysely( this, metodi, polku);
}

void PilviModel::sulje()
{
    nykyPilvi_ = AvattuPilvi();
}

void PilviModel::poistaNykyinenPilvi()
{
    if( !pilvi() )
        return;

    QString polku = QString("%1/clouds/%2").arg(pilviLoginOsoite()).arg( pilvi().id() );
    PilviKysely *poisto = new PilviKysely(this, KpKysely::DELETE, polku);
    connect( poisto, &KpKysely::vastaus, this, &PilviModel::poistettu);
    poisto->kysy();
}

QString PilviModel::token() const
{
    if( pilvi() )
        return pilvi().token();
    else
        return kayttajaToken_;
}

QString PilviModel::service(const QString &serviceName) const
{
    const QString cloudService = nykyPilvi_.service(serviceName);
    return cloudService.isEmpty() ? kayttaja_.service(serviceName) : cloudService;
}


void PilviModel::asetaPilviLoginOsoite(const QString &osoite)
{
    pilviLoginOsoite__ = osoite;
}

Tilitieto::TilitietoPalvelu *PilviModel::tilitietoPalvelu()
{
    return tilitietoPalvelu_;
}

bool PilviModel::tilausvoimassa() const
{
    return  kayttaja_.planId() || kayttaja_.trialPeriod().isValid() ||
            nykyPilvi_.planId() || nykyPilvi_.trial_period() ;
}


void PilviModel::kirjaudu(const QString sahkoposti, const QString &salasana, bool pyydaAvain)
{
    QVariantMap map;
    if( !salasana.isEmpty()) {
        map.insert("email", sahkoposti);
        map.insert("password", salasana);
        if( pyydaAvain )
            map.insert("requestKey",true);
    } else if( kp()->settings()->contains("AuthKey") ) {
        QVariantMap keyMap;
        QStringList keyData = kp()->settings()->value("AuthKey").toString().split(",");
        keyMap.insert("id",keyData.value(0));
        keyMap.insert("secret", keyData.value(1));
        map.insert("key", keyMap);
        map.insert("requestKey",true);
        kp()->settings()->remove("AuthKey");
    } else {
        return;
    }

    map.insert("application", qApp->applicationName());
    map.insert("version", qApp->applicationVersion());
    map.insert("build", KITSAS_BUILD);
    map.insert("os", QSysInfo::prettyProductName());

    QNetworkAccessManager *mng = kp()->networkManager();

    // Tähän pilviosoite!
    QNetworkRequest request(QUrl( pilviLoginOsoite() + "/auth") );


    request.setRawHeader("Content-Type","application/json");
    request.setRawHeader("User-Agent",QString(qApp->applicationName() + " " + qApp->applicationVersion() ).toLatin1());

    QNetworkReply *reply = mng->post( request, QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact) );
    connect( reply, &QNetworkReply::finished, this, &PilviModel::kirjautuminenValmis);

    connect( kp(), &Kirjanpito::tietokantaVaihtui, tilitietoPalvelu_, &Tilitieto::TilitietoPalvelu::lataa );
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
    QVariant variant = doc.toVariant();

    lueTiedotKirjautumisesta(variant);
    emit kirjauduttu(kayttaja_);

    if( kp()->settings()->value("Viimeisin").toInt() > 0 && !kp()->yhteysModel())
        avaaPilvesta( kp()->settings()->value("Viimeisin").toInt() );

}


void PilviModel::kirjauduUlos()
{
    nykyPilvi_ = AvattuPilvi();
    kayttaja_ = PilviKayttaja();
    kayttajaToken_.clear();
    tokenUusittu_ = QDateTime();
    kp()->settings()->remove("AuthKey");
    timer_->stop();

    // TODO: Logouttaamalla poistetaan kyseinen avain myös palvelinpuolelta

    kp()->yhteysAvattu(nullptr);
    emit kirjauduttu(PilviKayttaja());
}

void PilviModel::paivitaLista(int avaaPilvi)
{
    avaaPilvi_ = avaaPilvi;
    KpKysely* kysymys = kysely( pilviLoginOsoite() + "/auth/" );
    connect( kysymys, &KpKysely::vastaus, [this] (QVariant* data) { this->lueTiedotKirjautumisesta(*data); } );
    kysymys->kysy();
}

void PilviModel::nimiMuuttui()
{
    paivitaLista();
}



void PilviModel::pilviLisatty(QVariant *paluu)
{
    QVariantMap map = paluu->toMap();    
    paivitaLista(map.value("id").toInt());

}

void PilviModel::poistettu()
{
    kp()->yhteysAvattu(nullptr);
    paivitaLista();
}

void PilviModel::yritaUudelleenKirjautumista()
{
    qDebug() << "** Yritä kirjautumista uudelleen ** ";

    kayttaja_ = PilviKayttaja();

    timer_->stop();
    endResetModel();
    kp()->yhteysAvattu(nullptr);
    emit kirjauduttu(PilviKayttaja()); // Jotta etusivu ei näytä kirjautuneelta

    kirjaudu();
}

void PilviModel::tarkistaKirjautuminen()
{
    // Jos viimeisestä tokenin uusimisesta on yli tunti ja ollaan kirjautuneena,
    // yritetään uusia token

    if( kayttaja_ && tokenUusittu_.isValid() && tokenUusittu_.secsTo(QDateTime::currentDateTime()) > 60 * 60 ) {
        tokenUusittu_ = QDateTime();
        paivitaLista();
    }
}

void PilviModel::alustaPilvi(QVariant *data)
{
    nykyPilvi_ = AvattuPilvi(*data);
    alusta();
}

void PilviModel::lueTiedotKirjautumisesta(const QVariant &data)
{
    const QVariantMap map = data.toMap();

    kayttaja_ = map.value("user");
    asetaPilviLista( map.value("clouds").toList());

    kayttajaToken_ = map.value("token").toString();
    tokenUusittu_ = QDateTime::currentDateTime();        

    if(avaaPilvi_) {
        avaaPilvesta(avaaPilvi_);
        avaaPilvi_ = 0;
    } else if( nykyPilvi_) {
        KpKysely* uusinta = kysely( pilviLoginOsoite() + "/auth/" + nykyPilvi_.id(), KpKysely::GET );
        connect( uusinta, &KpKysely::vastaus, this, [this] (QVariant* data) { this->nykyPilvi_ = *data; });
    }
}

void PilviModel::asetaPilviLista(const QVariantList lista)
{
    beginResetModel();
    pilvet_.clear();
    pilvet_.reserve(lista.size());
    for(const auto &item : lista) {
        pilvet_.append(ListanPilvi(item));
    }
    endResetModel();
}


QString PilviModel::pilviLoginOsoite__;
