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
#include "eranvalintamodel.h"
#include "db/kirjanpito.h"

EranValintaModel::EranValintaModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant EranValintaModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case PVM: return tr("Pvm");
        case KUMPPANI: return tr("Asiakas/Toimittaja");
        case SELITE: return tr("Selite");
        case SALDO: return tr("Avoin saldo");
        }
    }
    return QVariant();
}

int EranValintaModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return erat_.count();
}

int EranValintaModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 4;
}

QVariant EranValintaModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const QVariantMap map = erat_.at(index.row()).toMap();

    if( role == Qt::DisplayRole) {
        const QString& selite = map.value("selite").toString();
        const QString& kumppani = map.value("kumppani").toMap().value("nimi").toString();
        switch (index.column()) {
            case PVM: return map.value("pvm").toDate();
            case KUMPPANI: return kumppani;
            case SELITE: return selite != kumppani ? selite : QString() ;
            case SALDO: return Euro::fromVariant(map.value("avoin")).display(false);
        }
    } else if( role == Qt::TextAlignmentRole && index.column() == SALDO) {
        return Qt::AlignRight;
    } else if( role == IdRooli) {
        return map.value("id").toInt();
    } else if( role == MapRooli) {
        return map;
    } else if( role == PvmRooli) {
        return map.value("pvm").toDate();
    } else if( role == TekstiRooli) {
        return map.value("selite").toString() +
               map.value("kumppani").toMap().value("nimi").toString() ;
    }

    // FIXME: Implement me!
    return QVariant();
}

void EranValintaModel::lataa(int tili, int asiakas)
{
    tili_ = tili;
    asiakas_ = asiakas;
    paivita(true);
}

void EranValintaModel::paivita(bool avoimet)
{
    KpKysely *kysely = kpk("/erat");
    kysely->lisaaAttribuutti("tili", tili_);
    if(asiakas_)
        kysely->lisaaAttribuutti("asiakas", asiakas_);
    if( !avoimet)
        kysely->lisaaAttribuutti("kaikki");
    connect(kysely, &KpKysely::vastaus, this, &EranValintaModel::eratSaapuu);
    kysely->kysy();
}

void EranValintaModel::eratSaapuu(const QVariant *data)
{
    beginResetModel();
    erat_ = data->toList();
    endResetModel();
}
