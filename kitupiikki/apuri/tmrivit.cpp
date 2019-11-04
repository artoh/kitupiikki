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
#include "model/tositevienti.h"


TmRivit::TmRivit(QObject *parent)
    : QAbstractTableModel(parent)
{

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
            Tili* tilini = kp()->tilit()->tili( rivit_.at(index.row()).tilinumero() );
            if( tilini )
                return  tilini->nimi() ;
        } else if( index.column() == EUROA)
        {            
            qlonglong sentit =
                rivit_.at( index.row() ).naytaBrutto() ?
                    rivit_.at( index.row() ).brutto() :
                    rivit_.at( index.row() ).netto() ;

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
    else if( role == Qt::DecorationRole && index.column() == EUROA) {
        return kp()->alvTyypit()->kuvakeKoodilla( rivit_.at(index.row()).alvkoodi() );
    }
    return QVariant();
}

void TmRivit::lisaa(const QVariantMap &map)
{
    TositeVienti vienti(map);

    if( vienti.tyyppi() % 100 == TositeVienti::KIRJAUS)
        rivit_.append( TulomenoRivi( vienti) );
    else if( vienti.tyyppi() % 100 == TositeVienti::OSTO + TositeVienti::ALVKIRJAUS) {
        rivit_[ rivit_.count() - 1 ].setAlvvahennys(true);
    }
    else if( vienti.tyyppi() == TositeVienti::OSTO + TositeVienti::MAAHANTUONTIVASTAKIRJAUS)
        rivit_[ rivit_.count() - 1 ].setAlvkoodi( AlvKoodi::MAAHANTUONTI_VERO );

}

int TmRivit::lisaaRivi()
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    rivit_.append(TulomenoRivi());
    endInsertRows();
    return rowCount() - 1;
}

void TmRivit::poistaRivi(int rivi)
{
    beginRemoveRows(QModelIndex(), rivi, rivi);
    rivit_.removeAt(rivi);
    endRemoveRows();
}

TulomenoRivi *TmRivit::rivi(int indeksi)
{
    return &rivit_[indeksi];
}

void TmRivit::clear()
{
    beginResetModel();
    rivit_.clear();
    endResetModel();
}

QVariantList TmRivit::viennit(Tosite* tosite)
{
    QVariantList lista;

    for(int i=0; i < rowCount(); i++)
        lista.append( rivit_.at(i).viennit(tosite) );

    return lista;
}

