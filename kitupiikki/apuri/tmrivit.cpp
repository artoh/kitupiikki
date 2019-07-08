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
#include "tmrivit.h"

#include "db/kirjanpito.h"

TmRivit::TmRivit(QObject *parent)
    : QAbstractTableModel(parent)
{
    lisaaRivi();    // Aloitetaan yhdellä tyhjällä rivillä
}

QVariant TmRivit::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignCenter | Qt::AlignVCenter);
    else if( role != Qt::DisplayRole )
        return QVariant();
    else if( orientation == Qt::Horizontal)
    {
        switch (section)
        {

            case TILI:
                return tr("Tili");
            case EUROA:
                return tr("€");
        }
    }
    return QVariant();
}

int TmRivit::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return rivit_.count();
}

int TmRivit::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 2;
}

QVariant TmRivit::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if( role == Qt::DisplayRole) {
        if( index.column() == TILI) {
            if( rivit_.at( index.row() ).tili.onkoValidi() )
                return  rivit_.at(index.row()).tili.nimi() ;
        } else if( index.column() == EUROA)
        {
            qlonglong sentit = rivit_.at( index.row() ).maara;
            if( sentit > 1e-5 )
               return QVariant( QString("%L1 €").arg(sentit / 100.0,0,'f',2));
        }
    }
    else if( role == Qt::TextAlignmentRole)
    {
        if( index.column()==EUROA)
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        else
            return QVariant( Qt::AlignLeft | Qt::AlignVCenter);

    }
    return QVariant();
}

void TmRivit::setTili(int rivi, Tili tili)
{
    rivit_[rivi].tili = tili;
    emit dataChanged(index(rivi,TILI),index(rivi,TILI));
}

Tili TmRivit::tili(int rivi) const
{
    return rivit_.at(rivi).tili;
}

void TmRivit::setMaara(int rivi, qlonglong senttia)
{
    rivit_[rivi].maara = senttia;
    emit dataChanged(index(rivi,EUROA),index(rivi,EUROA));
}

qlonglong TmRivit::maara(int rivi) const
{
    return rivit_.at(rivi).maara;
}

int TmRivit::lisaaRivi()
{
    beginInsertRows(QModelIndex(), rivit_.count(), rivit_.count());
    rivit_.append( Rivi() );
    endInsertRows();
    return rivit_.count()-1;
}