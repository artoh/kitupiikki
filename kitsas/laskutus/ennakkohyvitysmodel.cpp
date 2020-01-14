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
#include "ennakkohyvitysmodel.h"
#include "db/kirjanpito.h"
#include <QDate>


EnnakkoHyvitysModel::EnnakkoHyvitysModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant EnnakkoHyvitysModel::headerData(int section, Qt::Orientation orientation, int role) const
{    
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case PVM: return tr("Päivämäärä");
            case SELITE: return tr("Otsikko");
            case AVOINNA: return tr("Euroa");
        }
    }
    return QVariant();
}

int EnnakkoHyvitysModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return lista_.count();
}

int EnnakkoHyvitysModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 3;
}

QVariant EnnakkoHyvitysModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    QVariantMap map = lista_.value(index.row()).toMap();
    if( role == Qt::DisplayRole) {
        switch (index.column()) {
            case PVM: return map.value("pvm").toDate().toString("dd.MM.yyyy");
            case SELITE: return map.value("selite");
            case AVOINNA: return QString("%L1").arg(map.value("avoin").toDouble(),0,'f',2);
        }
    } else if( role == EraIdRooli)
        return map.value("id");
    else if( role == PvmRooli)
        return map.value("pvm");
    else if( role == EuroRooli)
        return map.value("avoin").toDouble();

    return QVariant();
}

void EnnakkoHyvitysModel::lataaErat(int asiakasId)
{
    KpKysely *kysely = kpk("/erat");
    kysely->lisaaAttribuutti("tili", kp()->asetukset()->luku("LaskuEnnakkotili"));
    kysely->lisaaAttribuutti("asiakas", asiakasId);
    connect(kysely, &KpKysely::vastaus, this, &EnnakkoHyvitysModel::eratSaapuu);
    kysely->kysy();
}

void EnnakkoHyvitysModel::eratSaapuu(const QVariant *data)
{
    beginResetModel();
    lista_ = data->toList();
    endResetModel();
}
