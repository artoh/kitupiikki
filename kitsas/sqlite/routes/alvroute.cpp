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
#include "alvroute.h"
#include "db/tositetyyppimodel.h"
#include "model/euro.h"

#include <QDate>

AlvRoute::AlvRoute(SQLiteModel *model)
    : SQLiteRoute(model, "/alv")
{

}

QVariant AlvRoute::get(const QString &polku, const QUrlQuery &urlquery)
{
    if(polku == "eu")
        return eu(urlquery);

    QSqlQuery kysely(db());

    kysely.exec( QString("SELECT id, json FROM Tosite WHERE tyyppi=%1 AND tila>=100 "
                         "ORDER BY pvm DESC").arg(TositeTyyppi::ALVLASKELMA));

    QVariantList kyselyntulos = resultList(kysely);
    QVariantList vastaus;

    for(const QVariant& item : qAsConst( kyselyntulos )) {
        QVariantMap map = item.toMap();
        QVariantMap vmap = map.value("alv").toMap();
        vmap.remove("koodit");
        vmap.insert("id", map.value("id"));
        vastaus.append(vmap);
    }
    return vastaus;
}

QVariant AlvRoute::eu(const QUrlQuery &urlquery)
{
    QDate pvm = QDate::fromString(urlquery.queryItemValue("pvm"), Qt::ISODate);
    QDate alkupvm = QDate(pvm.year(), pvm.month(), 1).addMonths(-1);
    QDate loppupvm = alkupvm.addMonths(1).addDays(-1);

    QSqlQuery kysely(db());

    // Check if a yhteenvetoilmoitus already exists for the period
    kysely.prepare("SELECT id FROM Tosite WHERE tyyppi=9110 AND pvm>=:alkupvm AND pvm<=:loppupvm AND tila>=100");
    kysely.bindValue(":alkupvm", alkupvm.toString(Qt::ISODate));
    kysely.bindValue(":loppupvm", loppupvm.toString(Qt::ISODate));
    kysely.exec();
    if(kysely.next()) {
        QVariantMap map;
        map.insert("tosite", true);
        return map;
    }

    // Get EU sales data grouped by customer
    // Use toimituspvm from invoice JSON when available (determines the VAT period
    // for accrual-based accounting), falling back to Vienti.pvm
    kysely.prepare("SELECT k.nimi, k.alvtunnus, "
                   "SUM(CASE WHEN v.alvkoodi=14 THEN COALESCE(v.kreditsnt,0) - COALESCE(v.debetsnt,0) ELSE 0 END) as tavarasnt, "
                   "SUM(CASE WHEN v.alvkoodi=15 THEN COALESCE(v.kreditsnt,0) - COALESCE(v.debetsnt,0) ELSE 0 END) as palvelusnt "
                   "FROM Vienti v "
                   "JOIN Tosite t ON v.tosite=t.id "
                   "JOIN Kumppani k ON v.kumppani=k.id "
                   "WHERE v.alvkoodi IN (14,15) "
                   "AND COALESCE(json_extract(t.json, '$.lasku.toimituspvm'), v.pvm) >= :alkupvm "
                   "AND COALESCE(json_extract(t.json, '$.lasku.toimituspvm'), v.pvm) <= :loppupvm "
                   "AND t.tila>=100 "
                   "GROUP BY k.id "
                   "ORDER BY k.alvtunnus");
    kysely.bindValue(":alkupvm", alkupvm.toString(Qt::ISODate));
    kysely.bindValue(":loppupvm", loppupvm.toString(Qt::ISODate));
    kysely.exec();

    QVariantList ilmoitus;
    while(kysely.next()) {
        QVariantMap rivi;
        rivi.insert("nimi", kysely.value("nimi"));
        rivi.insert("alvtunnus", kysely.value("alvtunnus"));
        rivi.insert("tavara", Euro(kysely.value("tavarasnt").toLongLong()).toString());
        rivi.insert("palvelu", Euro(kysely.value("palvelusnt").toLongLong()).toString());
        ilmoitus.append(rivi);
    }

    QVariantMap vastaus;
    vastaus.insert("pvm", loppupvm.toString(Qt::ISODate));
    vastaus.insert("ilmoitus", ilmoitus);
    return vastaus;
}
