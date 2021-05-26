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
#include "db/tositetyyppimodel.h"

#include <QIcon>
#include <QDebug>

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

    QVariantMap item = lista_.value(index.row()).toMap();    

    if( role == Qt::DisplayRole || role == Qt::EditRole) {
        return item.value("nimi");
    } else if( role == IdRooli) {
        return item.value("id");
    } else if( role == Qt::DecorationRole) {
        int tositetyyppi = item.value("tyyppi").toInt();
        return kp()->tositeTyypit()->kuvake(tositetyyppi);
    }

    return QVariant();
}

QString KiertoModel::nimi(int id) const
{
    for(auto& item : lista_) {
        if( item.toMap().value("id") == id)
            return item.toMap().value("nimi").toString();
    }
    return QString();
}

void KiertoModel::lataaData(QVariant *lista)
{
    lataa(lista->toList());
}

void KiertoModel::lataa(const QVariantList &lista)
{
    beginResetModel();
    lista_ = lista;    
    endResetModel();

    int portaalissa = 0;
    for(const auto& item : qAsConst(lista_)) {
        const QVariantMap &map = item.toMap();
            if( map.value("portaalissa").toBool())
                portaalissa++;
    }
    emit kiertojaPortaalissa(portaalissa > 0);
}

void KiertoModel::paivita()
{
    KpKysely *kysely = kpk("/kierrot");
    connect(kysely, &KpKysely::vastaus, this, &KiertoModel::lataaData);
    kysely->kysy();
}
