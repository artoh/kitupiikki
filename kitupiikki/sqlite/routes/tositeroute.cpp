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
#include "tositeroute.h"

#include "model/tosite.h"
#include "db/kirjanpito.h"

#include <QJsonDocument>
#include <QDate>

#include <QSqlError>
#include <QDebug>

TositeRoute::TositeRoute(SQLiteModel *model) :
    SQLiteRoute(model, "/tositteet")
{

}

QVariant TositeRoute::get(const QString &polku, const QUrlQuery &urlquery)
{
     qDebug() << " tosite get " << polku << " " << urlquery.toString();

    if( polku.length() > 1)
        return hae( polku.mid(1).toInt() );

    // Muuten tositteiden lista
    QStringList ehdot;
    if( urlquery.hasQueryItem("luonnos") )
        ehdot.append( QString("( tosite.tila > %1 and tosite.tila < %2").arg(Tosite::POISTETTU).arg(Tosite::KIRJANPIDOSSA) );
    else
        ehdot.append( QString("tosite.tila >= %1").arg(Tosite::KIRJANPIDOSSA));

    if( urlquery.hasQueryItem("alkupvm"))
        ehdot.append( QString("tosite.pvm >= '%1'").arg( urlquery.queryItemValue("alkupvm") ));
    if( urlquery.hasQueryItem("loppupvm"))
        ehdot.append( QString("tosite.pvm <= '%1'").arg( urlquery.queryItemValue("loppupvm")));

    QString jarjestys = "pvm";
    if( urlquery.queryItemValue("jarjestys") == "tyyppi,tosite")
        jarjestys = "tyyppi,sarja,tosite";
    else if( urlquery.queryItemValue("jarjestys") == "tosite")
        jarjestys = "sarja,tunniste";
    else if( urlquery.queryItemValue("jarjestys") == "tyyppi,pvm")
        jarjestys = "tyyppi,pvm";

    QString kysymys = "SELECT tosite.id AS id, tosite.pvm AS pvm, tyyppi, tila, tunniste, otsikko, kumppani.nimi as kumppani, tosite.sarja as sarja, "
                      "CAST( (SELECT COUNT(liite.id) FROM Liite WHERE liite.tosite=tosite.id) AS INT) AS liitteita "
                      " FROM Tosite LEFT OUTER JOIN Kumppani on tosite.kumppani=kumppani.id  WHERE ";
    kysymys.append( ehdot.join(" AND ") + QString(" ORDER BY ") + jarjestys );

    QSqlQuery kysely( db());
    kysely.exec(kysymys);
    qDebug() << kysely.lastQuery() << " - " << kysely.lastError().text();

    QVariantList tositteet = resultList(kysely);

    // Sitten tätä pitäisi vielä täydentää summalla
    for(int i=0; i<tositteet.count(); i++) {
        QVariantMap map = tositteet[i].toMap();
        kysely.exec(QString("SELECT SUM(debet) FROM Vienti WHERE tosite=%1").arg(map.value("id").toInt()));
        if( kysely.next() )
            map.insert("summa", kysely.value(0).toDouble());
        tositteet[i]=map;
    }
    return tositteet;
}

QVariant TositeRoute::post(const QString & /*polku*/, const QVariant &data)
{
    return hae( lisaaTaiPaivita(data) );
}

QVariant TositeRoute::put(const QString &polku, const QVariant &data)
{
    return hae( lisaaTaiPaivita(data, polku.mid(1).toInt()));
}

int TositeRoute::lisaaTaiPaivita(const QVariant pyynto, int tositeid)
{
    QVariantMap map = pyynto.toMap();
    QByteArray lokiin = QJsonDocument::fromVariant(pyynto).toJson(QJsonDocument::Compact);

    QDate pvm = map.take("pvm").toDate();
    int tyyppi = map.take("tyyppi").toInt();
    QVariantList viennit = map.take("viennit").toList();
    int tila = map.contains("tila") ? map.take("tila").toInt() : Tosite::KIRJANPIDOSSA;
    QString otsikko = map.take("otsikko").toString();
    QVariantList rivit = map.take("rivit").toList();
    int kumppani = map.take("kumppani").toInt();
    QVariantList liita = map.take("liita").toList();
    bool onkosarjaa =  !map.value("sarja", QVariant()).isNull();
    QString sarja = map.take("sarja").toString();
    int tunniste = map.take("tunniste").toInt();

    QSqlQuery kysely(db());
    db().transaction();

    Tilikausi kausi = kp()->tilikaudet()->tilikausiPaivalle(pvm);

    if( tunniste ) {
        // Tarkistetaan, pitääkö tunniste hakea uudelleen
        kysely.exec( QString("SELECT sarja, alkaa FROM Tosite JOIN Tilikausi ON Tosite.pvm BETWEEN Tilikausi.alkaa AND Tilikausi.loppuu "
                             "WHERE Tosite.id=%1").arg(tositeid) );
        if( !kysely.next() || kysely.value("alkaa").toDate() != kausi.alkaa() || kysely.value("sarja").toString() != sarja )
            tunniste = 0;
    }
    if( tila < Tosite::KIRJANPIDOSSA)
        tunniste = 0;

    // Tunnisteen hakeminen
    if( !tunniste && tila >= Tosite::KIRJANPIDOSSA) {
        if( onkosarjaa )
            kysely.exec( QString("SELECT MAX(tunniste) as tunniste FROM Tosite WHERE pvm BETWEEN '%1' AND '%2' AND sarja IS NULL")
                         .arg(kausi.alkaa().toString(Qt::ISODate)).arg(kausi.paattyy().toString(Qt::ISODate)));
        else
            kysely.exec( QString("SELECT MAX(tunniste) as tunniste FROM Tosite WHERE pvm BETWEEN '%1' AND '%2' AND sarja='%3'")
                         .arg(kausi.alkaa().toString(Qt::ISODate)).arg(kausi.paattyy().toString(Qt::ISODate)).arg(sarja));
        if( kysely.next())
            tunniste = kysely.value("tunniste").toInt() + 1;
    }
    // TODO Laskun numero ja viite

    // Lisätään itse tosite
    QSqlQuery tositelisays(db());

    if( tositeid ) {
        tositelisays.prepare("INSERT INTO Tosite (id, pvm, tyyppi, tila, tunniste, otsikko, kumppani, sarja, json) "
                             "VALUES (?,?,?,?,?,?,?,?,?) "
                             "SET pvm=EXCLUDED.pvm, tyyppi=EXCLUDED.tyyppi, tila=EXCLUDED.tila, tunniste=EXCLUDED.tunniste, otsikko=EXCLUDED.otsikko, "
                             "kumppani=EXCLUDED.kumppani, sarja=EXCLUDED.sarja, json=EXCLUDED.json");

        tositelisays.addBindValue(tositeid);
    } else {
        tositelisays.prepare("INSERT INTO Tosite (pvm, tyyppi, tila, tunniste, otsikko, kumppani, sarja, json) "
                             "VALUES (?,?,?,?,?,?,?,?)");
    }
    tositelisays.addBindValue(pvm);
    tositelisays.addBindValue(tyyppi);
    tositelisays.addBindValue(tila);
    tositelisays.addBindValue(tunniste);
    tositelisays.addBindValue(otsikko);
    tositelisays.addBindValue(kumppani);
    tositelisays.addBindValue(sarja);
    tositelisays.addBindValue( mapToJson(map) );
    tositelisays.exec();

    if( !tositeid)
        tositeid = tositelisays.lastInsertId().toInt();

    qDebug() << " Kysely " << tositelisays.lastQuery()
             << " virhe " << tositelisays.lastError().text();


    db().commit();
    return tositeid;
}

QVariant TositeRoute::hae(int tositeId)
{
    QSqlQuery kysely(db());
    kysely.exec(QString("SELECT * FROM Tosite WHERE id=%1").arg(tositeId));
    QVariantMap tosite = resultMap(kysely);

    qDebug() << " Tosite " << tositeId << " Sisältö " << QJsonDocument::fromVariant(tosite).toJson(QJsonDocument::Compact);

    // Sitten vielä aika paljon täydennettävää

    return tosite;
}
