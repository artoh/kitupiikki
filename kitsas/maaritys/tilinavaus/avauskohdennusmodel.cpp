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
#include "avauskohdennusmodel.h"

AvausKohdennusModel::AvausKohdennusModel(QObject *parent)
    : AvausEraKantaModel(parent)
{
    kohdennukset_ = kp()->kohdennukset()->vainKohdennukset( kp()->asetukset()->pvm("TilinavausPvm") );
}

QVariant AvausKohdennusModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        if(section == KOHDENNUS)
            return tr("Kohdennus");
        else
            return tr("Saldo");
    }

    return QVariant();
}

int AvausKohdennusModel::rowCount(const QModelIndex &parent) const
{
    if( parent.isValid() )
        return 0;

    return kohdennukset_.count();
}

int AvausKohdennusModel::columnCount(const QModelIndex &parent) const
{
    if( parent.isValid() )
        return 0;

    return 2;
}

QVariant AvausKohdennusModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if( role == Qt::DisplayRole || role==Qt::EditRole) {
        if( index.column() == KOHDENNUS)
            return kohdennukset_.at(index.row()).nimi();
        else {
            int kohdid = kohdennukset_.at(index.row()).id();
            Euro saldo = Euro::Zero;
            for( const auto& era : qAsConst( erat_ )) {
                if( era.kohdennus() == kohdid) {
                    saldo = era.saldo();
                    break;
                }
            }
            if( role == Qt::DisplayRole)
                return saldo.display();
            else
                return saldo.toString();
        }
    }

    return QVariant();
}

bool AvausKohdennusModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if( role == Qt::EditRole && index.column() == SALDO) {
        int kohid = kohdennukset_.at(index.row()).id();
        for(int i=0; i < erat_.count(); i++) {
            if( erat_.at(i).kohdennus() == kohid) {
                erat_.removeAt(i);
                break;
            }
        }
        // Sitten lisätään jos saldoa
        if( qAbs( value.toDouble() ) > 1e-5) {
            AvausEra era( qRound64( value.toDouble() * 100 ), QDate(), QString(), kohid);
            erat_.append(era);
        }
        emit dataChanged(index, index, QVector<int>() << role);
        return true;
    }
    return false;
}

Qt::ItemFlags AvausKohdennusModel::flags(const QModelIndex &index) const
{
    if (!index.isValid() || index.column() == KOHDENNUS)
        return Qt::ItemIsEnabled;

    return Qt::ItemIsEditable | Qt::ItemIsEnabled; // FIXME: Implement me!
}
