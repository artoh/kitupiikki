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

void LaskutettavatModel::lisaa(const QVariantMap &data)
{
    Laskutettava uusi;
    uusi.kumppaniId = data.value("id").toInt();
    uusi.nimi = data.value("nimi").toString();
    if( data.contains("osoite"))
        uusi.osoite = data.value("osoite").toString() + "\n" +
                data.value("postinumero").toString() + " " +
                data.value("kaupunki").toString();
    uusi.kieli = data.value("kieli").toString();
    uusi.alvtunnus = data.value("alvtunnus").toString();
    uusi.lahetystapa = data.value("laskutapa").toInt();
    uusi.ovttunnus = data.value("ovt").toString();
    uusi.valittaja = data.value("operaattori").toString();
    if( uusi.kumppaniId)
        kumppaniIdt_.insert(uusi.kumppaniId);
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    laskutettavat_.append(uusi);
    endInsertRows();
}
