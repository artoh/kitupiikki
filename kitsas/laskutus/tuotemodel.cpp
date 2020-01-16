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
    
    const QVariantMap &map = lista_.at(index.row()).toMap();
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
    else if( role == TuoteMapRooli) {
        QVariantMap tmap(map);
        tmap.insert("tuote", tmap.take("id"));
        return tmap;
    }
    return QVariant();
    
}


void TuoteModel::lataa()
{
    KpKysely *kys = kpk("/tuotteet");
    if( kys ) {
        connect( kys, &KpKysely::vastaus, this, &TuoteModel::dataSaapuu);
        kys->kysy();
    }
}

void TuoteModel::paivitaTuote(QVariantMap map)
{
    KpKysely *kysely = map.contains("id") ?
                kpk(QString("/tuotteet/%1").arg(map.value("id").toInt()), KpKysely::PUT) :
                kpk("/tuotteet", KpKysely::POST);
    connect( kysely, &KpKysely::vastaus, this, &TuoteModel::muokattu);
    kysely->kysy(map);
}

void TuoteModel::poistaTuote(int id)
{
    KpKysely *kysely = kpk(QString("/tuotteet/%1").arg(id), KpKysely::DELETE);
    kysely->kysy();

    for(int i=0; i<lista_.count(); i++) {
        if( lista_.at(i).toMap().value("id").toInt() == id) {
            beginRemoveRows(QModelIndex(),i,i);
            lista_.removeAt(i);
            endRemoveRows();
            return;
        }
    }
}


void TuoteModel::dataSaapuu(QVariant *data)
{
    beginResetModel();
    lista_ = data->toList();
    endResetModel();
}

void TuoteModel::muokattu(QVariant *data)
{
    QVariantMap map = data->toMap();
    int id = map.value("id").toInt();
    for(int i=0; i<lista_.count(); i++) {
        if( lista_.at(i).toMap().value("id").toInt() == id) {
            lista_[i] = map;
            emit dataChanged( index(i,0), index(i,BRUTTO) );
            return;
        }
    }
    // Ei löytynyt, lisätään
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    lista_.append(map);
    endInsertRows();
}
