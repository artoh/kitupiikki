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
#include "initroute.h"
#include <QSqlQuery>

InitRoute::InitRoute(SQLiteModel *model) :
    SQLiteRoute(model,"/init")
{

}

QVariant InitRoute::get(const QString & /*polku*/, const QUrlQuery& /*urlquery*/)
{
    QVariantMap map;
    // Asetukset
    QSqlQuery kysely(db());
    kysely.exec("SELECT avain,arvo FROM Asetus");

    QVariantMap asetukset;

    while( kysely.next()) {
        asetukset.insert( kysely.value(0).toString(),
                          kysely.value(1).toString());
    }
    map.insert("asetukset", asetukset);


    // Tilit

    kysely.exec( "select numero,tyyppi,json from ( select  cast(numero as text) as numero,'H'||taso as tyyppi,json,taso from otsikko "
                 " union select cast (numero as text),tyyppi,json,null from tili order by numero,taso) as sub");
    map.insert("tilit", resultList(kysely));


    // Kohdennukset
    kysely.exec("select * from Kohdennus");
    map.insert("kohdennukset", resultList(kysely));

    // Tilikaudet
    kysely.exec("select * from Tilikausi order by alkaa");
    map.insert("tilikaudet", resultList(kysely));

    return map;
}
