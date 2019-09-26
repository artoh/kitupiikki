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
#include "eraroute.h"

EraRoute::EraRoute(SQLiteModel *model) :
    SQLiteRoute(model, "/erat")
{

}

QVariant EraRoute::get(const QString &/*polku*/, const QUrlQuery &urlquery)
{
    QSqlQuery kysely( db() );
    QVariantList lista;

    QString kysymys("select vienti.eraid as eraid, sum(vienti.debet) as sd, sum(vienti.kredit) as sk, a.selite as selite, a.pvm as pvm, a.tili as tili from  Vienti "
                    "join Vienti as a on vienti.eraid = a.id ");

    if( urlquery.hasQueryItem("tili"))
        kysymys.append(QString("WHERE vienti.tili=%1 ").arg(urlquery.queryItemValue("tili")));

    kysymys.append("GROUP BY vienti.eraid, a.selite, a.pvm, a.tili "
                   "HAVING sum(vienti.debet) <> sum(vienti.kredit) OR sum(vienti.debet) IS NULL OR sum(vienti.kredit) IS NULL");

    kysely.exec(kysymys);
    while( kysely.next()) {
        QString tili = kysely.value(0).toString();
        double debet = kysely.value(1).toDouble();
        double kredit = kysely.value(2).toDouble();
        double avoin = tili.startsWith('1') ?
                    debet - kredit :
                    kredit - debet;

        QVariantMap map;
        map.insert("tili", tili);
        map.insert("avoin", avoin);
        lista.append(map);
    }
    return lista;
}
