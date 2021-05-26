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
#include "initroute.h"
#include <QSqlQuery>
#include <QDebug>
#include <QJsonDocument>

InitRoute::InitRoute(SQLiteModel *model) :
    SQLiteRoute(model,"/init")
{

}

QVariant InitRoute::get(const QString & /*polku*/, const QUrlQuery& /*urlquery*/)
{
    QVariantMap map;
    // Asetukset
    QSqlQuery kysely(db());
    kysely.exec("SELECT avain,arvo FROM Asetus");

    QVariantMap asetukset;

    while( kysely.next()) {
        asetukset.insert( kysely.value(0).toString(),
                          kysely.value(1).toString());
    }
    map.insert("asetukset", asetukset);


    // Tilit

    kysely.exec( "select numero,tyyppi,json,iban from ( select  cast(numero as text) as numero,'H'||taso as tyyppi,json,taso, NULL as iban from otsikko "
                 " union select cast (numero as text),tyyppi,json,99,iban from tili order by numero,taso) as sub");
    map.insert("tilit", resultList(kysely));


    // Kohdennukset
    kysely.exec("select * from Kohdennus");
    map.insert("kohdennukset", resultList(kysely));

    // Tilikaudet
    kysely.exec("select * from Tilikausi order by alkaa");
    map.insert("tilikaudet", resultList(kysely));

    QStringList sarjat;
    kysely.exec("SELECT DISTINCT sarja FROM Tosite WHERE tila > 0");
    while( kysely.next())
        sarjat.append(kysely.value(0).toString());
    map.insert("tositesarjat", sarjat);


    map.insert("kierrot", QVariantList());

    return map;
}

QVariant InitRoute::patch(const QString & /*polku*/, const QVariant &data)
{
    QVariantMap map = data.toMap();
    paivitaAsetukset( map.value("asetukset").toMap() );
    paivitaTilit( map.value("tilit").toList());

    return QVariant();
}

void InitRoute::paivitaAsetukset(const QVariantMap &map)
{
    QMapIterator<QString,QVariant> iter(map);
    QStringList muokatut;

    QSqlQuery qry(db());
    qry.exec("SELECT avain FROM Asetus WHERE muokattu IS NOT NULL");
    while (qry.next()) {
        muokatut.append( qry.value(0).toString() );
    }

    qry.prepare("INSERT INTO Asetus (avain,arvo) VALUES (?,?) "
                    "ON CONFLICT (avain) DO UPDATE SET arvo = EXCLUDED.arvo ");

    while( iter.hasNext()) {
        iter.next();
        if( !muokatut.contains(iter.key()) ) {
            qry.addBindValue( iter.key() );
            if( iter.value().toString().isEmpty() )
                qry.addBindValue( QString::fromUtf8( QJsonDocument::fromVariant( iter.value() ).toJson(QJsonDocument::Compact) )  );
            else
                qry.addBindValue( iter.value().toString() );
            qry.exec();
        }
    }
}

void InitRoute::paivitaTilit(const QVariantList &list)
{
    QList<int> muokatutTilit;
    QList<QPair<int,int>> muokatutOtsikot;

    QSqlQuery qry(db());
    qry.exec("SELECT numero FROM Tili WHERE muokattu IS NOT NULL");
    while (qry.next()) {
        muokatutTilit.append( qry.value(0).toInt() );
    }

    qry.exec("SELECT numero,taso FROM Otsikko WHERE muokattu IS NOT NULL");
    while (qry.next()) {
        muokatutOtsikot.append( qMakePair( qry.value(0).toInt(), qry.value(1).toInt() ) );
    }

    QSqlQuery tiliKysely(db());
    QSqlQuery otsikkoKysely(db());

    tiliKysely.prepare("INSERT INTO Tili (numero, tyyppi, json) VALUES "
                       "(?,?,?) "
                       "ON CONFLICT(numero) DO UPDATE SET "
                       "tyyppi=EXCLUDED.tyyppi, json=EXCLUDED.json");
    otsikkoKysely.prepare("INSERT INTO Otsikko (numero, taso, json) VALUES "
                          "(?,?,?) "
                          "ON CONFLICT(numero,taso) DO UPDATE SET "
                          "json=EXCLUDED.json");


    for( const auto& item : qAsConst( list )) {
        QVariantMap map = item.toMap();

        int numero = map.take("numero").toInt();
        QString tyyppi = map.take("tyyppi").toString();
        QByteArray json = mapToJson(map);
        if( tyyppi.startsWith('H')) {
            int taso = tyyppi.mid(1).toInt();
            if( muokatutOtsikot.contains( qMakePair(numero,taso) ))
                continue;
            otsikkoKysely.addBindValue(numero);
            otsikkoKysely.addBindValue(taso);
            otsikkoKysely.addBindValue(json);
            otsikkoKysely.exec();
        } else {
            if( muokatutTilit.contains(numero))
                continue;

            tiliKysely.addBindValue(numero);
            tiliKysely.addBindValue(tyyppi);
            tiliKysely.addBindValue(json);
            tiliKysely.exec();
        }
    }
}
