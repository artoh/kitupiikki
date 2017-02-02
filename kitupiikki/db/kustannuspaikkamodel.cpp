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

#include "kustannuspaikkamodel.h"



Kustannuspaikka::Kustannuspaikka(const QString kpnimi)
    : id_(0), nimi_(kpnimi), muokattu_(false)
{

}

Kustannuspaikka::Kustannuspaikka(int id, const QString kpnimi)
    : id_(id), nimi_(kpnimi), muokattu_(false)
{

}

//
//
//


KustannuspaikkaModel::KustannuspaikkaModel(QObject *parent)
    : QAbstractTableModel(parent)
{

}

int KustannuspaikkaModel::rowCount(const QModelIndex & /* parent */) const
{
    return kustannuspaikat_.count();
}

int KustannuspaikkaModel::columnCount(const QModelIndex & /* parent */) const
{
    return 1;
}

QVariant KustannuspaikkaModel::headerData(int section, Qt::Orientation orientation, int role) const
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
        }

    }
    return QVariant( section + 1);
}

QVariant KustannuspaikkaModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return QVariant();

    Kustannuspaikka kp = kustannuspaikat_[index.row()];

    if( role == IdRooli)
        return QVariant( kp.id() );
    else if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignLeft | Qt::AlignVCenter);
    else if( role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch( index.column())
        {
            case NIMI: return QVariant( kp.nimi() );
        }
    }
    return QVariant();
}

Qt::ItemFlags KustannuspaikkaModel::flags(const QModelIndex &index) const
{
    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

bool KustannuspaikkaModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if( role != Qt::EditRole)
        return false;

    if( index.column() == NIMI)
        kustannuspaikat_[ index.row() ].asetaNimi(value.toString());

    return true;
}

QString KustannuspaikkaModel::nimi(int id) const
{
    return kustannuspaikka(id).nimi();
}

Kustannuspaikka KustannuspaikkaModel::kustannuspaikka(int id) const
{
    foreach (Kustannuspaikka kp, kustannuspaikat())
    {
        if( kp.id() == id)
            return kp;
    }
    return Kustannuspaikka();
}

QList<Kustannuspaikka> KustannuspaikkaModel::kustannuspaikat() const
{
    return kustannuspaikat_;
}

void KustannuspaikkaModel::lataa()
{
    beginResetModel();
    kustannuspaikat_.clear();
    QSqlQuery kysely("SELECT id, nimi FROM kustannuspaikka");
    while( kysely.next() )
    {
        kustannuspaikat_.append( Kustannuspaikka( kysely.value(0).toInt(),
                                                  kysely.value(1).toString()) );
    }
    endResetModel();
}

void KustannuspaikkaModel::lisaaUusi(const QString &nimi)
{
    beginInsertRows( QModelIndex(), kustannuspaikat_.count(), kustannuspaikat_.count());
    Kustannuspaikka uusi;
    kustannuspaikat_.append( Kustannuspaikka(nimi));
    endInsertRows();
}

