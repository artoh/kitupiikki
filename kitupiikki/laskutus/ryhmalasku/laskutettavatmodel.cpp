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
#include "laskutettavatmodel.h"
#include "db/kirjanpito.h"
#include "../myyntilaskuntulostaja.h"

LaskutettavatModel::LaskutettavatModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant LaskutettavatModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case NIMI : return tr("Nimi");
        case KIELI: return tr("Kieli");
        case LAHETYSTAPA: return tr("Lähetystapa");
        }
    }
    return QVariant();
}


int LaskutettavatModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return laskutettavat_.count();
}

int LaskutettavatModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 3;
}

QVariant LaskutettavatModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if( role == Qt::DisplayRole) {
        Laskutettava laskutettava = laskutettavat_.at(index.row());
        switch (index.column()) {
        case NIMI:
            return laskutettava.nimi;
        case KIELI:
            if( laskutettava.kieli == "SV")
                return tr("ruotsi");
            else if( laskutettava.kieli == "EN")
                return tr("englanti");
            else
                return tr("suomi");
        case LAHETYSTAPA:
            switch (laskutettava.lahetystapa) {
                case LaskuDialogi::TULOSTETTAVA:
                    return tr("Tulosta");
                case LaskuDialogi::SAHKOPOSTI:
                    return tr("Sähköposti");
                case LaskuDialogi::POSTITUS:
                    return tr("Postitus");
                case LaskuDialogi::VERKKOLASKU:
                    return tr("Verkkolasku");
            }
        }
    }
    // Todo: Decorationit sekä toimitustaparajaukset

    // FIXME: Implement me!
    return QVariant();
}

bool LaskutettavatModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (data(index, role) != value) {
        // FIXME: Implement me!
        emit dataChanged(index, index, QVector<int>() << role);
        return true;
    }
    return false;
}

Qt::ItemFlags LaskutettavatModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    if( index.column() > NIMI)
        return Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void LaskutettavatModel::tallennaLaskut(const QVariantMap &data)
{
    if( rowCount())
        tallennaLasku(data, 0);
}

void LaskutettavatModel::lisaa(int kumppaniId)
{
    KpKysely *kysely = kpk(QString("/kumppanit/%1").arg(kumppaniId));
    connect(kysely, &KpKysely::vastaus, this, &LaskutettavatModel::lisaaAsiakas);
    kysely->kysy();
}

void LaskutettavatModel::tallennaLasku(const QVariantMap &tallennettava, int indeksi)
{
    Laskutettava laskutettava = laskutettavat_.value(indeksi);
    QVariantMap data(tallennettava);
    QVariantMap lasku = data.value("lasku").toMap();

    if( laskutettava.kumppaniId)
        data.insert("kumppani", laskutettava.kumppaniId);
    lasku.insert("osoite", laskutettava.osoite);
    if( !laskutettava.alvtunnus.isEmpty())
        lasku.insert("alvtunnus", laskutettava.alvtunnus);
    lasku.insert("kieli", laskutettava.kieli);
    lasku.insert("laskutapa", laskutettava.lahetystapa);
    if( !laskutettava.email.isEmpty())
        lasku.insert("email", laskutettava.email);
    data.insert("lasku", lasku);

    QVariantList viennit = data.value("viennit").toList();
    QVariantList uusiviennit;
    for(auto item : viennit) {
        QVariantMap map = item.toMap();
        map.insert("kumppani",laskutettava.kumppaniId);
        uusiviennit.append(map);
    }
    data.insert("viennit", uusiviennit);

    KpKysely* kysely = kpk("/tositteet", KpKysely::POST);
    connect( kysely, &KpKysely::vastaus, [this, tallennettava, indeksi] (QVariant *vastaus) { this->laskuTallennettu(tallennettava, indeksi, vastaus);} );
    kysely->kysy(data);
}

void LaskutettavatModel::laskuTallennettu(const QVariantMap &tallennettava, int indeksi, QVariant *vastaus)
{
    // Tallennetaan ensin liite
    QVariantMap map = vastaus->toMap();

    QByteArray liite = MyyntiLaskunTulostaja::pdf( map );
    KpKysely *liitetallennus = kpk( QString("/liitteet/%1/lasku").arg(map.value("id").toInt()), KpKysely::PUT);
    QMap<QString,QString> meta;
    meta.insert("Filename", QString("lasku%1.pdf").arg( map.value("lasku").toMap().value("numero").toInt() ) );
    liitetallennus->lahetaTiedosto(liite, meta);

    if( indeksi < rowCount() - 1) {
        tallennaLasku(tallennettava, indeksi+1);
    } else {
        emit tallennettu();
        emit kp()->kirjanpitoaMuokattu();
    }
}

void LaskutettavatModel::lisaaAsiakas(QVariant* data)
{
    QVariantMap map = data->toMap();
    Laskutettava uusi;

    uusi.kumppaniId = map.value("id").toInt();
    uusi.nimi = map.value("nimi").toString();
    if( map.contains("osoite"))
        uusi.osoite = uusi.nimi + "\n" +
                map.value("osoite").toString() + "\n" +
                map.value("postinumero").toString() + " " +
                map.value("kaupunki").toString();
    uusi.kieli = map.value("kieli").toString();
    uusi.alvtunnus = map.value("alvtunnus").toString();
    uusi.lahetystapa = map.value("laskutapa").toInt();
    uusi.ovttunnus = map.value("ovt").toString();
    uusi.valittaja = map.value("operaattori").toString();
    if( uusi.kumppaniId)
        kumppaniIdt_.insert(uusi.kumppaniId);
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    laskutettavat_.append(uusi);
    endInsertRows();
}
