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
#include "kiertoselausmodel.h"
#include "model/tosite.h"
#include "db/kirjanpito.h"
#include "kiertomodel.h"
#include "db/tositetyyppimodel.h"

KiertoSelausModel::KiertoSelausModel(QObject *parent)
    : QAbstractTableModel(parent)
{

}

QVariant KiertoSelausModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case TILA: return tr("Tila");
        case KIERTO: return tr("Kierto");
        case PVM: return tr("Pvm");
        case ERAPVM: return tr("Eräpäivä");
        case SUMMA: return tr("Summa");
        case KUMPPANI: return tr("Toimittaja");
        case OTSIKKO: return tr("Otsikko");
        }
    }
    return QVariant();
}

int KiertoSelausModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return lista_.count();
}

int KiertoSelausModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 7;
}

QVariant KiertoSelausModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const QVariantMap& map = lista_.at(index.row()).toMap();

    if( role == Qt::EditRole || role == Qt::DisplayRole) {
        switch (index.column()) {
        case TILA: return Tosite::tilateksti(map.value("tila").toInt());
        case KIERTO: return kp()->kierrot()->nimi(map.value("kierto").toInt());
        case PVM: return map.value("pvm").toDate();
        case ERAPVM: return map.value("erapvm").toDate();
        case SUMMA:
            if( role == Qt::EditRole)
                return map.value("summa").toDouble();
            return QString("%L1 €").arg(map.value("summa").toDouble(),0,'f',2);
        case KUMPPANI: return map.value("kumppaninimi");
        case OTSIKKO: return map.value("otsikko");
        }
    } else if( role == Qt::DecorationRole) {
        switch (index.column()) {
        case TILA: return Tosite::tilakuva(map.value("tila").toInt());
        case KIERTO: return kp()->tositeTyypit()->kuvake(map.value("tyyppi").toInt());
        }
    } else if( role == Qt::ForegroundRole && index.column() == ERAPVM) {
        if(  kp()->paivamaara() > map.value("erapvm").toDate())
            return QColor(Qt::red);                
    } else if( role == IdRooli) {
        return map.value("id").toInt();
    }

    return QVariant();
}

void KiertoSelausModel::lataa()
{
    KpKysely *kysely = kpk( kaikki_ ? "/kierrot/kaikki" : "/kierrot/tyolista");
    connect( kysely, &KpKysely::vastaus, this, &KiertoSelausModel::saapuu);
    kysely->kysy();
}

void KiertoSelausModel::naytaKaikki(bool nayta)
{
    kaikki_ = nayta;
    lataa();
}

void KiertoSelausModel::saapuu(QVariant *data)
{
    beginResetModel();
    lista_ = data->toList();
    endResetModel();
}
