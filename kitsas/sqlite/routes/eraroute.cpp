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
#include "eraroute.h"
#include "db/tili.h"
#include "db/kirjanpito.h"

#include <QDate>
#include <QDebug>
#include <QSqlError>

EraRoute::EraRoute(SQLiteModel *model) :
    SQLiteRoute(model, "/erat")
{

}

QVariant EraRoute::get(const QString &polku, const QUrlQuery &urlquery)
{
    if(polku == "erittely")
        return erittely( QDate::fromString(urlquery.queryItemValue("alkaa"),Qt::ISODate),
                         QDate::fromString(urlquery.queryItemValue("loppuu"),Qt::ISODate) );



    QSqlQuery kysely( db() );
    QVariantList lista;

    QString kysymys("select vienti.eraid as eraid, sum(vienti.debetsnt) as sd, sum(vienti.kreditsnt) as sk, a.selite as selite, tosite.pvm as pvm, a.tili as tili, "
                    "tosite.tunniste as tunniste, tosite.sarja as sarja, tosite.tyyppi as tositetyyppi, "
                    "a.kumppani, kumppani.nimi "
                    "FROM  Vienti JOIN Tosite AS t ON vienti.tosite=t.id "
                    "JOIN Vienti AS a ON vienti.eraid = a.id JOIN Tosite ON a.Tosite=Tosite.id  "
                    "LEFT OUTER JOIN Kumppani ON a.kumppani=Kumppani.id "
                    "WHERE Tosite.tila >= 100 AND t.tila >= 100 ");

    if( urlquery.hasQueryItem("tili"))
        kysymys.append(QString("AND a.tili=%1 ").arg(urlquery.queryItemValue("tili")));
    if( urlquery.hasQueryItem("asiakas"))
        kysymys.append(QString("AND a.kumppani=%1 ").arg(urlquery.queryItemValue("asiakas")));

    kysymys.append("GROUP BY vienti.eraid ");
    if( !urlquery.hasQueryItem("kaikki") )
                   kysymys.append("HAVING sum(vienti.debetsnt) <> sum(vienti.kreditsnt) OR sum(vienti.debetsnt) IS NULL OR sum(vienti.kreditsnt) IS NULL");

    qDebug() << kysymys;

    kysely.exec(kysymys);
    while( kysely.next()) {
        QString tili = kysely.value("tili").toString();
        double debet = kysely.value(1).toLongLong() / 100.0;
        double kredit = kysely.value(2).toLongLong() / 100.0;
        double avoin = tili.startsWith('1') ?
                    debet - kredit :
                    kredit - debet;

        QVariantMap map;
        map.insert("id", kysely.value(0).toInt());
        map.insert("tili", tili);
        map.insert("avoin", avoin);
        map.insert("selite", kysely.value("selite"));
        map.insert("pvm", kysely.value("pvm").toDate());
        map.insert("tunniste", kysely.value("tunniste"));
        if( !kysely.value("sarja").toString().isEmpty())
            map.insert("sarja", kysely.value("sarja"));
        if( kysely.value("kumppani").toInt()) {
           QVariantMap kumppaniMap;
           kumppaniMap.insert("nimi", kysely.value("nimi"));
           kumppaniMap.insert("id", kysely.value("kumppani"));
           map.insert("kumppani", kumppaniMap);
        }
        lista.append(map);
    }
    return lista;
}

QVariant EraRoute::erittely(const QDate &mista, const QDate &pvm)
{
    QMap<QString,Euro> alkusaldot;
    QMap<QString,Euro> loppusaldot;

    // Haetaan loppusaldot
    QSqlQuery kysely( db() );
    kysely.exec( QString("SELECT tili, SUM(debetsnt), SUM(kreditsnt) FROM vienti "
                              "JOIN Tosite ON Vienti.tosite=Tosite.id WHERE "
                              "Vienti.pvm <= '%1' AND Tosite.tila >= 100 "
                              "AND CAST (tili AS text) < 3 GROUP BY tili").arg(pvm.toString(Qt::ISODate)) );
    while( kysely.next()) {
        QString tilinro = kysely.value(0).toString();
        Tili* tili = kp()->tilit()->tili( tilinro.toInt() );
        if( !tili) continue;
        Euro debet = Euro( kysely.value(1).toLongLong() );
        Euro kredit = Euro( kysely.value(2).toLongLong());
        if( tilinro.startsWith('1'))
            loppusaldot.insert(tilinro, debet - kredit);
        else
            loppusaldot.insert(tilinro, kredit - debet);
    }

    // Haetaan alkusaldot
    kysely.exec( QString("SELECT tili, SUM(debetsnt), SUM(kreditsnt) FROM vienti "
                              "JOIN Tosite ON Vienti.tosite=Tosite.id WHERE "
                              "Vienti.pvm < '%1' AND Tosite.tila >= 100 "
                              "AND CAST (tili AS text) < 3 GROUP BY tili").arg(mista.toString(Qt::ISODate)) );
    while( kysely.next()) {
        QString tilinro = kysely.value(0).toString();
        Tili* tili = kp()->tilit()->tili( tilinro.toInt() );
        if( !tili) continue;
        Euro debet = Euro( kysely.value(1).toLongLong() );
        Euro kredit = Euro( kysely.value(2).toLongLong());
        if( tilinro.startsWith('1'))
            alkusaldot.insert(tilinro, debet - kredit);
        else
            alkusaldot.insert(tilinro, kredit - debet);
    }

    // Sitten muodostetaan tase-erittely asetusten mukaisia erittelytapoja käyttäen
    QVariantMap ulos;

    QMapIterator<QString,Euro> iter(loppusaldot);
    while(iter.hasNext()) {
        iter.next();
        QString tiliStr = iter.key();
        Tili* tili = kp()->tilit()->tili(tiliStr.toInt());
        if( !tili) continue;
        Euro loppusaldo = iter.value();
        Euro alkusaldo = alkusaldot.value(iter.key());
        if( tili->taseErittelyTapa() == Tili::TASEERITTELY_TAYSI) {
            ulos.insert( tiliStr + "T", taysiErittely(tili, mista, pvm, alkusaldo, loppusaldo) );
        } else if( tili->taseErittelyTapa() == Tili::TASEERITTELY_LISTA) {
            ulos.insert( tiliStr + "E", listaErittely(tili, mista, pvm, alkusaldo, loppusaldo));
        } else if( tili->taseErittelyTapa() == Tili::TASEERITTELY_MUUTOKSET) {
            ulos.insert( tiliStr + "M", muutosErittely(tili, mista, pvm, alkusaldo, loppusaldo));
        } else {
            ulos.insert(tiliStr + "S", loppusaldo.toString());  // Pelkkä tilin loppusaldo
        }

    }

    // Tulokset
    int betili = kp()->tilit()->tiliTyypilla(TiliLaji::EDELLISTENTULOS).numero();
    qlonglong edelliset = 0;
    kysely.exec(QString("SELECT SUM(kreditsnt), SUM(debetsnt) FROM Vienti JOIN Tosite ON Vienti.tosite=Tosite.id "
                "WHERE vienti.pvm<'%1' AND CAST (tili AS text) >= '3' AND Tosite.tila >= 100").arg(mista.toString(Qt::ISODate)));
    if(kysely.next())
        edelliset = kysely.value(0).toLongLong() - kysely.value(1).toLongLong();

    kysely.exec(QString("SELECT SUM(debetsnt), SUM(kreditsnt) FROM Vienti JOIN Tosite ON Vienti.tosite=Tosite.id "
                "WHERE Vienti.pvm<='%1' AND Vienti.tili=%2 AND Tosite.tila>=100")
                .arg(pvm.toString(Qt::ISODate))
                .arg(betili));
    if( kysely.next() )
        edelliset += kysely.value(1).toLongLong() - kysely.value(0).toLongLong();

    ulos.insert(QString("%1S").arg(betili), edelliset / 100.0 );

    int ttili = kp()->tilit()->tiliTyypilla(TiliLaji::KAUDENTULOS).numero();
    kysely.exec(QString("SELECT SUM(kreditsnt), SUM(debetsnt) FROM Vienti JOIN Tosite ON Vienti.tosite=Tosite.id "
                "WHERE vienti.pvm BETWEEN '%1' AND '%2' AND CAST (tili AS text) >= '3' AND Tosite.tila >= 100").arg(mista.toString(Qt::ISODate)).arg(pvm.toString(Qt::ISODate)));
    if(kysely.next())
        ulos.insert(QString("%1S").arg(ttili), (kysely.value(0).toLongLong() - kysely.value(1).toLongLong()) / 100.0 );


    return ulos;
}

QVariant EraRoute::taysiErittely(Tili *tili, const QDate &mista, const QDate &mihin, const Euro &alkusaldo, const Euro &loppusaldo)
{
    QVariantList erat;
    Euro erittellytAlussa;
    Euro eritellytLopussa;

    // Tase-erät
    QSqlQuery erakysely(db());
    erakysely.exec(QString("select vienti.eraid, vienti.debetsnt, vienti.kreditsnt, vienti.selite, Tosite.pvm as pvm, Tosite.sarja, "
                           "Tosite.tunniste, tosite.id, Vienti.pvm as vientipvm, Kumppani.nimi AS kumppaninimi "
                           "FROM Vienti JOIN Tosite ON Vienti.tosite = Tosite.id LEFT OUTER JOIN Kumppani ON Vienti.kumppani=Kumppani.id "
                           "WHERE Vienti.tili=%1 AND Vienti.id=Vienti.eraid "
                           "AND Vienti.pvm <= '%2' AND Tosite.tila >= 100 ORDER BY Vienti.pvm")
                   .arg( tili->numero() ).arg(mihin.toString(Qt::ISODate)));

    while( erakysely.next()) {
        // Tässä haetaan erän aloittavat
        int eraid = erakysely.value(0).toInt();
        Euro eraAlussa = Euro( tili->onko(TiliLaji::VASTAAVAA) ?
                    erakysely.value(1).toLongLong() - erakysely.value(2).toLongLong() :
                    erakysely.value(2).toLongLong() - erakysely.value(1).toLongLong() );

        QSqlQuery apukysely( db() );
        Euro eranAloitus;

        apukysely.exec(QString("SELECT sum(debetsnt), sum(kreditsnt) FROM Vienti JOIN Tosite ON Vienti.tosite=Tosite.id WHERE eraid=%1 AND Vienti.pvm<'%2' AND Tosite.tila >= 100 ")
                       .arg(eraid).arg(mista.toString(Qt::ISODate)));
        if( apukysely.next()) {
           eranAloitus = Euro(tili->onko(TiliLaji::VASTAAVAA) ?
                        apukysely.value(0).toLongLong() - apukysely.value(1).toLongLong() :
                        apukysely.value(1).toLongLong() - apukysely.value(0).toLongLong() );
        }

        apukysely.exec(QString("select vienti.debetsnt, vienti.kreditsnt, vienti.selite, Tosite.pvm as pvm, Tosite.sarja, "
                               "Tosite.tunniste, tosite.id, Vienti.pvm as vientipvm, Kumppani.nimi AS kumppaninimi "
                               "FROM Vienti  JOIN Tosite ON Vienti.tosite = Tosite.id  "
                               "LEFT OUTER JOIN Kumppani ON Vienti.kumppani = Kumppani.id "
                               "WHERE Vienti.eraid=%1 AND Vienti.id<>Vienti.eraid "
                               "AND Vienti.pvm BETWEEN '%2' AND '%3' AND Tosite.tila >= 100 ORDER BY Vienti.pvm")
                       .arg(QString::number(eraid), mista.toString(Qt::ISODate), mihin.toString(Qt::ISODate)));
        QVariantList muutokset;

        // Jos erä alkaa tältä tilikaudelta, on erän aloitus osa muutosta
        Euro eranMuutos = erakysely.value(4).toDate() < mista ? Euro::Zero : eranAloitus ;


        while( apukysely.next() )
        {
            QVariantMap map;
            Euro summa = Euro(tili->onko(TiliLaji::VASTAAVAA) ?
                        apukysely.value(0).toLongLong() - apukysely.value(1).toLongLong() :
                        apukysely.value(1).toLongLong() - apukysely.value(0).toLongLong() );

            map.insert("pvm", apukysely.value(3).toDate());
            map.insert("sarja", apukysely.value(4));
            map.insert("tunniste", apukysely.value(5));
            map.insert("id", apukysely.value(6).toInt());
            map.insert("vientipvm", apukysely.value(7).toDate());
            map.insert("selite", apukysely.value(2).toString());
            map.insert("eur", summa.toString());
            map.insert("kumppani", apukysely.value("kumppani"));
            eranMuutos += summa;
            muutokset.append(map);
        }
        if( !eranMuutos && !eranAloitus )
            continue;
        QVariantMap era;
        era.insert("id", erakysely.value(7).toInt());
        era.insert("vientipvm", erakysely.value(8).toDate());
        era.insert("pvm", erakysely.value(4).toDate());
        era.insert("sarja",erakysely.value(5));
        era.insert("tunniste", erakysely.value(6));
        era.insert("selite", erakysely.value("selite"));
        era.insert("kumppani", erakysely.value("kumppaninimi"));
        era.insert("eur", eranAloitus);

        QVariantMap emap;
        emap.insert("era", era);
        emap.insert("ennen", eraAlussa);
        emap.insert("kausi", eranMuutos);
        emap.insert("saldo",  eraAlussa + eranMuutos);
        erat.append(emap);

        erittellytAlussa += eraAlussa;
        eritellytLopussa += eraAlussa + eranMuutos;
    }

    Euro erittelematonAlussa = alkusaldo - erittellytAlussa;
    Euro erittelematonLopussa = loppusaldo - eritellytLopussa;
    Euro erittelematonKausiSumma;

    erakysely.exec(QString("select vienti.debetsnt, vienti.kreditsnt, vienti.selite, Tosite.pvm as pvm, Tosite.sarja, "
                           "Tosite.tunniste, tosite.id, Vienti.pvm as vientipvm, Kumppani.nimi AS kumppani "
                           "FROM Vienti  JOIN Tosite ON Vienti.tosite = Tosite.id  "
                           "LEFT OUTER JOIN Kumppani ON Vienti.kumppani = Kumppani.id "
                           "WHERE Vienti.tili=%1 AND Vienti.eraid IS NULL "
                           "AND Vienti.pvm BETWEEN '%2' AND '%3' AND Tosite.tila >= 100 ORDER BY Vienti.pvm")
                   .arg( tili->numero())
                   .arg(mista.toString(Qt::ISODate))
                   .arg(mihin.toString(Qt::ISODate)));

    while(erakysely.next()) {
        QVariantMap map;
        Euro summa = (tili->onko(TiliLaji::VASTAAVAA) ?
                    erakysely.value(0).toLongLong() - erakysely.value(1).toLongLong() :
                    erakysely.value(1).toLongLong() - erakysely.value(0).toLongLong()) ;
        map.insert("pvm", erakysely.value(3).toDate());
        map.insert("sarja", erakysely.value(4));
        map.insert("tunniste", erakysely.value(5));
        map.insert("id", erakysely.value(6).toInt());
        map.insert("vientipvm", erakysely.value(7).toDate());
        map.insert("selite", erakysely.value(2).toString());
        map.insert("eur", summa);
        map.insert("kumppani", erakysely.value("kumppani"));
        erittelematonKausiSumma += summa;
    }

    if( erittelematonAlussa || erittelematonLopussa || erittelematonKausiSumma) {
        QVariantMap emap;
        emap.insert("ennen", erittelematonAlussa);
        emap.insert("kausi", erittelematonLopussa - erittelematonLopussa);
        emap.insert("saldo", erittelematonLopussa);
        erat.append(emap);
    }
    return erat;
}

QVariant EraRoute::listaErittely(Tili *tili, const QDate & /* mista */, const QDate &mihin, const Euro & /* alkusaldo */, const Euro &loppusaldo)
{
    QSqlQuery apukysely( db() );
    QVariantList erat;
    Euro erittelematta = loppusaldo;

    apukysely.exec(QString("select vienti.eraid, sum(vienti.debetsnt) as sd, sum(vienti.kreditsnt) as sk, a.selite, tosite.pvm, "
                           "tosite.sarja, tosite.tunniste, Vienti.pvm, Kumppani.nimi AS Kumppani "
                           "FROM Vienti "
                           "join Vienti as a on vienti.eraid = a.id "
                           "join Tosite on vienti.tosite=tosite.id "
                           "LEFT OUTER JOIN Kumppani ON a.kumppani=Kumppani.id "
                           "WHERE vienti.tili=%1 AND vienti.pvm <= '%2'  AND Tosite.tila >= 100 GROUP BY vienti.eraid, a.selite, a.pvm, a.tili "
                           "HAVING sum(vienti.debetsnt) <> sum(vienti.kreditsnt) OR sum(vienti.debetsnt) IS NULL OR sum(vienti.kreditsnt) IS NULL;"
                           ).arg(tili->numero()).arg(mihin.toString(Qt::ISODate)));

    while( apukysely.next()) {
        QVariantMap era;
        era.insert("id", apukysely.value(0).toInt());
        era.insert("pvm", apukysely.value(4).toDate() );
        era.insert("sarja", apukysely.value(5));
        era.insert("tunniste", apukysely.value(6));
        era.insert("vientipvm", apukysely.value(7).toDate());
        era.insert("selite", apukysely.value(3));
        era.insert("kumppani", apukysely.value("kumppani"));
        Euro summa = Euro(tili->onko(TiliLaji::VASTAAVAA) ?
                    apukysely.value(1).toLongLong() - apukysely.value(2).toLongLong() :
                    apukysely.value(2).toLongLong() - apukysely.value(1).toLongLong() );
        era.insert("eur", summa);
        erat.append(era);
        erittelematta -= summa;
    }

    // Erittelemättömät loppuun
    if( erittelematta ) {
        QVariantMap erittelematon;
        erittelematon.insert("eur", erittelematta);
        erat.append(erittelematon);
    }
    return erat;
}

QVariant EraRoute::muutosErittely(Tili *tili, const QDate &mista, const QDate &mihin, const Euro &alkusaldo, const Euro &loppusaldo)
{
    QSqlQuery apukysely( db());
    apukysely.exec(QString("select vienti.debetsnt, vienti.kreditsnt, vienti.selite, Tosite.pvm, Tosite.sarja, "
                           "Tosite.tunniste, Vienti.pvm, Kumppani.nimi AS Kumppani "
                           "FROM Vienti JOIN Tosite ON Vienti.tosite = Tosite.id  "
                           "LEFT OUTER JOIN Kumppani ON Vienti.kumppani=Kumppani.id "
                           "WHERE Vienti.tili=%1 "
                           "AND Vienti.pvm BETWEEN '%2' AND '%3' AND Tosite.tila >= 100 ORDER BY vienti.pvm")
                   .arg(tili->numero())
                   .arg(mista.toString(Qt::ISODate))
                   .arg(mihin.toString(Qt::ISODate)));

    QVariantList muutokset;
    while( apukysely.next() )
    {
        QVariantMap map;
        Euro summa = Euro(tili->onko(TiliLaji::VASTAAVAA) ?
                    apukysely.value(0).toLongLong() - apukysely.value(1).toLongLong() :
                    apukysely.value(1).toLongLong() - apukysely.value(0).toLongLong()) ;

        map.insert("pvm", apukysely.value(3).toDate());
        map.insert("sarja", apukysely.value(4));
        map.insert("tunniste", apukysely.value(5));
        map.insert("vientipvm", apukysely.value(6).toDate());
        map.insert("selite", apukysely.value(2).toString());
        map.insert("kumppani", apukysely.value("kumppani"));
        map.insert("eur", summa);
        muutokset.append(map);
    }
    QVariantMap map;
    map.insert("saldo", loppusaldo);
    map.insert("kausi", muutokset);
    map.insert("ennen", alkusaldo);
    return map;
}
