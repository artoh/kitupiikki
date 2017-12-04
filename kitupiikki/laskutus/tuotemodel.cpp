/*
   Copyright (C) 2017 Arto Hyvättinen

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

#include "tuotemodel.h"

TuoteModel::TuoteModel(QObject *parent) :
    QAbstractTableModel(parent)
{
    
}

int TuoteModel::rowCount(const QModelIndex & /* parent */) const
{
    return tuotteet_.count();    
}

int TuoteModel::columnCount(const QModelIndex & /* parent */) const
{
    return 2;
}

QVariant TuoteModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignCenter);
    else if( role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        if( section == NIMIKE )
            return tr("Nimike");
        else if(section == HINTA)
            return tr("Hinta");
    }
    return QVariant();
}

QVariant TuoteModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return QVariant();
    
    LaskuRivi rivi = tuotteet_.value(index.row());
    
    if( role == Qt::DisplayRole)
    {
        if( index.column() == NIMIKE )
            return rivi.nimike;
        else if(index.column() == HINTA)
            return QString("%L1 €").arg(rivi.ahintaSnt / 100.0,0,'f',2);
    }
    return QVariant();
}

void TuoteModel::lisaaTuote(LaskuRivi tuote)
{
    beginInsertRows(QModelIndex(), tuotteet_.count(), tuotteet_.count() );
    tuotteet_.append( tuote );
    endInsertRows();
}

void TuoteModel::poistaTuote(int indeksi)
{
    beginRemoveRows(QModelIndex(), indeksi, indeksi);
    tuotteet_.removeAt(indeksi);
    endRemoveRows();
}

LaskuRivi TuoteModel::tuote(int indeksi) const
{
    return tuotteet_.value(indeksi);
}
