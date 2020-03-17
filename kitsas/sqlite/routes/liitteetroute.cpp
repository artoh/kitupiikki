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
#include "liitteetroute.h"

#include <QRegularExpression>
#include <QRegularExpressionMatch>

#include <QCryptographicHash>
#include <QDebug>
#include <QNetworkReply>

LiitteetRoute::LiitteetRoute(SQLiteModel *model) :
    SQLiteRoute(model, "/liitteet")
{

}

QVariant LiitteetRoute::get(const QString &polku, const QUrlQuery &urlquery)
{
    QSqlQuery kysely(db());
    if( polku.isEmpty()) {
        QString kysymys = QString("SELECT tosite.pvm, tosite.sarja, tosite.tunniste, liite.id, liite.nimi, liite.tyyppi "
                                  "FROM Tosite JOIN Liite ON Liite.tosite=Tosite.id WHERE Tosite.tila >= 100 AND "
                                  "Tosite.pvm BETWEEN '%1' AND '%2' ORDER BY sarja, tunniste")
                .arg(urlquery.queryItemValue("alkupvm"))
                .arg(urlquery.queryItemValue("loppupvm"));
        kysely.exec(kysymys);
        return resultList(kysely);

    }
    if( polku.toInt()) {
        if(!kysely.exec(QString("SELECT data FROM Liite WHERE id=%1").arg(polku.toInt()) ) )
            throw SQLiteVirhe(kysely);
    } else {
        QRegularExpression re(R"((\d+)\/(\S+))");
        QRegularExpressionMatch match = re.match(polku);
        kysely.exec(QString("SELECT data FROM Liite WHERE tosite=%1 AND roolinimi='%2'")
                    .arg(match.captured(1).toInt())
                    .arg(match.captured(2)) );
    }
    if( kysely.next())
        return kysely.value(0).toByteArray();

    throw SQLiteVirhe("Liitettä ei löydy",QNetworkReply::ContentNotFoundError);
}

QPair<const QVariant, int> LiitteetRoute::byteArray(SQLiteKysely *kysely, const QByteArray &ba, const QMap<QString, QString> &meta)
{
    QString loppu = kysely->polku().mid( polku().length()+1 );
    QRegularExpression re(R"((\d+)/(\S+)?)");
    QRegularExpressionMatch match = re.match(loppu);
    QSqlQuery query(db());
    QVariantMap palautus;

    if( kysely->metodi() == KpKysely::POST) {
        query.prepare("INSERT INTO Liite(nimi,data,tyyppi,sha,tosite) VALUES (?,?,?,?,?)");
        query.addBindValue( meta.value("Filename", QString()) );
        query.addBindValue( ba );
        query.addBindValue( meta.value("Content-type", QString()));
        query.addBindValue( hash( ba) );
        if( loppu.toInt()) {
            query.addBindValue( loppu.toInt() );
        } else {
            // Liite odottamaan tositetta
            query.addBindValue( QVariant());
        }
    } else if( kysely->metodi() == KpKysely::PUT)
    {
        query.prepare("INSERT INTO Liite (tosite,nimi,data,tyyppi,sha,roolinimi) VALUES (:tosite, :nimi, :data, :tyyppi, :sha, :roolinimi) "
                      " ON CONFLICT (tosite,roolinimi) DO UPDATE SET nimi=EXCLUDED.nimi, data=EXCLUDED.data, tyyppi=EXCLUDED.tyyppi, sha=EXCLUDED.sha, "
                      " roolinimi=EXCLUDED.roolinimi, luotu=current_timestamp"  );

        query.bindValue(":tosite", match.captured(1).toInt());
        query.bindValue(":nimi", meta.value("Filename", QString()) );
        query.bindValue(":data", ba);
        query.bindValue(":tyyppi", meta.value("Content-type", QString()) );
        query.bindValue(":sha", hash(ba));
        query.bindValue(":roolinimi", match.captured(2));

    }
    if( !query.exec() )
        throw SQLiteVirhe(query);


    palautus.insert("liite", query.lastInsertId());
    if( match.hasMatch() )
        palautus.insert("tosite", match.captured(1).toInt());

    return qMakePair<const QVariant,int>(palautus, query.lastInsertId().toInt());

}

QVariant LiitteetRoute::doDelete(const QString &polku)
{
    int id = polku.toInt();
    if( id ) {
        QSqlQuery kysely( db());
        kysely.exec( QString("DELETE FROM Liite WHERE id=%1").arg(id));
    }
    return QVariant();
}

QByteArray LiitteetRoute::hash(const QByteArray &ba)
{
    QCryptographicHash laskin(QCryptographicHash::Sha256);
    laskin.addData(ba);
    return laskin.result().toHex();
}

