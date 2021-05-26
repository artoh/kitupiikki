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

AlvRoute::AlvRoute(SQLiteModel *model)
    : SQLiteRoute(model, "/alv")
{

}

QVariant AlvRoute::get(const QString &/*polku*/, const QUrlQuery &/*urlquery*/)
{
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
