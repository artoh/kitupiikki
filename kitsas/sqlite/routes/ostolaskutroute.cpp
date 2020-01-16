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
    QString kysymys("select tosite.id as tosite, vienti.viite as viite,vienti.laskupvm as pvm, vienti.erapvm as erapvm, "
                    "kreditsnt as summasnt, ds, ks, kumppani.nimi as toimittaja, kumppani.id as toimittajaid, vienti.eraid as eraid, vienti.tili as tili, "
                    "vienti.selite as selite, tosite.tunniste as tunniste, tosite.sarja as sarja, tosite.tyyppi as tyyppi "
                    "from Tosite JOIN "
                    "Vienti ON vienti.tosite=tosite.id ");

    if( !urlquery.hasQueryItem("avoin") && !urlquery.hasQueryItem("eraantynyt"))
        kysymys.append(" LEFT OUTER ");

    kysymys.append("JOIN ( SELECT eraid, sum(debetsnt) as ds, sum(kreditsnt) as ks FROM Vienti JOIN Tosite ON Vienti.tosite=Tosite.id WHERE Tosite.tila >= 100 ");

    if( urlquery.hasQueryItem("saldopvm"))
        kysymys.append(QString(" AND Vienti.pvm <= '%1' ").arg(urlquery.queryItemValue("saldopvm")));

    kysymys.append("GROUP BY eraid");

    if( urlquery.hasQueryItem("avoin") || urlquery.hasQueryItem("eraantynyt"))
        kysymys.append(" HAVING sum(kreditsnt) <> sum(debetsnt) OR sum(debetsnt) IS NULL ");
    kysymys.append(QString(")  AS q  ON vienti.eraid=q.eraid LEFT OUTER JOIN "
                           "Kumppani ON vienti.kumppani=kumppani.id "
                           "WHERE vienti.tyyppi=%1 AND tosite.tila >= %2")
                           .arg(TositeVienti::OSTO + TositeVienti::VASTAKIRJAUS)
                           .arg(Tosite::KIRJANPIDOSSA) );

    if( urlquery.hasQueryItem("eraantynyt"))
        kysymys.append(" AND vienti.erapvm < current_date ");    

    if( urlquery.hasQueryItem("alkupvm"))
        kysymys.append(QString(" AND vienti.laskupvm >= '%1' ")
                       .arg(urlquery.queryItemValue("alkupvm")));    
    if( urlquery.hasQueryItem("loppupvm"))
        kysymys.append(QString(" AND vienti.laskupvm <= '%1' ")
                       .arg(urlquery.queryItemValue("loppupvm")));
    kysymys.append(" ORDER BY pvm, viite");

    if( urlquery.hasQueryItem("eraalkupvm"))
        kysymys.append(QString(" AND vienti.erapvm >= '%1' ")
                       .arg(urlquery.queryItemValue("eraalkupvm")));
    if( urlquery.hasQueryItem("eraloppupvm"))
        kysymys.append(QString(" AND vienti.erapvm <= '%1' ")
                       .arg(urlquery.queryItemValue("eraloppupvm")));

    QSqlQuery kysely(db());
    kysely.exec(kysymys);

    QVariantList lista = resultList(kysely);

    for(int i=0; i < lista.count(); i++) {
        QVariantMap map = lista.at(i).toMap();
        double ds = map.take("ds").toLongLong() / 100.0;
        double ks = map.take("ks").toLongLong() / 100.0;
        map.insert("avoin", ks - ds);
        lista[i] = map;
    }


    return lista;
}
