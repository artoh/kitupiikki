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
#include "planmodel.h"

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"

PlanModel::PlanModel(QObject *parent)
    : QAbstractTableModel(parent)

{
    pilvia_ = kp()->pilvi()->omatPilvet();
}

QVariant PlanModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {

        switch (section) {
            case NIMI: return tr("Paketti");
        case PILVIA: return tr("Pilvikirjanpitoja");
        case HINTA:
            return tr("Hinta");
        }
    }
    return QVariant();
}

int PlanModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return plans_.count();
}

int PlanModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 3;
}

QVariant PlanModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    QVariantMap map = plans_.at(index.row()).toMap();

    if( role == Qt::DisplayRole) {

        switch (index.column()) {
        case NIMI: return map.value("planname");
        case PILVIA: return map.value("clouds");
        case HINTA:
            if( kuukausittain_ )
                return tr("%1 € / kk")
                        .arg( map.value("monthly").toDouble(),0,'f',2 );
            else
                return tr("%1 € / vuosi")
                        .arg( map.value("annually").toDouble(),0,'f',2 );
        }
    }

    else if( role == PlanRooli)
        return map.value("planid");
    else if( role == HintaRooli)
        return kuukausittain_ ? map.value("monthly") : map.value("annually");
    else if( role == NimiRooli)
        return map.value("planname");
    else if( role == PilviaRooli)
        return map.value("clouds");



    return QVariant();
}

Qt::ItemFlags PlanModel::flags(const QModelIndex &index) const
{
    QVariantMap map = plans_.at(index.row()).toMap();

    if( map.value("clouds").toInt() < pilvia_ )
        return Qt::NoItemFlags;
    else
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;

}

int PlanModel::rowForPlan(int plan) const
{
    for(int i=0; i < rowCount(); i++) {
        QVariantMap map = plans_.at(i).toMap();
        if( map.value("planid").toInt() == plan)
            return i;
    }
    return -1;
}

void PlanModel::alusta(const QVariantList &plans, bool kuukausittain)
{
    beginResetModel();
    plans_ = plans;
    kuukausittain_ =kuukausittain;
    endResetModel();
}

void PlanModel::naytaKuukausittain(bool kuukausittain)
{
    kuukausittain_ = kuukausittain;
    emit dataChanged( index(0, HINTA),
                      index(rowCount()-1, HINTA));
}
