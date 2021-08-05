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
#include "kielidelegaatti.h"
#include "toimitustapadelegaatti.h"
#include "model/lasku.h"
#include "rekisteri/maamodel.h"

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

    Laskutettava laskutettava = laskutettavat_.at(index.row());
    if( role == Qt::DisplayRole) {        
        switch (index.column()) {
        case NIMI:
            return laskutettava.nimi();
        case KIELI:
            return KieliDelegaatti::kieliKoodilla(laskutettava.kieli());
        case LAHETYSTAPA:       
            return ToimitustapaDelegaatti::toimitustapa(laskutettava.lahetystapa());
        }
    } else if( role == Qt::EditRole) {

        switch (index.column()) {
        case NIMI:
            return laskutettava.nimi();
        case KIELI:
            return laskutettava.kieli();
        case LAHETYSTAPA:
            return laskutettava.lahetystapa();
        }
    } else if( role == Qt::DecorationRole) {
        if( index.column() == KIELI)
            return QIcon(QString(":/liput/%1.png").arg(laskutettava.kieli().toLower()));
        else if(index.column() == LAHETYSTAPA)
            return ToimitustapaDelegaatti::icon(laskutettava.lahetystapa());
    } else if( role == LahetysTavatRooli) {
        return laskutettava.lahetystavat();
    }

    return QVariant();
}

bool LaskutettavatModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (data(index, role) != value) {
        if( index.column() == KIELI && role == Qt::EditRole)
            laskutettavat_[index.row()].setKieli(value.toString());
        else if( index.column() == LAHETYSTAPA && role == Qt::EditRole)
            laskutettavat_[index.row()].setLahetystapa(value.toInt());

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


bool LaskutettavatModel::onkoKumppania(int kumppaniId) const
{
    return kumppaniIdt_.contains(kumppaniId);
}

QList<LaskutettavatModel::Laskutettava> LaskutettavatModel::laskutettavat() const
{
    QList<Laskutettava> lista;
    for(const auto& item : laskutettavat_)
        lista.append(item);
    return lista;
}

void LaskutettavatModel::lisaa(int kumppaniId)
{
    KpKysely *kysely = kpk(QString("/kumppanit/%1").arg(kumppaniId));
    connect(kysely, &KpKysely::vastaus, this, &LaskutettavatModel::lisaaAsiakas);
    kysely->kysy();
}

void LaskutettavatModel::lisaaAsiakas(QVariant* data)
{
    QVariantMap map = data->toMap();
    Laskutettava uusi(map);
    if( kumppaniIdt_.contains(uusi.id())) {
        return;
    }
    kumppaniIdt_.insert(uusi.id());

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    laskutettavat_.append(uusi);
    endInsertRows();
}

void LaskutettavatModel::poista(int indeksi)
{
    kumppaniIdt_.remove(laskutettavat_.value(indeksi).id());

    beginRemoveRows(QModelIndex(), indeksi, indeksi);
    laskutettavat_.removeAt(indeksi);
    endRemoveRows();
}


LaskutettavatModel::Laskutettava::Laskutettava()
{

}

LaskutettavatModel::Laskutettava::Laskutettava(const QVariantMap &map) :
    kumppani_(map)
{
    lahetystapa_ = map.value("laskutapa").toInt();
    kieli_ = map.value("kieli","FI").toString();
}


QString LaskutettavatModel::Laskutettava::kieli() const
{
    return kieli_;
}

void LaskutettavatModel::Laskutettava::setKieli(const QString &kieli)
{
    kieli_ = kieli;
}

int LaskutettavatModel::Laskutettava::lahetystapa() const
{
    return lahetystapa_;
}

void LaskutettavatModel::Laskutettava::setLahetystapa(const int lahetystapa)
{
    lahetystapa_ = lahetystapa;
}


QVariantList LaskutettavatModel::Laskutettava::lahetystavat() const
{
    QVariantList lista;
    lista << Lasku::TULOSTETTAVA;
    if( email().contains("@"))
        lista << Lasku::SAHKOPOSTI;
    if( kumppani_.value("osoite").toString().length() > 3 &&
        kumppani_.value("postinumero").toString().length() > 3 &&
        kumppani_.value("kaupunki").toString().length() > 1)
        lista << Lasku::POSTITUS;
    if( kumppani_.value("ovt").toString().length() > 5 &&
        kumppani_.value("operaattori").toString().length() > 3)
        lista << Lasku::VERKKOLASKU;
    lista << Lasku::PDF;
    lista << Lasku::EITULOSTETA;
    return lista;
}

QString LaskutettavatModel::Laskutettava::nimi() const
{
    return kumppani_.value("nimi").toString();
}

QString LaskutettavatModel::Laskutettava::email() const
{
    return kumppani_.value("email").toString();
}

QString LaskutettavatModel::Laskutettava::osoite() const
{
    return MaaModel::instanssi()->muotoiltuOsoite(kumppani_);
}

QVariantMap LaskutettavatModel::Laskutettava::map() const
{
    return kumppani_;
}

int LaskutettavatModel::Laskutettava::id() const
{
    return kumppani_.value("id").toInt();
}
