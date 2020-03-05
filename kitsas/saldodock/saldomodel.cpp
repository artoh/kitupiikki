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
#include "saldomodel.h"

#include "db/kirjanpito.h"

SaldoModel::SaldoModel(QObject *parent)
    : QAbstractTableModel(parent)
{    
}

QVariant SaldoModel::headerData(int /*section*/, Qt::Orientation /*orientation*/, int /*role*/) const
{
    return QVariant();
}

int SaldoModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return data_.count();
}

int SaldoModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 3;
}

QVariant SaldoModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    else if( role == Qt::DisplayRole) {
        switch (index.column()) {
        case NUMERO:
            return data_.at(index.row()).first;
        case NIMI:
            return kp()->tilit()->tiliNumerolla( data_.at(index.row()).first ).nimi();
        case SALDO:
            double saldo = data_.at(index.row()).second;
            return QString("%L1 €").arg(saldo,0,'f',2);
        }
    }

    else if( role == Qt::TextAlignmentRole && index.column() == SALDO)
        return Qt::AlignRight;

    else if( role == SuosioRooli) {
        return kp()->tilit()->tiliNumerolla( data_.at(index.row()).first ).tila();
    }

    else if( role == TyyppiRooli) {
        return kp()->tilit()->tiliNumerolla( data_.at(index.row()).first ).tyyppiKoodi();
    }

    return QVariant();
}

void SaldoModel::paivitaSaldot()
{
    KpKysely *kysely = kpk("/saldot");
    Tilikausi tk = kp()->tilikaudet()->tilikausiIndeksilla( kp()->tilikaudet()->rowCount()-1 );
    kysely->lisaaAttribuutti("pvm", tk.paattyy().toString(Qt::ISODate));
    kysely->lisaaAttribuutti("alkupvm", tk.alkaa().toString(Qt::ISODate));
    connect( kysely, &KpKysely::vastaus, this, &SaldoModel::saldotSaapuu);
    kysely->kysy();
}

void SaldoModel::saldotSaapuu(QVariant *data)
{
    beginResetModel();
    data_.clear();
    QMapIterator<QString,QVariant> iter(data->toMap());
    while( iter.hasNext()) {
        iter.next();
        data_.append( qMakePair(iter.key().toInt(), iter.value().toDouble()) );
    }
    endResetModel();
}
