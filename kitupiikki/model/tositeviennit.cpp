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
#include "tositeviennit.h"

#include <QDate>
#include "db/tili.h"
#include "db/verotyyppimodel.h"
#include "db/kirjanpito.h"

#include <QDebug>

TositeViennit::TositeViennit(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant TositeViennit::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignCenter | Qt::AlignVCenter);
    else if( role != Qt::DisplayRole )
        return QVariant();
    else if( orientation == Qt::Horizontal)
    {
        switch (section)
        {
            case PVM:
                return tr("Pvm");
            case TILI:
                return tr("Tili");
            case DEBET :
                return tr("Debet");
            case KREDIT:
                return tr("Kredit");
            case KOHDENNUS :
                return tr("Kohdennus");
            case ALV:
                return tr("Alv");
            case SELITE:
                return tr("Selite");
        }
    }
    return QVariant( section + 1);
}


int TositeViennit::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return viennit_.count();
}

int TositeViennit::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 7;
}

QVariant TositeViennit::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    QVariantMap vienti = viennit_.at(index.row()).toMap();

    if( role == Qt::DisplayRole )
    {
        switch ( index.column()) {
        case PVM:
            return vienti.value("pvm").toDate();
        case TILI:
        {
            Tili *tili = kp()->tilit()->tiliNumerolla( vienti.value("tili").toInt() );
            if( tili )
                return QString("%1 %2").arg(tili->numero()).arg(tili->nimi());
            return QVariant();
        }
        case DEBET:
        {
            double debet = vienti.value("debet").toDouble();
             if( debet > 1e-5 )
                return QVariant( QString("%L1 €").arg(debet,0,'f',2));
             return QVariant();
        }
        case KREDIT:
        {
            double kredit = vienti.value("kredit").toDouble();
            if( kredit > 1e-5)
                return QVariant( QString("%L1 €").arg(kredit,0,'f',2));
             return QVariant();
        }
        case ALV:
        {
            int alvkoodi = vienti.value("alvkoodi").toInt();
            if( alvkoodi == AlvKoodi::EIALV )
                return QVariant();
            else
            {
                if( alvkoodi == AlvKoodi::MAKSETTAVAALV)
                    return tr("VERO");
                else if(alvkoodi == AlvKoodi::TILITYS)
                    return QString();
                else
                    return QVariant( QString("%1 %").arg( vienti.value("alvprosentti").toInt() ));
            }
        }
        case SELITE:
            return vienti.value("selite");
        case KOHDENNUS:
            return QVariant();

        }

    }
    else if( role == Qt::EditRole )
    {
        switch ( index.column())
        {
        case PVM:
            return vienti.value("pvm").toDate();
        case TILI:
            return vienti.value("tili").toInt();
        case DEBET:
            return vienti.value("debetsnt").toLongLong() / 100.0 ;
        case KREDIT:
            return vienti.value("kreditsnt").toLongLong() / 100.0;
        }
    }
    else if( role == Qt::TextAlignmentRole)
    {
        if( index.column()==KREDIT || index.column() == DEBET || index.column() == ALV)
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        else
            return QVariant( Qt::AlignLeft | Qt::AlignVCenter);

    }

    return QVariant();
}

bool TositeViennit::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (data(index, role) != value) {
        // FIXME: Implement me!
        emit dataChanged(index, index, QVector<int>() << role);
        return true;
    }
    return false;
}

Qt::ItemFlags TositeViennit::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::NoItemFlags;
    // Tämä vain jos voi muokata suoraan ;)
    return Qt::ItemIsEditable; // FIXME: Implement me!
}

bool TositeViennit::insertRows(int row, int count, const QModelIndex &parent)
{
    beginInsertRows(parent, row, row + count - 1);
    // FIXME: Implement me!
    endInsertRows();    
}


bool TositeViennit::removeRows(int row, int count, const QModelIndex &parent)
{
    beginRemoveRows(parent, row, row + count - 1);
    // FIXME: Implement me!
    endRemoveRows();
}


void TositeViennit::asetaViennit(QVariantList viennit)
{
    beginResetModel();
    viennit_ = viennit;
    endResetModel();

    qDebug() << viennit_;
}
