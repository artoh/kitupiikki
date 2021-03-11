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
#include "huoneistomodel.h"
#include "../viitenumero.h"

HuoneistoModel::HuoneistoModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    QVariantMap huoneisto1;
    huoneisto1.insert("id",1);
    huoneisto1.insert("asukas",1);
    huoneisto1.insert("nimi","Huoneisto A");
    huoneistot_.append(huoneisto1);

    QVariantMap huoneisto2;
    huoneisto2.insert("id",2);
    huoneisto2.insert("asukas",2);
    huoneisto2.insert("nimi","Huoneisto B");
    huoneistot_.append(huoneisto2);
}

QVariant HuoneistoModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case NIMI: return tr("Nimi");
        case VIITE: return tr("Viite");
        case ASUKAS: return tr("Asukas");
        }
    }
    return QVariant();
}

int HuoneistoModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 2;
    // FIXME: Implement me!
}

int HuoneistoModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 3;
    // FIXME: Implement me!
}

QVariant HuoneistoModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const QVariantMap& map = huoneistot_.at(index.row()).toMap();

    if( role == Qt::DisplayRole) {
        switch (index.column()) {
        case NIMI: return map.value("nimi");
        case VIITE: {
                ViiteNumero viite(ViiteNumero::KOHDE, map.value("id").toInt());
                return viite.valeilla();
            }
        case ASUKAS:
            return QVariant();
        }
    }

    if( role == ViiteRooli) {
        ViiteNumero viite(ViiteNumero::KOHDE, map.value("id").toInt());
        return viite.viite();
    }

    // FIXME: Implement me!
    return QVariant();
}
