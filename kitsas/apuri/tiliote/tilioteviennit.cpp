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
#include "tilioteviennit.h"
#include "db/kitsasinterface.h"
#include "db/tilimodel.h"

TilioteViennit::TilioteViennit(KitsasInterface* interface, QObject *parent)
    : QAbstractTableModel(parent), kitsasInterface_(interface)
{
}

QVariant TilioteViennit::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case TILI:
            return tr("Tili");
        case EURO:
            return "€";
        }
    }
    return QVariant();
}

int TilioteViennit::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return viennit_.count();
}

int TilioteViennit::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 2;
}

QVariant TilioteViennit::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const TositeVienti& rivi = vienti(index.row());

    if( role == Qt::DisplayRole) {
        if( index.column() == TILI ) {
            Tili* tili = kitsasInterface_->tilit()->tili( rivi.tili() );
            if( tili )
                return tili->nimiNumero();
            return QVariant();
        } else if( index.column() == EURO) {
            double summa = rivi.kredit() - rivi.debet();
            return qAbs(summa) > 1e-5 ? QString("%L1 €").arg( summa,0,'f',2) : QVariant();
        }
    }
    return QVariant();
}

void TilioteViennit::tyhjenna()
{
    beginResetModel();
    viennit_.clear();
    endResetModel();
}

void TilioteViennit::lisaaVienti(const TositeVienti &vienti)
{
    beginInsertRows(QModelIndex(), viennit_.count(), viennit_.count());
    viennit_.append(vienti);
    endInsertRows();
}

void TilioteViennit::poistaVienti(int indeksi)
{
    beginRemoveRows(QModelIndex(), indeksi, indeksi);
    viennit_.removeAt(indeksi);
    endRemoveRows();
}

void TilioteViennit::asetaVienti(int indeksi, const TositeVienti &vienti)
{
    viennit_[indeksi] = vienti;
    emit dataChanged( index(indeksi, TILI),
                      index(indeksi, EURO),
                      QVector<int>() << Qt::DisplayRole);
}

TositeVienti TilioteViennit::vienti(int indeksi) const
{
    return viennit_.value(indeksi);
}

QList<TositeVienti> TilioteViennit::viennit() const
{
    return viennit_;
}

qlonglong TilioteViennit::summa() const
{
    qlonglong sentit = 0l;
    for(const auto& vienti : viennit_) {
        sentit += qRound64( vienti.kredit() * 100.0 );
        sentit -= qRound64( vienti.debet() * 100.0);
    }
    return sentit;
}
