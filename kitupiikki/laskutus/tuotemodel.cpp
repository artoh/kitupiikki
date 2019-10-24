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
#include "db/kirjanpito.h"

#include <QSqlQuery>
#include <QDebug>
#include <QSqlError>

TuoteModel::TuoteModel(QObject *parent) :
    QAbstractTableModel(parent)
{

}

int TuoteModel::rowCount(const QModelIndex & /* parent */) const
{
    return lista_.count();
}

int TuoteModel::columnCount(const QModelIndex & /* parent */) const
{
    return 3;
}

QVariant TuoteModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignCenter);
    else if( role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        if( section == NIMIKE )
            return tr("Nimike");
        else if( section == NETTO)
            return tr("Nettohinta");
        else if(section == BRUTTO)
            return tr("Bruttohinta");
    }        
    return QVariant();
}

QVariant TuoteModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return QVariant();
    
    QVariantMap map = lista_.at(index.row()).toMap();
    if( role == Qt::DisplayRole)
    {
        if( index.column() == NIMIKE )
            return map.value("nimike");
        else if( index.column() == NETTO)
        {
            double netto = map.value("ahinta").toDouble();
            return QString("%L1 €").arg( netto ,0,'f',2);
        }
        else if(index.column() == BRUTTO)
        {
            double netto = map.value("ahinta").toDouble();

            double alvprossa = map.value("alvkoodi").toInt() == AlvKoodi::MYYNNIT_NETTO ?
                    map.value("alvprosentti").toDouble() : 0.0;

            double brutto = netto * (100 + alvprossa) / 100.0;

            return QString("%L1 €").arg( brutto ,0,'f',2);
        }
    }
    else if( role == IdRooli)
        return map.value("id");
    else if( role == MapRooli)
        return map;
    return QVariant();
    
}


QVariantMap TuoteModel::tuoteMap(int indeksi) const
{
    QVariantMap tuote = lista_.at(indeksi).toMap();
    tuote.insert("tuote", tuote.value("id"));
    tuote.remove("id");
    return tuote;

}

void TuoteModel::lataa()
{
    KpKysely *kys = kpk("/tuotteet");
    connect( kys, &KpKysely::vastaus, this, &TuoteModel::dataSaapuu);
    kys->kysy();

    return;

}

void TuoteModel::dataSaapuu(QVariant *data)
{
    beginResetModel();
    lista_ = data->toList();
    endResetModel();
}
