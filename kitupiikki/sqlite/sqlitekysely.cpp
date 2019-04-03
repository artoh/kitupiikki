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
#include "sqlitekysely.h"
#include "sqliteyhteys.h"

#include <QSqlQuery>

#include <QDebug>
#include <QDateTime>
#include <QMessageBox>
#include <QSqlError>
#include <QJsonDocument>

SQLiteKysely::SQLiteKysely(SQLiteYhteys *parent, KpKysely::Metodi metodi, QString polku)
    : KpKysely (parent, metodi, polku)
{

}

void SQLiteKysely::kysy(const QVariant &data)
{
    if( polku().isEmpty()) {
        alustusKysely();
        return;
    }

    sanat_ = polku().split('/');

    if( metodi() == PATCH && sanat_.first() == "asetukset")
        teeAsetus(data.toMap());
    if( metodi() == GET && sanat_.first() == "liitteet")
        lataaLiite();
    if( metodi() == GET && sanat_.first() == "tositteet" )
    {
        if( sanat_.count() == 2)
        {
            vastaus_.insert("data", tosite( sanat_.at(1).toInt() ));
            vastaa();
        }
        else
        {
            tositelista();
        }
    }
    if( metodi() == GET && sanat_.first() == "viennit")
        vientilista();
}

QSqlDatabase SQLiteKysely::tietokanta()
{
    return static_cast<SQLiteYhteys*>( parent() )->tietokanta();
}

void SQLiteKysely::alustusKysely()
{
    vastaus_.insert("asetukset", asetukset());
    vastaus_.insert("tilit", tilit());
    vastaus_.insert("kohdennukset", kohdennukset());
    vastaus_.insert("tositelajit", tositelajit());
    vastaus_.insert("tilikaudet", tilikaudet());

    QJsonDocument json = QJsonDocument::fromVariant(vastaus_);
    qDebug() << json.toJson();

    vastaa();
}

QVariantList SQLiteKysely::asetukset()
{
    QVariantList lista;

    QSqlQuery query( tietokanta() );
    query.exec("SELECT avain, arvo, muokattu FROM asetus");
    while( query.next())
    {
        QVariantMap item;
        item.insert("avain", query.value("avain"));
        item.insert("arvo", query.value("arvo"));
        item.insert("muokattu", query.value("muokattu"));
        lista.append(item);
    }

    return lista;
}

QVariantList SQLiteKysely::tilit()
{
    QVariantList lista;
    QSqlQuery kysely( tietokanta() );

    kysely.exec("SELECT id, nro, nimi, tyyppi, tila, json, ysiluku, muokattu "
                " FROM tili ORDER BY ysiluku");


    while(kysely.next())
    {
        QJsonDocument json = QJsonDocument::fromJson( kysely.value("json").toByteArray() );

        QVariantMap map = json.toVariant().toMap();

        map.insert("id", kysely.value("id"));
        map.insert("nro", kysely.value("nro"));
        map.insert("ysiluku", kysely.value("ysiluku"));

        QVariantMap kieliMap;
        kieliMap.insert("fi", kysely.value("nimi"));

        map.insert("nimi", kieliMap);

        map.insert("tyyppi", kysely.value("tyyppi"));
        map.insert("tila", kysely.value("tila"));

        map.insert("muokattu", kysely.value("muokattu"));

        lista.append(map);
    }
    return lista;
}

QVariantList SQLiteKysely::kohdennukset()
{
    QVariantList lista;
    QSqlQuery kysely( tietokanta());

    kysely.exec("SELECT id, tyyppi, nimi, alkaa, loppuu FROM kohdennus");
    while( kysely.next() )
    {
        QVariantMap map;
        map.insert("id", kysely.value("id"));
        map.insert("tyyppi", kysely.value("tyyppi"));
        map.insert("nimi", kysely.value("nimi"));
        map.insert("alkaa", kysely.value("alkaa"));
        map.insert("loppuu", kysely.value("loppuu"));
        lista.append(map);
    }
    return lista;
}

QVariantList SQLiteKysely::tositelajit()
{
    QVariantList lista;
    QSqlQuery kysely( tietokanta());

    kysely.exec("SELECT id, tunnus, nimi, json FROM tositelaji ORDER BY id");

    while( kysely.next())
    {
        QJsonDocument json = QJsonDocument::fromJson( kysely.value("json").toByteArray() );
        QVariantMap map = json.toVariant().toMap();

        map.insert("id", kysely.value("id"));
        map.insert("tunnus", kysely.value("tunnus"));
        map.insert("nimi", kysely.value("nimi"));

        lista.append(map);
    }
    return lista;
}

QVariantList SQLiteKysely::tilikaudet()
{
    QVariantList lista;
    QSqlQuery kysely( tietokanta());

    kysely.exec("SELECT alkaa, loppuu, json FROM tilikausi ORDER BY alkaa");
    while( kysely.next())
    {
        QJsonDocument json = QJsonDocument::fromJson( kysely.value("json").toByteArray() );
        QVariantMap map = json.toVariant().toMap();

        map.insert("alkaa", kysely.value("alkaa"));
        map.insert("loppuu", kysely.value("loppuu"));

        lista.append(map);
    }
    return lista;
}

void SQLiteKysely::teeAsetus(const QVariantMap& params)
{
    QSqlQuery query(tietokanta());

    if( params.value("arvo").isNull())
    {
        query.prepare("DELETE FROM asetus WHERE avain=:avain");
    }
    else
    {
        query.prepare("REPLACE INTO asetus(avain, arvo, muokattu) VALUES(:avain,:arvo,:muokattu)");
        query.bindValue(":arvo", params.value("arvo"));
        query.bindValue(":muokattu", QDateTime::currentDateTime() );
    }

    query.bindValue(":avain", params.value("avain"));

    if( query.exec())
        vastaa();
    else
        QMessageBox::critical(nullptr, tr("Tietokantavirhe"),
                              tr("Asetuksen tallentaminen epäonnistui seuraavan virheen takia:%1")
                              .arg(tietokanta().lastError().text()));

}

void SQLiteKysely::lataaLiite()
{
    qDebug() << "Liitteen lataus " << polku();
    QStringList sanat = polku().split('/');

    if( sanat.count() < 2)
        return;

    bool ok;
    sanat.at(1).toInt(&ok);

    QSqlQuery query( tietokanta());

    if( !ok )
        query.exec( QString("SELECT data, otsikko FROM liite WHERE tosite IS NULL AND otsikko=\"%1\"").arg( sanat.at(1)  ));


    if( query.next())
    {
        qDebug() << "Liite "<< query.value("data").toByteArray().length();
        vastaus_.insert("liite",query.value("data"));
        vastaus_.insert("otsikko", query.value("otsikko"));
        vastaa();
    }
}

QVariantMap SQLiteKysely::tosite(int id)
{
    QSqlQuery kysely( tietokanta());
    kysely.exec( QString("SELECT pvm, otsikko, kommentti, tunniste, laji, "
                         "tiliote, json, luotu, muokattu FROM tosite "
                         "WHERE id=%1").arg(id) );

    if( kysely.next() )
    {
        QJsonDocument json = QJsonDocument::fromJson( kysely.value("json").toByteArray() );

        QVariantMap map = json.toVariant().toMap();
        map.insert("id", id);
        map.insert("pvm", kysely.value("pvm"));
        map.insert("otsikko", kysely.value("otsikko"));
        map.insert("kommentti", kysely.value("kommentti"));
        map.insert("tunniste", kysely.value("tunniste"));
        map.insert("tositelaji",kysely.value("laji"));
        map.insert("tiliotetili", kysely.value("tiliote"));
        map.insert("luotu", kysely.value("luotu"));
        map.insert("muokattu", kysely.value("muokattu"));

        QSqlQuery vientikysely( tietokanta());
        QVariantList viennit;

        // TÄSTÄ PUUTTUU VIELÄ TAVARAA
        vientikysely.exec( QString("SELECT id, pvm, tili, debetsnt, kreditsnt, selite, json "
                                   "FROM vienti WHERE tosite=%1 "
                                   "ORDER BY vientirivi ").arg(id));
        while(vientikysely.next())
        {
            QVariantMap vienti = QJsonDocument::fromJson( vientikysely.value("json").toByteArray() ).toVariant().toMap();
            vienti.insert("id", vientikysely.value("id"));
            vienti.insert("pvm", vientikysely.value("pvm"));
            vienti.insert("tili", vientikysely.value("tili"));
            vienti.insert("debetsnt", vientikysely.value("debetsnt"));
            vienti.insert("kreditsnt", vientikysely.value("kreditsnt"));
            vienti.insert("selite", vientikysely.value("selite"));

            QSqlQuery kohdennysKysely( tietokanta() );
            QVariantList kohdennuslista;

            kohdennysKysely.exec( QString("SELECT kohdennus FROM merkkaus WHERE vienti=%1").arg( vientikysely.value("id").toInt() ));
            while( kohdennysKysely.next())
            {
                kohdennuslista.append( kohdennysKysely.value("kohdennus") );
            }
            vienti.insert("merkkaukset", kohdennuslista);

            viennit.append(vienti);
        }
        map.insert("viennit", viennit);

        QSqlQuery liitekysely( tietokanta());
        liitekysely.exec( QString("SELECT id, liiteno, otsikko, sha "
                                  "FROM liite WHERE tosite=%1 ORDER BY liiteno").arg( id ));
        QVariantList liitteet;
        while( liitekysely.next())
        {
            QVariantMap liite;
            liite.insert("id", liitekysely.value("id"));
            liite.insert("liiteno", liitekysely.value("liiteno"));
            liite.insert("otsikko", liitekysely.value("otsikko"));
            liite.insert("sha", liitekysely.value("sha"));
            liitteet.append(liite);
        }
        map.insert("liitteet", liitteet);

        return map;
    }
    return QVariantMap();
}

void SQLiteKysely::tositelista()
{
    QSqlQuery kysely(tietokanta());

    QStringList ehdot;
    if( kysely_.hasQueryItem("alkupvm"))
        ehdot.append( QString("pvm >= '%1'").arg( attribuutti("alkupvm") ) );
    if( kysely_.hasQueryItem("loppupvm"))
        ehdot.append( QString("pvm <= '%1'").arg( attribuutti("loppupvm") ) );

    QString ehtolause;
    if( ehdot.count())
        ehtolause = "WHERE " + ehdot.join(" AND ");

    QString kysymys = QString("SELECT id, pvm, otsikko, laji, tunniste "
                              "FROM tosite %1"
                              "ORDER BY tosite.pvm, tosite.id").arg(ehtolause);

    qDebug() << kysymys;

    kysely.exec(kysymys);

    QVariantList lista;

    while(kysely.next())
    {
        QVariantMap map;
        int id = kysely.value("id").toInt();
        map.insert("id", id);
        map.insert("pvm", kysely.value("pvm"));
        map.insert("otsikko", kysely.value("otsikko"));
        map.insert("tunniste", kysely.value("tunniste"));
        map.insert("tositelaji", kysely.value("laji"));

        QSqlQuery summakysely( QString("SELECT sum(debetsnt), sum(kreditsnt) FROM vienti "
                                       "WHERE tosite=%1").arg( id ));

        if( summakysely.next())
        {
            qlonglong debet = summakysely.value(0).toLongLong();
            qlonglong kredit = summakysely.value(1).toLongLong();

            // Yleensä kreditin ja debetin pitäisi täsmätä ;)
            if( debet > kredit)
                map.insert("summa", debet);
            else
                map.insert("summa", kredit);
        }

        QSqlQuery liitekysely( QString("SELECT count(id) FROM liite WHERE tosite=%1").arg(id));
        if( liitekysely.next())
        {
            map.insert("liitteita", liitekysely.value(0));
        }

        lista.append(map);
    }
    vastaus_.insert("tositteet", lista);
    vastaa();
}

void SQLiteKysely::vientilista()
{

    QStringList ehdot;
    if( kysely_.hasQueryItem("alkupvm"))
        ehdot.append( QString("vienti.pvm >= '%1'").arg( attribuutti("alkupvm") ) );
    if( kysely_.hasQueryItem("loppupvm"))
        ehdot.append( QString("vienti.pvm <= '%1'").arg( attribuutti("loppupvm") ) );

    QString ehtolause;
    if( ehdot.count())
        ehtolause = "WHERE " + ehdot.join(" AND ");

    QString kysymys = QString("SELECT vienti.id, vienti.pvm, tili.nro, debetsnt, "
                              "kreditsnt, selite, kohdennus, eraid, tosite.laji, "
                              "tosite.tunniste, vienti.id, tosite.pvm, tosite.id, count(liite.id) as liitteita "
                              "FROM vienti JOIN tosite ON vienti.tosite=tosite.id "
                              "LEFT JOIN liite ON tosite.id=liite.tosite "
                              "JOIN tili ON vienti.tili=tili.id"
                              " %1 "
                              "GROUP BY vienti.id "
                              "ORDER BY vienti.pvm, vienti.id").arg(ehtolause);
    QSqlQuery kysely( tietokanta());
    kysely.exec(kysymys);

    qDebug() << kysymys;
    qDebug() << kysely.lastError().text();

    QVariantList lista;
    while( kysely.next() )
    {
        QVariantMap map;
        int id = kysely.value("vienti.id").toInt();
        int tili = kysely.value("tili.nro").toInt();

        if( !tili )
            continue;

        map.insert("id", id);
        map.insert("pvm", kysely.value("vienti.pvm"));
        map.insert("tili", tili );
        if( !kysely.value("debetsnt").isNull())
            map.insert("debetsnt", kysely.value("debetsnt"));
        if( !kysely.value("kreditsnt").isNull())
            map.insert("kreditsnt", kysely.value("kreditsnt"));
        map.insert("kohdennus", kysely.value("kohdennus"));
        map.insert("liitteita", kysely.value("liitteita"));
        map.insert("selite", kysely.value("selite"));

        QVariantMap tositeMap;
        tositeMap.insert("id", kysely.value("tosite.id"));
        tositeMap.insert("tositelaji", kysely.value("tosite.laji"));
        tositeMap.insert("tunniste", kysely.value("tosite.tunniste"));
        tositeMap.insert("pvm", kysely.value("tosite.pvm"));

        map.insert("tosite", tositeMap);

        // TODO: Erän käsittely
        int eraid = kysely.value("eraid").toInt();
        if (eraid )
        {
            QVariantMap eramap;
            eramap.insert("id", eraid);
            if( eraid != id)
            {
                // Tähän erän tunnistetiedot
                QSqlQuery tunnistekysely( QString("SELECT laji, tosite.pvm, tunniste "
                                                  "FROM vienti JOIN tosite ON vienti.tosite=tosite.id "
                                                  "WHERE vienti.id=%1").arg(eraid));
                tunnistekysely.exec();
                if( tunnistekysely.next())
                {
                    eramap.insert("pvm", tunnistekysely.value("tosite.pvm"));
                    eramap.insert("tunniste", tunnistekysely.value("tunniste"));
                    eramap.insert("tositelaji", tunnistekysely.value("laji"));
                }
            }
            QSqlQuery eraKysely( QString("SELECT SUM(debetsnt), SUM(kreditsnt) FROM vienti WHERE eraid=%1")
                                 .arg(eraid));
            eraKysely.exec();

            if( eraKysely.next())
                eramap.insert("saldo", eraKysely.value(0).toLongLong() - eraKysely.value(1).toLongLong());
            map.insert("era", eramap);
        }

        QSqlQuery kohdennysKysely( tietokanta() );
        QVariantList kohdennuslista;

        kohdennysKysely.exec( QString("SELECT kohdennus FROM merkkaus WHERE vienti=%1").arg( id ) );
        while( kohdennysKysely.next())
        {
            kohdennuslista.append( kohdennysKysely.value("kohdennus") );
        }
        if( !kohdennuslista.isEmpty())
            map.insert("merkkaukset", kohdennuslista);



        lista.append( map );
    }

    vastaus_.insert("viennit", lista);
    vastaa();

}
