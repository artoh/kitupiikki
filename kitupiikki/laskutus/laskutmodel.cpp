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

#include "laskutmodel.h"
#include "db/kirjanpito.h"
#include <QSqlQuery>


LaskutModel::LaskutModel(QObject *parent) :
    QAbstractTableModel(parent)
{

}

int LaskutModel::rowCount(const QModelIndex & /* parent */) const
{
    return laskut.count();
}

int LaskutModel::columnCount(const QModelIndex & /* parent */) const
{
    return 6;
}

QVariant LaskutModel::data(const QModelIndex &item, int role) const
{
    if( !item.isValid())
        return QVariant();

    AvoinLasku lasku = laskut.value(item.row());

    if( role == Qt::DisplayRole || role == Qt::EditRole)
    {

        switch (item.column())
        {
        case NUMERO:
            return lasku.viitenro;
        case PVM:
            return lasku.pvm;
        case ERAPVM:
            if( lasku.kirjausperuste == LaskuModel::KATEISLASKU ||  lasku.json.luku("Hyvityslasku"))
                return QString();
            return lasku.erapvm;
        case SUMMA:
            if( role == Qt::DisplayRole)
                return QString("%L1 €").arg(lasku.summaSnt / 100.0,0,'f',2);
            else
               return lasku.summaSnt;
        case MAKSAMATTA:
            if( role == Qt::DisplayRole)
            {
                if( lasku.avoinSnt)
                    return QString("%L1 €").arg(lasku.avoinSnt / 100.0,0,'f',2);
                else
                    return QVariant();  // Nollalle tyhjää
            }
            else
                return lasku.avoinSnt;
        case ASIAKAS:
            return lasku.asiakas;
        }
    }
    else if( role == Qt::TextAlignmentRole)
    {
        if( item.column() == SUMMA || item.column() == MAKSAMATTA )
            return QVariant( Qt::AlignRight | Qt::AlignVCenter);
    }
    else if( role == Qt::TextColorRole && item.column() == ERAPVM)
    {
        if( kp()->paivamaara().daysTo( lasku.erapvm) < 0 && lasku.avoinSnt )
            return QColor(Qt::red);
    }
    else if( role == Qt::DecorationRole && item.column() == PVM)
    {
        if( lasku.json.luku("Hyvityslasku") )
            return QIcon(":/pic/poista.png");

        switch (lasku.kirjausperuste) {
        case LaskuModel::SUORITEPERUSTE:
            return QIcon(":/pic/suorite.png");
        case LaskuModel::LASKUTUSPERUSTE:
            return QIcon(":/pic/kirje.png");
        case LaskuModel::MAKSUPERUSTE :
            return QIcon(":/pic/euro.png");
        case LaskuModel::KATEISLASKU :
            return QIcon(":/pic/kateinen.png");

        }
    }

    else if( role == TositeRooli)
        return lasku.tosite;
    else if( role == AvoinnaRooli)
        return lasku.avoinSnt;
    else if( role == JsonRooli )
        return lasku.json.toJson();
    else if( role == ViiteRooli)
        return lasku.viitenro;
    else if( role == AsiakasRooli)
        return lasku.asiakas;
    else if( role == LiiteRooli)
        return lasku.json.str("Liite");
    else if( role == HyvitysLaskuRooli)
        return lasku.json.luku("Hyvityslasku");
    else if( role == KirjausPerusteRooli)
        return lasku.kirjausperuste;

    return QVariant();
}

QVariant LaskutModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignCenter);
    else if( role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section) {
        case NUMERO: return tr("Viitenumero");
        case PVM: return tr("Laskun pvm");
        case ERAPVM: return tr("Eräpvm");
        case SUMMA: return tr("Summa");
        case MAKSAMATTA: return tr("Maksamatta");
        case ASIAKAS: return tr("Asiakas");
        }
    }
    return QVariant();
}

AvoinLasku LaskutModel::laskunTiedot(int indeksi) const
{
    return laskut.value(indeksi);
}

void LaskutModel::lataaAvoimet()
{
    paivita( AVOIMET) ;
}

void LaskutModel::paivita(int valinta, QDate mista, QDate mihin)
{
    QString kysely = "SELECT id, laskupvm, erapvm, summaSnt, avoinSnt, asiakas, tosite, kirjausperuste, json from lasku";

    QStringList ehdot;
    if(valinta == AVOIMET)
        ehdot.append("avoinsnt > 0");
    if(valinta == ERAANTYNEET)
        ehdot.append(QString("avoinsnt > 0 AND erapvm < '%1' ").arg( kp()->paivamaara().toString(Qt::ISODate) ) );
    if(mista.isValid())
        ehdot.append(QString("laskupvm >= '%1'").arg(mista.toString(Qt::ISODate)));
    if(mihin.isValid())
        ehdot.append(QString("laskupvm <= '%1'").arg(mihin.toString(Qt::ISODate)));
    if(!ehdot.isEmpty())
        kysely.append(" WHERE " +  ehdot.join(" AND ") );

    beginResetModel();
    laskut.clear();
    QSqlQuery query;
    query.exec( kysely );

    while( query.next())
    {
        AvoinLasku lasku;
        lasku.viitenro = query.value("id").toInt();
        lasku.pvm = query.value("laskupvm").toDate();
        lasku.erapvm = query.value("erapvm").toDate();
        lasku.summaSnt = query.value("summaSnt").toInt();
        lasku.avoinSnt = query.value("avoinSnt").toInt();
        lasku.asiakas = query.value("asiakas").toString();
        lasku.tosite = query.value("tosite").toInt();
        lasku.kirjausperuste = query.value("kirjausperuste").toInt();
        lasku.json.fromJson( query.value("json").toByteArray() );
        laskut.append(lasku);
    }
    endResetModel();
}

void LaskutModel::maksa(int indeksi, int senttia)
{
    laskut[indeksi].avoinSnt -= senttia;
    if( laskut.value(indeksi).avoinSnt == 0)
    {
        beginRemoveRows(QModelIndex(), indeksi, indeksi);
        laskut.removeAt(indeksi);
        endRemoveRows();
    }
}
