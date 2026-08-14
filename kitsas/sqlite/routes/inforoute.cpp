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
#include "inforoute.h"

#include "db/kirjanpito.h"
#include "sql/sqlmodel.h"

#include "model/tosite.h"

InfoRoute::InfoRoute(SqlModel *model) :
    SQLiteRoute(model,"/info")
{

}

QVariant InfoRoute::get(const QString &/*polku*/, const QUrlQuery &/*urlquery*/)
{
    QVariantMap map;
    if( auto *sql = qobject_cast<SqlModel*>(kp()->yhteysModel()) )
        map.insert("koko", sql->tietokannanKoko());

    QSqlQuery kysely(db());
    kysely.exec("SELECT COUNT(id) FROM Tosite");
    if(kysely.next())
        map.insert("tositteita", kysely.value(0).toInt());

    kysely.exec(QString("SELECT COUNT(Tosite.id) AS lkm FROM Tosite WHERE tila=%1").arg(Tosite::VALMISLASKU) );
    if( kysely.next())
        map.insert("lahetettavia", kysely.value(0).toInt());

    kysely.exec(QString("SELECT COUNT(Tosite.id) AS lkm FROM Tosite WHERE tila=%1").arg(Tosite::SAAPUNUT));
    if(kysely.next())
        map.insert("tyolista", kysely.value(0).toInt());

    return map;

}
