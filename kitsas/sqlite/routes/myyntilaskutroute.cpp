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
    QString ehdot = " AND ( tosite.tila ";

    if( urlquery.hasQueryItem("luonnos"))
        ehdot.append(QString(" = %1 ").arg( Tosite::LUONNOS ));
    else if( urlquery.hasQueryItem("lahetettava"))
        ehdot.append(QString(" = %1 OR tosite.tila = %2 OR tosite.tila = %3 ").arg( Tosite::VALMISLASKU ).arg(Tosite::LAHETETAAN).arg(Tosite::LAHETYSVIRHE));
    else
        ehdot.append(QString(" >= %1 ").arg( Tosite::KIRJANPIDOSSA ));
    ehdot.append(") ");

    if( urlquery.hasQueryItem("alkupvm"))
        ehdot.append(QString(" AND tosite.laskupvm >= '%1' ")
                       .arg(urlquery.queryItemValue("alkupvm")));
    if( urlquery.hasQueryItem("loppupvm"))
        ehdot.append(QString(" AND tosite.laskupvm <= '%1' ")
                       .arg(urlquery.queryItemValue("loppupvm")));

    if( urlquery.hasQueryItem("eraalkupvm"))
        ehdot.append(QString(" AND tosite.erapvm >= '%1' ")
                       .arg(urlquery.queryItemValue("eraalkupvm")));

    if( urlquery.hasQueryItem("eraloppupvm")) {
        if( urlquery.hasQueryItem("eraantynyt"))
            ehdot.append(QString(" AND tosite.erapvm < '%1' ")
                       .arg(urlquery.queryItemValue("eraloppupvm")));
        else
            ehdot.append(QString(" AND tosite.erapvm <= '%1' ")
                       .arg(urlquery.queryItemValue("eraloppupvm")));
    }


    QSqlQuery kysely( db());
    kysely.exec(sqlKysymys(urlquery, ehdot, false));
    QVariantList lista = resultList(kysely);

    if( urlquery.queryItemValue("avoin") == "maksut") {
        kysely.exec(sqlKysymys(urlquery, ehdot, true));
        lista.append( resultList(kysely) );
    }


    for(int i=0; i < lista.count(); i++) {
        QVariantMap map = lista.at(i).toMap();


        QVariantMap laskumap = map.take("lasku").toMap();
        if( laskumap.contains("laskutapa"))
            map.insert("laskutapa", laskumap.value("laskutapa"));
        if( laskumap.contains("numero"))
            map.insert("numero", laskumap.value("numero").toString());
        if( laskumap.contains("maksutapa"))
            map.insert("maksutapa", laskumap.value("maksutapa"));
        if( laskumap.contains("valvonta"))
            map.insert("valvonta", laskumap.value("valvonta"));

        lista[i] = map;
    }

    // Lisäksi haetaan valvomattomat laskut (Vakioviite ja Valvomaton)
    if( !urlquery.hasQueryItem("avoin") && !urlquery.hasQueryItem("eraantynyt")) {
        QString kysymys = "SELECT Tosite.id, Kumppani.id, Kumppani.nimi, Tosite.json, Tosite.tyyppi "
                   "FROM Tosite LEFT OUTER JOIN Kumppani ON Tosite.kumppani=Kumppani.id "
                  "WHERE Tosite.tyyppi >= 210 AND Tosite.tyyppi <= 219 " + ehdot;
        kysely.exec(kysymys);
        while( kysely.next()) {
            QVariantMap lasku = QJsonDocument::fromJson( kysely.value(3).toByteArray() ).toVariant().toMap().value("lasku").toMap();
            int valvonta = lasku.value("valvonta").toInt();
            if( valvonta == Lasku::VAKIOVIITE || valvonta == Lasku::VALVOMATON) {
                QVariantMap ulos;
                ulos.insert("tosite", kysely.value(0));
                ulos.insert("pvm", lasku.value("pvm"));
                ulos.insert("erapvm", lasku.value("erapvm"));
                ulos.insert("viite", lasku.value("viite"));
                ulos.insert("asiakas", kysely.value(2) );
                ulos.insert("asiakasid", kysely.value(1) );
                ulos.insert("tyyppi", kysely.value(4));
                ulos.insert("laskutapa", lasku.value("laskutapa"));
                ulos.insert("numero", lasku.value("numero"));
                ulos.insert("maksutapa", lasku.value("maksutapa"));
                ulos.insert("valvonta", lasku.value("valvonta"));
                ulos.insert("selite", lasku.value("otsikko"));
                ulos.insert("summa", lasku.value("summa"));
                lista.append(ulos);
            }
        }
    }

    return lista;
}

QString MyyntilaskutRoute::sqlKysymys(const QUrlQuery &urlquery, const QString &ehdot, bool hyvitys) const
{

    QString kysymys("select tosite.id as tosite, tosite.laskupvm as pvm, tosite.erapvm as erapvm, tosite.viite, tosite.json as json, "
                        "COALESCE(debetsnt,0) - COALESCE(kreditsnt,0) AS summasnt, avoinsnt, kumppani.nimi as asiakas, kumppani.id as asiakasid, vienti.eraid as eraid, vienti.tili as tili,"
                        "tosite.tyyppi as tyyppi, vienti.selite as selite, tosite.tunniste as tunniste, tosite.sarja as sarja, tosite.tila as tila, tosite.pvm as tositepvm  "
                        "FROM tosite JOIN Vienti ON vienti.tosite=tosite.id ");

    if( !urlquery.hasQueryItem("avoin") && !urlquery.hasQueryItem("eraantynyt"))
        kysymys.append("LEFT OUTER ");

    kysymys.append("JOIN (select eraid,  COALESCE(SUM(debetsnt),0) - COALESCE(SUM(kreditsnt),0) AS avoinsnt FROM Vienti JOIN Tosite ON Vienti.tosite=Tosite.id WHERE Tosite.tila >= 100 ");

    if( urlquery.hasQueryItem("saldopvm"))
        kysymys.append(QString(" AND Vienti.pvm <= '%1' ").arg(urlquery.queryItemValue("saldopvm")));

    kysymys.append(" GROUP BY eraid ");
    if( urlquery.hasQueryItem("avoin") || urlquery.hasQueryItem("eraantynyt"))
        kysymys.append(QString(" HAVING COALESCE(SUM(kreditsnt),0) %1 COALESCE(SUM(debetsnt),0) ")
                .arg( urlquery.queryItemValue("avoin")=="maksut" ? "<" : "<>" ));

    kysymys.append(QString(") as q ON vienti.eraid=q.eraid LEFT OUTER JOIN "
            "Kumppani ON vienti.kumppani=kumppani.id WHERE vienti.tyyppi = %1")
            .arg( hyvitys ? TositeVienti::OSTO + TositeVienti::VASTAKIRJAUS : TositeVienti::MYYNTI + TositeVienti::VASTAKIRJAUS) );


    kysymys.append(ehdot);

    if( urlquery.hasQueryItem("kitsaslaskut"))
        kysymys.append(" AND tosite.tyyppi >= 210 AND tosite.tyyppi <= 219 ");
    if( urlquery.queryItemValue("avoin") == "myynnit")
        kysymys.append(" AND Vienti.eraid=Vienti.id ");

    kysymys.append(" ORDER BY tosite.laskupvm, tosite.viite");

    return kysymys;
}
