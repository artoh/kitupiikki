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
#include "ostolaskutroute.h"

#include "model/tosite.h"
#include "model/tositevienti.h"

OstolaskutRoute::OstolaskutRoute(SQLiteModel *model)
    : SQLiteRoute(model, "/ostolaskut")
{

}

QVariant OstolaskutRoute::get(const QString &/*polku*/, const QUrlQuery &urlquery)
{
    QString kysymys("select tosite.id as tosite, vienti.viite as viite,vienti.pvm as pvm, vienti.erapvm as erapvm, "
                    "kredit as summa, ds, ks, kumppani.nimi as toimittaja, kumppani.id as toimittajaid, vienti.eraid as eraid, vienti.tili as tili "
                    "from Tosite JOIN "
                    "Vienti ON vienti.tosite=tosite.id ");

    if( !urlquery.hasQueryItem("avoin") && !urlquery.hasQueryItem("eraantynyt"))
        kysymys.append(" LEFT OUTER ");

    kysymys.append("JOIN ( SELECT eraid, sum(debet) as ds, sum(kredit) as ks FROM Vienti GROUP BY eraid");

    if( urlquery.hasQueryItem("avoin"))
        kysymys.append(" HAVING sum(kredit) <> sum(debet) OR sum(debet) IS NULL ");
    kysymys.append(QString(")  AS q  ON vienti.eraid=q.eraid LEFT OUTER JOIN "
                           "Kumppani ON vienti.kumppani=kumppani.id "
                           "WHERE vienti.tyyppi=%1 AND tosite.tila >= %2")
                           .arg(TositeVienti::OSTO + TositeVienti::VASTAKIRJAUS)
                           .arg(Tosite::KIRJANPIDOSSA) );

    if( urlquery.hasQueryItem("eraantynyt"))
        kysymys.append(" AND vienti.erapvm < current_date ");
    kysymys.append(" ORDER BY pvm, viite");

    QSqlQuery kysely(db());
    kysely.exec(kysymys);

    QVariantList lista = resultList(kysely);

    for(int i=0; i < lista.count(); i++) {
        QVariantMap map = lista.at(i).toMap();
        double ds = map.take("ds").toDouble();
        double ks = map.take("ks").toDouble();
        map.insert("avoin", ds - ks);
        lista[i] = map;
    }


    return lista;
}
