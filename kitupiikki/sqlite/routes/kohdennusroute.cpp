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
#include "kohdennusroute.h"

KohdennusRoute::KohdennusRoute(SQLiteModel *model)
    : SQLiteRoute(model, "/kohdennukset")
{

}

QVariant KohdennusRoute::get(const QString &/*polku*/, const QUrlQuery &/*urlquery*/)
{
    QSqlQuery kysely( db() );

    kysely.exec("SELECT id, tyyppi, json, kt.lkm + b.lkm as lkm FROM Kohdennus "
                "LEFT OUTER JOIN (SELECT kohdennus, COUNT(id) as lkm FROM "
                "Vienti GROUP BY kohdennus) AS kt ON kt.kohdennus=id "
                "LEFT OUTER JOIN ( SELECT kohdennus, COUNT(vienti) as lkm FROM Merkkaus GROUP BY kohdennus) AS b ON b.kohdennus=id"
                );

    return resultList(kysely);
}

QVariant KohdennusRoute::post(const QString &/*polku*/, const QVariant &data)
{
    QSqlQuery kysely(db());
    QVariantMap map = data.toMap();
    QVariantMap jemma = map;

    kysely.prepare("INSERT INTO Kohdennus (tyyppi,json) VALUES (?,?)");
    kysely.addBindValue( map.take("tyyppi").toString() );
    kysely.addBindValue( mapToJson(map) );
    kysely.exec();

    jemma.insert("id", kysely.lastInsertId().toInt());
    return jemma;
}

QVariant KohdennusRoute::put(const QString &polku, const QVariant &data)
{
    QSqlQuery kysely(db());
    QVariantMap map = data.toMap();
    QVariantMap jemma = map;

    kysely.prepare("UPDATE Kohdennus SET tyyppi=?, json=? WHERE id=?");
    kysely.addBindValue( map.take("tyyppi").toString() );
    kysely.addBindValue( mapToJson(map) );
    kysely.addBindValue(polku.toInt());
    kysely.exec();
    return QVariant();
}

QVariant KohdennusRoute::doDelete(const QString &polku)
{
    db().exec(QString("DELETE FROM Kohdennus WHERE id=%1").arg(polku.toInt()));
    return QVariant();
}
