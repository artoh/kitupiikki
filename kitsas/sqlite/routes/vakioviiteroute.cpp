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
#include "vakioviiteroute.h"
#include <QDate>

VakioviiteRoute::VakioviiteRoute(SQLiteModel *model) :
    SQLiteRoute(model, "/vakioviitteet")
{

}

QVariant VakioviiteRoute::get(const QString &polku, const QUrlQuery &/*urlquery*/)
{
    QSqlQuery kysely(db());
    if( polku.isEmpty()) {
        kysely.exec("SELECT viite, tili, kohdennus, otsikko FROM Vakioviite");
        return resultList(kysely);
    }

    kysely.exec(QString("SELECT * FROM Vakioviite WHERE viite=%1").arg(polku.toInt()));
    return resultMap(kysely);
}

QVariant VakioviiteRoute::post(const QString &/*polku*/, const QVariant &data)
{
    QVariantMap map = data.toMap();
    QVariantMap kopio(map);
    QString viite=seuraavaViite();

    QSqlQuery kysely(db());
    kysely.prepare("INSERT INTO Vakioviite(viite,tili,kohdennus,otsikko,json) VALUES(?,?,?,?,?)");
    kysely.addBindValue(viite);
    kysely.addBindValue(map.take("tili"));
    kysely.addBindValue(map.value("kohdennus"));
    kysely.addBindValue(map.value("otsikko"));
    kysely.addBindValue(mapToJson(map));

    if(!kysely.exec())
        throw  SQLiteVirhe(kysely);

    kopio.insert("viite", viite);
    return kopio;
}

QVariant VakioviiteRoute::put(const QString &polku, const QVariant &data)
{
    int viite = polku.toInt();
    QVariantMap map = data.toMap();
    QVariantMap kopio(map);
    map.remove("viite");

    QSqlQuery kysely(db());
    kysely.prepare("INSERT INTO Vakioviite(viite,tili,kohdennus,otsikko,json) "
                   "VALUES (?,?,?,?,?) ON CONFLICT (viite) DO UPDATE SET "
                   "tili=EXCLUDED.tili, kohdennus=EXCLUDED.kohdennus, otsikko=EXCLUDED.otsikko, "
                   "json=EXCLUDED.json ");

    kysely.addBindValue(viite);
    kysely.addBindValue(map.take("tili"));
    kysely.addBindValue(map.value("kohdennus"));
    kysely.addBindValue(map.value("otsikko"));
    kysely.addBindValue(mapToJson(map));

    if(!kysely.exec())
        throw  SQLiteVirhe(kysely);

    kopio.insert("viite", viite);
    return kopio;
}

QVariant VakioviiteRoute::doDelete(const QString &polku)
{
    QSqlQuery kysely(db());
    kysely.exec(QString("DELETE FROM Vakioviite WHERE viite=%1").arg(polku.toInt()));
    return QVariant();
}

QString VakioviiteRoute::seuraavaViite()
{
    QSqlQuery kysely(db());

    qlonglong viiteluku = 999;
    kysely.exec("SELECT MAX(viite) FROM Vakioviite");
    if( kysely.next()) {
        if( kysely.value(0).toLongLong())
            viiteluku = kysely.value(0).toLongLong();
    }
    viiteluku = (viiteluku + 1) / 10;
    QString numero = QString::number(viiteluku);

    int summa = 0;
    int indeksi = 0;
    for( int i = numero.length() - 1; i > -1; i--) {
        QChar ch = numero.at(i);
        int numero = ch.digitValue();

        if( indeksi % 3 == 0)
            summa += 7 * numero;
        else if( indeksi % 3 == 1)
            summa += 3 * numero;
        else
            summa += numero;

        indeksi++;
    }
    int tarkaste = ( 10 - summa % 10) % 10;
    return numero + QString::number(tarkaste);
}
