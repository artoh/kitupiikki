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
    // TODO: Tähän oikea signaali
    connect( Kirjanpito::db(), SIGNAL(tietokantaVaihtui()), this, SLOT(lataa()));
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
    if( role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch (index.column())
        {
            case TUNNUS: return QVariant( lajit_[ index.row() ].tunnus() );

            case NIMI:
                if( lajit_[index.row()].nimi().isEmpty() && role==Qt::DisplayRole)
                    return QVariant( tr("<Uusi tositelaji>"));
                else
                    return QVariant( lajit_[index.row()].nimi() );
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

bool TositeLajiModel::setData(const QModelIndex &index, const QVariant &value, int role)
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

void TositeLajiModel::lataa()
{
    beginResetModel();

    lajit_.clear();
    QSqlQuery kysely("SELECT _rowid_,tunnus,nimi FROM tositelaji");
    while( kysely.next())
    {

        lajit_.append( TositeLaji(kysely.value(0).toInt(), kysely.value(1).toString(),
                                  kysely.value(2).toString()) );
    }

    endResetModel();
}

bool TositeLajiModel::tallenna()
{
    QSqlQuery tallennus;
    foreach (TositeLaji laji, lajit_)
    {
        if( (laji.muokattu()) && (!laji.nimi().isEmpty()) && ( laji.id() == 0 | !laji.tunnus().isEmpty() ) )
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

            tallennus.exec();
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
