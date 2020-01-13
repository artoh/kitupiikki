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
#include "db/tositetyyppimodel.h"
#include "tositeselausmodel.h"

#include <QDebug>

SelausModel::SelausModel(QObject *parent) :
    QAbstractTableModel(parent)
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
        switch (index.column())
        {
            case TOSITE:
                return kp()->tositeTunnus( map.value("tosite").toMap().value("tunniste").toInt(),
                                           map.value("tosite").toMap().value("pvm").toDate(),
                                           map.value("tosite").toMap().value("sarja").toString(),
                                           samakausi_,
                                           role == Qt::EditRole );                            

            case PVM:
            if( role == Qt::DisplayRole)
                return QVariant( map.value("pvm").toDate() );
            else
                return QString("%1 %2").arg( map.value("pvm").toString() ).arg( map.value("id").toInt(), 8, 10, QChar('0') );

            case TILI:
            {
                Tili *tili = kp()->tilit()->tili( map.value("tili").toInt() );
                if( !tili )
                    return QVariant();
                if( role == Qt::EditRole)
                    return tili->numero();
                else if( tili->numero())
                    return QVariant( QString("%1 %2").arg(tili->numero()).arg(tili->nimi()) );
                else
                    return QVariant();
            }

            case DEBET:
            {
                double debet = map.value("debet").toDouble();
                if( role == Qt::EditRole)
                    return QVariant( debet );
                else if( qAbs(debet) > 1e-5 )
                    return QVariant( QString("%L1 €").arg( debet ,0,'f',2));
                else
                    return QVariant();
            }

            case KREDIT:
            {
                double kredit = map.value("kredit").toDouble();

                if( role == Qt::EditRole)
                    return QVariant( kredit);
                else if( qAbs(kredit) > 1e-5 )
                    return QVariant( QString("%L1 €").arg(kredit,0,'f',2));
                else
                    return QVariant();
            }
            case SELITE: return map.value("selite");

            case KOHDENNUS :
                QString txt;

                Kohdennus kohdennus = kp()->kohdennukset()->kohdennus( map.value("kohdennus").toInt() );
                if( kohdennus.tyyppi() != Kohdennus::EIKOHDENNETA)
                    txt = kohdennus.nimi();

                if( map.contains("merkkaukset") )
                {
                    QStringList tagilista;
                    for( auto kohdennusVar : map.value("merkkaukset").toList())
                    {
                        int kohdennusId = kohdennusVar.toInt();
                        tagilista.append( kp()->kohdennukset()->kohdennus(kohdennusId).nimi() );
                    }
                    if( !txt.isEmpty())
                        txt.append(" ");
                    txt.append(  tagilista.join(", ") );
                }

                if( map.value("era").toMap().contains("tunniste")  )
                {
                    if( map.value("era").toMap().value("tunniste") != map.value("tosite").toMap().value("tunniste") ||
                        map.value("era").toMap().value("pvm") != map.value("tosite").toMap().value("pvm")) {
                        if( !txt.isEmpty())
                            txt.append(" ");
                        txt.append( QString("%1/%2")
                                .arg( map.value("era").toMap().value("tunniste").toInt() )
                                .arg( kp()->tilikaudet()->tilikausiPaivalle( map.value("era").toMap().value("pvm").toDate() ).kausitunnus()) );
                    }
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
        Kohdennus kohdennus = kp()->kohdennukset()->kohdennus( map.value("kohdennus").toInt() );
        if(kohdennus.tyyppi())
            return kp()->kohdennukset()->kohdennus( map.value("kohdennus").toInt()).tyyppiKuvake();
        else
            return QIcon(":/pic/tyhja.png");

    }
    else if( role == Qt::DecorationRole && index.column() == TOSITE)
    {
        if( map.value("tosite").toMap().value("liitteita").toInt() )
            return QIcon(":/pic/liite.png");
        else
            return QIcon(":/pic/tyhja.png");
    }
    else if( role == Qt::DecorationRole && index.column() == PVM)
    {
        int tyyppi = map.value("tosite").toMap().value("tyyppi").toInt();
        return kp()->tositeTyypit()->kuvake(tyyppi);
    }
    else if( role == TositeSelausModel::TositeTyyppiRooli) {
        return map.value("tosite").toMap().value("tyyppi").toInt();
    }


    return QVariant();
}

void SelausModel::lataa(const QDate &alkaa, const QDate &loppuu)
{
    samakausi_ = kp()->tilikausiPaivalle(alkaa).alkaa() == kp()->tilikausiPaivalle(loppuu).alkaa();

    KpKysely *kysely = kpk("/viennit");
    kysely->lisaaAttribuutti("alkupvm", alkaa);
    kysely->lisaaAttribuutti("loppupvm", loppuu);
    connect( kysely, &KpKysely::vastaus, this, &SelausModel::tietoSaapuu);

    kysely->kysy();

    return;
}

void SelausModel::tietoSaapuu(QVariant *var)
{

    beginResetModel();
    tileilla.clear();
    lista_ = var->toList();


    for(auto rivi : lista_)
    {
        int tiliId = rivi.toMap().value("tili").toInt();
        Tili* tili = kp()->tilit()->tili(tiliId);
        QString tilistr = QString("%1 %2")
                    .arg(tili->numero())
                    .arg(tili->nimi());
        if( !tileilla.contains(tilistr))
            tileilla.append(tilistr);
    }
    tileilla.sort();

    endResetModel();
}
