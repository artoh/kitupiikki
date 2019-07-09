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

#include "db/kpkysely.h"

#include "db/tositetyyppimodel.h"

#include <algorithm>

TositeSelausModel::TositeSelausModel()
{

}

int TositeSelausModel::rowCount(const QModelIndex & /* parent */) const
{
    return lista_.count();
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
        case TOSITETYYPPI:
            return QVariant("Laji");
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

    QVariantMap map = lista_.at( index.row() ).toMap();

    if( role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch (index.column())
        {

        case TUNNISTE:
            if( role == Qt::EditRole)
                // Lajittelua varten tasaleveä kenttä
                return QVariant(QString("%2 %1")
                       .arg( map.value("tunniste").toInt() ,8,10,QChar('0'))
                       .arg( kp()->tilikaudet()->tilikausiPaivalle( map.value("pvm").toDate() ).kausitunnus() ));
             else
                return QVariant(QString("%1/%2")
                    .arg( map.value("tunniste").toInt() )
                    .arg( kp()->tilikaudet()->tilikausiPaivalle( map.value("tunniste").toDate()  ).kausitunnus() ));
        case PVM:
            return QVariant( map.value("pvm").toDate() );

        case TOSITETYYPPI:
            return kp()->tositeTyypit()->nimi( map.value("tyyppi").toInt() ) ;   // TODO: Tyyppikoodien käsittely

        case OTSIKKO:
            return map.value("otsikko");

        case SUMMA:
            double summa = map.value("summa").toDouble();
            if( role == Qt::EditRole)
                return summa;
            else if( summa > 1e-5 )
                return QVariant( QString("%L1 €").arg(summa,0,'f',2));
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
        return map.value("id");
    }
    else if( role == Qt::DecorationRole && index.column()==TUNNISTE )
    {
        if(  map.value("liitteita").toInt() )
            return QIcon(":/pic/liite.png");
        else
            return QIcon(":/pic/tyhja.png");
    }
    else if( role == Qt::DecorationRole && index.column() == TOSITETYYPPI )
        return kp()->tositeTyypit()->kuvake( map.value("tyyppi").toInt() );
    else if( role == TositeTyyppiRooli)
    {
        return map.value("tyyppi").toInt();
    }
    return QVariant();
}

QList<int> TositeSelausModel::tyyppiLista() const
{
    QList<int> lista = kaytetytTyypit_.toList();
    std::sort( lista.begin(), lista.end() );
    return lista;
}



void TositeSelausModel::lataa(const QDate &alkaa, const QDate &loppuu)
{
    if( kp()->yhteysModel())
    {
        KpKysely *kysely = kpk("/tositteet");
        kysely->lisaaAttribuutti("alkupvm", alkaa);
        kysely->lisaaAttribuutti("loppupvm", loppuu);
        connect( kysely, &KpKysely::vastaus, this, &TositeSelausModel::tietoSaapuu);
        kysely->kysy();
    }

    return;
    /*

    QString kysymys = QString("SELECT tosite.id, tosite.pvm, tosite.otsikko, laji, tunniste, liite.id "
                              "FROM tosite LEFT OUTER JOIN liite ON tosite.id=liite.tosite "
                              "WHERE tosite.pvm BETWEEN \"%1\" AND \"%2\" "
                              "ORDER BY tosite.pvm, tosite.id ")
            .arg(alkaa.toString(Qt::ISODate)).arg(loppuu.toString(Qt::ISODate)) ;

    beginResetModel();

    rivit.clear();
    kaytetytLajinimet.clear();

    QSqlQuery kysely;
    kysely.exec(kysymys);
    int edellinenId = -1;

    while( kysely.next())
    {
        if( kysely.value(0).toInt() == edellinenId)
            continue;   // Jotta tosite ei toistu

        TositeSelausRivi rivi;
        rivi.tositeId = kysely.value(0).toInt();
        rivi.pvm = kysely.value(1).toDate();
        rivi.otsikko = kysely.value(2).toString();
        rivi.tositeLaji = kysely.value(3).toInt();
        rivi.tositeTunniste = kysely.value(4).toInt();
        rivi.liitteita = !kysely.value(5).isNull();

        edellinenId = rivi.tositeId;

        // #138 Jotta viennittömät kirjaukset näytettäisiin, kysellään summat erikseen
        QSqlQuery summakysely( QString("SELECT sum(debetsnt), sum(kreditsnt) FROM vienti WHERE tosite=%1").arg(rivi.tositeId));
        if( summakysely.next())
        {
            qlonglong debet = summakysely.value(0).toLongLong();
            qlonglong kredit = summakysely.value(1).toLongLong();

            // Yleensä kreditin ja debetin pitäisi täsmätä ;)
            if( debet > kredit)
                rivi.summa = debet;
            else
                rivi.summa = kredit;
        }

        rivit.append(rivi);

        // Listalla käytettyjen lajien tunnukset
        QString tositelajinimi = kp()->tositelajit()->tositelajiVanha( rivi.tositeLaji ).nimi();
        if( !kaytetytLajinimet.contains(tositelajinimi))
            kaytetytLajinimet.append(tositelajinimi);

    }

    kaytetytLajinimet.sort();
    endResetModel();
    */
}

void TositeSelausModel::tietoSaapuu(QVariant *var)
{
    beginResetModel();
    kaytetytTyypit_.clear();
    lista_ = var->toList();

    for( QVariant item : lista_ ) {
        kaytetytTyypit_.insert( item.toMap().value("tyyppi").toInt() );
    }


    /*
    kaytetytLajinimet.clear();

    for(QVariant item : map->value("tositteet").toList())
    {
        QVariantMap tosite = item.toMap();
        TositeSelausRivi rivi;
        rivi.tositeId = tosite.value("id").toInt();
        rivi.pvm = tosite.value("pvm").toDate();
        rivi.otsikko = tosite.value("otsikko").toString();
        rivi.tositeLaji = tosite.value("tositelaji").toInt();
        rivi.tositeTunniste = tosite.value("tunniste").toInt();
        rivi.liitteita = tosite.value("liitteita").toInt() > 0;
        rivi.summa = qRound( tosite.value("summa").toDouble() * 100.0 );

        QString tositelajinimi = kp()->tositelajit()->tositelajiVanha( rivi.tositeLaji ).nimi();
        if( !kaytetytLajinimet.contains(tositelajinimi))
            kaytetytLajinimet.append(tositelajinimi);

        rivit.append(rivi);
    }
    kaytetytLajinimet.sort();
    */

    endResetModel();
}
