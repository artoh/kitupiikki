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
#include "tilitroute.h"

#include <QSqlQuery>

TilitRoute::TilitRoute(SQLiteModel *model) :
    SQLiteRoute(model, "/tilit")
{

}

QVariant TilitRoute::put(const QString &osoite, const QVariant &data)
{
    QVariantMap map = data.toMap();
    QSqlQuery query( db());
    QString tyyppi = map.take("tyyppi").toString();

    if( osoite.contains('/')) {
        int kautta = osoite.indexOf('/');
        int numero = osoite.left(kautta).toInt();
        int taso = osoite.mid(kautta+1).toInt();

        map.take("numero");
        map.take("tyyppi");

        query.prepare("INSERT INTO Otsikko (numero, taso, json, muokattu) VALUES "
                      "(?,?,?,CURRENT_TIMESTAMP) "
                      "ON CONFLICT(numero,taso) DO UPDATE SET "
                      "json=EXCLUDED.json, muokattu=CURRENT_TIMESTAMP");
        query.addBindValue(numero);
        query.addBindValue( taso );
        query.addBindValue( mapToJson(map) );
    } else {
        int numero = map.take("numero").toInt();
        query.prepare("INSERT INTO Tili (numero, tyyppi, json, muokattu) VALUES "
                      "(?,?,?,CURRENT_TIMESTAMP) "
                      "ON CONFLICT(numero) DO UPDATE SET "
                      "tyyppi=EXCLUDED.tyyppi, json=EXCLUDED.json, muokattu=CURRENT_TIMESTAMP");
        query.addBindValue(numero);
        query.addBindValue(tyyppi);
        query.addBindValue( mapToJson(map) );
    }
    query.exec();

    return QVariant();
}

QVariant TilitRoute::doDelete(const QString &polku)
{
    QSqlQuery kysely( db() );
    if( polku.contains('/')) {
        kysely.exec( QString("DELETE FROM Otsikko WHERE numero=%1 AND taso=%2")
                     .arg(polku.left(polku.indexOf('/')).toInt())
                     .arg(polku.mid(polku.indexOf('/')+1).toInt()));
    } else {
        kysely.exec( QString("DELETE FROM Tili WHERE numero=%1").arg(polku.toInt()));
    }
    return QVariant();
}
