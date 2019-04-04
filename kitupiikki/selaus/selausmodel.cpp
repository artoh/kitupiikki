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
    return lista_.count();
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

    QVariantMap map = lista_.at( index.row()).toMap();

    if( role == Qt::DisplayRole || role == Qt::EditRole)
    {
        Tili *tili = kp()->tilit()->tiliNumerolla( map.value("tili").toInt() );
        switch (index.column())
        {
            case TOSITE:
                if( role == Qt::EditRole)
                {
                    return QString("%1%2/%3")
                            .arg( kp()->tositelajit()->tositelajiVanha( map.value("tosite").toMap().value("tositelaji").toInt() ).tunnus() )
                            .arg( map.value("tosite").toMap().value("tunniste").toInt(),8,10,QChar('0'))
                            .arg( kp()->tilikaudet()->tilikausiPaivalle( map.value("tosite").toMap().value("pvm").toDate() ).kausitunnus());
                }
                return QString("%1 %2/%3")
                        .arg( kp()->tositelajit()->tositelajiVanha( map.value("tosite").toMap().value("tositelaji").toInt()  ).tunnus() )
                        .arg( map.value("tosite").toMap().value("tunniste").toInt()  )
                        .arg( kp()->tilikaudet()->tilikausiPaivalle( map.value("tosite").toMap().value("pvm").toDate() ).kausitunnus() );

            case PVM: return QVariant( map.value("pvm").toDate() );

            case TILI:
                if( role == Qt::EditRole)
                    return tili->numero();
                else if( tili->numero())
                    return QVariant( QString("%1 %2").arg(tili->numero()).arg(tili->nimi()) );
                else
                    return QVariant();

            case DEBET:
            {
                qlonglong debetsnt = map.value("debetsnt").toLongLong();
                if( role == Qt::EditRole)
                    return QVariant( debetsnt );
                else if( debetsnt )
                    return QVariant( QString("%L1 €").arg( debetsnt / 100.0,0,'f',2));
                else
                    return QVariant();
            }

            case KREDIT:
            {
                qlonglong kreditsnt = map.value("kreditsnt").toLongLong();

                if( role == Qt::EditRole)
                    return QVariant( kreditsnt);
                else if( kreditsnt )
                    return QVariant( QString("%L1 €").arg(kreditsnt / 100.0,0,'f',2));
                else
                    return QVariant();
            }
            case SELITE: return map.value("selite");

            case KOHDENNUS :
                QString txt;
                Kohdennus kohdennus = kp()->kohdennukset()->kohdennus( map.value("kohdennus").toInt() );
                if( kohdennus.tyyppi() != Kohdennus::EIKOHDENNETA)
                    txt = kohdennus.nimi();

                if( map.value("era").toMap().contains("tunniste") )
                {
                    txt.append( QString("%1%2/%3")
                            .arg( kp()->tositelajit()->tositelajiVanha( map.value("era").toMap().value("tositelaji").toInt() ).tunnus() )
                            .arg( map.value("era").toMap().value("tunniste").toInt())
                            .arg( kp()->tilikaudet()->tilikausiPaivalle( map.value("era").toMap().value("pvm").toDate() ).kausitunnus()) );
                }

                if( map.contains("merkkaukset") )
                {
                    QStringList tagilista;
                    for( auto kohdennusVar : map.value("merkkaukset").toList())
                    {
                        int kohdennusId = kohdennusVar.toInt();
                        tagilista.append( kp()->kohdennukset()->kohdennus(kohdennusId).nimi() );
                    }
                    if( !txt.isEmpty())
                        txt.append(" \n");
                    txt.append(  tagilista.join(", ") );
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
        return QVariant( map.value("tosite").toMap().value("id").toInt() );
    }
    else if( role == Qt::DecorationRole && index.column() == KOHDENNUS )
    {
        if( map.contains("era") && map.value("era").toMap().value("saldo") == 0 )
            return QIcon(":/pic/ok.png");
        // return rivi.kohdennus.tyyppiKuvake();
    }
    else if( role == Qt::DecorationRole && index.column() == TOSITE)
    {
        if( map.value("tosite").toMap().value("liitteita").toInt() )
            return QIcon(":/pic/liite.png");
        else
            return QIcon(":/pic/tyhja.png");
    }


    return QVariant();
}

void SelausModel::lataa(const QDate &alkaa, const QDate &loppuu)
{
    KpKysely *kysely = kpk("viennit");
    kysely->lisaaAttribuutti("alkupvm", alkaa);
    kysely->lisaaAttribuutti("loppupvm", loppuu);
    connect( kysely, &KpKysely::vastaus, this, &SelausModel::tietoSaapuu);

    qDebug() << QDateTime::currentDateTime() << " C1 ";
    kysely->kysy();
    qDebug() << QDateTime::currentDateTime() << " C2 ";

    return;


    QString kysymys = QString("SELECT vienti.tosite, vienti.pvm, tili, debetsnt, kreditsnt, selite, kohdennus, eraid, "
                              "tosite.laji, tosite.tunniste, vienti.id, liite.id "
                              "FROM vienti, tosite LEFT OUTER JOIN liite ON tosite.id=liite.tosite "
                              "WHERE vienti.pvm BETWEEN \"%1\" AND \"%2\" "
                              "AND vienti.tosite=tosite.id AND tili is not null ORDER BY vienti.pvm, vienti.id")
                              .arg( alkaa.toString(Qt::ISODate ) )
                              .arg( loppuu.toString(Qt::ISODate)) ;

    beginResetModel();
    rivit.clear();
    tileilla.clear();

    int edellinenVientiId = -1;

    QSqlQuery query;
    query.exec(kysymys);
    while( query.next())
    {
        if( query.value("vienti.id").toInt() == edellinenVientiId)
            continue;

        SelausRivi rivi;
        rivi.tositeId = query.value(0).toInt();
        rivi.pvm = query.value(1).toDate();
        rivi.tili = kp()->tilit()->tiliIdllaVanha( query.value(2).toInt());
        rivi.debetSnt = query.value(3).toLongLong();
        rivi.kreditSnt = query.value(4).toLongLong();
        rivi.selite = query.value(5).toString();
        rivi.kohdennus = kp()->kohdennukset()->kohdennus( query.value(6).toInt());
        rivi.taseEra = TaseEra( query.value(7).toInt());
        if( kp()->asetukset()->onko("Samaansarjaan"))
        {
            rivi.tositetunniste = QString("%1/%2")
                                           .arg( query.value(9).toInt()  )
                                           .arg( kp()->tilikaudet()->tilikausiPaivalle(rivi.pvm).kausitunnus() );

            rivi.lajiteltavaTositetunniste = QString("%1/%2")
                                           .arg( query.value(9).toInt(),8,10,QChar('0'))
                                           .arg( kp()->tilikaudet()->tilikausiPaivalle(rivi.pvm).kausitunnus() );
        } else {
            rivi.tositetunniste = QString("%1 %2/%3")
                                           .arg( kp()->tositelajit()->tositelajiVanha( query.value(8).toInt()  ).tunnus() )
                                           .arg( query.value(9).toInt()  )
                                           .arg( kp()->tilikaudet()->tilikausiPaivalle(rivi.pvm).kausitunnus() );

            rivi.lajiteltavaTositetunniste = QString("%1%2/%3")
                                       .arg( kp()->tositelajit()->tositelajiVanha( query.value(8).toInt()  ).tunnus() )
                                       .arg( query.value(9).toInt(),8,10,QChar('0'))
                                       .arg( kp()->tilikaudet()->tilikausiPaivalle(rivi.pvm).kausitunnus() );
        }

        rivi.vientiId = query.value("vienti.id").toInt();
        rivi.liitteita = !query.value("liite.id").isNull();

        edellinenVientiId = rivi.vientiId;

        if( query.value("eraid").toInt() && rivi.tili.eritellaankoTase() )
        {
            TaseEra era( query.value("eraid").toInt() );
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

void SelausModel::tietoSaapuu(QVariantMap *map, int /* status */)
{

    beginResetModel();
    tileilla.clear();

    qDebug() << QDateTime::currentDateTime() << " A1 ";
    lista_ = map->value("viennit").toList();

    qDebug() << QDateTime::currentDateTime() << " A2 ";

    for(auto rivi : lista_)
    {
        int tiliId = rivi.toMap().value("tili").toInt();
        Tili tili = kp()->tilit()->tiliIdllaVanha(tiliId);
        QString tilistr = QString("%1 %2")
                    .arg(tili.numero())
                    .arg(tili.nimi());
        if( !tileilla.contains(tilistr))
            tileilla.append(tilistr);
    }

    qDebug() << QDateTime::currentDateTime() << " A3 ";

    endResetModel();
}
