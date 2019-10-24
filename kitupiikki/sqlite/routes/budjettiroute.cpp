/*
   Copyright (C) 2019 Arto Hyvättinen

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
#include "budjettiroute.h"

#include <QDebug>

BudjettiRoute::BudjettiRoute(SQLiteModel *model)
    : SQLiteRoute(model, "/budjetti")
{

}

QVariant BudjettiRoute::get(const QString &polku, const QUrlQuery &urlquery)
{
    QSqlQuery kysely(db());
    QVariantMap vastaus;

    if( urlquery.hasQueryItem("kohdennus")) {
        kysely.exec(QString("SELECT tili,euro FROM Budjetti WHERE tilikausi='%1' AND kohdennus=%2 "
                            "ORDER BY CAST(tili AS text)")
                    .arg(polku).arg(urlquery.queryItemValue("kohdennus")));
        while( kysely.next())
            vastaus.insert( kysely.value(0).toString(), kysely.value(1).toDouble() );
    } else if( urlquery.hasQueryItem("kohdennukset")) {
        kysely.exec(QString("SELECT kohdennus, tili, euro FROM Budjetti WHERE tilikausi='%1' "
                            "ORDER BY CAST(tili AS text) ")
                    .arg(polku));
        while( kysely.next() ) {
            QString kohdennus = kysely.value(0).toString();
            QVariantMap kmap = vastaus.value(kohdennus, QVariantMap()).toMap();
            kmap.insert(kysely.value(1).toString(), kysely.value(2).toDouble());
            vastaus.insert(kohdennus, kmap);
        }
    } else {
        kysely.exec(QString("SELECT tili, SUM(euro) as summa FROM Budjetti "
                            "WHERE tilikausi='%1' GROUP BY tili ORDER BY CAST(tili as text)")
                    .arg(polku) );
        while( kysely.next())
            vastaus.insert(kysely.value(0).toString(), kysely.value(1).toDouble());
    }
    return vastaus;
}

QVariant BudjettiRoute::put(const QString &polku, const QVariant &data)
{
    QSqlQuery kysely(db());
    db().transaction();

    kysely.exec(QString("DELETE FROM Budjetti WHERE tilikausi='%1'")
                .arg(polku));

    kysely.prepare("INSERT INTO Budjetti(tilikausi,kohdennus,tili, euro) VALUES(?,?,?,?)");

    QVariantMap map = data.toMap();
    QMapIterator<QString,QVariant> paaIter(map);

    while( paaIter.hasNext()) {
        paaIter.next();
        QVariantMap alimap = paaIter.value().toMap();
        QMapIterator<QString,QVariant> aliIter( alimap );
        while( aliIter.hasNext()) {
            aliIter.next();
            kysely.addBindValue(polku);
            kysely.addBindValue(paaIter.key().toInt());
            kysely.addBindValue(aliIter.key().toInt());
            kysely.addBindValue(aliIter.value().toDouble());
            if( !kysely.exec() ) {
                db().rollback();
                throw SQLiteVirhe(kysely);
            }
        }
    }
    db().commit();
    return QVariant();
}
