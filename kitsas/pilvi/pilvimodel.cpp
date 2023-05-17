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
#include <QProgressDialog>

#include "uusikirjanpito/uusivelho.h"
#include "aloitussivu/kirjanpitodelegaatti.h"

PilviModel::PilviModel(QObject *parent, const QString &token) :
    YhteysModel (parent),
    kayttajaToken_(token),
    paivitysInfo_{new PaivitysInfo(this)},
    tilitietoPalvelu_(new Tilitieto::TilitietoPalvelu(this))
{
    // Tokenilla alustettaessa ollaan PilveenSiirron yksityisessä kirjoituspilvessä
    if( token.isEmpty() ) {
        timer_ = new QTimer(this);
        connect(timer_, &QTimer::timeout, this, &PilviModel::tarkistaKirjautuminen);
        connect( kp(), &Kirjanpito::perusAsetusMuuttui, this, [this] { QTimer::singleShot(1500, this, &PilviModel::nimiMuuttui); });
        ilmoitusTimer_ = new QTimer(this);
        connect( ilmoitusTimer_, &QTimer::timeout, this, &PilviModel::haeIlmoitusPaivitys);
    }
}

int PilviModel::rowCount(const QModelIndex & /* parent */) const
{
    return pilvet_.count();
}

QVariant PilviModel::data(const QModelIndex &index, int role) const
{
    const ListanPilvi& pilvi = pilvet_.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case NimiRooli:
        return pilvi.nimi();
    case IdRooli:
        return pilvi.id();
    case KirjanpitoDelegaatti::LogoRooli:
        return pilvi.logo();
    case KirjanpitoDelegaatti::HarjoitusRooli:
        return pilvi.kokeilu();
    case KirjanpitoDelegaatti::AlustettuRooli:
        return pilvi.ready();
    case KirjanpitoDelegaatti::BadgesRooli:
        return pilvi.badges().badges();
    default:
        return QVariant();
    }
}



QString PilviModel::pilviLoginOsoite()
{
    return pilviLoginOsoite__;
}

void PilviModel::uusiPilvi(const QVariant &initials)
{
    QWidget* main = qApp->topLevelWidgets().isEmpty() || qApp->topLevelWidgets().first()->isHidden() ? nullptr : qApp->topLevelWidgets().first();
    progressDialog_ = new QProgressDialog( tr("Kirjanpitoa luodaan..."), tr("Keskeytä"), 0, 100, main);
    connect( progressDialog_, &QProgressDialog::canceled, this, &PilviModel::keskeytaLataus);
    progressDialog_->setValue(5);
    progressDialog_->setMinimumDuration(10);

    PilviKysely* kysely = new PilviKysely(this, KpKysely::POST, pilviLoginOsoite() + "/clouds");
    connect( kysely, &PilviKysely::vastaus, this, &PilviModel::pilviLisatty);
    connect( kysely, &PilviKysely::virhe, [] (int koodi, const QString& viesti) {QMessageBox::critical(nullptr, tr("Kirjanpidon luominen epäonnistui"),
                                                                                     tr("Kirjanpitoa luotaessa tapahtui virhe:\n%1 %2").arg(koodi).arg(viesti)); });
    kysely->kysy(initials);
}

void PilviModel::alusta()
{
    if( !nykyPilvi_) return;    // Lataus keskeytetty

    if(progressDialog_) progressDialog_->setValue(60);
    KpKysely *initkysely = kysely("/init");
    connect( initkysely, &KpKysely::vastaus, this, &PilviModel::lataaInit );
    initkysely->kysy();
}

void PilviModel::lataaInit(QVariant *reply)
{
    if( !nykyPilvi_) return;    // Lataus keskeytetty

    if( progressDialog_) progressDialog_->setValue(90);

    YhteysModel::lataaInit(reply);
    if( progressDialog_ ) {
        progressDialog_->setValue(100);
        progressDialog_->deleteLater();
        progressDialog_ = nullptr;
    }

    kp()->yhteysAvattu(this);
    kp()->odotusKursori(false);
}

void PilviModel::keskeytaLataus()
{
    nykyPilvi_ = AvattuPilvi();
    kp()->odotusKursori(false);
    kp()->yhteysAvattu(nullptr);

}

void PilviModel::haeIlmoitusPaivitys()
{
    if( kayttajaPilvessa() ) {
        KpKysely* kysely = loginKysely("/notifications");
        connect( kysely, &KpKysely::vastaus, this, &PilviModel::paivitaIlmoitukset);
        kysely->kysy();
    }
}

void PilviModel::paivitaIlmoitukset(QVariant *data)
{
    QVariantMap map = data->toMap();
    QVariantList pilvet = map.value("books").toList();
    QVariantList notifikaatiot = map.value("notifications").toList();

    nykyPilvi_.asetaNotifikaatiot(notifikaatiot);

    QMap<int,QStringList> badgeMap;
    for(const auto& item : pilvet) {
        const QVariantMap map = item.toMap();
        badgeMap.insert(map.value("id").toInt(), map.value("badges").toStringList());
    }

    for(int i=0; i < pilvet_.count(); i++) {
        const int id = pilvet_.at(i).id();
        pilvet_[i].asetaBadget( badgeMap.value(id) );
    }
    emit dataChanged(index(0), index(pilvet_.count()-1), QList<int>() << Qt::DisplayRole);

    ilmoitusTimer_->start();
}



void PilviModel::avaaPilvesta(int pilviId, bool siirrossa)
{
    if(!siirrossa && !progressDialog_) {
        kp()->odotusKursori(true);
        progressDialog_ = new QProgressDialog( tr("Kirjanpitoa avataan..."), tr("Keskeytä"), 0, 100, qApp->activeWindow());
        connect( progressDialog_, &QProgressDialog::canceled, this, &PilviModel::keskeytaLataus);
        progressDialog_->setMinimumDuration(100);
    }
    if(progressDialog_) progressDialog_->setValue(40);

    if( nykyPilvi_ ) {
        haeIlmoitusPaivitys();
    }

    // Autentikoidaan ensin
    KpKysely* kysymys = kysely( QString("%1/auth/%2").arg(pilviLoginOsoite()).arg(pilviId));
    connect( kysymys, &KpKysely::vastaus, this, [this, siirrossa](QVariant* data) { this->alustaPilvi(data, siirrossa);});
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

void PilviModel::asetaAlias(const QString &alias)
{
    nykyPilvi_.asetaAlias(alias);
}

void PilviModel::poistaNotify(const int id)
{
    nykyPilvi_.poistaNotify(id);
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
    ilmoitusTimer_->stop();

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


    emit kirjauduttu(kayttaja_);
    if( kayttaja_ && timer_ && ilmoitusTimer_) {
        // Tarkastetaan tokenin uusintatarve kerran minuutissa
        timer_->start(1000 * 60);
        ilmoitusTimer_->start( 1000 * 60 * 10);  // Päivitys 10 min välein - voisi olla harvemmin?
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
    if(progressDialog_) progressDialog_->setValue(15);
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
    // Jos viimeisestä tokenin uusimisesta on yli 12 tuntia ja ollaan kirjautuneena,
    // yritetään uusia token

    if( kayttaja_ && tokenUusittu_.isValid() && tokenUusittu_.secsTo(QDateTime::currentDateTime()) > 60 * 60 * 12 ) {
        tokenUusittu_ = QDateTime();
        paivitaLista();
    }
}

void PilviModel::alustaPilvi(QVariant *data, bool siirrossa)
{
    if( !progressDialog_ && !siirrossa) {
        progressDialog_ = new QProgressDialog( tr("Kirjanpitoa alustetaan..."), tr("Keskeytä"), 0, 100, qApp->activeWindow());
        connect( progressDialog_, &QProgressDialog::canceled, this, &PilviModel::keskeytaLataus);
        progressDialog_->setMinimumDuration(50);
    }

    if(progressDialog_) progressDialog_->setValue(30);

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
        if(siirrossa)
            emit siirtoPilviAvattu();
        else {
            alusta();
        }

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
