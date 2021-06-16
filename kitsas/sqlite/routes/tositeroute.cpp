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
#include "kumppanitroute.h"

#include "kitsas.h"

#include "model/tosite.h"
#include "db/kirjanpito.h"
#include "db/tositetyyppimodel.h"

#include "laskutus/viitenumero.h"

#include <QJsonDocument>
#include <QDate>
#include <QSqlError>
#include <QDebug>
#include <QRegularExpression>

TositeRoute::TositeRoute(SQLiteModel *model) :
    SQLiteRoute(model, "/tositteet")
{

}

QString TositeRoute::kysymys(const QUrlQuery &urlquery)
{
    QStringList ehdot;
    if( urlquery.hasQueryItem("luonnos") )
        ehdot.append( QString("( tosite.tila >= %1 and tosite.tila < %2 )").arg(Tosite::LUONNOS).arg(Tosite::KIRJANPIDOSSA) );
    else if( urlquery.hasQueryItem("saapuneet"))
        ehdot.append( QString("( tosite.tila > %1 and tosite.tila < %2 )").arg(Tosite::MALLIPOHJA).arg(Tosite::LUONNOS) );
    else if( urlquery.hasQueryItem("malli"))
        ehdot.append(QString("tosite.tila = %1").arg(Tosite::MALLIPOHJA));
    else if( urlquery.hasQueryItem("poistetut"))
        ehdot.append("tosite.tila = 0");
    else
        ehdot.append( QString("tosite.tila >= %1").arg(Tosite::KIRJANPIDOSSA));

    if( urlquery.hasQueryItem("alkupvm"))
        ehdot.append( QString("tosite.pvm >= '%1'").arg( urlquery.queryItemValue("alkupvm") ));
    if( urlquery.hasQueryItem("loppupvm"))
        ehdot.append( QString("tosite.pvm <= '%1'").arg( urlquery.queryItemValue("loppupvm")));

    if( urlquery.hasQueryItem("pvm"))
        ehdot.append( QString("tosite.pvm = '%1'").arg(urlquery.queryItemValue("pvm")));
    if( urlquery.hasQueryItem("kumppani"))
        ehdot.append( QString("kumppani = %1").arg(urlquery.queryItemValue("kumppani")));
    if( urlquery.hasQueryItem("tyyppi"))
        ehdot.append( QString("tosite.tyyppi = %1").arg(urlquery.queryItemValue("tyyppi")));
    if( urlquery.hasQueryItem("laskupvm"))
        ehdot.append( QString("tosite.laskupvm = '%1'").arg(urlquery.queryItemValue("laskupvm")));
    if( urlquery.hasQueryItem("viite"))
        ehdot.append( QString("tosite.viite = '%1'").arg(urlquery.queryItemValue("viite").remove(QRegularExpression("\\W")).remove("^0+")));




    QString jarjestys = "pvm";
    if( urlquery.queryItemValue("jarjestys") == "tyyppi,tosite")
        jarjestys = "tyyppi,sarja,tunniste";
    else if( urlquery.queryItemValue("jarjestys") == "tosite")
        jarjestys = "sarja,tunniste";
    else if( urlquery.queryItemValue("jarjestys") == "tyyppi,pvm")
        jarjestys = "tyyppi,pvm";
    else if( urlquery.hasQueryItem("malli"))
        jarjestys = "otsikko";

    QString kysymys = "SELECT tosite.id AS id, tosite.pvm AS pvm, tyyppi, tila, tunniste, otsikko, kumppani.nimi as kumppani, "
                      "tosite.sarja as sarja, liitteita, summa "
                      " FROM Tosite LEFT OUTER JOIN Kumppani on tosite.kumppani=kumppani.id  "
                      " LEFT OUTER JOIN (SELECT tosite, COUNT(id) AS liitteita FROM Liite GROUP BY tosite) AS lq ON tosite.id=lq.tosite "
                      " LEFT OUTER JOIN (SELECT tosite, SUM(debetsnt) / 100.0 AS summa FROM Vienti GROUP BY tosite) as sq ON tosite.id=sq.tosite "
                      "WHERE ";
    kysymys.append( ehdot.join(" AND ") + QString(" ORDER BY ") + jarjestys );
    return kysymys;
}

QVariant TositeRoute::get(const QString &polku, const QUrlQuery &urlquery)
{
    if( !polku.isEmpty())
        return hae( polku.toInt() );
    else if( urlquery.hasQueryItem("vienti"))
        return hae( 0 - urlquery.queryItemValue("vienti").toInt());

    // Muuten tositteiden lista

    QSqlQuery kysely( db());
    kysely.exec(kysymys(urlquery));

    QVariantList tositteet = resultList(kysely);

    return tositteet;
}

QVariant TositeRoute::post(const QString & /*polku*/, const QVariant &data)
{
    return hae( lisaaTaiPaivita(data) );
}

QVariant TositeRoute::put(const QString &polku, const QVariant &data)
{
    return hae( lisaaTaiPaivita(data, polku.toInt() ) );
}

QVariant TositeRoute::patch(const QString &polku, const QVariant &data)
{
    QVariantMap map = data.toMap();
    int tositeid = polku.toInt();
    int tila = map.value("tila").toInt();

    // Haetaan tunniste
    db().transaction();
    QSqlQuery kysely(db());
    int tunniste = 0;

    kysely.exec(QString("SELECT tunniste, pvm, sarja FROM Tosite WHERE id=%1").arg(tositeid));
    if( kysely.next() ) {
        if( kysely.value(0).toInt())
            tunniste = kysely.value(0).toInt();
        else if( tila >= Tosite::KIRJANPIDOSSA) {
            QDate pvm = kysely.value(1).toDate();
            Tilikausi kausi = kp()->tilikaudet()->tilikausiPaivalle(pvm);
            QString sarja = kysely.value(2).toString();
            if( sarja.isEmpty()) {
                kysely.exec(QString("SELECT MAX(tunniste) FROM Tosite WHERE pvm BETWEEN '%1' AND '%2' AND sarja IS NULL")
                            .arg( kausi.alkaa().toString(Qt::ISODate) ).arg( kausi.paattyy().toString( Qt::ISODate )) );
            } else {
                kysely.exec(QString("SELECT MAX(tunniste) FROM Tosite WHERE pvm BETWEEN '%1' AND '%2' AND sarja = '%3'")
                            .arg(kausi.alkaa().toString(Qt::ISODate)).arg(kausi.paattyy().toString(Qt::ISODate)).arg(sarja));
            }
            if( kysely.next()) {
                tunniste = kysely.value(0).toInt() + 1;
            }
        }
    }

    // Päivitetään
    if(!kysely.exec(QString("UPDATE Tosite SET tila=%1, tunniste=%2 WHERE id=%3")
                .arg(tila).arg(tunniste).arg(tositeid)))
        throw SQLiteVirhe(kysely);

    // Lisätään tositelokiin
    kysely.prepare("INSERT INTO Tositeloki (tosite, tila, data) VALUES (?,?,?) ");
    kysely.addBindValue(tositeid);
    kysely.addBindValue(tila);
    kysely.addBindValue(mapToJson(map));
    kysely.exec();

    db().commit();
    return QVariant();

}

QVariant TositeRoute::doDelete(const QString &polku)
{
    int tositeid = polku.toInt();

    QSqlQuery kysely(db());

    if(!kysely.exec(QString("UPDATE Tosite SET tila=0 WHERE id=%1")
                .arg(tositeid)))
        throw SQLiteVirhe(kysely);

    // Lisätään tositelokiin
    kysely.prepare("INSERT INTO Tositeloki (tosite, tila) VALUES (?,0) ");
    kysely.addBindValue(tositeid);
    kysely.exec();

    return QVariant();
}



int TositeRoute::lisaaTaiPaivita(const QVariant pyynto, const int paivitettavanTositeId)
{
    QVariantMap map = pyynto.toMap();
    QByteArray lokiin = QJsonDocument::fromVariant(pyynto).toJson(QJsonDocument::Compact);

    QSqlQuery kysely(db());
    db().transaction();

    QDate pvm = map.take("pvm").toDate();
    int tyyppi = map.take("tyyppi").toInt();
    QVariantList viennit = map.take("viennit").toList();
    int tila = map.contains("tila") ? map.take("tila").toInt() : Tosite::KIRJANPIDOSSA;
    QString otsikko = map.take("otsikko").toString();
    QVariantList rivit = map.take("rivit").toList();
    int kumppani = kumppaniMapista(map);
    QVariantList liita = map.take("liita").toList();
    QString sarja = map.take("sarja").toString();
    int tunniste = map.take("tunniste").toInt();
    QDate laskupvm = map.take("laskupvm").toDate();
    QDate erapvm = map.take("erapvm").toDate();
    QString viitenro = map.take("viite").toString();


    Tilikausi kausi = kp()->tilikaudet()->tilikausiPaivalle(pvm);

    if( tunniste && paivitettavanTositeId) {
        // Tarkistetaan, pitääkö tunniste hakea uudelleen
        kysely.exec( QString("SELECT sarja, alkaa FROM Tosite JOIN Tilikausi ON Tosite.pvm BETWEEN Tilikausi.alkaa AND Tilikausi.loppuu "
                             "WHERE Tosite.id=%1").arg(paivitettavanTositeId) );
        if( !kysely.next() || kysely.value("alkaa").toDate() != kausi.alkaa() || kysely.value("sarja").toString() != sarja )
            tunniste = 0;
    }
    if( tila < Tosite::KIRJANPIDOSSA)
        tunniste = 0;

    // Tunnisteen hakeminen
    if( !tunniste && tila >= Tosite::KIRJANPIDOSSA) {
        if( sarja.isNull() )
            kysely.exec( QString("SELECT MAX(tunniste) as tunniste FROM Tosite WHERE pvm BETWEEN '%1' AND '%2' AND sarja IS NULL AND tila >= 100")
                         .arg(kausi.alkaa().toString(Qt::ISODate)).arg(kausi.paattyy().toString(Qt::ISODate)));
        else
            kysely.exec( QString("SELECT MAX(tunniste) as tunniste FROM Tosite WHERE pvm BETWEEN '%1' AND '%2' AND sarja='%3' AND tila >= 100")
                         .arg(kausi.alkaa().toString(Qt::ISODate)).arg(kausi.paattyy().toString(Qt::ISODate)).arg(sarja));
        if( kysely.next())
            tunniste = kysely.value("tunniste").toInt() + 1;
    }
    // Laskun numero ja viite
    if( map.contains("lasku") && !map.value("lasku").toMap().contains("numero") && tila >= Tosite::KIRJANPIDOSSA &&
            tyyppi >= TositeTyyppi::MYYNTILASKU && tyyppi <= TositeTyyppi::MAKSUMUISTUTUS) {
        // LaskuSeuraavaId käsitellään käsin, jotta ei tule päällekkäisiä numeroita
        // vaikka olisi monta instanssia.
        qulonglong laskunumero = 0;
        QSqlQuery laskunumerokysely( db());
        laskunumerokysely.exec("SELECT arvo FROM Asetus WHERE avain='LaskuSeuraavaId'");
        if( laskunumerokysely.next())
            laskunumero = laskunumerokysely.value(0).toULongLong();

        if( laskunumero < kp()->asetukset()->asetus("LaskuNumerointialkaa").toULongLong())
            laskunumero = kp()->asetukset()->asetus("LaskuNumerointialkaa").toULongLong();

        QVariantMap laskumap = map.value("lasku").toMap();
        kp()->asetukset()->aseta("LaskuSeuraavaId", laskunumero + 1);
        laskumap.insert("numero", laskunumero);

        if( viitenro.isEmpty()) {
            ViiteNumero viite(ViiteNumero::LASKU, laskunumero);
            laskumap.insert("viite", viite.viite() );
            viitenro = viite.viite();
        }
        map.insert("lasku", laskumap);
    }

    // Lisätään itse tosite
    QSqlQuery tositelisays(db());

    if( !laskupvm.isValid())
        laskupvm = pvm;

    if( paivitettavanTositeId ) {
        tositelisays.prepare("INSERT INTO Tosite (id, pvm, tyyppi, tila, tunniste, otsikko, kumppani, sarja, laskupvm, erapvm, viite, json) "
                             "VALUES (?,?,?,?,?,?,?,?,?,?,?,?) "
                             "ON CONFLICT(id) DO UPDATE "
                             "SET pvm=EXCLUDED.pvm, tyyppi=EXCLUDED.tyyppi, tila=EXCLUDED.tila, tunniste=EXCLUDED.tunniste, otsikko=EXCLUDED.otsikko, "
                             "kumppani=EXCLUDED.kumppani, sarja=EXCLUDED.sarja, laskupvm=EXCLUDED.laskupvm, erapvm=EXCLUDED.erapvm, viite=EXCLUDED.viite, json=EXCLUDED.json");

        tositelisays.addBindValue(paivitettavanTositeId);
    } else {
        tositelisays.prepare("INSERT INTO Tosite (pvm, tyyppi, tila, tunniste, otsikko, kumppani, sarja, laskupvm, erapvm, viite, json) "
                             "VALUES (?,?,?,?,?,?,?,?,?,?,?)");
    }
    tositelisays.addBindValue(pvm);
    tositelisays.addBindValue(tyyppi);
    tositelisays.addBindValue(tila);
    tositelisays.addBindValue(tunniste);
    tositelisays.addBindValue(otsikko);
    tositelisays.addBindValue(kumppani ? kumppani : QVariant());
    tositelisays.addBindValue(sarja);
    tositelisays.addBindValue(laskupvm);
    tositelisays.addBindValue(erapvm);
    tositelisays.addBindValue(viitenro);
    tositelisays.addBindValue( mapToJson(map) );
    tositelisays.exec();


    int tositeId = paivitettavanTositeId ? paivitettavanTositeId : tositelisays.lastInsertId().toInt();


    // Lisätään viennit
    QSet<int> vanhatviennit;
    if( paivitettavanTositeId) {
        kysely.exec( QString("SELECT id FROM Vienti WHERE tosite=%1").arg(paivitettavanTositeId));
        while(kysely.next())
            vanhatviennit.insert(kysely.value(0).toInt());
    }

    int rivinumero = 0;
    for( auto const& vientivar : viennit ) {
        QVariantMap vientimap = vientivar.toMap();

        int vientiid = paivitettavanTositeId ? vientimap.take("id").toInt() : 0;
        QDate vientipvm = vientimap.take("pvm").toDate();
        int tili = vientimap.take("tili").toInt();
        int kohdennus = vientimap.take("kohdennus").toInt();
        QString selite = vientimap.take("selite").toString();
        qlonglong debet = qRound64( vientimap.take("debet").toDouble() * 100 );
        qlonglong kredit =  qRound64( vientimap.take("kredit").toDouble() * 100);
        QVariantList merkkaukset = vientimap.take("merkkaukset").toList();
        int kumppani = kumppaniMapista(vientimap);
        QDate jaksoalkaa = vientimap.take("jaksoalkaa").toDate();
        QDate jaksoloppuu = vientimap.take("jaksoloppuu").toDate();
        int vientityyppi = vientimap.take("tyyppi").toInt();
        double alvprosentti = vientimap.value("alvprosentti",0.0).toDouble();
        vientimap.take("alvprosentti");
        int alvkoodi = vientimap.take("alvkoodi").toInt();
        int eraid = vientimap.take("era").toMap().value("id").toInt();
        QString arkistotunnus = vientimap.take("arkistotunnus").toString();

        rivinumero++;

        if( vientiid ) {
            vanhatviennit.remove(vientiid);
            kysely.prepare("INSERT INTO Vienti (id, tosite, pvm, tili, kohdennus, selite, debetsnt, kreditsnt, eraid, json, alvkoodi, alvprosentti, rivi, kumppani, jaksoalkaa, jaksoloppuu, tyyppi, arkistotunnus) "
                           "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
                           "ON CONFLICT (id) DO UPDATE SET tosite=EXCLUDED.tosite, pvm=EXCLUDED.pvm, tili=EXCLUDED.tili, kohdennus=EXCLUDED.kohdennus,"
                           "selite=EXCLUDED.selite, debetsnt=EXCLUDED.debetsnt, kreditsnt=EXCLUDED.kreditsnt, eraid=EXCLUDED.eraid,"
                           "json=EXCLUDED.json, alvkoodi=EXCLUDED.alvkoodi, alvprosentti=EXCLUDED.alvprosentti, rivi=EXCLUDED.rivi,"
                           "kumppani=EXCLUDED.kumppani, jaksoalkaa=EXCLUDED.jaksoalkaa, jaksoloppuu=EXCLUDED.jaksoloppuu,"
                           "tyyppi=EXCLUDED.tyyppi, arkistotunnus=EXCLUDED.arkistotunnus");
            kysely.addBindValue( vientiid );
        } else {
            kysely.prepare("INSERT INTO Vienti (tosite, pvm, tili, kohdennus, selite, debetsnt, kreditsnt, eraid, json, alvkoodi, alvprosentti, rivi, kumppani, jaksoalkaa, jaksoloppuu, tyyppi, arkistotunnus) "
                           "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) ");
        }
        kysely.addBindValue(tositeId);
        kysely.addBindValue(vientipvm);
        kysely.addBindValue(tili);
        kysely.addBindValue(kohdennus);
        kysely.addBindValue(selite);

        kysely.addBindValue( debet ? debet : QVariant());
        kysely.addBindValue( kredit ? kredit : QVariant());

        kysely.addBindValue( eraid > 0 ? eraid : QVariant());
        kysely.addBindValue( mapToJson(vientimap) );
        kysely.addBindValue( alvkoodi );
        kysely.addBindValue( alvkoodi ? QString::number(alvprosentti,'f',2) : QVariant());
        kysely.addBindValue( rivinumero );
        kysely.addBindValue( kumppani ? kumppani : QVariant());
        kysely.addBindValue( jaksoalkaa );
        kysely.addBindValue( jaksoloppuu );
        kysely.addBindValue( vientityyppi );
        kysely.addBindValue( arkistotunnus );

        kysely.exec();                


        if( !vientiid)
            vientiid = kysely.lastInsertId().toInt();

        // Uusi erä käyttöön
        if( eraid == Kitsas::UUSI_ERA)
            kysely.exec(QString("UPDATE Vienti SET eraid=%1 WHERE id=%1").arg(vientiid) );

        if( vientiid )
            kysely.exec(QString("DELETE FROM Merkkaus WHERE vienti=%1").arg(vientiid));

        // Merkkaukset
        for(const auto& merkkaus : merkkaukset) {
            kysely.exec(QString("INSERT INTO Merkkaus(vienti,kohdennus) VALUES (%1,%2)")
                        .arg(vientiid)
                        .arg(merkkaus.toInt()));
        }

    }

    // Kiinnitetään esilähetetyt liitteet
    for(const auto& liite : liita)
        kysely.exec(QString("UPDATE Liite SET tosite=%1 WHERE id=%2")
                    .arg(tositeId).arg(liite.toInt()) );

    if( paivitettavanTositeId )
        kysely.exec(QString("DELETE FROM Rivi WHERE tosite=%1").arg(paivitettavanTositeId));

    kysely.prepare("INSERT INTO Rivi(tosite,rivi,tuote,myyntikpl,ostokpl, ahinta, json) VALUES (?,?,?,?,?,?,?) ");
    for(int rivi=0; rivi < rivit.count(); rivi++)
    {
        QVariantMap rmap = rivit.at(rivi).toMap();
        kysely.addBindValue(tositeId);
        kysely.addBindValue(rivi + 1);
        kysely.addBindValue(rmap.take("tuote").toString());
        kysely.addBindValue(rmap.take("myyntikpl").toDouble());
        kysely.addBindValue(rmap.take("ostokpl").toDouble());
        kysely.addBindValue(rmap.take("ahinta").toDouble());
        kysely.addBindValue( mapToJson(rmap) );
        kysely.exec();
    }


    // Poistettujen poistamiset
    for(int poistoid : vanhatviennit)
        kysely.exec(QString("DELETE FROM Vienti WHERE id=%1").arg(poistoid));


    // Lisätään lokitieto
    kysely.prepare("INSERT INTO Tositeloki (tosite, tila, data) VALUES (?,?,?)");
    kysely.addBindValue(tositeId);
    kysely.addBindValue(tila);
    kysely.addBindValue(lokiin);
    kysely.exec();


    db().commit();
    return tositeId;
}

QVariantList TositeRoute::lokinpurku(QSqlQuery &kysely) const
{
    QVariantList lista;
    while( kysely.next()) {
        // Sijoitetaan ensin json-kenttä
        QVariantMap map;
        map.insert("data", QJsonDocument::fromJson( kysely.value("data").toString().toUtf8() ).toVariant().toMap());
        map.insert("aika", kysely.value("aika"));
        map.insert("tila", kysely.value("tila"));
        lista.append(map);
    }
    return lista;
}

QVariant TositeRoute::hae(int tositeId)
{
    QSqlQuery kysely(db());
    if( tositeId > 0)
        kysely.exec(QString("SELECT tosite.id as id, pvm, tyyppi, tila, tunniste, sarja, otsikko, Tosite.laskupvm, Tosite.erapvm, Tosite.viite, tosite.json as json, "
                        "tosite.kumppani as kumppani FROM Tosite "
                        "WHERE tosite.id=%1").arg(tositeId));
    else
        kysely.exec(QString("SELECT tosite.id as id, tosite.pvm as pvm, tosite.tyyppi as tosite, tila, tunniste, sarja, otsikko, Tosite.laskupvm, Tosite.erapvm, Tosite.viite, tosite.json as json, "
                        "tosite.kumppani as kumppani FROM Vienti JOIN Tosite ON Vienti.tosite=Tosite.id"
                        "WHERE vienti.id=%1").arg(0-tositeId));


    QVariantMap tosite = resultMap(kysely);

    const int kumppaniId = tosite.value("kumppani").toInt();
    if( kumppaniId ) {
        kysely.exec(QString("SELECT * FROM Kumppani WHERE id=%1").arg(kumppaniId));
        tosite.insert("kumppani", resultMap(kysely));
    }

    tositeId = tosite.value("id").toInt();

    // Viennit
    kysely.exec(QString("SELECT vienti.id as id, tyyppi, pvm, tili, kohdennus, selite, debetsnt, kreditsnt, eraid as era_id, alvprosentti, alvkoodi, "
                "kumppani.id as kumppani_id, kumppani.nimi as kumppani_nimi, jaksoalkaa, jaksoloppuu, arkistotunnus, vienti.json as json FROM Vienti "
                "LEFT OUTER JOIN kumppani ON vienti.kumppani=kumppani.id "
                "WHERE tosite=%1 ORDER BY rivi").arg(tositeId) );
    QVariantList viennit = resultList(kysely);
    taydennaEratJaMerkkaukset(viennit);
    tosite.insert("viennit", viennit);

    // Liitteet
    kysely.exec(QString("SELECT id, nimi, roolinimi, tyyppi, json FROM Liite WHERE tosite=%1").arg(tositeId));
    tosite.insert("liitteet", resultList(kysely));

    // Rivit
    kysely.exec(QString("SELECT tuote, myyntikpl, ostokpl, ahinta, json FROM Rivi WHERE tosite=%1")
                .arg(tositeId));
    tosite.insert("rivit", resultList(kysely));

    // Loki
    kysely.exec(QString("SELECT aika, tila, data FROM Tositeloki WHERE tosite=%1 ORDER BY aika DESC")
                .arg(tositeId));        
    tosite.insert("loki", lokinpurku(kysely));

    return tosite;
}

int TositeRoute::kumppaniMapista(QVariantMap &map)
{
    int kumppaniId = 0;

    QVariantMap kumppani = map.take("kumppani").toMap();
    if( kumppani.isEmpty())
        return 0;

    if( kumppani.contains("id"))
        kumppaniId = kumppani.value("id").toInt();

    const QString& nimi = kumppani.value("nimi").toString();
    if( !kumppaniId && nimi.isEmpty() )
        return 0;

    if( !kumppaniId && kumppaniCache_.contains(nimi))
        kumppaniId = kumppaniCache_.value(nimi);

    QSqlQuery kumppaniKysely( db() );

    if( !kumppaniId && !map.value("alvtunnus").toString().isEmpty()) {
        kumppaniKysely.exec(QString("SELECT id FROM Kumppani WHERE alvtunnus='%1'").arg(map.value("alvtunnus").toString()));
        if(kumppaniKysely.next()) {
            kumppaniId = kumppaniKysely.value("id").toInt();
        }
    }

    if( !kumppaniId && !map.value("iban").toList().isEmpty()) {
        for(const QVariant& iban : map.value("iban").toList()) {
            kumppaniKysely.exec(QString("SELECT kumppani FROM KumppaniIban WHERE iban='%1'").arg(iban.toString()));
            if(kumppaniKysely.next()) {
                kumppaniId = kumppaniKysely.value("kumppani").toInt();
                break;
            }
        }
    }

    if( !kumppaniId && !nimi.contains("'")) {
        kumppaniKysely.exec(QString("SELECT id FROM Kumppani WHERE nimi='%1'").arg(nimi));
        if(kumppaniKysely.next()) {
            kumppaniId = kumppaniKysely.value("id").toInt();
        }
    }


    // Kumppani pitää lisätä
    if(!kumppaniId) {
        kumppaniId = KumppanitRoute::kumppaninLisays(kumppani, kumppaniKysely) ;
        if( kumppaniId ) {
            kumppaniCache_.insert(nimi, kumppaniId);
        } else {
            db().rollback();
            throw SQLiteVirhe(kumppaniKysely);
       }
    } else if (!map.value("iban").toList().isEmpty()) {
        kumppaniKysely.prepare("INSERT INTO KumppaniIban (kumppani,iban) VALUES (?,?) ON CONFLICT (iban) DO UPDATE SET kumppani=EXCLUDED.kumppani");
        for(const auto& var : map.value("iban").toList()) {
            kumppaniKysely.addBindValue(kumppaniId);
            kumppaniKysely.addBindValue(var.toString());
            if(!kumppaniKysely.exec()) {
                db().rollback();
                throw SQLiteVirhe(kumppaniKysely);
            }
        }
    }

    return kumppaniId;
}
