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

QVariant TilitRoute::put(const QString &polku, const QVariant &data)
{

    QVariantMap map = data.toMap();
    int numero = map.take("numero").toInt();
    QString tyyppi = map.take("tyyppi").toString();
    QSqlQuery query( db());

    if( tyyppi.startsWith('H')) {
        // Otsikko
        query.prepare("INSERT INTO Otsikko (numero, taso, json, muokattu) VALUES "
                      "(?,?,?,CURRENT_TIMESTAMP) "
                      "ON CONFLICT(numero,taso) DO UPDATE SET "
                      "json=EXCLUDED.json, muokattu=CURRENT_TIMESTAMP");
        query.addBindValue(numero);
        query.addBindValue(tyyppi.mid(1).toInt());
        query.addBindValue( mapToJson(map) );
    } else {
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
