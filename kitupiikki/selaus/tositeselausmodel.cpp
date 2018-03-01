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

#include <QDebug>
#include <QSqlError>

#include "tositeselausmodel.h"
#include "db/kirjanpito.h"

TositeSelausModel::TositeSelausModel()
{

}

int TositeSelausModel::rowCount(const QModelIndex & /* parent */) const
{
    return rivit.count();
}

int TositeSelausModel::columnCount(const QModelIndex & /* parent */) const
{
    return 5;
}

QVariant TositeSelausModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignCenter | Qt::AlignVCenter);
    else if( role != Qt::DisplayRole)
        return QVariant();
    else if( orientation == Qt::Horizontal)
    {
        switch (section) {
        case TUNNISTE:
            return QVariant("Tosite");
        case PVM:
            return QVariant("Pvm");
        case TOSITELAJI:
            return QVariant("Tositelaji");
        case SUMMA:
            return QVariant("Summa");
        case OTSIKKO:
            return QVariant("Otsikko");
        }
    }
    return QVariant( section + 1);
}

QVariant TositeSelausModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return QVariant();

    TositeSelausRivi rivi = rivit.at( index.row());

    if( role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch (index.column())
        {

        case TUNNISTE:
            return QVariant(QString("%1%2/%3")
                    .arg( kp()->tositelajit()->tositelaji( rivi.tositeLaji ).tunnus() )
                    .arg( rivi.tositeTunniste)
                    .arg( kp()->tilikaudet()->tilikausiPaivalle(rivi.pvm).kausitunnus() ));
        case PVM:
            return QVariant( rivi.pvm );

        case TOSITELAJI:
            return kp()->tositelajit()->tositelaji( rivi.tositeLaji ).nimi();

        case OTSIKKO:
            return QVariant( rivi.otsikko );

        case SUMMA:
            if( role == Qt::EditRole)
                return QVariant( rivi.summa);
            else if( rivi.summa )
                return QVariant( QString("%L1 €").arg(rivi.summa / 100.0,0,'f',2));
            else
                return QVariant();
        }

    }
    else if( role == Qt::TextAlignmentRole)
    {
        if( index.column() == SUMMA )
            return QVariant( Qt::AlignRight | Qt::AlignVCenter);
        else
            return QVariant( Qt::AlignLeft | Qt::AlignVCenter );
    }
    else if( role == Qt::UserRole )
    {
        // UserRolessa on id
        return rivi.tositeId;
    }
    else if( role == TositeLajiIdRooli)
    {
        return QVariant( rivi.tositeLaji );
    }
    return QVariant();
}



void TositeSelausModel::lataa(const QDate &alkaa, const QDate &loppuu)
{
    QString kysymys = QString("SELECT tosite.id, tosite.pvm, otsikko, laji, tunniste, SUM(debetsnt), SUM(kreditsnt) "
                              "FROM tosite, vienti WHERE tosite.pvm BETWEEN \"%1\" AND \"%2\" "
                              "AND vienti.tosite=tosite.id GROUP BY tosite.id ORDER BY tosite.pvm, tosite.id ")
            .arg(alkaa.toString(Qt::ISODate)).arg(loppuu.toString(Qt::ISODate)) ;

    beginResetModel();

    rivit.clear();
    kaytetytLajinimet.clear();

    QSqlQuery kysely;
    kysely.exec(kysymys);

    qDebug() << kysely.lastError().text() << kysely.lastQuery();

    while( kysely.next())
    {
        TositeSelausRivi rivi;
        rivi.tositeId = kysely.value(0).toInt();
        rivi.pvm = kysely.value(1).toDate();
        rivi.otsikko = kysely.value(2).toString();
        rivi.tositeLaji = kysely.value(3).toInt();
        rivi.tositeTunniste = kysely.value(4).toInt();

        qlonglong debet = kysely.value(5).toLongLong();
        qlonglong kredit = kysely.value(6).toLongLong();

        // Yleensä kreditin ja debetin pitäisi täsmätä ;)
        if( debet > kredit)
            rivi.summa = debet;
        else
            rivi.summa = kredit;

        rivit.append(rivi);

        // Listalla käytettyjen lajien tunnukset
        QString tositelajinimi = kp()->tositelajit()->tositelaji( rivi.tositeLaji ).nimi();
        if( !kaytetytLajinimet.contains(tositelajinimi))
            kaytetytLajinimet.append(tositelajinimi);

    }

    kaytetytLajinimet.sort();
    endResetModel();

}
