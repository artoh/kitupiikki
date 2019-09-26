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
#include "asiakkaatroute.h"

AsiakkaatRoute::AsiakkaatRoute(SQLiteModel *model) :
    SQLiteRoute(model, "/asiakkaat")
{

}

QVariant AsiakkaatRoute::get(const QString &/*polku*/, const QUrlQuery &/*urlquery*/)
{
        QSqlQuery kysely(db());
        kysely.exec("select kumppani.id, kumppani.nimi, sum(summa.debet) as summa, sum(avoin.sd) as avd, sum(avoin.sk) as avk, sum(vanha.sd) as vad, sum(vanha.sk) as vak from "
                "Kumppani JOIN ( select debet, vienti.kumppani, vienti.id as vienti FROM "
                "Vienti JOIN Tosite ON vienti.tosite=tosite.id WHERE vienti.tyyppi=202 AND tosite.tila > 0) as summa ON summa.kumppani = kumppani.id "
                "LEFT OUTER JOIN ( select eraid,sum(debet) as sd, SUM(kredit) AS sk FROM Vienti GROUP BY eraid) AS avoin ON avoin.eraid = summa.vienti LEFT OUTER JOIN ( SELECT a.eraid as eraid, SUM(a.kredit) AS sk, SUM(a.debet) as sd FROM "
                "Vienti as a JOIN Vienti as b ON a.eraid=b.id WHERE b.erapvm < current_date GROUP BY a.eraid) AS vanha ON vanha.eraid = summa.vienti group by kumppani.id order by kumppani.nimi ");
        QVariantList lista = resultList(kysely);

        for(int i=0; i < lista.count(); i++) {
            QVariantMap map = lista.at(i).toMap();
            double avd = map.take("avd").toDouble();
            double avk = map.take("avk").toDouble();
            map.insert("avoin", avd - avk);
            double evd = map.take("vad").toDouble();
            double evk = map.take("vak").toDouble();
            map.insert("eraantynyt", evd - evk);
            lista[i] = map;
        }
        return lista;
}
