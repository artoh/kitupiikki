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
#include "avauseramodel.h"

AvausEraModel::AvausEraModel(QList<AvausEra> erat, QObject *parent)
    : AvausEraKantaModel(erat, parent)
{
    if( erat_.isEmpty()) {
        erat_.append(AvausEra());
    }
}

QVariant AvausEraModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        if(section == KUMPPANI)
            return tr("Asiakas/toimittaja");
        else if(section == NIMI)
            return tr("Erän nimi");
        else
            return tr("Saldo");
    }

    return QVariant();
}

int AvausEraModel::rowCount(const QModelIndex &parent) const
{
    if( parent.isValid() )
        return 0;

    return erat_.count();
}


QVariant AvausEraModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    AvausEra era = erat_.at(index.row());

    if( role == Qt::UserRole && index.column() == KUMPPANI)
        return era.kumppaniId();

    if( role == Qt::DisplayRole || role == Qt::EditRole) {
        if( index.column() == NIMI)
            return era.eranimi();
        else if( index.column() == KUMPPANI)
                return era.kumppaniNimi();
        else if( role == Qt::DisplayRole)
            return QString("%L1 €").arg( era.saldo() / 100.0, 10,'f',2);
        else
            return era.saldo() / 100.0;
    }


    return QVariant();
}

bool AvausEraModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (data(index, role) != value) {
        AvausEra era = erat_.at(index.row());

        if( index.column() == NIMI) {
            era.asetaNimi( value.toString() );
        } else if (index.column() == SALDO){
            era.asetaSaldo( qRound64( value.toDouble() * 100) );
        } else if( role == Qt::DisplayRole) {
            era.asetaKumppani( value.toString());
        } else if( role == Qt::UserRole) {
            era.asetaKumppani( value.toInt());
        }


        erat_[index.row()] = era;
        emit dataChanged(index, index, QVector<int>() << role);

        if( index.column() == SALDO && index.row() == rowCount() - 1) {

        }

        return true;
    }
    return false;
}

Qt::ItemFlags AvausEraModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;    

    return Qt::ItemIsEditable | Qt::ItemIsEnabled; // FIXME: Implement me!
}

void AvausEraModel::lisaaRivi()
{
    beginInsertRows(QModelIndex(),rowCount()-1, rowCount()-1);
    erat_.append(AvausEra());
    endInsertRows();
}

