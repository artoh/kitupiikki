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
#include "sqliteroute.h"
#include <QJsonDocument>
#include <QSqlRecord>
#include <QDebug>
#include <QSqlError>

SQLiteRoute::SQLiteRoute(SQLiteModel *model, const QString &polku)
    : model_(model), polku_(polku)
{

}

SQLiteRoute::~SQLiteRoute()
{

}

QVariant SQLiteRoute::route(SQLiteKysely *kysely, const QVariant &data)
{
    QString loppu = kysely->polku().mid( polku().length() );

    if( loppu.startsWith(QChar('/')) )
        loppu = loppu.mid(1);

    qDebug() << "* route " << kysely->metodi() <<  " " << polku() << "  " << loppu << " "
             << QJsonDocument::fromVariant(data).toJson(QJsonDocument::Compact).left(30);

    switch (kysely->metodi()) {
    case KpKysely::GET:
        return get(loppu, kysely->urlKysely());
    case KpKysely::POST:
        return post(loppu, data);
    case KpKysely::PUT:
        return put(loppu, data);
    case KpKysely::PATCH:
        return patch(loppu, data);
    case KpKysely::DELETE:
        return doDelete(loppu);
    }

    qDebug() << " ***** Ei reititetty ******** " << kysely->polku();
}

QVariant SQLiteRoute::byteArray(SQLiteKysely * /*reititettavaKysely*/, const QByteArray & /*ba*/, const QVariantMap& /*meta*/)
{
    return QVariant();
}

QVariant SQLiteRoute::get(const QString & /*polku*/, const QUrlQuery& /*urlquery*/)
{
    return QVariant();
}

QVariant SQLiteRoute::put(const QString & /*polku*/, const QVariant &/*data*/)
{
    return QVariant();
}

QVariant SQLiteRoute::post(const QString &/*polku*/, const QVariant &/*data*/)
{
    return QVariant();
}

QVariant SQLiteRoute::patch(const QString &/*polku*/, const QVariant &/*data*/)
{
    return QVariant();
}

QVariant SQLiteRoute::doDelete(const QString &/*polku*/)
{
    return QVariant();
}

QVariantList SQLiteRoute::resultList(QSqlQuery &kysely)
{
    if( kysely.lastError().type() != QSqlError::NoError) {
        qDebug() << " *SQLVIRHE* "
                  << kysely.lastError().text()
                  << kysely.lastQuery();
    }


    QVariantList lista;
    while( kysely.next()) {
        // Sijoitetaan ensin json-kenttä

        QSqlRecord tietue = kysely.record();
        QVariantMap map = QJsonDocument::fromJson( tietue.value("json").toString().toUtf8() ).toVariant().toMap();

        for(int i=0; i < tietue.count(); i++) {
            QString kenttanimi = tietue.fieldName(i);
            // Jos kenttänimi esim. era_id, tulee era.id
            if( kenttanimi.contains(QChar('_'))) {
                int viivanpaikka = kenttanimi.indexOf('_');
                QString ryhma = kenttanimi.left(viivanpaikka);
                QString alakentta = kenttanimi.mid(viivanpaikka+1);
                QVariantMap rmap = map.value(ryhma, QVariantMap()).toMap();
                rmap.insert(alakentta, tietue.value(i));
                map.insert(ryhma, rmap);
            }
            else if( kenttanimi != "json")
                map.insert( tietue.fieldName(i), tietue.value(i) );
        }
        lista.append(map);
    }
    return lista;
}

QVariantMap SQLiteRoute::resultMap(QSqlQuery &kysely)
{
    // Kyselyssä voi olla vain yksi rivi
    QVariantList lista = resultList(kysely);
    if( lista.isEmpty())
        return QVariantMap();
    else
        return lista.first().toMap();
}

QByteArray SQLiteRoute::mapToJson(const QVariantMap &map)
{
    return QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact);
}

QSqlDatabase SQLiteRoute::db()
{
    return model_->tietokanta();
}


void SQLiteRoute::taydennaErat(QVariantList &vientilista)
{
    QSqlQuery kysely(db());

    for(int i=0; i < vientilista.count(); i++) {
        QVariantMap map = vientilista.at(i).toMap();
        if( map.contains("era")) {
            QVariantMap eramap = map.value("era").toMap();
            int eraid = eramap.value("id").toInt();
            if( eraid ) {
                kysely.exec(QString("SELECT Vienti.id as id, Tosite.tunniste as tunniste, Tosite.sarja as sarja, Tosite.pvm as pvm "
                                    "FROM Vienti JOIN Tosite ON Vienti.tosite=Tosite.id "
                                    "WHERE Vienti.id=%1")
                            .arg(eraid));
                eramap = resultMap(kysely);

                kysely.exec(QString("SELECT SUM(debet) as debetit, SUM(kredit) as kreditit FROM Vienti WHERE eraid=%1").arg(eraid));
                eramap.insert("saldo", kysely.value(0).toDouble() - kysely.value(1).toDouble());
                map.insert("era", eramap);
                vientilista[i] = map;
            }
        }
    }
}
