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
#include "ryhmatroute.h"


RyhmatRoute::RyhmatRoute(SQLiteModel *model) :
    SQLiteRoute(model, "/ryhmat")
{

}

QVariant RyhmatRoute::get(const QString &polku, const QUrlQuery &/*urlquery*/)
{
    QSqlQuery kysely(db());

    if( polku.isEmpty()) {
        kysely.exec("SELECT id, nimi FROM Ryhma ORDER BY nimi");
        return resultList(kysely);
    }

    kysely.exec(QString("SELECT * FROM Ryhma WHERE id=%1").arg(polku.toInt()));
    return resultMap(kysely);
}

QVariant RyhmatRoute::post(const QString &/*polku*/, const QVariant &data)
{
    QVariantMap map = data.toMap();
    QVariantMap kopio(map);

    QSqlQuery kysely(db());    
    kysely.prepare("INSERT INTO Ryhma(nimi,json) VALUES (?,?)");
    kysely.addBindValue(map.take("nimi"));
    kysely.addBindValue(mapToJson(map));
    if(!kysely.exec()) {        
        throw SQLiteVirhe(kysely);
    }
    kopio.insert("id", kysely.lastInsertId());
    return kopio;
}

QVariant RyhmatRoute::put(const QString &polku, const QVariant &data)
{
    QVariantMap map = data.toMap();
    QVariantMap kopio(map);
    map.remove("id");

    QSqlQuery kysely(db());
    kysely.prepare("INSERT INTO Ryhma(id, nimi,json) VALUES (?,?,?)"
                   "ON CONFLICT (id) DO UPDATE SET nimi=EXCLUDED.nimi, json=EXCLUDED.json");
    kysely.addBindValue(polku.toInt());
    kysely.addBindValue(map.take("nimi"));
    kysely.addBindValue(mapToJson(map));
    if(!kysely.exec()) {        
        throw SQLiteVirhe(kysely);
    }
    return kopio;
}

QVariant RyhmatRoute::doDelete(const QString &polku)
{
    QSqlQuery kysely(db());
    kysely.exec(QString("DELETE FROM Ryhma WHERE id=%1").arg(polku.toInt()));
    return QVariant();
}
