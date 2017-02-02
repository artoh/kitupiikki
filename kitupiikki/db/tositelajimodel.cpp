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

#include "tositelajimodel.h"
#include "db/kirjanpito.h"

#include <QSqlQuery>

#include <QDebug>

//
//  TositeLaji
//

TositeLaji::TositeLaji() :
    id_(0), muokattu_(false)
{

}

TositeLaji::TositeLaji(int id, QString tunnus, QString nimi) :
    id_(id), tunnus_(tunnus), nimi_(nimi), muokattu_(false)
{

}

void TositeLaji::asetaId(int id)
{
    id_ = id;
    muokattu_ = true;
}

void TositeLaji::asetaTunnus(const QString &tunnus)
{
    tunnus_ = tunnus;
    muokattu_ = true;
}

void TositeLaji::asetaNimi(const QString &nimi)
{
    nimi_ = nimi;
    muokattu_ = true;
}

void TositeLaji::nollaaMuokattu()
{
    muokattu_ = false;
}

//
//  TositeLajiModel
//

TositeLajiModel::TositeLajiModel(QObject *parent)
    : QAbstractTableModel(parent)
{

}

int TositeLajiModel::rowCount(const QModelIndex & /* parent */ ) const
{
    return lajit_.count();

}

int TositeLajiModel::columnCount(const QModelIndex & /* parent */) const
{
    return 2;
}

QVariant TositeLajiModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignCenter | Qt::AlignVCenter);
    else if( orientation == Qt::Horizontal && role == Qt::DisplayRole )
    {
        switch (section)
        {
        case TUNNUS :
            return QVariant("Tunnus");
        case NIMI:
            return QVariant("Nimi");
        }
    }
    return QVariant();
}

QVariant TositeLajiModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return QVariant();

    TositeLaji laji = lajit_[index.row()];

    if( role == IdRooli)
        return QVariant( laji.id() );
    else if( role == TunnusRooli)
        return QVariant( laji.tunnus());
    else if( role == NimiRooli)
        return QVariant( laji.nimi());

    else if( role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch (index.column())
        {
            case TUNNUS: return QVariant(laji.tunnus() );

            case NIMI:
                if( laji.nimi().isEmpty() && role==Qt::DisplayRole)
                    return QVariant( tr("<Uusi tositelaji>"));
                else
                    return QVariant( laji.nimi() );
        }
    }
    else if( role == Qt::TextAlignmentRole)
    {
        return QVariant( Qt::AlignLeft | Qt::AlignVCenter);
    }
    return QVariant();
}

Qt::ItemFlags TositeLajiModel::flags(const QModelIndex &index) const
{
    // TODO: Tässä pitäisi selvittää, onko tyyppi käytössä että saako sitä muokata...

    TositeLaji laji = lajit_[ index.row()];

    if( laji.tunnus() == "*" || ( index.column()==TUNNUS && laji.tunnus()=="" && laji.id()) )
        return QAbstractTableModel::flags(index);

    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

bool TositeLajiModel::setData(const QModelIndex &index, const QVariant &value, int /* role */)
{
    switch (index.column()) {
    case TUNNUS:
        lajit_[ index.row()].asetaTunnus( value.toString());
        return true;
    case NIMI:
        lajit_[ index.row()].asetaNimi(value.toString());
        return true;
    default:
        ;
    }

    return false;
}

TositeLaji TositeLajiModel::tositelaji(int id) const
{
    if( id == 0)
        return TositeLaji(0,"***","Järjestelmän tosite");

    foreach (TositeLaji laji, lajit_)
    {
        if( laji.id() == id)
            return laji;
    }
    return TositeLaji();
}

void TositeLajiModel::lataa()
{
    beginResetModel();

    lajit_.clear();
    QSqlQuery kysely("SELECT id,tunnus,nimi FROM tositelaji");
    qDebug() << kysely.lastQuery();
    while( kysely.next())
    {
        qDebug() << kysely.value(0).toInt() << " - " << kysely.value(1).toString() << " - " << kysely.value(2).toString() ;
        // Järjestelmätositetta ei laiteta näkyviin
        if( kysely.value(0).toInt() > 0)
            lajit_.append( TositeLaji(kysely.value(0).toInt(), kysely.value(1).toString(),
                                      kysely.value(2).toString()) );
    }

    endResetModel();
}

bool TositeLajiModel::tallenna()
{
    QSqlQuery tallennus;
    for(int i=0; i < lajit_.count(); i++)
    {
        TositeLaji laji = lajit_[i];

        if( (laji.muokattu()) && (!laji.nimi().isEmpty()) && ( laji.id() == 0 || !laji.tunnus().isEmpty() ) )
        {
            if( laji.id() )
            {
                tallennus.prepare("UPDATE tositelaji SET tunnus=:tunnus, nimi=:nimi WHERE _rowid_=:id");
                tallennus.bindValue(":id", laji.id());
            }
            else
            {
                tallennus.prepare("INSERT INTO tositelaji(tunnus,nimi) VALUES(:tunnus,:nimi)");
            }
            tallennus.bindValue(":tunnus", laji.tunnus() );
            tallennus.bindValue(":nimi", laji.nimi() );

            if(tallennus.exec())
                lajit_[i].nollaaMuokattu();

            if( laji.id())
                lajit_[i].asetaId( tallennus.lastInsertId().toInt());
        }

    }
    return true;
}

void TositeLajiModel::lisaaRivi()
{
    beginInsertRows( QModelIndex(), lajit_.count(), lajit_.count() );
    lajit_.append( TositeLaji());
    endInsertRows();
}
