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
#include "ryhmatmodel.h"
#include "db/kirjanpito.h"

RyhmatModel::RyhmatModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

QVariant RyhmatModel::headerData(int /*section*/, Qt::Orientation orientation, int role) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return tr("Ryhmät");
    return QVariant();
}

int RyhmatModel::rowCount(const QModelIndex &parent) const
{

    if (parent.isValid())
        return 0;

    return ryhmat_.count()+1;
}

QVariant RyhmatModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if( index.row() == 0) {
        if (role == Qt::DisplayRole || role == Qt::EditRole)
            return tr("Kaikki ryhmät");
        else if( role == IdRooli)
            return 0;
    }

    if (role == Qt::DisplayRole || role == Qt::EditRole)
        return ryhmat_.value(index.row()-1).toMap().value("nimi");
    else if( role == IdRooli)
        return ryhmat_.value(index.row()-1).toMap().value("id");

    return QVariant();
}

void RyhmatModel::paivita()
{
    KpKysely *kysely = kpk("/ryhmat");
    if( kysely ) {
        connect(kysely, &KpKysely::vastaus, this, &RyhmatModel::tietoSaapuu);
        kysely->kysy();
    }
}

void RyhmatModel::tietoSaapuu(QVariant* var)
{
    beginResetModel();
    ryhmat_ = var->toList();
    endResetModel();
}
