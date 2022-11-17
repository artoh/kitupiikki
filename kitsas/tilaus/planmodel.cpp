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
    pilvia_ = kp()->pilvi()->kayttaja().cloudCount();
}

QVariant PlanModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {

        switch (section) {
            case NIMI: return tr("Paketti");
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

    return 2;
}

QVariant PlanModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    QVariantMap map = plans_.at(index.row()).toMap();

    if( role == Qt::DisplayRole) {

        switch (index.column()) {
        case NIMI: return map.value("planname").toString() + "\n" +
                    map.value("info").toString();
        case HINTA:
            if( puolivuosittain_ )
                return tr("%L1 € / 6 kk")
                        .arg( map.value("annually").toDouble() / 2,0,'f',2 );
            else
                return tr("%L1 € / vuosi")
                        .arg( map.value("annually").toDouble(),0,'f',2 );
        }
    }

    else if( role == PlanRooli)
        return map.value("planid");
    else if( role == HintaRooli)
        return puolivuosittain_ ? map.value("annually").toDouble() / 2.0 : map.value("annually");
    else if( role == NimiRooli)
        return map.value("planname");
    else if( role == PilviaRooli)
        return map.value("clouds");
    else if( role == LisaPilviHinta)
        return puolivuosittain_ ? map.value("extraprice").toDouble() / 2.0 : map.value("extraprice");
    else if( role == InfoRooli)
        return map.value("info");
    else if( role == LisaPilviKkHinta)
        return map.value("extramonthly");



    return QVariant();
}

Qt::ItemFlags PlanModel::flags(const QModelIndex &/*index*/) const
{
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

void PlanModel::alusta(const QVariantList &plans, bool puolivuosittain)
{
    beginResetModel();
    plans_ = plans;
    puolivuosittain_ =puolivuosittain;
    endResetModel();
}

void PlanModel::naytaPuolivuosittain(bool puolivuosittain)
{
    puolivuosittain_ = puolivuosittain;
    emit dataChanged( index(0, HINTA),
                      index(rowCount()-1, HINTA));
}
