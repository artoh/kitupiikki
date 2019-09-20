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
