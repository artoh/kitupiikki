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

#include "tilikausimodel.h"

TilikausiModel::TilikausiModel(QSqlDatabase *tietokanta, QObject *parent) :
    QAbstractTableModel(parent), tietokanta_(tietokanta)
{

}

int TilikausiModel::rowCount(const QModelIndex & /* parent */) const
{
    return kaudet_.count();
}

int TilikausiModel::columnCount(const QModelIndex & /* parent */) const
{
    return 2;
}

QVariant TilikausiModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return QVariant();

    Tilikausi kausi = kaudet_.value(index.row());

    if( role == Qt::DisplayRole)
    {
        if( index.column() == ALKAA)
            return QVariant( kausi.alkaa());
        else if( index.column() == PAATTYY)
            return QVariant( kausi.paattyy());

    }
    return QVariant();
}

void TilikausiModel::lisaaTilikausi(Tilikausi tilikausi)
{
    beginInsertRows( QModelIndex(), kaudet_.count(), kaudet_.count());

    kaudet_.append( tilikausi );

    endInsertRows();
}

Tilikausi TilikausiModel::tilikausiPaivalle(const QDate &paiva) const
{
    foreach (Tilikausi kausi, kaudet_)
    {
        // Osuuko pyydetty päivä kysyttyyn jaksoon
        if( kausi.alkaa().daysTo(paiva) >= 0 and paiva.daysTo(kausi.paattyy()) >= 0)
            return kausi;
    }
    return Tilikausi(QDate(), QDate()); // Kelvoton tilikausi

}

QDate TilikausiModel::kirjanpitoAlkaa() const
{
    if( kaudet_.count())
        return kaudet_.first().alkaa();
    return QDate();
}

QDate TilikausiModel::kirjanpitoLoppuu() const
{
    if( kaudet_.count())
        return kaudet_.last().paattyy();
    return QDate();
}

void TilikausiModel::lataa()
{
    beginResetModel();
    kaudet_.clear();

    QSqlQuery kysely(*tietokanta_);

    kysely.exec("SELECT alkaa, loppuu FROM tilikausi ORDER BY alkaa");
    while( kysely.next())
    {
        kaudet_.append( Tilikausi(kysely.value(0).toDate(), kysely.value(1).toDate()));
    }
    endResetModel();
}

void TilikausiModel::tallenna()
{
    // Tilikausi tallennetaan aina kirjoittamalla se kokonaan uudelleen
    tietokanta_->transaction();

    QSqlQuery kysely(*tietokanta_);
    kysely.exec("DELETE FROM tilikausi");

    kysely.prepare("INSERT INTO tilikausi(alkaa,loppuu) VALUES(:alku,:loppu)");
    foreach (Tilikausi kausi, kaudet_)
    {
        kysely.bindValue(":alku", kausi.alkaa());
        kysely.bindValue(":loppu", kausi.paattyy());
        kysely.exec();
    }

    tietokanta_->commit();
}
