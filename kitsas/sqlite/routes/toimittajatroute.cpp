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
#include "toimittajatroute.h"

ToimittajatRoute::ToimittajatRoute(SQLiteModel *model)
    : SQLiteRoute(model, "/toimittajat")
{

}

QVariant ToimittajatRoute::get(const QString &/*polku*/, const QUrlQuery &/*urlquery*/)
{
    QSqlQuery kysely(db());
    kysely.exec("select kumppani.id, kumppani.nimi, sum(summa.kreditsnt) as summasnt, sum(avoin.sd) as avd, sum(avoin.sk) as avk, sum(vanha.sd) as vad, sum(vanha.sk) as vak from "
            "Kumppani JOIN ( select kreditsnt, vienti.kumppani, vienti.id as vienti FROM "
            "Vienti JOIN Tosite ON vienti.tosite=tosite.id WHERE vienti.tyyppi=102 AND tosite.tila > 0) as summa ON summa.kumppani = kumppani.id "
            "LEFT OUTER JOIN ( select eraid,sum(debetsnt) as sd, SUM(kreditsnt) AS sk FROM Vienti GROUP BY eraid) AS avoin ON avoin.eraid = summa.vienti LEFT OUTER JOIN ( SELECT a.eraid as eraid, SUM(a.kreditsnt) AS sk, SUM(a.debetsnt) as sd FROM "
            "Vienti as a JOIN Vienti as b ON a.eraid=b.id JOIN tosite AS tb ON tb.id=b.tosite "
            "WHERE tb.erapvm < current_date GROUP BY a.eraid) AS vanha ON vanha.eraid = summa.vienti group by kumppani.id order by kumppani.nimi ");
    QVariantList lista = resultList(kysely);

    for(int i=0; i < lista.count(); i++) {
        QVariantMap map = lista.at(i).toMap();
        double avd = map.take("avd").toLongLong();
        double avk = map.take("avk").toLongLong();
        map.insert("avoin",  (avk - avd) / 100.0 );
        double evd = map.take("vad").toLongLong();
        double evk = map.take("vak").toLongLong();
        map.insert("eraantynyt", (evk - evd) / 100.0);
        lista[i] = map;
    }
    return lista;
}
