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

#include "vientimodel.h"
#include "kirjauswg.h"
#include "db/kirjanpito.h"

VientiModel::VientiModel(Kirjanpito *kp, KirjausWg *kwg) : kirjanpito(kp), kirjauswg(kwg)
{

}

int VientiModel::rowCount(const QModelIndex &parent) const
{
    return viennit.count();
}

int VientiModel::columnCount(const QModelIndex &parent) const
{
    return 7;
}

QVariant VientiModel::headerData(int section, Qt::Orientation orientation, int role) const
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
                return QVariant("Pvm");
            case TILI:
                return QVariant("Tili");
            case DEBET :
                return QVariant("Debet");
            case KREDIT:
                return QVariant("Kredit");
            case SELITE:
                return QVariant("Selite");
            case KUSTANNUSPAIKKA :
                return QVariant("Kustannuspaikka");
            case PROJEKTI:
                return QVariant("Projekti");
        }

    }
    return QVariant( section + 1);
}

QVariant VientiModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return QVariant();
    if( role==Qt::DisplayRole || role == Qt::EditRole)
    {
        VientiRivi rivi = viennit[index.row()];
        switch (index.column())
        {
            case PVM: return QVariant( rivi.pvm );

            case TILI:
                if( rivi.tili.numero())
                    return QVariant( QString("%1 %2").arg(rivi.tili.numero()).arg(rivi.tili.nimi()) );
                else
                    return QVariant();

            case DEBET: return QVariant( float( rivi.debetSnt / 100) );
            case KREDIT: return QVariant( float( rivi.kreditSnt / 100) );
            case SELITE: return QVariant( rivi.selite );
            case KUSTANNUSPAIKKA: return QVariant( "");
            case PROJEKTI: return QVariant("");
        }
    }
    return QVariant();
}

bool VientiModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    switch (index.column())
    {
    case TILI:
        viennit[index.row()].tili = kirjanpito->tili( value.toInt() );
        return true;
    case SELITE:
        viennit[index.row()].selite = value.toString();
        return true;
    default:
        return false;
    }
}

Qt::ItemFlags VientiModel::flags(const QModelIndex &index) const
{
    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;

    return QAbstractTableModel::flags(index);
}

bool VientiModel::insertRows(int row, int count, const QModelIndex & /* parent */)
{
    beginInsertRows( QModelIndex(), row, row + count - 1);
    for(int i=0; i < count; i++)
        viennit.insert(row, uusiRivi() );
    endInsertRows();
    return true;
}

bool VientiModel::lisaaRivi()
{
    return insertRows( rowCount(QModelIndex()), 1, QModelIndex() );

}

VientiRivi VientiModel::uusiRivi()
{
    VientiRivi uusirivi;

    uusirivi.pvm = kirjauswg->tositePvm();

    return uusirivi;
}



