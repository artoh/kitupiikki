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

#include <QSqlQuery>

#include "projektimodel.h"
#include "db/kirjanpito.h"
#include "db/tilikausi.h"

ProjektiModel::ProjektiModel(QObject *parent) :
    QAbstractTableModel(parent)
{

}

int ProjektiModel::rowCount(const QModelIndex & /* parent */) const
{
    return projektit_.count();
}

int ProjektiModel::columnCount(const QModelIndex & /* parent */) const
{
    return 3;
}

QVariant ProjektiModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignCenter | Qt::AlignVCenter);
    else if( role != Qt::DisplayRole )
        return QVariant();
    else if( orientation == Qt::Horizontal)
    {
        switch (section)
        {
            case NIMI:
                return QVariant("Nimi");
            case ALKAA :
                return QVariant("Alkaa");
            case PAATTYY:
                return QVariant("Päättyy");
        }

    }
    return QVariant( section + 1);
}

QVariant ProjektiModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return QVariant();

    Projekti projekti = projektit_[index.row()];

    if( role == IdRooli)
        return QVariant( projekti.projektiId);
    else if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignLeft | Qt::AlignVCenter);
    else if( role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch( index.column())
        {
            case NIMI: return QVariant( projekti.nimi);
            case ALKAA: return QVariant(projekti.alkaa);
            case PAATTYY: return QVariant(projekti.paattyy);
        }
    }
    return QVariant();
}

Qt::ItemFlags ProjektiModel::flags(const QModelIndex &index) const
{
    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

bool ProjektiModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if( role != Qt::EditRole)
        return false;

    projektit_[ index.row() ].muokattu = true;
    if( index.column() == NIMI)
        projektit_[ index.row() ].nimi = value.toString();
    else if( index.column() == ALKAA )
        projektit_[ index.row() ].alkaa = value.toDate();
    else if( index.column() == PAATTYY)
        projektit_[ index.column()].paattyy = value.toDate();

    return true;
}

Projekti ProjektiModel::projekti(int id)
{
    foreach (Projekti projekti, projektit_)
    {
        if( projekti.projektiId == id)
            return projekti;
    }
    return Projekti();
}

void ProjektiModel::lataa()
{
    beginResetModel();
    projektit_.clear();
    QSqlQuery kysely("select id, nimi, alkaa, loppuu FROM projekti");
    while( kysely.next() )
    {
        Projekti uusi;
        uusi.projektiId = kysely.value(0).toInt();
        uusi.nimi = kysely.value(1).toString();
        uusi.alkaa = kysely.value(2).toDate();
        uusi.paattyy = kysely.value(3).toDate();
        projektit_.append(uusi);
    }
    endResetModel();
}

void ProjektiModel::lisaaUusi(const QString nimi)
{
    beginInsertRows(QModelIndex(), projektit_.count(), projektit_.count());
    Projekti uusi;
    uusi.nimi = nimi;

    Tilikausi nykyinen = Kirjanpito::db()->tilikausiPaivalle( Kirjanpito::db()->paivamaara() );

    uusi.alkaa = nykyinen.alkaa();
    uusi.paattyy = nykyinen.paattyy();

    projektit_.append( uusi );
    endInsertRows();
}
