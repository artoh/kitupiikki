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
#include "tilikaudetroute.h"
#include <QDate>

TilikaudetRoute::TilikaudetRoute(SQLiteModel *model) :
    SQLiteRoute(model, "/tilikaudet")
{

}

QVariant TilikaudetRoute::get(const QString &/*polku*/, const QUrlQuery &/*urlquery*/)
{
    QSqlQuery kysely(db());
    kysely.exec("SELECT alkaa,loppuu,json FROM Tilikausi ORDER BY alkaa");

    QVariantList list = resultList(kysely);
    for(int i=0; i < list.count(); i++){
        QVariantMap map = list.at(i).toMap();

        // Tase
        kysely.exec( QString("SELECT sum(debet), sum(kredit) FROM vienti WHERE pvm BETWEEN '%1' AND '%2' AND CAST(tili as text) < '3'")
                     .arg(map.value("alkaa").toString())
                     .arg(map.value("loppuu").toString()) );
        if( kysely.next())
            map.insert("tase", qMax( kysely.value(0).toDouble(), kysely.value(1).toDouble()));
        // Tulos
        kysely.exec( QString("SELECT sum(kredit), sum(debet) FROM vienti WHERE pvm BETWEEN '%1' AND '%2' AND CAST(tili as text) >= '3'")
                     .arg(map.value("alkaa").toString())
                     .arg(map.value("loppuu").toString()) );
        if( kysely.next())
            map.insert("tulos", QString::number(kysely.value(0).toDouble() - kysely.value(1).toDouble(), 'f', 2));

        // Liikevaihto
        kysely.exec( QString("SELECT sum(kredit), sum(debet) FROM vienti "
                             "JOIN Tili ON Vienti.tili=Tili.numero "
                             "WHERE pvm BETWEEN '%1' AND '%2' AND CAST(tili as text) >= '3' "
                             "AND (tili.tyyppi='CL' OR tili.tyyppi='CLX')")
                     .arg(map.value("alkaa").toString())
                     .arg(map.value("loppuu").toString()) );
        if( kysely.next())
            map.insert("liikevaihto", QString::number(kysely.value(0).toDouble() - kysely.value(1).toDouble(), 'f', 2));

        // Viimeiselle kaudelle viimeinen tosite
        if( i == list.count() - 1) {
            kysely.exec(QString("SELECT MAX(pvm) FROM Tosite WHERE pvm >= '%1'")
                        .arg(map.value("alkaa").toString()) );
            if( kysely.next())
                map.insert("viimeinen", kysely.value(0));
        }

        list[i] = map;
    }

    return list;
}

QVariant TilikaudetRoute::put(const QString &polku, const QVariant &data)
{
    QVariantMap map = data.toMap();
    QDate alkaa = QDate::fromString(polku, Qt::ISODate);
    QDate loppuu = map.take("loppuu").toDate();
    map.remove("alkaa");

    QSqlQuery kysely(db());
    kysely.prepare("INSERT INTO Tilikausi (alkaa,loppuu,json) VALUES (?,?,?) "
                   "ON CONFLICT (alkaa) DO UPDATE SET loppuu=EXCLUDED.loppuu, json=EXCLUDED.json");
    kysely.addBindValue(alkaa);
    kysely.addBindValue(loppuu);
    kysely.addBindValue( mapToJson(map) );
    kysely.exec();

    return QVariant();
}

QVariant TilikaudetRoute::doDelete(const QString &polku)
{
    QDate alkaa = QDate::fromString(polku, Qt::ISODate);
    db().exec(QString("DELETE FROM Tilikausi WHERE alkaa='%1'").arg(alkaa.toString(Qt::ISODate)));
    return QVariant();
}
