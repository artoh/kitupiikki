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
#include "tositeloki.h"
#include "model/tosite.h"
#include <QDateTime>

TositeLoki::TositeLoki(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void TositeLoki::lataa(const QVariant &data)
{
    beginResetModel();
    data_ = data.toList();
    endResetModel();
}

QVariant TositeLoki::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role != Qt::DisplayRole )
        return QVariant();
    else if( orientation == Qt::Horizontal)
    {
        switch (section)
        {
            case AIKA:
                return tr("Muokattu");
            case KAYTTAJA:
                return tr("Käyttäjä");
            case TILA :
                return tr("Tila");
        }
    }
    return QVariant();
}

int TositeLoki::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return data_.count();
}

int TositeLoki::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 3;
}

QVariant TositeLoki::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    QVariantMap map = data_.at(index.row()).toMap();

    if( role == Qt::DisplayRole) {
        switch (index.column()) {
        case AIKA:
            return map.value("aika").toDateTime().toLocalTime();
        case KAYTTAJA:
            return map.value("nimi").toString();
        case TILA:
            return Tosite::tilateksti( map.value("tila").toInt() );
        }
    }    
    else if( role == Qt::UserRole)
        return map.value("data");

    return QVariant();
}
