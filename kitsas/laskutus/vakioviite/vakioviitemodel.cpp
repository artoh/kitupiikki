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
#include "vakioviitemodel.h"

#include "db/kirjanpito.h"

VakioViiteModel::VakioViiteModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant VakioViiteModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case VIITE: return tr("Viite");
        case TILI: return tr("Tili");
        case KOHDENNUS: return tr("Kohdennus");
        case OTSIKKO: return tr("Otsikko");
        }
    }
    return QVariant();
}

int VakioViiteModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return lista_.count();
}

int VakioViiteModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 4;
}

QVariant VakioViiteModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    QVariantMap map = lista_.value(index.row()).toMap();

    if( role == Qt::DisplayRole) {
        switch (index.column()) {
        case VIITE: return map.value("viite");
        case TILI: {
                Tili* tili = kp()->tilit()->tili(map.value("tili").toInt());
                if(tili)
                    return tili->nimiNumero();
                return QString();
            }
        case KOHDENNUS:
            return kp()->kohdennukset()->kohdennus(map.value("kohdennus").toInt()).nimi();
        case OTSIKKO:
            return map.value("otsikko");
        }
    } else if( role == MapRooli) {
        return map;
    } else if( role == ViiteRooli) {
        return map.value("viite");
    }

    return QVariant();
}

void VakioViiteModel::lataa()
{
    KpKysely* kysely = kpk("/vakioviitteet");
    if( kysely) {
       connect(kysely, &KpKysely::vastaus, this, &VakioViiteModel::dataSaapuu);
       kysely->kysy();
    }

}

void VakioViiteModel::dataSaapuu(QVariant *data)
{
    beginResetModel();
    lista_ = data->toList();
    endResetModel();
}
