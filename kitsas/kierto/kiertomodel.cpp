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
#include "kiertomodel.h"
#include "db/kirjanpito.h"

KiertoModel::KiertoModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int KiertoModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    return lista_.count();
}

QVariant KiertoModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if( role == Qt::DisplayRole || role == Qt::EditRole) {
        return lista_.at(index.row()).second;
    } else if( role == IdRooli) {
        return lista_.at(index.row()).first;
    }

    return QVariant();
}

void KiertoModel::lataaData(QVariant *lista)
{
    const QVariantList& list = lista->toList();
    beginResetModel();
    lista_.clear();
    for(auto item : list) {
        const QVariantMap& map = item.toMap();
        lista_.append(qMakePair(map.value("id").toInt(), map.value("nimi").toString()));
    }
    endResetModel();
}

void KiertoModel::lataa(const QVariantList &lista)
{
    beginResetModel();
    lista_.clear();
    for(auto item : lista) {
        const QVariantMap& map = item.toMap();
        lista_.append(qMakePair(map.value("id").toInt(), map.value("nimi").toString()));
    }
    endResetModel();
}

void KiertoModel::paivita()
{
    KpKysely *kysely = kpk("/kierrot");
    connect(kysely, &KpKysely::vastaus, this, &KiertoModel::lataaData);
    kysely->kysy();
}
