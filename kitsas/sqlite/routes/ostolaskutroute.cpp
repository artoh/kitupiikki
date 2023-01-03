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

#include "model/euro.h"

#include <QDebug>

OstolaskutRoute::OstolaskutRoute(SQLiteModel *model)
    : SQLiteRoute(model, "/ostolaskut")
{

}

QString OstolaskutRoute::sqlKysymys(const QUrlQuery &urlquery, bool hyvitys) const {

    QString kysymys("select tosite.id as tosite, tosite.viite as viite,tosite.laskupvm as pvm, tosite.erapvm as erapvm, "
                    "COALESCE(kreditsnt,0) - COALESCE(debetsnt,0) as summasnt, q.avoinsnt AS avoinsnt, kumppani.nimi as toimittaja, kumppani.id as toimittajaid, vienti.eraid as eraid, vienti.tili as tili, "
                    "vienti.selite as selite, tosite.tunniste as tunniste, tosite.sarja as sarja, tosite.tyyppi as tyyppi, tosite.pvm as tositepvm "
                    "from Tosite JOIN "
                    "Vienti ON vienti.tosite=tosite.id ");

    if( !urlquery.hasQueryItem("avoin") && !urlquery.hasQueryItem("eraantynyt"))
        kysymys.append(" LEFT OUTER ");

    kysymys.append("JOIN ( SELECT eraid, COALESCE(SUM(kreditsnt),0) - COALESCE(SUM(debetsnt),0) as avoinsnt FROM Vienti JOIN Tosite ON Vienti.tosite=Tosite.id WHERE Tosite.tila >= 100 ");

    if( urlquery.hasQueryItem("saldopvm"))
        kysymys.append(QString(" AND Vienti.pvm <= '%1' ").arg(urlquery.queryItemValue("saldopvm")));

    kysymys.append("GROUP BY eraid");

    if( urlquery.hasQueryItem("avoin") || urlquery.hasQueryItem("eraantynyt"))
        kysymys.append(QString(" HAVING COALESCE(SUM(kreditsnt),0) %1 COALESCE(SUM(debetsnt),0) ")
                .arg( urlquery.queryItemValue("avoin")=="maksut" ?  ">" : "<>" ));
    kysymys.append(QString(")  AS q  ON vienti.eraid=q.eraid LEFT OUTER JOIN "
                           "Kumppani ON vienti.kumppani=kumppani.id "
                           "WHERE vienti.tyyppi=%1 AND tosite.tila >= %2")
                           .arg(hyvitys ? TositeVienti::MYYNTI + TositeVienti::VASTAKIRJAUS : TositeVienti::OSTO + TositeVienti::VASTAKIRJAUS)
                           .arg(Tosite::KIRJANPIDOSSA) );

    if( urlquery.hasQueryItem("alkupvm"))
        kysymys.append(QString(" AND tosite.laskupvm >= '%1' ")
                       .arg(urlquery.queryItemValue("alkupvm")));
    if( urlquery.hasQueryItem("loppupvm"))
        kysymys.append(QString(" AND tosite.laskupvm <= '%1' ")
                       .arg(urlquery.queryItemValue("loppupvm")));

    if( urlquery.hasQueryItem("eraalkupvm"))
        kysymys.append(QString(" AND tosite.erapvm >= '%1' ")
                       .arg(urlquery.queryItemValue("eraalkupvm")));
    if( urlquery.hasQueryItem("eraloppupvm")) {
        if( urlquery.hasQueryItem("eraantynyt"))
            kysymys.append(QString(" AND tosite.erapvm < '%1' ")
                       .arg(urlquery.queryItemValue("eraloppupvm")));
        else
            kysymys.append(QString(" AND tosite.erapvm <= '%1' ")
                       .arg(urlquery.queryItemValue("eraloppupvm")));
    }
    if( urlquery.queryItemValue("avoin")=="maksut")
        kysymys.append(" AND Vienti.eraid=Vienti.id ");

    kysymys.append(" ORDER BY pvm, viite");
    return kysymys;
}


QVariant OstolaskutRoute::get(const QString &/*polku*/, const QUrlQuery &urlquery)
{

    QSqlQuery kysely(db());
    kysely.exec(sqlKysymys(urlquery, false));
    QVariantList lista = resultList(kysely);

    if( urlquery.queryItemValue("avoin") == "maksut") {
        kysely.exec(sqlKysymys(urlquery, true));
        lista.append(resultList(kysely));
    }

    return lista;
}
