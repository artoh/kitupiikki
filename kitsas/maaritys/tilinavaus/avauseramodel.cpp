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

AvausEraModel::AvausEraModel(QObject *parent, int poistoaika)
    : AvausEraKantaModel(parent),
      poistoAika_{poistoaika}
{

}

QVariant AvausEraModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        if(section == KUMPPANI)
            return tr("Asiakas/toimittaja");
        else if(section == NIMI)
            return tr("Erän nimi");
        else if(section == SALDO)
            return tr("Saldo");
        else if(section == POISTOAIKA)
            return tr("Poistoaika");
    }

    return QVariant();
}

int AvausEraModel::rowCount(const QModelIndex &parent) const
{
    if( parent.isValid() )
        return 0;

    return erat_.count();
}

int AvausEraModel::columnCount(const QModelIndex &parent) const
{
    if( parent.isValid() )
        return 0;
    return poistoAika_ ? 4 : 3;
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
        else if( role == Qt::DisplayRole) {
            if( index.column() == SALDO)
                return era.saldo().display();
            else if(index.column() == POISTOAIKA)
                return tr("%1 vuotta").arg(era.tasapoisto() / 12);
        } else {
            if( index.column() == SALDO)
                return era.saldo().toString();
            else if(index.column() == POISTOAIKA)
                return era.tasapoisto() / 12;
        }
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
        }
        else if( index.column() == KUMPPANI) {
            era.asetaKumppani( value.toMap());
        } else if( index.column() == POISTOAIKA) {
            era.asetaTasapoisto( value.toInt() * 12 );
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
    erat_.append(AvausEra(Euro::Zero, QDate(), QString(), 0, 0, 0, QString(), poistoAika_));
    endInsertRows();
}

void AvausEraModel::lataa(QList<AvausEra> erat)
{
    AvausEraKantaModel::lataa(erat);
    lisaaRivi();
}

