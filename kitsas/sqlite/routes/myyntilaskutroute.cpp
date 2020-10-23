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
#include "myyntilaskutroute.h"

#include "model/tositevienti.h"
#include "model/tosite.h"

#include <QJsonDocument>
#include <QDebug>

MyyntilaskutRoute::MyyntilaskutRoute(SQLiteModel *model)
    : SQLiteRoute(model, "/myyntilaskut")
{

}

QVariant MyyntilaskutRoute::get(const QString &/*polku*/, const QUrlQuery &urlquery)
{
    // Laskutapa on json:n sisällä !


    QString kysymys("select tosite.id as tosite, tosite.laskupvm as pvm, tosite.erapvm as erapvm, tosite.viite, tosite.json as json, "
                        "debetsnt as debetia, kreditsnt as kreditia, ds, ks, kumppani.nimi as asiakas, kumppani.id as asiakasid, vienti.eraid as eraid, vienti.tili as tili,"
                        "tosite.tyyppi as tyyppi, vienti.selite as selite, tosite.tunniste as tunniste, tosite.sarja as sarja, tosite.tila as tila, tosite.pvm as tositepvm  "
                        "FROM tosite JOIN Vienti ON vienti.tosite=tosite.id ");

    if( !urlquery.hasQueryItem("avoin") && !urlquery.hasQueryItem("eraantynyt"))
        kysymys.append("LEFT OUTER ");

    kysymys.append("JOIN (select eraid, sum(debetsnt) as ds, sum(kreditsnt) as ks FROM Vienti JOIN Tosite ON Vienti.tosite=Tosite.id WHERE Tosite.tila >= 100 ");

    if( urlquery.hasQueryItem("saldopvm"))
        kysymys.append(QString(" AND Vienti.pvm <= '%1' ").arg(urlquery.queryItemValue("saldopvm")));

    kysymys.append(" GROUP BY eraid ");
    if( urlquery.hasQueryItem("avoin") || urlquery.hasQueryItem("eraantynyt"))
        kysymys.append("HAVING SUM(kreditsnt) <> SUM(debetsnt) OR sum(kreditsnt) IS NULL ");

    kysymys.append(QString(") as q ON vienti.eraid=q.eraid LEFT OUTER JOIN "
            "Kumppani ON vienti.kumppani=kumppani.id WHERE vienti.tyyppi = %1"
            " AND ( tosite.tila ").arg( TositeVienti::MYYNTI + TositeVienti::VASTAKIRJAUS) );

    if( urlquery.hasQueryItem("luonnos"))
        kysymys.append(QString(" = %1 ").arg( Tosite::LUONNOS ));
    else if( urlquery.hasQueryItem("lahetettava"))
        kysymys.append(QString(" = %1 OR tosite.tila = %2 OR tosite.tila = %3 ").arg( Tosite::VALMISLASKU ).arg(Tosite::LAHETETAAN).arg(Tosite::LAHETYSVIRHE));
    else
        kysymys.append(QString(" >= %1 ").arg( Tosite::KIRJANPIDOSSA ));
    kysymys.append(") ");

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

    if( urlquery.hasQueryItem("kitsaslaskut"))
        kysymys.append(" AND tosite.tyyppi >= 210 AND tosite.tyyppi <= 219 ");

    kysymys.append(" ORDER BY tosite.laskupvm, tosite.viite");

    qDebug() << kysymys;

    QSqlQuery kysely( db());
    kysely.exec(kysymys);

    QVariantList lista = resultList(kysely);
    for(int i=0; i < lista.count(); i++) {
        QVariantMap map = lista.at(i).toMap();
        double ds = map.take("ds").toLongLong() / 100.0;
        double ks = map.take("ks").toLongLong() / 100.0;

        QVariantMap laskumap = map.take("lasku").toMap();
        if( laskumap.contains("laskutapa"))
            map.insert("laskutapa", laskumap.value("laskutapa"));
        if( laskumap.contains("numero"))
            map.insert("numero", laskumap.value("numero"));
        if( laskumap.contains("maksutapa"))
            map.insert("maksutapa", laskumap.value("maksutapa"));

        map.insert("avoin", ds - ks);
        map.insert("summa", (map.take("debetia").toLongLong() - map.take("kreditia").toLongLong()) / 100.0);
        lista[i] = map;
    }

    return lista;
}
