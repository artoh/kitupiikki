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
#include "tuotteetroute.h"

#include "db/kirjanpito.h"
#include <QJsonDocument>
#include <QDebug>
#include "db/tositetyyppimodel.h"

TuotteetRoute::TuotteetRoute(SQLiteModel *model)
    : SQLiteRoute(model, "/tuotteet")
{

}

QVariant TuotteetRoute::get(const QString &polku, const QUrlQuery &urlquery)
{
    if( polku == "myynti")
        return myynti(urlquery);

    QSqlQuery kysely( db() );
    kysely.exec("SELECT * FROM Tuote ORDER BY nimike");
    return resultList(kysely);
}

QVariant TuotteetRoute::post(const QString &/*polku*/, const QVariant &data)
{
    QVariantMap map = data.toMap();
    QVariantMap jemma = map;
    QSqlQuery kysely( db() );
    kysely.prepare("INSERT INTO Tuote (nimike, json) VALUES (?,?)");
    kysely.addBindValue(map.take("nimike").toString());
    kysely.addBindValue(mapToJson(map));
    kysely.exec();
    jemma.insert("id", kysely.lastInsertId().toInt());
    return jemma;
}

QVariant TuotteetRoute::put(const QString &polku, const QVariant &data)
{
    QVariantMap map = data.toMap();
    QVariantMap jemma = map;
    QSqlQuery kysely( db() );
    kysely.prepare( "INSERT INTO Tuote (id, nimike,json) VALUES (?,?,?) "
                    "ON CONFLICT (id) DO UPDATE SET nimike=EXCLUDED.nimike, json=EXCLUDED.json" );
    kysely.addBindValue( polku.toInt() );
    map.remove("id");
    kysely.addBindValue(map.take("nimike").toString());
    kysely.addBindValue(mapToJson(map));
    kysely.exec();
    return jemma;
}

QVariant TuotteetRoute::doDelete(const QString &polku)
{
    QSqlQuery kysely( db() );
    kysely.exec(QString("DELETE FROM Tuote WHERE id=%1").arg(polku.toInt()));
    return QVariant();
}



QVariant TuotteetRoute::myynti(const QUrlQuery &urlquery)
{
    QStringList ehdot;
    ehdot << "Tosite.tila >= 100";
    ehdot << QString("Tosite.tyyppi <> %1 ").arg(TositeTyyppi::MYYNTILASKU);
    if( urlquery.hasQueryItem("alkupvm"))
        ehdot.append(QString("pvm >= '%1'").arg(urlquery.queryItemValue("alkupvm")));
    if( urlquery.hasQueryItem("loppupvm"))
        ehdot.append(QString("pvm <= '%1'").arg(urlquery.queryItemValue("loppupvm")));

    QSqlQuery kysely( db() );
    QVariantList vastaus;

    kysely.exec( "SELECT tuote, myyntikpl, ahinta, Rivi.json AS json FROM Rivi JOIN Tosite ON Rivi.tosite=Tosite.id "
                 + ( ehdot.isEmpty() ? "" : " WHERE " + ehdot.join(" AND ")  )
                 +  " ORDER BY tuote"  );

    qDebug() << kysely.lastQuery();

    int tuote = -1;
    double brutto = 0;
    double netto = 0;
    double kpl = 0;

    while( kysely.next() ) {
        if( tuote != -1 && tuote != kysely.value("tuote").toInt()) {
            QVariantMap map;
            map.insert("brutto", brutto);
            map.insert("kpl", kpl);
            map.insert("myynti", netto);
            if(tuote) {
                map.insert("nimike", kp()->tuotteet()->nimike(tuote));
                map.insert("tuote", tuote ? tuote : QVariant());
            }
            vastaus.append(map);
            brutto = 0;
            kpl = 0;
            netto = 0;
        }
        tuote = kysely.value("tuote").toInt();
        double ahinta = kysely.value("ahinta").toDouble();
        double maara = kysely.value("myyntikpl").toDouble();
        QVariantMap jsonMap = QJsonDocument::fromJson(kysely.value("json").toByteArray()).toVariant().toMap();
        double alennus = jsonMap.value("aleprosentti").toDouble();
        double alv = jsonMap.value("alvkoodi").toInt() == AlvKoodi::MYYNNIT_NETTO ? jsonMap.value("alvprosentti").toDouble() : 0;


        tuote = kysely.value("tuote").toInt();
        kpl += maara;
        netto += maara * ahinta * ( 100.0 - alennus ) / 100.0;
        brutto += maara * ahinta * ( 100.0 - alennus ) * ( 100 + alv) / 10000.0;
    }
    if( tuote != -1) {
        QVariantMap map;
        map.insert("brutto", brutto);
        map.insert("kpl", kpl);
        map.insert("myynti", netto);
        if(tuote) {
            map.insert("nimike", kp()->tuotteet()->nimike(tuote));
            map.insert("tuote", tuote ? tuote : QVariant());
        }
        vastaus.append(map);
    }
    return vastaus;
}
