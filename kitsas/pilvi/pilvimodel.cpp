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
#include "maaritys/tilitieto/tilitietopalvelu.h"
#include "paivitysinfo.h"

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
#include <QApplication>

#include "uusikirjanpito/uusivelho.h"

PilviModel::PilviModel(QObject *parent, const QString &token) :
    YhteysModel (parent),
    kayttajaToken_(token),
    paivitysInfo_{new PaivitysInfo(this)},
    tilitietoPalvelu_(new Tilitieto::TilitietoPalvelu(this))
{
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &PilviModel::tarkistaKirjautuminen);    
    connect( kp(), &Kirjanpito::perusAsetusMuuttui, this, [this] { QTimer::singleShot(1500, this, &PilviModel::nimiMuuttui); });
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
        if( pilvi.ready())
            return pilvi.logo();
        else
            return QIcon(":/pic/lisaa.png");
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

KpKysely *PilviModel::loginKysely(const QString &polku, KpKysely::Metodi metodi)
{
    return new PilviKysely( this, metodi, pilviLoginOsoite() + polku);
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
    if( !cloudService.isEmpty()) return cloudService;

    const QString userService = kayttaja_.service(serviceName);
    if( !userService.isEmpty()) return userService;

    const QString infoService = paivitysInfo_->service(serviceName);
    if( !infoService.isEmpty()) return infoService;

    return QString();
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


void PilviModel::kirjauduUlos()
{    

    if( kp()->settings()->contains("AuthKey") ) {
            // Logouttaamalla poistetaan kyseinen avain myös palvelinpuolelta
            const QStringList keyData = kp()->settings()->value("AuthKey").toString().split(",");
            QVariantMap payload;
            payload.insert("keyid", keyData.value(0));
            KpKysely* kysymys = loginKysely("/logout", KpKysely::POST);
            kysymys->kysy(payload);
            kp()->settings()->remove("AuthKey");
    }

    nykyPilvi_ = AvattuPilvi();
    kayttaja_ = PilviKayttaja();

    kayttajaToken_.clear();
    tokenUusittu_ = QDateTime();        
    timer_->stop();

    beginResetModel();
    pilvet_.clear();
    endResetModel();

    kp()->yhteysAvattu(nullptr);
    emit kirjauduttu(PilviKayttaja());
}

void PilviModel::paivitaLista(int avaaPilvi)
{
    avaaPilvi_ = avaaPilvi;
    KpKysely* kysymys = kysely( pilviLoginOsoite() + "/auth/" );
    connect( kysymys, &KpKysely::vastaus, [this] (QVariant* data) { this->kirjautuminen(data->toMap()); } );
    kysymys->kysy();
}

void PilviModel::nimiMuuttui()
{
    paivitaLista();
}

void PilviModel::kirjautuminen(const QVariantMap &data, int avaaPilvi)
{
    if( avaaPilvi) {
        avaaPilvi_ = avaaPilvi;
    }    

    kayttaja_ = data.value("user");
    asetaPilviLista( data.value("clouds").toList());

    kayttajaToken_ = data.value("token").toString();
    tokenUusittu_ = QDateTime::currentDateTime();

    {
        emit kirjauduttu(kayttaja_);
    }

    if(avaaPilvi_) {
        avaaPilvesta(avaaPilvi_);
        avaaPilvi_ = 0;
    } else if( nykyPilvi_) {
        KpKysely* uusinta = kysely(QString("%1/auth/%2").arg(pilviLoginOsoite()).arg(nykyPilvi_.id()), KpKysely::GET );
        connect( uusinta, &KpKysely::vastaus, this, [this] (QVariant* data) { this->nykyPilvi_ = *data; });
    }

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
    AvattuPilvi pilvi(*data);
    if( !pilvi.alustettu() ) {
        kp()->odotusKursori(false);
        if( !(pilvi.oikeudet() & YhteysModel::ASETUKSET)) {
            // Pilveä ei ole vielä alustettu, eikä siihen ole myöskään oikeuksia
            QMessageBox::information( qApp->activeWindow() , tr("Kirjanpidon avaaminen"), tr("Kirjanpitoa %1 ei ole vielä alustettu").arg(pilvi.nimi()));
            return;
        } else {
            nykyPilvi_ = pilvi;
            UusiVelho velho(qApp->activeWindow());
            QVariantMap data = velho.alustusVelho( pilvi.ytunnus(), pilvi.nimi(), pilvi.kokeilu() );
            if( data.isEmpty()) { sulje(); return; }

            nykyPilvi_.asetaNimi( data.value("name").toString() );
            nykyPilvi_.asetaYTunnus(data.value("businessid").toString());

            KpKysely* kyssari = kysely("/init", KpKysely::PUT);
            connect( kyssari, &KpKysely::vastaus, this, &PilviModel::uusiPilviAlustettu);
            kyssari->kysy(data.value("init"));
        }
    } else {
        nykyPilvi_ = pilvi;
        alusta();
    }
}

void PilviModel::uusiPilviAlustettu()
{
    QVariantMap payload;
    payload.insert("name", nykyPilvi_.nimi());
    payload.insert("trial", nykyPilvi_.kokeilu());
    payload.insert("businessid", nykyPilvi_.ytunnus());

    KpKysely* patchKysely = loginKysely(QString("/clouds/%1").arg(nykyPilvi_.id()), KpKysely::PATCH);
    connect( patchKysely, &KpKysely::vastaus, this, [this] { this->paivitaLista( this->nykyPilvi_.id() ); });
    patchKysely->kysy(payload);
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
