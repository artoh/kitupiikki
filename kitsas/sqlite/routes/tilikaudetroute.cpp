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
#include "tilikaudetroute.h"
#include "db/kirjanpito.h"
#include "db/tositetyyppimodel.h"
#include <QDate>
#include <QJsonDocument>
#include <QVariant>
#include <QDebug>
#include <QSqlTableModel>
#include <QSqlRecord>

TilikaudetRoute::TilikaudetRoute(SQLiteModel *model) :
    SQLiteRoute(model, "/tilikaudet")
{

}

QVariant TilikaudetRoute::get(const QString &polku, const QUrlQuery &/*urlquery*/)
{
    if( QDate::fromString(polku, Qt::ISODate).isValid() )
        return laskelma( kp()->tilikaudet()->tilikausiPaivalle( QDate::fromString(polku, Qt::ISODate) ) );

    QSqlQuery kysely(db());
    kysely.exec("SELECT alkaa,loppuu,json FROM Tilikausi ORDER BY alkaa");

    QVariantList list = resultList(kysely);
    for(int i=0; i < list.count(); i++){
        QVariantMap map = list.at(i).toMap();

        // Tase
        kysely.exec( QString("SELECT sum(debetsnt), sum(kreditsnt) FROM vienti JOIN Tosite ON Vienti.tosite=Tosite.id WHERE vienti.pvm <= '%1' AND CAST(tili as text) < '2' AND Tosite.tila >= 100 ")
                     .arg(map.value("loppuu").toString()) );
        if( kysely.next())
            map.insert("tase", QString::number( qAbs(kysely.value(1).toLongLong() - kysely.value(0).toLongLong()) / 100.0,'f',2));
        // Tulos
        kysely.exec( QString("SELECT sum(kreditsnt), sum(debetsnt) FROM vienti JOIN Tosite ON Vienti.tosite=Tosite.id WHERE vienti.pvm BETWEEN '%1' AND '%2' AND Tosite.tila >= 100 AND CAST(tili as text) >= '3' ")
                     .arg(map.value("alkaa").toString())
                     .arg(map.value("loppuu").toString()) );
        if( kysely.next())
            map.insert("tulos", QString::number((kysely.value(0).toLongLong() - kysely.value(1).toLongLong()) / 100.0, 'f', 2));

        // Liikevaihto
        kysely.exec( QString("SELECT sum(kreditsnt), sum(debetsnt) FROM vienti "
                             "JOIN Tili ON Vienti.tili=Tili.numero JOIN Tosite ON vienti.tosite=tosite.id "
                             "WHERE vienti.pvm BETWEEN '%1' AND '%2' AND CAST(tili as text) >= '3' AND Tosite.tila >= 100 "
                             "AND (tili.tyyppi='CL' OR tili.tyyppi='CLZ')")
                     .arg(map.value("alkaa").toString())
                     .arg(map.value("loppuu").toString()) );
        if( kysely.next())
            map.insert("liikevaihto", QString::number((kysely.value(0).toLongLong() - kysely.value(1).toLongLong()) / 100.0, 'f', 2));

        // Kauden viimeinen tosite
        kysely.exec(QString("SELECT MAX(pvm) FROM Tosite WHERE pvm BETWEEN '%1' AND '%2' AND Tosite.tila >= 100")
                    .arg(map.value("alkaa").toString()).arg(map.value("loppuu").toString()) );
        qDebug() << kysely.lastQuery();
        if( kysely.next())
            map.insert("viimeinen", kysely.value(0));

        // Tilikauden päivitys
        kysely.exec(QString("SELECT MAX(aika) FROM Tositeloki JOIN Tosite ON Tositeloki.tosite=Tosite.id "
                            "WHERE Tosite.pvm BETWEEN '%1' AND '%2' AND Tosite.tila >= 100").arg(map.value("alkaa").toString()).arg(map.value("loppuu").toString()) );
        if( kysely.next()) {
            QDateTime paivitetty = kysely.value(0).toDateTime();
            paivitetty.setTimeSpec(Qt::UTC);
            map.insert("paivitetty", paivitetty.toLocalTime());
        }

        list[i] = map;
    }

    return list;
}

QVariant TilikaudetRoute::put(const QString &polku, const QVariant &data)
{
    QVariantMap map = data.toMap();

    QDate vanhaAlkaa = QDate::fromString(polku, Qt::ISODate);
    QDate loppuu = map.take("loppuu").toDate();
    QDate uusiAlkaa = map.take("alkaa").toDate();

    if(vanhaAlkaa == uusiAlkaa) {
        QSqlQuery kysely(db());
        kysely.prepare("INSERT INTO Tilikausi (alkaa,loppuu,json) VALUES (?,?,?) "
                       "ON CONFLICT (alkaa) DO UPDATE SET loppuu=EXCLUDED.loppuu, json=EXCLUDED.json");
        kysely.addBindValue(uusiAlkaa);
        kysely.addBindValue(loppuu);
        kysely.addBindValue( mapToJson(map) );
        kysely.exec();
    } else {
        QSqlTableModel model( nullptr, db() );
        model.setTable("Tilikausi");
        model.setFilter(QString("alkaa = '%1'").arg(vanhaAlkaa.toString(Qt::ISODate)));
        model.select();
        if(model.rowCount() == 1) {
            QSqlRecord record = model.record(0);
            record.setValue("alkaa", uusiAlkaa);
            record.setValue("loppuu", loppuu);
            record.setValue("json", mapToJson(map));
            model.setRecord(0, record);
            model.submitAll();
        }
    }

    return QVariant();
}

QVariant TilikaudetRoute::doDelete(const QString &polku)
{
    QDate alkaa = QDate::fromString(polku, Qt::ISODate);
    db().exec(QString("DELETE FROM Tilikausi WHERE alkaa='%1'").arg(alkaa.toString(Qt::ISODate)));
    return QVariant();
}

QVariant TilikaudetRoute::post(const QString &polku, const QVariant &data)
{
    if( polku !="numerointi")
        return QVariant();

    QVariantMap map = data.toMap();
    QString alkaa = map.value("alkaa").toDate().toString(Qt::ISODate);
    QString loppuu = map.value("loppuu").toDate().toString(Qt::ISODate);

    QDate alku = map.value("alkaa").toDate();
    QDate kausialkaa = kp()->tilikaudet()->tilikausiPaivalle(alku).alkaa();
    bool alusta = alku == kausialkaa;

    QMap<QString,int> numerot;

    QSqlQuery query(db());

    if( !alusta) {

    query.exec(QString("SELECT sarja, MAX(tunniste) FROM Tosite "
               "WHERE pvm < '%1' AND pvm >= '%2' AND tila >= 100 "
                "GROUP BY tunniste").arg(alkaa).arg(kausialkaa.toString(Qt::ISODate)));
    while( query.next()) {
        int min = query.value(1).toInt() + 1;
        numerot.insert(query.value(0).toString(), min);
    }
    }


    query.exec(QString("SELECT id, sarja FROM Tosite "
               "WHERE pvm BETWEEN '%1' AND '%2' AND tila >= 100 "
                "ORDER BY sarja, pvm, id").arg(alkaa).arg(loppuu));

    db().transaction();
    QSqlQuery update(db());

    while( query.next()) {
        int id = query.value(0).toInt();
        QString sarja = query.value(1).toString();

        int tunniste =  numerot.contains(sarja) ? numerot.value(sarja) : 1;
        numerot[sarja] = tunniste + 1;
        update.exec(QString("UPDATE Tosite SET tunniste=%1 WHERE id=%2")
                .arg(tunniste).arg(id));
    }
    db().commit();
    return QVariant();
}

QVariant TilikaudetRoute::laskelma(const Tilikausi &kausi)
{
    QSqlQuery kysely( db() );
    QVariantMap ulos;


    // Laaditaan poisto- ja jaksotuslaskelmat

    // Tarkistetaan, onko poistoja jo kirjattu
    kysely.exec(QString("SELECT id FROM Tosite WHERE pvm = '%1' "
                        "AND tyyppi=%2 AND tila >= 100 LIMIT 1")
                .arg(kausi.paattyy().toString(Qt::ISODate))
                .arg( TositeTyyppi::POISTOLASKELMA ));
    if( kysely.next() ) {
        ulos.insert("poistot","kirjattu");
    } else {
        QVariantList poistot;

        // Poistolaskelmaa ei ole vielä kirjattu, joten lasketaan poistot

        // Ensin menojäännöspoistot

        kysely.exec( QString("select tili.numero, sum(debetsnt), sum(kreditsnt), kohdennus from vienti "
                             "join tili on vienti.tili=tili.numero join tosite on vienti.tosite=tosite.id "
                             "where tili.tyyppi='APM' and vienti.pvm <= '%1' and tosite.tila >= 100 group by tili.numero, kohdennus order by tili.numero, kohdennus")
                     .arg(kausi.paattyy().toString(Qt::ISODate)));
        while( kysely.next()) {
            Tili* tili = kp()->tilit()->tili(kysely.value(0).toInt());
            if( !tili)
                continue;

            qlonglong summa = kysely.value(1).toLongLong() -  kysely.value(2).toLongLong();
            int poistoprosentti = tili->luku("menojaannospoisto");
            qlonglong poisto = poistoprosentti * summa / 100;

            QVariantMap map;
            map.insert("tili", tili->numero());
            map.insert("ennen", summa / 100.0);
            map.insert("poisto", poisto / 100.0);
            map.insert("kohdennus", kysely.value(3).toInt());
            poistot.append(map);

        }
        // Laaditaan laskelma APT-eristä
        // Ladataan avoimet erät
        QSqlQuery apukysely( db() );
        kysely.exec(QString("select tili.numero, vienti.eraid as eraid, sum(debetsnt), sum(kreditsnt) from vienti "
                            "join tili on vienti.tili=tili.numero join tosite on vienti.tosite=tosite.id "
                            "where tili.tyyppi='APT' and vienti.pvm <= '%1' and tosite.tila >= 100 group by tili.numero, vienti.eraid "
                            "order by tili.numero, vienti.eraid").arg(kausi.paattyy().toString(Qt::ISODate)) );

        while(kysely.next()) {
            int eraid = kysely.value("eraid").toInt();

            apukysely.exec(QString("select debetsnt,kreditsnt,selite,json,pvm, kohdennus from vienti where id=%1").arg(eraid));
            if( apukysely.next()) {
                QVariantMap jsonmap = QJsonDocument::fromJson( apukysely.value(3).toByteArray() ).toVariant().toMap();
                qlonglong alkusumma = apukysely.value(0).toLongLong() - apukysely.value(1).toLongLong();
                int poistokk = jsonmap.value("tasaerapoisto").toInt();
                qlonglong saldo = kysely.value(2).toLongLong() - kysely.value(3).toLongLong();
                QDate hankintapaiva = apukysely.value(4).toDate();

                int kuukauttaKulunut = kausi.paattyy().year() * 12 + kausi.paattyy().month() -
                                       hankintapaiva.year() * 12 - hankintapaiva.month() + 1;
                qlonglong laskennallinenpoisto = poistokk ? alkusumma * kuukauttaKulunut / poistokk : 0;
                if( laskennallinenpoisto > saldo)
                    laskennallinenpoisto = saldo;
                qlonglong poisto = laskennallinenpoisto - alkusumma + saldo;
                if( poisto > 0) {
                    QVariantMap map;
                    map.insert("eraid", eraid);
                    map.insert("tili", kysely.value(0).toInt());
                    map.insert("nimike", apukysely.value(2).toString());
                    map.insert("pvm", hankintapaiva);
                    map.insert("ennen", saldo / 100.0);
                    map.insert("poisto", poisto / 100.0);
                    map.insert("poistoaika", poistokk );
                    map.insert("kohdennus", apukysely.value(5).toInt());
                    poistot.append(map);
                }
            }

        }
        ulos.insert("poistot", poistot);
    }


    // Tarkistetaan, onko jaksotukset jo kirjattu
    kysely.exec(QString("SELECT id FROM Tosite WHERE pvm = '%1' "
                        "AND tyyppi=%2 AND tila >= 100 LIMIT 1")
                .arg(kausi.paattyy().toString(Qt::ISODate))
                .arg( TositeTyyppi::JAKSOTUS ));
    if( kysely.next()) {
        ulos.insert("jaksotukset","kirjattu");
    } else {
        QVariantList jaksotukset;
        // Haetaan jaksotettavat viennit
        kysely.exec(QString("select debetsnt,kreditsnt,tili,selite,jaksoalkaa,jaksoloppuu, kohdennus, tosite.pvm, tosite.sarja, tosite.tunniste, vienti.pvm from vienti "
                            "join tosite on vienti.tosite=tosite.id "
                            "where jaksoalkaa is not null and tosite.tila >= 100 "
                            "AND vienti.pvm >= '%1' AND jaksoalkaa IS NOT NULL ORDER BY tili, vienti.id")
                    .arg(kausi.alkaa().toString(Qt::ISODate)));
        while( kysely.next()) {
            qlonglong debet = kysely.value(0).toLongLong();
            qlonglong kredit = kysely.value(1).toLongLong();
            int tili = kysely.value(2).toInt();
            QString selite = kysely.value(3).toString();
            QDate alkaa = kysely.value(4).toDate();
            QDate loppuu = kysely.value(5).toDate();
            QDate vientipvm = kysely.value(10).toDate();

            if( vientipvm <= kausi.paattyy() && ( alkaa < kausi.alkaa()
                                                  ||  ( loppuu.isValid() && loppuu <= kausi.paattyy())
                                                  || (!loppuu.isValid() && alkaa <= kausi.paattyy() ) ))
                continue;   // Kokonaan tämän vuoden puolella
            else if (vientipvm > kausi.paattyy() && alkaa > kausi.paattyy())
                continue;   // Kokonaan tulevaisuudessa

            double jaksotettavaa = 1.0;
            if( vientipvm <= kausi.paattyy() && loppuu.isValid() && alkaa <= kausi.paattyy()) {
                qlonglong ennen = alkaa.daysTo( kausi.paattyy() ) + 1;
                qlonglong jalkeen = kausi.paattyy().daysTo( loppuu ) - 1;
                jaksotettavaa = 1.00 * jalkeen / (ennen + jalkeen);
            } else if( vientipvm > kausi.paattyy() && alkaa < kausi.paattyy() && loppuu.isValid()) {
                qlonglong ennen = alkaa.daysTo( kausi.paattyy()) + 1;
                qlonglong jalkeen = kausi.paattyy().daysTo(loppuu) - 1;
                jaksotettavaa = 1.00 * ennen / (ennen + jalkeen);
            }

            qlonglong jaksodebet = vientipvm <= kausi.paattyy() ? qRound64( jaksotettavaa * kredit ) : qRound64( jaksotettavaa * debet );
            qlonglong jaksokredit = vientipvm <= kausi.paattyy() ? qRound64( jaksotettavaa * debet ) : qRound64( jaksotettavaa * kredit );

            QVariantMap map;
            map.insert("tili", tili);
            if( qAbs(jaksodebet) > 1e-5)
                map.insert("debet", jaksodebet / 100.0);
            if( qAbs(jaksokredit) > 1e-5)
                map.insert("kredit", jaksokredit / 100.0);
            map.insert("selite", selite);
            map.insert("jaksoalkaa", alkaa);
            if( loppuu.isValid())
                map.insert("jaksoloppuu", loppuu);
            map.insert("kohdennus", kysely.value(6).toInt());
            map.insert("pvm", kysely.value(7).toDate());
            map.insert("sarja", kysely.value(8));
            map.insert("tunniste", kysely.value(9));
            jaksotukset.append(map);
        }
        ulos.insert("jaksotukset", jaksotukset);

        // Negatiivinen alv-velka jaksotetaan alv-saamiseksi tilinpäätökseen
        kysely.exec(QString("SELECT sum(debetsnt), sum(kreditsnt) FROM Vienti JOIN Tosite ON Vienti.tosite=Tosite.id "
                            "WHERE tili=%1 AND tila>=100 AND vienti.pvm <= '%2'")
                    .arg(kp()->tilit()->tiliTyypilla(TiliLaji::VEROVELKA).numero())
                    .arg(kausi.paattyy().toString(Qt::ISODate)));        
        if( kysely.next() ) {
            qlonglong velka = kysely.value(0).toLongLong() - kysely.value(1).toLongLong();
            if( velka > 0)
                ulos.insert("verosaaminen", velka / 100.0);
        }
    }

    verolaskelma( kausi, ulos);

    return ulos;
}

void TilikaudetRoute::verolaskelma(const Tilikausi &kausi, QVariantMap &ulos)
{
    QSqlQuery kysely(db());
    // Tarkistetaan, onko tulovero jo kirjattu
    kysely.exec(QString("SELECT id FROM Tosite WHERE pvm = '%1' "
                        "AND tyyppi=%2 AND tila >= 100 LIMIT 1")
                .arg(kausi.paattyy().toString(Qt::ISODate))
                .arg( TositeTyyppi::TULOVERO ));
    if( kysely.next()) {
        ulos.insert("tulovero","kirjattu");
    } else {
        QVariantMap vmap;
        // Tulot, Vähennykset ja Ennakkot
        // TULOT, tilityypit C ja CL
        kysely.exec(QString("SELECT sum(kreditsnt), sum(debetsnt) FROM Vienti JOIN Tosite ON Vienti.tosite=Tosite.id "
                    "JOIN Tili ON Vienti.tili=Tili.numero "
                    "WHERE Tosite.tila >= 100 AND (Tili.tyyppi='C' OR Tili.tyyppi='CL') AND "
                    "Vienti.pvm BETWEEN '%1' AND '%2'").arg(kausi.alkaa().toString(Qt::ISODate))
                                                       .arg(kausi.paattyy().toString(Qt::ISODate)));
        if( kysely.next() )
            vmap.insert("tulo", (kysely.value(0).toLongLong() - kysely.value(1).toLongLong()) / 100.0 );

        // VÄHENNYKSET
        qlonglong vahennykset = 0;
        kysely.exec(QString("SELECT sum(kreditsnt), sum(debetsnt) FROM Vienti JOIN Tosite ON Vienti.tosite=Tosite.id "
                    "JOIN Tili ON Vienti.tili=Tili.numero "
                    "WHERE Tosite.tila >= 100 AND (Tili.tyyppi='D' or Tili.tyyppi='DP') AND "
                    "Vienti.pvm BETWEEN '%1' AND '%2'").arg(kausi.alkaa().toString(Qt::ISODate))
                                                       .arg(kausi.paattyy().toString(Qt::ISODate)));
        if( kysely.next() )
            vahennykset = kysely.value(1).toLongLong() - kysely.value(0).toLongLong() ;

        // 50% VÄHENNYKSET
        kysely.exec(QString("SELECT sum(kreditsnt), sum(debetsnt) FROM Vienti JOIN Tosite ON Vienti.tosite=Tosite.id "
                    "JOIN Tili ON Vienti.tili=Tili.numero "
                    "WHERE Tosite.tila >= 100 AND Tili.tyyppi='DH' AND "
                    "Vienti.pvm BETWEEN '%1' AND '%2'").arg(kausi.alkaa().toString(Qt::ISODate))
                                                       .arg(kausi.paattyy().toString(Qt::ISODate)));
        if( kysely.next() )
            vahennykset += ( kysely.value(1).toLongLong() - kysely.value(0).toLongLong()) / 2;
        vmap.insert("vahennys", vahennykset / 100.0);

        // ENNAKOT
        kysely.exec(QString("SELECT sum(kreditsnt), sum(debetsnt) FROM Vienti JOIN Tosite ON Vienti.tosite=Tosite.id "
                    "JOIN Tili ON Vienti.tili=Tili.numero "
                    "WHERE Tosite.tila >= 100 AND Tili.tyyppi='DVE' AND "
                    "Vienti.pvm BETWEEN '%1' AND '%2'").arg(kausi.alkaa().toString(Qt::ISODate))
                                                       .arg(kausi.paattyy().toString(Qt::ISODate)));
        if( kysely.next() )
            vmap.insert("ennakko",(kysely.value(1).toLongLong() - kysely.value(0).toLongLong()) / 100);

        ulos.insert("tulovero", vmap);
    }
}
