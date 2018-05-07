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

#include "selausmodel.h"

#include <QSqlQuery>
#include "db/kirjanpito.h"

#include <QDebug>

SelausModel::SelausModel()
{

}

int SelausModel::rowCount(const QModelIndex & /* parent */) const
{
    return rivit.count();
}

int SelausModel::columnCount(const QModelIndex & /* parent */) const
{
    return 7;
}

QVariant SelausModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignCenter | Qt::AlignVCenter);
    else if( role != Qt::DisplayRole)
        return QVariant();
    else if( orientation == Qt::Horizontal )
    {
        switch (section)
        {
        case TOSITE:
            return QVariant("Tosite");
        case PVM :
            return QVariant("Pvm");
        case TILI:
            return QVariant("Tili");
        case DEBET :
            return QVariant("Debet");
        case KREDIT:
            return QVariant("Kredit");
        case KOHDENNUS:
            return QVariant("Kohdennus");
        case SELITE:
            return QVariant("Selite");
        }
    }

    return QVariant( section + 1  );
}

QVariant SelausModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return QVariant();

    SelausRivi rivi = rivit.at( index.row());

    if( role == Qt::DisplayRole || role == Qt::EditRole)
    {

        switch (index.column())
        {
            case TOSITE:
                if( role == Qt::EditRole)
                    return rivi.lajiteltavaTositetunniste;
                return QVariant( rivi.tositetunniste);

            case PVM: return QVariant( rivi.pvm );

            case TILI:
                if( role == Qt::EditRole)
                    return rivi.tili.numero();
                else if( rivi.tili.numero())
                    return QVariant( QString("%1 %2").arg(rivi.tili.numero()).arg(rivi.tili.nimi()) );
                else
                    return QVariant();

            case DEBET:
                if( role == Qt::EditRole)
                    return QVariant( rivi.debetSnt);
                else if( rivi.debetSnt )
                    return QVariant( QString("%L1 €").arg(rivi.debetSnt / 100.0,0,'f',2));
                else
                    return QVariant();

            case KREDIT:
                if( role == Qt::EditRole)
                    return QVariant( rivi.kreditSnt);
                else if( rivi.kreditSnt )
                    return QVariant( QString("%L1 €").arg(rivi.kreditSnt / 100.0,0,'f',2));
                else
                    return QVariant();

            case SELITE: return QVariant( rivi.selite );

            case KOHDENNUS :
                QString txt;

                if( rivi.kohdennus.tyyppi() != Kohdennus::EIKOHDENNETA)
                    txt = rivi.kohdennus.nimi();

                if( rivi.taseEra.eraId )
                {
                    if( !txt.isEmpty())
                        txt.append(" \n");
                    txt.append( rivi.taseEra.tositteenTunniste() );
                }

                if( rivi.tagit.count())
                {
                    if( !txt.isEmpty())
                        txt.append(" \n");
                    txt.append(  rivi.tagit.join(", ") );
                }
                return txt;

        }
    }
    else if( role == Qt::TextAlignmentRole)
    {
        if( index.column()==KREDIT || index.column() == DEBET)
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        else
            return QVariant( Qt::AlignLeft | Qt::AlignVCenter);

    }
    else if( role == Qt::UserRole)
    {
        // UserRolena on tositeid, jotta selauksesta pääsee helposti tositteeseen
        return QVariant( rivi.tositeId );
    }
    else if( role == Qt::DecorationRole && index.column() == KOHDENNUS )
    {
        if( rivi.eraMaksettu)
            return QIcon(":/pic/ok.png");
        return rivi.kohdennus.tyyppiKuvake();
    }

    return QVariant();
}

void SelausModel::lataa(const QDate &alkaa, const QDate &loppuu)
{
    QString kysymys = QString("SELECT vienti.tosite, vienti.pvm, tili, debetsnt, kreditsnt, selite, kohdennus, eraid, "
                              "tosite.laji, tosite.tunniste, vienti.id "
                              "FROM vienti, tosite WHERE vienti.pvm BETWEEN \"%1\" AND \"%2\" "
                              "AND vienti.tosite=tosite.id ORDER BY vienti.pvm, vienti.id")
                              .arg( alkaa.toString(Qt::ISODate ) )
                              .arg( loppuu.toString(Qt::ISODate)) ;

    beginResetModel();
    rivit.clear();
    tileilla.clear();

    QSqlQuery query;
    query.exec(kysymys);
    while( query.next())
    {
        SelausRivi rivi;
        rivi.tositeId = query.value(0).toInt();
        rivi.pvm = query.value(1).toDate();
        rivi.tili = kp()->tilit()->tiliIdlla( query.value(2).toInt());
        rivi.debetSnt = query.value(3).toLongLong();
        rivi.kreditSnt = query.value(4).toLongLong();
        rivi.selite = query.value(5).toString();
        rivi.kohdennus = kp()->kohdennukset()->kohdennus( query.value(6).toInt());
        rivi.taseEra = TaseEra( query.value(7).toInt());
        rivi.tositetunniste = QString("%1 %2/%3")
                                       .arg( kp()->tositelajit()->tositelaji( query.value(8).toInt()  ).tunnus() )
                                       .arg( query.value(9).toInt()  )
                                       .arg( kp()->tilikaudet()->tilikausiPaivalle(rivi.pvm).kausitunnus() );
        rivi.lajiteltavaTositetunniste = QString("%1%2/%3")
                                       .arg( kp()->tositelajit()->tositelaji( query.value(8).toInt()  ).tunnus() )
                                       .arg( query.value(9).toInt(),8,10,QChar('0'))
                                       .arg( kp()->tilikaudet()->tilikausiPaivalle(rivi.pvm).kausitunnus() );


        if( query.value("eraid").toInt() == query.value("vienti.id").toInt() )
        {
            TaseEra era( query.value("vienti.id").toInt() );
            rivi.eraMaksettu = era.saldoSnt == 0 ;
        }

        QSqlQuery tagikysely( QString("SELECT kohdennus FROM merkkaus WHERE vienti=%1").arg( query.value("vienti.id").toInt() ));
        while( tagikysely.next())
        {
            rivi.tagit.append( kp()->kohdennukset()->kohdennus( tagikysely.value(0).toInt() ).nimi() );
        }

        rivit.append(rivi);

        QString tilistr = QString("%1 %2")
                    .arg(rivi.tili.numero())
                    .arg(rivi.tili.nimi());
        if( !tileilla.contains(tilistr))
            tileilla.append(tilistr);
    }

    tileilla.sort();
    endResetModel();
}
