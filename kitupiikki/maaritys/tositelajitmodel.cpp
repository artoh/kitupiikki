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

#include "tositelajitmodel.h"
#include <QSqlQuery>

#include <QDebug>

TositelajitModel::TositelajitModel()
{
    connect( Kirjanpito::db(), SIGNAL(tietokantaVaihtui()), this, SLOT(lataa()));
}

int TositelajitModel::rowCount(const QModelIndex &parent) const
{
    return lajit_.count();
}

int TositelajitModel::columnCount(const QModelIndex &parent) const
{
    return 2;
}

QVariant TositelajitModel::headerData(int section, Qt::Orientation orientation, int role) const
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

QVariant TositelajitModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return QVariant();
    if( role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch (index.column())
        {
            case TUNNUS: return QVariant( lajit_[ index.row() ].tunnus );

            case NIMI:
                if( lajit_[index.row()].nimi.isEmpty() && role==Qt::DisplayRole)
                    return QVariant( tr("<Uusi tositelaji>"));
                else
                    return QVariant( lajit_[index.row()].nimi );
        }
    }
    else if( role == Qt::TextAlignmentRole)
    {
        return QVariant( Qt::AlignLeft | Qt::AlignVCenter);
    }
    return QVariant();
}

Qt::ItemFlags TositelajitModel::flags(const QModelIndex &index) const
{
    TositeLajiModelSisainen::Tositelaji laji = lajit_[index.row()];

    // Järjestelmätosityyppiä ei saa muokata
    // Oletustositetyypin "" tunnusta ei saa muokata
    // Käytössä olevan tositetyypi tunnusta ei saa muokata

    if( laji.tunnus == "*" || ( index.column()==TUNNUS && ( laji.kaytossa || (laji.tunnus=="" && laji.riviId) )))
        return QAbstractTableModel::flags(index);
    else
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

bool TositelajitModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    lajit_[index.row()].muokattu = true;

    switch (index.column()) {
    case TUNNUS:
        lajit_[ index.row()].tunnus = value.toString();
        return true;
    case NIMI:
        lajit_[ index.row()].nimi = value.toString();
        return true;
    default:
        return false;
    }

}

void TositelajitModel::lataa()
{
    beginResetModel();

    lajit_.clear();
    QSqlQuery kysely("SELECT _rowid_,tunnus,nimi FROM tositelaji");
    while( kysely.next())
    {
        TositeLajiModelSisainen::Tositelaji laji;
        laji.riviId = kysely.value(0).toInt();
        laji.tunnus = kysely.value(1).toString();
        laji.nimi = kysely.value(2).toString();

        // Jos tositteella on kirjauksia, ei lajin tunnusta saa muuttaa tai lajia poistaa!
        QSqlQuery apukysely( QString("SELECT id FROM tosite WHERE laji=\"%1\" LIMIT 1").arg(laji.tunnus));
        laji.kaytossa = apukysely.next();

        lajit_.append( laji );
    }

    endResetModel();
}

bool TositelajitModel::tallenna()
{
    // Tämä onkin sitten jo vähän haastavampi

    QSqlQuery tallennus;
    foreach (TositeLajiModelSisainen::Tositelaji laji, lajit_)
    {
        if( laji.muokattu && !laji.nimi.isEmpty() && ( laji.riviId == 0 | !laji.tunnus.isEmpty() ) )
        {
            if( laji.riviId )
            {
                tallennus.prepare("UPDATE tositelaji SET tunnus=:tunnus, nimi=:nimi WHERE _rowid_=:id");
                tallennus.bindValue(":id", laji.riviId);
            }
            else
            {
                tallennus.prepare("INSERT INTO tositelaji(tunnus,nimi) VALUES(:tunnus,:nimi)");
            }
            tallennus.bindValue(":tunnus", laji.tunnus);
            tallennus.bindValue(":nimi", laji.nimi);

            tallennus.exec();
        }

    }

    // Ladataan tietokanta uudelleen, jotta rakenteen muutos päivittyy kaikkialle
    return Kirjanpito::db()->lataaUudelleen();
}

void TositelajitModel::lisaaRivi()
{
    beginInsertRows( QModelIndex(), lajit_.count(), lajit_.count() );
    TositeLajiModelSisainen::Tositelaji laji;
    lajit_.append(laji);
    endInsertRows();
}


