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

#include "model/tosite.h"

#include <algorithm>

TositeSelausModel::TositeSelausModel(QObject *parent) :
    QAbstractTableModel(parent)
{

}

int TositeSelausModel::rowCount(const QModelIndex & /* parent */) const
{
    return lista_.count();
}

int TositeSelausModel::columnCount(const QModelIndex & /* parent */) const
{
    return 6;
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
            if( tila_ < KIRJANPIDOSSA)
                return tr("Tila");
            else
                return tr("Tosite");
        case PVM:
            return tr("Pvm");
        case TOSITETYYPPI:
            return tr("Laji");
        case SUMMA:
            return tr("Summa");
        case ASIAKASTOIMITTAJA:
            return tr("Asiakas/Toimittaja");
        case OTSIKKO:
            return tr("Otsikko");
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
            if( tila_ < KIRJANPIDOSSA) {   // Tila
                if( map.value("tila").toInt() == Tosite::LAHETETAAN)
                    return tr("Lähettäminen epäonnistui");
                else
                    return Tosite::tilateksti( map.value("tila").toInt() );   // TODO Tilojen nimet
            }
            return kp()->tositeTunnus( map.value("tunniste").toInt(),
                                       map.value("pvm").toDate(),
                                       map.value("sarja").toString(),
                                       samakausi_,
                                       role == Qt::EditRole);
        case PVM:
            if( role == Qt::DisplayRole)
                return QVariant( map.value("pvm").toDate() );
            else
                return QString("%1 %2").arg( map.value("pvm").toString() ).arg( map.value("id").toInt(), 8, 10, QChar('0') );

        case TOSITETYYPPI:
            return kp()->tositeTyypit()->nimi( map.value("tyyppi").toInt() ) ;   // TODO: Tyyppikoodien käsittely

        case ASIAKASTOIMITTAJA:
            return map.value("kumppani");

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
     } else if( role == EtsiRooli) {
        return QString("%1 %2 %3").arg(kp()->tositeTunnus( map.value("tunniste").toInt(),
                                                           map.value("pvm").toDate(),
                                                           map.value("sarja").toString(),
                                                           samakausi_,
                                                           false))
                .arg(map.value("kumppani").toString())
                .arg(map.value("otsikko").toString());
    }
    else if( role == Qt::DecorationRole && index.column()==SUMMA )
    {
        if(  map.value("liitteita").toInt() )
            return QIcon(":/pic/liite.png");
        else
            return QIcon(":/pic/tyhja.png");
    }
    else if( role == Qt::DecorationRole && index.column() == TOSITETYYPPI )
        return kp()->tositeTyypit()->kuvake( map.value("tyyppi").toInt() );
    else if( role == Qt::DecorationRole && index.column() == TUNNISTE )
        return Tosite::tilakuva(map.value("tila").toInt());
    else if( role == TositeTyyppiRooli)
    {
        return map.value("tyyppi");
    } else if( role == TositeSarjaRooli) {
        return map.value("sarja");
    }
    return QVariant();
}

QList<int> TositeSelausModel::tyyppiLista() const
{
    QList<int> lista = kaytetytTyypit_.toList();
    std::sort( lista.begin(), lista.end() );
    return lista;
}

QStringList TositeSelausModel::sarjaLista() const
{
    QList<QString> lista = kaytetytSarjat_.toList();
    std::sort( lista.begin(), lista.end());
    return lista;
}



void TositeSelausModel::lataa(const QDate &alkaa, const QDate &loppuu, int tila)
{
    tila_ = tila;
    samakausi_ = kp()->tilikausiPaivalle(alkaa).alkaa() == kp()->tilikausiPaivalle(loppuu).alkaa();

    if( kp()->yhteysModel())
    {
        KpKysely *kysely = kpk("/tositteet");
        kysely->lisaaAttribuutti("alkupvm", alkaa);
        kysely->lisaaAttribuutti("loppupvm", loppuu);
        if( tila == LUONNOKSET )
            kysely->lisaaAttribuutti("luonnos", QString());
        else if( tila == SAAPUNEET)
            kysely->lisaaAttribuutti("saapuneet", QString());
        connect( kysely, &KpKysely::vastaus, this, &TositeSelausModel::tietoSaapuu);
        kysely->kysy();
    }
}

void TositeSelausModel::tietoSaapuu(QVariant *var)
{
    beginResetModel();
    kaytetytTyypit_.clear();
    kaytetytSarjat_.clear();
    lista_ = var->toList();

    for( QVariant item : lista_ ) {
        QVariantMap map = item.toMap();
        kaytetytTyypit_.insert( map.value("tyyppi").toInt() );
        if( map.contains("sarja"))
            kaytetytSarjat_.insert( map.value("sarja").toString());
    }

    endResetModel();
}
