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
#include "asetuksetroute.h"

AsetuksetRoute::AsetuksetRoute(SQLiteModel *model) :
    SQLiteRoute(model, "/asetukset")
{

}

QVariant AsetuksetRoute::get(const QString &/*polku*/, const QUrlQuery &/*urlquery*/)
{
    QVariantMap asetukset;
    QSqlQuery kysely(db());
    kysely.exec("SELECT avain,arvo FROM Asetus");
    while( kysely.next())
        asetukset.insert(kysely.value(0).toString(), kysely.value(1));
    return asetukset;
}

QVariant AsetuksetRoute::patch(const QString &/*polku*/, const QVariant &data)
{
    QSqlQuery lisaaja(db());
    lisaaja.prepare("INSERT INTO Asetus (avain,arvo) VALUES (?,?) ON CONFLICT (avain) DO UPDATE SET arvo = EXCLUDED.arvo");
    QSqlQuery poistaja(db());

    QVariantMap map = data.toMap();
    QMapIterator<QString,QVariant> iter(map);
    while(iter.hasNext()) {
        iter.next();
        if( iter.value().isNull()) {
            poistaja.exec(QString("DELETE FROM asetus WHERE avain = '%1'").arg(iter.key()));
        } else {
            lisaaja.addBindValue(iter.key());
            lisaaja.addBindValue(iter.value());
            lisaaja.exec();
        }
    }
    return QVariant();
}
