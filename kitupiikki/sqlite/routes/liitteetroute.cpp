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

LiitteetRoute::LiitteetRoute(SQLiteModel *model) :
    SQLiteRoute(model, "/liitteet")
{

}

QVariant LiitteetRoute::get(const QString &polku, const QUrlQuery &/*urlquery*/)
{
    QSqlQuery kysely(db());
    if( polku.toInt()) {
        if(!kysely.exec(QString("SELECT data FROM Liite WHERE id=%1").arg(polku.toInt()) ) )
            throw SQLiteVirhe(kysely);
    } else {
        QRegularExpression re(R"((\d+)/(\S+))");
        QRegularExpressionMatch match = re.match(polku);
        kysely.exec(QString("SELECT data FROM Liite WHERE tosite=%1 AND nimi='%2'")
                    .arg(match.captured(1).toInt())
                    .arg(match.captured(2)) );
    }
    if( kysely.next())
        return kysely.value(0).toByteArray();
    throw SQLiteVirhe("Liitettä ei löydy",404);
}

QVariant LiitteetRoute::byteArray(SQLiteKysely *kysely, const QByteArray &ba, const QMap<QString, QString> &meta)
{
    QRegularExpression re(R"(/liitteet/(\d+)(/\S+)?)");
    QRegularExpressionMatch match = re.match(kysely->polku());
    QSqlQuery query(db());
    QVariantMap palautus;

    if( kysely->metodi() == KpKysely::POST) {
        query.prepare("INSERT INTO Liite(nimi,data,tyyppi,sha,tosite) VALUES (?,?,?,?,?)");
        query.addBindValue( meta.value("Filename", QString()) );
        query.addBindValue( ba );
        query.addBindValue( meta.value("Content-type", QString()));
        query.addBindValue( hash( ba) );
        if( match.hasMatch()) {
            query.addBindValue( match.captured(1).toInt() );
        } else {
            // Liite odottamaan tositetta
            query.addBindValue( QVariant());
        }
    } else if( kysely->metodi() == KpKysely::PUT)
    {
        query.prepare("INSERT INTO Liite (tosite,nimi,data,tyyppi,sha) VALUES(?,?,?,?,?) "
                      " ON CONFLICT (tosite,nimi) DO UPDATE SET data=EXCLUDED.data, sha=EXCLUDED.sha, luotu=current_datetime");
        query.addBindValue( match.captured(1).toInt() );
        query.addBindValue( match.captured(2) );
        query.addBindValue( ba );
        query.addBindValue( meta.value("Content-type", QString()));
        query.addBindValue( hash(ba));

    }
    if( !query.exec() )
        throw SQLiteVirhe(query);


    palautus.insert("liite", query.lastInsertId());
    if( match.hasMatch() )
        palautus.insert("tosite", match.captured(1).toInt());

    return palautus;
}

QByteArray LiitteetRoute::hash(const QByteArray &ba)
{
    QCryptographicHash laskin(QCryptographicHash::Sha256);
    laskin.addData(ba);
    return laskin.result().toHex();
}
