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
#include "kumppanitroute.h"
#include "model/tositevienti.h"

KumppanitRoute::KumppanitRoute(SQLiteModel *model) :
    SQLiteRoute(model, "/kumppanit")
{

}

QVariant KumppanitRoute::get(const QString &polku, const QUrlQuery &urlquery)
{
    QSqlQuery kysely(db());

    if( polku.isEmpty())
    {
        if( urlquery.hasQueryItem("ryhma")) {
            kysely.exec(QString("SELECT id,nimi,alvtunnus FROM Kumppani "
                                "JOIN KumppaniRyhmassa ON Kumppani.id=KumppaniRyhmassa.kumppani "
                                "WHERE ryhma=%1 ORDER BY nimi").arg(urlquery.queryItemValue("ryhma").toInt()));
            return resultList(kysely);
        } else if( urlquery.hasQueryItem("alv")) {
            kysely.exec(QString("SELECT * FROM Kumppani WHERE alvtunnus='%1'").arg(urlquery.queryItemValue("alv")));
        } else if(urlquery.hasQueryItem("iban")) {
            kysely.exec(QString("SELECT id, nimi, alvtunnus, json FROM KumppaniIban JOIN Kumppani ON KumppaniIban.kumppani=Kumppani.id "
                                "WHERE iban='%1'").arg(urlquery.queryItemValue("iban")));
        }
        else {
            kysely.exec("SELECT id,nimi,alvtunnus FROM Kumppani ORDER BY nimi");
            return resultList(kysely);
        }
    } else {
        kysely.exec(QString("SELECT * FROM Kumppani WHERE id=%1").arg(polku.toInt()));
    }


    QVariantMap kumppani = resultMap(kysely);
    if(kumppani.isEmpty())
        return QVariantMap();

    int kumppaniid = kumppani.value("id").toInt();

    // Tilit
    QVariantList tililista;
    kysely.exec(QString("SELECT iban FROM KumppaniIban WHERE kumppani=%1").arg(kumppaniid));
    while(kysely.next())
        tililista.append(kysely.value(0));
    kumppani.insert("iban",tililista);

    // Menotili
    kysely.exec(QString("SELECT tili FROM Vienti WHERE tyyppi=%1 AND kumppani=%2 GROUP BY tili ORDER BY count(tili) DESC LIMIT 1")
                .arg(TositeVienti::OSTO + TositeVienti::KIRJAUS).arg(kumppaniid));
    if( kysely.next())
        kumppani.insert("menotili", kysely.value(0));

    // Tulotili
    kysely.exec(QString("SELECT tili FROM Vienti WHERE tyyppi=%1 AND kumppani=%2 GROUP BY tili ORDER BY count(tili) DESC LIMIT 1")
                .arg(TositeVienti::MYYNTI + TositeVienti::KIRJAUS).arg(kumppaniid));
    if( kysely.next())
        kumppani.insert("tulotili", kysely.value(0));

    // Ryhmat
    QVariantList ryhmat;
    kysely.exec(QString("SELECT ryhma FROM KumppaniRyhmassa WHERE kumppani=%1").arg(kumppaniid));
    while(kysely.next())
        ryhmat.append(kysely.value(0));
    if(!ryhmat.isEmpty())
        kumppani.insert("ryhmat", ryhmat);

    return kumppani;
}

QVariant KumppanitRoute::post(const QString &polku, const QVariant &data)
{

    QVariantMap map = data.toMap();

    if(polku == "yhdista")
        return yhdista(map);

    QVariantMap kopio(map);

    QSqlQuery kysely(db());
    db().transaction();

    int id = kumppaninLisays(map, kysely);
    if( id ) {
        db().commit();
        kopio.insert("id", id);
        return kopio;
    } else {
        db().rollback();
        throw SQLiteVirhe(kysely);
    }
}

QVariant KumppanitRoute::put(const QString &polku, const QVariant &data)
{
    QVariantMap map = data.toMap();
    QVariantMap kopio(map);

    int id = polku.toInt();
    kopio.insert("id", id);

    QString nimi = map.take("nimi").toString();
    QString alvtunnus = map.take("alvtunnus").toString();
    QVariantList iban = map.take("iban").toList();
    QVariantList ryhmat = map.take("ryhmat").toList();

    map.remove("menotili");
    map.remove("tulotili");

    QSqlQuery kysely(db());

    db().transaction();

    kysely.prepare("INSERT INTO Kumppani (id,nimi,alvtunnus,json) VALUES (?,?,?,?)"
                   "ON CONFLICT (id) DO UPDATE SET nimi=EXCLUDED.nimi, alvtunnus=EXCLUDED.alvtunnus, json=EXCLUDED.json");
    kysely.addBindValue(id);
    kysely.addBindValue(nimi);
    kysely.addBindValue(alvtunnus);
    kysely.addBindValue( mapToJson(map) );
    if(!kysely.exec()) {
        db().rollback();
        throw SQLiteVirhe(kysely);
    }

    kysely.exec(QString("DELETE FROM KumppaniIban WHERE kumppani=%1").arg(id));

    kysely.prepare("INSERT INTO KumppaniIban (kumppani,iban) VALUES (?,?) ON CONFLICT (iban) DO UPDATE SET kumppani=EXCLUDED.kumppani");
    for(const QVariant& var : qAsConst( iban )) {
        kysely.addBindValue(id);
        kysely.addBindValue(var.toString());
        if(!kysely.exec()) {
            db().rollback();
            throw SQLiteVirhe(kysely);
        }
    }

    kysely.exec(QString("DELETE FROM KumppaniRyhmassa WHERE kumppani=%1").arg(id));

    kysely.prepare("INSERT INTO KumppaniRyhmassa (kumppani,ryhma) VALUES (?,?)");
    for( const QVariant& ryhma : qAsConst( ryhmat )) {
        kysely.addBindValue(id);
        kysely.addBindValue(ryhma.toInt());
        if(!kysely.exec()) {
            db().rollback();
            throw SQLiteVirhe(kysely);
        }
    }

    db().commit();

    return kopio;
}

QVariant KumppanitRoute::doDelete(const QString &polku)
{
    QSqlQuery kysely(db());
    kysely.exec(QString("DELETE FROM Kumppani WHERE id=%1").arg(polku.toInt()));
    return QVariant();

}

int KumppanitRoute::kumppaninLisays(QVariantMap &map, QSqlQuery &kysely)
{
    QString nimi = map.take("nimi").toString();
    QString alvtunnus = map.take("alvtunnus").toString();
    QVariantList iban = map.take("iban").toList();
    QVariantList ryhmat = map.take("ryhmat").toList();

    kysely.prepare("INSERT INTO Kumppani (nimi,alvtunnus,json) VALUES (?,?,?)");
    kysely.addBindValue(nimi);
    kysely.addBindValue(alvtunnus);
    kysely.addBindValue( mapToJson(map) );
    if(!kysely.exec()) {
        return 0;
    }

    int id = kysely.lastInsertId().toInt();

    kysely.prepare("INSERT INTO KumppaniIban (kumppani,iban) VALUES (?,?) ON CONFLICT (iban) DO UPDATE SET kumppani=EXCLUDED.kumppani");
    for(const QVariant& var : qAsConst( iban )) {
        kysely.addBindValue(id);
        kysely.addBindValue(var.toString());
        if(!kysely.exec()) {
            return 0;
        }
    }

    kysely.prepare("INSERT INTO KumppaniRyhmassa (kumppani,ryhma) VALUES (?,?)");
    for( const QVariant& ryhma : qAsConst( ryhmat )) {
        kysely.addBindValue(id);
        kysely.addBindValue(ryhma.toInt());
        if(!kysely.exec()) {
            return 0;
        }
    }
    return id;

}

QVariant KumppanitRoute::yhdista(const QVariantMap& data)
{
    int vanha = data.value("vanha").toInt();
    int uusi = data.value("uusi").toInt();

    QSqlQuery kysely(db());
    db().transaction();

    kysely.exec(QString("UPDATE Vienti SET kumppani=%1 WHERE kumppani=%2").arg(uusi).arg(vanha));
    kysely.exec(QString("UPDATE Tosite SET kumppani=%1 WHERE kumppani=%3").arg(uusi).arg(vanha));
    kysely.exec(QString("DELETE FROM KumppaniIban WHERE kumppani=%1").arg(vanha));
    kysely.exec(QString("DELETE FROM Kumppani WHERE id=%1").arg(vanha));

    db().commit();
    return QVariant();
}
