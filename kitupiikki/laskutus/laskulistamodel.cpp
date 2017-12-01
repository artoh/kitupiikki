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

#include "laskulistamodel.h"
#include "db/kirjanpito.h"

LaskulistaModel::LaskulistaModel(QObject *parent)
    : QSqlQueryModel(parent)
{

}

QVariant LaskulistaModel::data(const QModelIndex &item, int role) const
{
    if( !item.isValid())
        return QVariant();

    if( role == Qt::DisplayRole)
    {
        QVariant data = QSqlQueryModel::data(item, role);
        if( item.column() == PVM || item.column() == ERAPVM)
            return QDate::fromString( data.toString(), Qt::ISODate).toString(Qt::SystemLocaleShortDate);
        else if( item.column() == SUMMA || item.column() == MAKSAMATTA)
        {
            int sentit = data.toInt();
            if( sentit )
                return QString("%L1 €").arg(sentit / 100.0,0,'f',2);
            else
                return QVariant();
        }
    }
    else if( role == Qt::TextAlignmentRole)
    {
        if( item.column() == SUMMA || item.column() == MAKSAMATTA )
            return QVariant( Qt::AlignRight | Qt::AlignVCenter);
    }

    return QSqlQueryModel::data(item, role);
}

void LaskulistaModel::paivita(int valinta, QDate mista, QDate mihin)
{
    QString kysely = "SELECT id, laskupvm, erapvm, summaSnt, avoinSnt, tosite, asiakas from lasku";

    QStringList ehdot;
    if(valinta == AVOIMET)
        ehdot.append("avoinsnt > 0");
    if(valinta == ERAANTYNEET)
        ehdot.append(QString("avoinsnt > 0 AND erapvm < '%1'").arg( kp()->paivamaara().toString(Qt::ISODate) ) );
    if(mista.isValid())
        ehdot.append(QString("laskupvm >= '%1'").arg(mista.toString(Qt::ISODate)));
    if(mihin.isValid())
        ehdot.append(QString("laskupvm <= '%1'").arg(mihin.toString(Qt::ISODate)));
    if(!ehdot.isEmpty())
        kysely.append(" WHERE " +  ehdot.join(" AND ") );

    setQuery(kysely);

    setHeaderData(NUMERO, Qt::Horizontal, tr("Numero"));
    setHeaderData(PVM, Qt::Horizontal, tr("Laskun pvm"));
    setHeaderData(ERAPVM, Qt::Horizontal, tr("Eräpvm"));
    setHeaderData(SUMMA, Qt::Horizontal, tr("Summa"));
    setHeaderData(MAKSAMATTA, Qt::Horizontal, tr("Maksamatta"));
    setHeaderData(ASIAKAS, Qt::Horizontal, tr("Asiakas"));

}
