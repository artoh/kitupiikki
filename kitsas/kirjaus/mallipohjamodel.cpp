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
#include "mallipohjamodel.h"

#include "db/kirjanpito.h"
#include "db/tositetyyppimodel.h"
#include "db/yhteysmodel.h"

MallipohjaModel::MallipohjaModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(kp(), &Kirjanpito::tietokantaVaihtui, this, &MallipohjaModel::haeLista);
    connect(kp(), &Kirjanpito::kirjanpitoaMuokattu, this, &MallipohjaModel::haeLista);
}

int MallipohjaModel::rowCount(const QModelIndex &/*parent*/) const
{
    return lista_.count();
}

QVariant MallipohjaModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    QVariantMap map = lista_.value(index.row()).toMap();

    if(role == Qt::DisplayRole)
        return map.value("otsikko");
    if(role == Qt::DecorationRole)
        return kp()->tositeTyypit()->kuvake(map.value("tyyppi").toInt());
    if(role == Qt::UserRole)
        return map.value("id");

    // FIXME: Implement me!
    return QVariant();
}

MallipohjaModel *MallipohjaModel::instanssi()
{
    if( !instanssi__) {
        instanssi__ = new MallipohjaModel();
        instanssi__->haeLista();
    }
    return instanssi__;
}

void MallipohjaModel::haeLista()
{
    if( !kp()->yhteysModel() || !kp()->yhteysModel()->onkoOikeutta(YhteysModel::TOSITE_SELAUS) )
        return;

    KpKysely* kysely = kpk("/tositteet");
    if(kysely) {
        kysely->lisaaAttribuutti("malli");
        connect(kysely, &KpKysely::vastaus, this, &MallipohjaModel::listaSaapuu);
        kysely->kysy();
    }
}

void MallipohjaModel::listaSaapuu(QVariant *data)
{
    beginResetModel();
    lista_ = data->toList();
    endResetModel();
}

MallipohjaModel* MallipohjaModel::instanssi__ = nullptr;
