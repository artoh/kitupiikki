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
#include "asiakastoimittajataydentaja.h"
#include "db/kirjanpito.h"

#include <QDebug>

AsiakasToimittajaTaydentaja::AsiakasToimittajaTaydentaja(QObject *parent)
    : QAbstractListModel(parent)
{
}

int AsiakasToimittajaTaydentaja::rowCount(const QModelIndex &parent) const
{

    if (parent.isValid())
        return 0;

    return data_.count();
}

QVariant AsiakasToimittajaTaydentaja::data(const QModelIndex &index, int role) const
{

    if (!index.isValid())
        return QVariant();

    // FIXME: Implement me!
    if( role == Qt::DisplayRole || role == Qt::EditRole)
    {
        return data_.at(index.row()).first;
    }

    return QVariant();
}

int AsiakasToimittajaTaydentaja::haeNimella(const QString &nimi) const
{
    if( nimi.isEmpty())
        return 0;

    for( auto item : data_) {
        if( item.first == nimi)
            return item.second;
    }
    return -1;
}

void AsiakasToimittajaTaydentaja::lataa()
{
    KpKysely* kysely = kpk("/kumppanit");
    connect( kysely, &KpKysely::vastaus, this, &AsiakasToimittajaTaydentaja::saapuu);
    kysely->kysy();
}

void AsiakasToimittajaTaydentaja::saapuu(QVariant *variant)
{
    beginResetModel();
    data_.clear();
    QVariantList lista = variant->toList();

    for( auto item : lista) {
        QVariantMap map = item.toMap();
        data_.append( qMakePair( map.value("nimi").toString(), map.value("id").toInt()) );
    }

    endResetModel();
}
