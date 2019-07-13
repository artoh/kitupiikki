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
            Tili tili = kp()->tilit()->tiliNumerollaVanha( rivit_.at( index.row() ).tilinumero );
            if( tili.onkoValidi() )
                return  tili.nimi() ;
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

void TmRivit::clear()
{
    beginResetModel();
    rivit_.clear();
    endResetModel();
}

void TmRivit::setTili(int rivi, int tilinumero)
{
    rivit_[rivi].tilinumero = tilinumero;
    emit dataChanged(index(rivi,TILI),index(rivi,TILI));
}

Tili TmRivit::tili(int rivi) const
{
    return kp()->tilit()->tiliNumerollaVanha( rivit_.at(rivi).tilinumero );
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

void TmRivit::setNetto(int rivi, qlonglong senttia)
{
    rivit_[rivi].netto = senttia;
}

qlonglong TmRivit::netto(int rivi) const
{
    return rivit_.at(rivi).netto;
}

void TmRivit::setAlvKoodi(int rivi, int koodi)
{
    rivit_[rivi].verokoodi = koodi;
}

int TmRivit::alvkoodi(int rivi) const
{
    return rivit_.at(rivi).verokoodi;
}

void TmRivit::setAlvProsentti(int rivi, double prosentti)
{
    rivit_[rivi].veroprosentti = prosentti;
}

double TmRivit::alvProsentti(int rivi) const
{
    return rivit_.at(rivi).veroprosentti;
}

void TmRivit::setSelite(int rivi, const QString &selite)
{
    rivit_[rivi].selite = selite;

}

void TmRivit::setEiVahennysta(int rivi, bool eivahennysta)
{
    rivit_[rivi].eivahennysta = eivahennysta;
}

bool TmRivit::eiVahennysta(int rivi) const
{
    return rivit_.at(rivi).eivahennysta;
}

void TmRivit::setKohdennus(int rivi, int kohdennus)
{
    rivit_[rivi].kohdennus = kohdennus;
}

int TmRivit::kohdennus(int rivi) const
{
    return rivit_.at(rivi).kohdennus;
}

void TmRivit::setMerkkaukset(int rivi, QVariantList merkkaukset)
{
    rivit_[rivi].merkkaukset = merkkaukset;
}

QVariantList TmRivit::merkkaukset(int rivi) const
{
    return  rivit_.at(rivi).merkkaukset;
}

void TmRivit::setJaksoalkaa(int rivi, const QDate &pvm)
{
    rivit_[rivi].jaksoalkaa = pvm;
}

QDate TmRivit::jaksoalkaa(int rivi) const
{
    return rivit_.at(rivi).jaksoalkaa;
}

void TmRivit::setJaksoloppuu(int rivi, const QDate &pvm)
{
    rivit_[rivi].jaksoloppuu = pvm;
}

QDate TmRivit::jaksoloppuu(int rivi) const
{
    return rivit_.at(rivi).jaksoloppuu;
}

QString TmRivit::selite(int rivi) const
{
    return rivit_.at(rivi).selite;
}

int TmRivit::lisaaRivi()
{
    beginInsertRows(QModelIndex(), rivit_.count(), rivit_.count());
    rivit_.append( Rivi() );
    endInsertRows();
    return rivit_.count()-1;
}

void TmRivit::poistaRivi(int rivi)
{
    beginInsertRows( QModelIndex(), rivi, rivi);
    rivit_.removeAt(rivi);
    endRemoveRows();
}
