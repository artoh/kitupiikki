/*
   Copyright (C) 2017 Arto Hyvättinen

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

#include <QDebug>

#include <QDate>
#include <QString>
#include <QVariant>

#include <QMessageBox>

#include <QJsonDocument>

#include "asetusmodel.h"

#include "kpkysely.h"
#include "kirjanpito.h"

#include "kieli/kielet.h"

AsetusModel::AsetusModel(QObject *parent)
    :   QObject(parent)
{

}

QString AsetusModel::asetus(int avain, const QString &oletus) const
{
    return asetus( avaimet__.at(avain), oletus);
}

void AsetusModel::aseta(const QString &avain, const QString &arvo)
{
    if( arvo.isNull())
        asetukset_.remove(avain);
    else
        asetukset_[avain] = arvo;

    KpKysely* paivitys = kpk("/asetukset", KpKysely::PATCH);

    QVariantMap asetus;
    asetus.insert(avain, arvo);

    paivitys->kysy(asetus);

}

void AsetusModel::aseta(const QVariantMap &map)
{
    QMapIterator<QString,QVariant> iter(map);
    while(iter.hasNext()) {
        iter.next();
        if( iter.value().isNull())
            asetukset_.remove( iter.key() );
        else
            asetukset_.insert( iter.key(), iter.value().toString());
    }
    KpKysely* paivitys = kpk("/asetukset", KpKysely::PATCH);
    connect(paivitys, &KpKysely::vastaus, this, &AsetusModel::asetusMuuttui);
    paivitys->kysy(map);
}

void AsetusModel::aseta(int tunnus, const QString &arvo)
{
    aseta( avaimet__.at(tunnus), arvo );
}

void AsetusModel::poista(const QString &avain)
{
    aseta(avain, QString());
}

QDate AsetusModel::pvm(const QString &avain, const QDate oletus) const
{
    if( !asetukset_.contains(avain) )
        return oletus;
    return QDate::fromString( asetus(avain), Qt::ISODate );
}

void AsetusModel::aseta(const QString& avain, const QDate &pvm)
{
    aseta( avain, pvm.toString(Qt::ISODate));
}

bool AsetusModel::onko(const QString &avain) const
{
    if( !asetukset_.contains(avain)   ||  asetus(avain).isEmpty() || asetus(avain) == "0" || asetus(avain) == "EI")
        return false;
    else
        return true;
}

bool AsetusModel::onko(int tunnus) const
{
    return onko( avaimet__.at(tunnus));
}

void AsetusModel::aseta(const QString &avain, bool totuusarvo)
{
    if( totuusarvo )
        aseta( avain, QString("ON"));
    else
        aseta( avain, QString("EI"));
}

void AsetusModel::asetaVar(const QString &avain, const QVariant &arvo)
{
    if( arvo.isNull())
    {
        poista(avain);
    }
    else if( arvo.type() == QVariant::Date)
        aseta( avain, arvo.toDate().toString(Qt::ISODate));
    else if( arvo.type() == QVariant::Bool)
    {
        if( arvo.toBool())
            aseta( avain, true);
        else
            aseta( avain, false );
    }
    else if( arvo.type() == QVariant::StringList)
    {
        aseta( avain, arvo.toStringList().join('\n'));
    }
    else
        aseta( avain, arvo.toString());
}

QStringList AsetusModel::lista(const QString &avain) const
{
    if( asetus(avain).isEmpty())
        return QStringList();

    return asetus(avain).split('\n');
}

void AsetusModel::aseta(const QString &avain, const QStringList &arvo)
{

    aseta( avain, arvo.join('\n'));
}

int AsetusModel::luku(const QString &avain, int oletusarvo) const
{
    if( asetukset_.contains(avain))
        return asetukset_.value(avain).toInt();
    else
        return oletusarvo;
}

int AsetusModel::luku(int tunnus, int oletusarvo) const
{
    return luku( avaimet__.at(tunnus), oletusarvo);
}

qulonglong AsetusModel::isoluku(const QString &avain, qulonglong oletusarvo) const
{
    if( asetukset_.contains(avain))
        return asetukset_.value(avain).toULongLong();
    else
        return oletusarvo;

}

void AsetusModel::aseta(const QString& avain, int luku)
{
    if( !luku)
        poista(avain);  // Nolla-arvolla asetus poistetaan (on joka tapauksessa tallella)
    else
        aseta(avain, QString::number(luku));
}

void AsetusModel::aseta(const QString &avain, qulonglong luku)
{
    if( !luku)
        poista(avain);  // Nolla-arvolla asetus poistetaan (on joka tapauksessa tallella)
    else
        aseta(avain, QString::number(luku));
}

QStringList AsetusModel::avaimet(const QString &avaimenAlku) const
{
    if( avaimenAlku.isEmpty())
        return asetukset_.keys();
    QStringList vastaus;
    foreach (QString avain, asetukset_.keys())
    {
        if( avain.startsWith(avaimenAlku))
            vastaus.append( avain);
    }
    return vastaus;
}

QStringList AsetusModel::kielet() const
{
    QStringList lista;
    for(const auto &kieli : Kielet::instanssi()->kielet()) {
        lista.append(kieli.nimi());
    }
    return lista;
}

QString AsetusModel::kieli(const QString &lyhenne) const
{
    for( const auto &kieli : Kielet::instanssi()->kielet()) {
        if( kieli.lyhenne() == lyhenne)
            return kieli.nimi();
    }
    return QString();
}


void AsetusModel::lataa(const QVariantMap &lista)
{
    asetukset_.clear();
    QMapIterator<QString,QVariant> iter(lista);
    while( iter.hasNext()) {
        iter.next();
        asetukset_.insert( iter.key(), iter.value().toString() );
    }

    // Ladataan kielet
    Kielet::instanssi()->asetaKielet( asetus("kielet") );    
}

std::map<int,QString> AsetusModel::avaimet__ = {
    { AlvVelvollinen, "AlvVelvollinen" },
    { EriSarjaan, "erisarjaan"},
    { KateisSarjaan, "kateissarjaan"},
    { OrganisaatioNimi, "Nimi"},
    { Katuosoite, "Katuosoite"},
    { Postinumero, "Postinumero"},
    { Kaupunki, "Kaupunki"},
    { Aputoiminimi, "Aputoiminimi"},
    { Logonsijainti, "LogonSijainti"},
    { LaskuIkkunaKorkeus, "LaskuIkkunaKorkeus"},
    { Puhelin, "Puhelin" },
    { Email, "Email" },
    { Kotisivu, "Kotisivu" },
    { Kotipaikka, "Kotipaikka" },
    { Ytunnus, "Ytunnus"},
    { LaskuIbanit, "LaskuIbanit"},
    { LaskuRF, "LaskuRF"},
    { LaskuQR, "LaskuQR"},
    { LaskuViivakoodi, "LaskuViivakoodi"},
    { LaskuVirtuaaliviivakoodi, "LaskuVirtuaaliViivakoodi"},
    { LaskuTilisiirto, "LaskuTilisiirto"},
    { LaskuKateistili, "LaskuKateistili"},
    { LaskuEnnakkosaatavaTili, "LaskuEnnakkoSaatavatili"},
    { LaskuSaatavaTili, "LaskuSaatavatili"},
    { EnnakkoAlvTili, "LaskuEnnakkoALV"}
};
