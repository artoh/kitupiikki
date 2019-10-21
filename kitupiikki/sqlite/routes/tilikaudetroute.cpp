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
        kysely.exec( QString("SELECT sum(debet), sum(kredit) FROM vienti JOIN Tosite ON Vienti.tosite=Tosite.id WHERE vienti.pvm < '%1' AND CAST(tili as text) < '3' AND Tosite.tila >= 100 ")
                     .arg(map.value("loppuu").toString()) );
        if( kysely.next())
            map.insert("tase", QString::number(qMax( kysely.value(0).toDouble(), kysely.value(1).toDouble()),'f',2));
        // Tulos
        kysely.exec( QString("SELECT sum(kredit), sum(debet) FROM vienti JOIN Tosite ON Vienti.tosite=Tosite.id WHERE vienti.pvm BETWEEN '%1' AND '%2' AND Tosite.tila >= 100 AND CAST(tili as text) >= '3' ")
                     .arg(map.value("alkaa").toString())
                     .arg(map.value("loppuu").toString()) );
        if( kysely.next())
            map.insert("tulos", QString::number(kysely.value(0).toDouble() - kysely.value(1).toDouble(), 'f', 2));

        // Liikevaihto
        kysely.exec( QString("SELECT sum(kredit), sum(debet) FROM vienti "
                             "JOIN Tili ON Vienti.tili=Tili.numero JOIN Tosite ON vienti.tosite=tosite.id "
                             "WHERE vienti.pvm BETWEEN '%1' AND '%2' AND CAST(tili as text) >= '3' AND Tosite.tila >= 100 "
                             "AND (tili.tyyppi='CL' OR tili.tyyppi='CLX')")
                     .arg(map.value("alkaa").toString())
                     .arg(map.value("loppuu").toString()) );
        if( kysely.next())
            map.insert("liikevaihto", QString::number(kysely.value(0).toDouble() - kysely.value(1).toDouble(), 'f', 2));

        // Viimeiselle kaudelle viimeinen tosite
        kysely.exec(QString("SELECT MAX(pvm) FROM Tosite WHERE pvm BETWEEN '%1' AND '%2'")
                    .arg(map.value("alkaa").toString()).arg(map.value("paattyy").toString()) );
        if( kysely.next())
            map.insert("viimeinen", kysely.value(0));

        list[i] = map;
    }

    return list;
}

QVariant TilikaudetRoute::put(const QString &polku, const QVariant &data)
{
    QVariantMap map = data.toMap();
    QDate alkaa = QDate::fromString(polku, Qt::ISODate);
    QDate loppuu = map.take("loppuu").toDate();
    map.remove("alkaa");

    QSqlQuery kysely(db());
    kysely.prepare("INSERT INTO Tilikausi (alkaa,loppuu,json) VALUES (?,?,?) "
                   "ON CONFLICT (alkaa) DO UPDATE SET loppuu=EXCLUDED.loppuu, json=EXCLUDED.json");
    kysely.addBindValue(alkaa);
    kysely.addBindValue(loppuu);
    kysely.addBindValue( mapToJson(map) );
    kysely.exec();

    return QVariant();
}

QVariant TilikaudetRoute::doDelete(const QString &polku)
{
    QDate alkaa = QDate::fromString(polku, Qt::ISODate);
    db().exec(QString("DELETE FROM Tilikausi WHERE alkaa='%1'").arg(alkaa.toString(Qt::ISODate)));
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

        kysely.exec( QString("select tili.numero, sum(debet), sum(kredit), kohdennus from vienti "
                             "join tili on vienti.tili=tili.numero join tosite on vienti.tosite=tosite.id "
                             "where tili.tyyppi='APM' and vienti.pvm <= '2019-12-31' and tosite.tila >= 100 group by tili.numero, kohdennus order by tili.numero, kohdennus"));
        while( kysely.next()) {
            Tili* tili = kp()->tilit()->tili(kysely.value(0).toInt());
            if( !tili)
                continue;

            qlonglong summa = qRound64( kysely.value(1).toDouble() * 100.0 ) - qRound64( kysely.value(2).toDouble() * 100.0 );
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
        kysely.exec(QString("select tili.numero, vienti.eraid, sum(debet), sum(kredit) from vienti "
                            "join tili on vienti.tili=tili.numero join tosite on vienti.tosite=tosite.id"
                            " where tili.tyyppi='APT' and vienti.pvm <= '%1' and tosite.tila >= 100 group by tili.numero "
                            "order by tili.numero, vienti.eraid").arg(kausi.paattyy().toString(Qt::ISODate)) );

        while(kysely.next()) {
            int eraid = kysely.value(1).toInt();
            apukysely.exec(QString("select debet,kredit,selite,json,pvm, kohdennus from vienti where id=%1").arg(eraid));
            if( apukysely.next()) {
                QVariantMap jsonmap = QJsonDocument::fromJson( apukysely.value(3).toByteArray() ).toVariant().toMap();
                qDebug() << jsonmap;
                qlonglong alkusumma = qRound64( apukysely.value(0).toDouble() * 100) - qRound64( apukysely.value(1).toDouble() * 100);
                int poistokk = jsonmap.value("tasaerapoisto").toInt();
                qlonglong saldo = qRound64( kysely.value(2).toDouble() * 100) - qRound64( kysely.value(3).toDouble() * 100);
                QDate hankintapaiva = apukysely.value(4).toDate();

                int kuukauttaKulunut = kausi.paattyy().year() * 12 + kausi.paattyy().month() -
                                       hankintapaiva.year() * 12 - hankintapaiva.month() + 1;
                qlonglong laskennallinenpoisto = poistokk ? alkusumma * kuukauttaKulunut / poistokk : 0;
                if( laskennallinenpoisto > saldo)
                    laskennallinenpoisto = saldo;
                qlonglong poisto = alkusumma - saldo + laskennallinenpoisto;
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
    // Tarkistetaan, onko poistoja jo kirjattu
    kysely.exec(QString("SELECT id FROM Tosite WHERE pvm = '%1' "
                        "AND tyyppi=%2 AND tila >= 100 LIMIT 1")
                .arg(kausi.paattyy().toString(Qt::ISODate))
                .arg( TositeTyyppi::JAKSOTUS ));
    if( kysely.next()) {
        ulos.insert("jaksotukset","kirjattu");
    } else {
        QVariantList jaksotukset;
        // Haetaan jaksotettavat viennit
        kysely.exec(QString("select debet,kredit,tili,selite,jaksoalkaa,jaksoloppuu, kohdennus, tosite.pvm, tosite.sarja, tosite.tunniste, vienti.pvm from vienti "
                            "join tosite on vienti.tosite=tosite.id "
                            "where jaksoalkaa is not null and tosite.tila >= 100 "
                            "AND vienti.pvm >= '%1' ORDER BY tili, vienti.id")
                    .arg(kausi.alkaa().toString(Qt::ISODate)));
        while( kysely.next()) {
            qlonglong debet = qRound64( kysely.value(0).toDouble() * 100);
            qlonglong kredit = qRound64( kysely.value(1).toDouble() * 100);
            int tili = kysely.value(2).toInt();
            QString selite = kysely.value(3).toString();
            QDate alkaa = kysely.value(4).toDate();
            QDate loppuu = kysely.value(5).toDate();
            QDate vientipvm = kysely.value(10).toDate();

            if( vientipvm <= kausi.paattyy() && ( alkaa < kausi.alkaa() ||  ( loppuu.isValid() && loppuu <= kausi.paattyy()) || (!loppuu.isValid() && alkaa <= kausi.paattyy() ) ))
                continue;   // Kokonaan tämän vuoden puolella

            double jaksotettavaa = 1.0;
            if( vientipvm <= kausi.paattyy() && loppuu.isValid() && alkaa <= kausi.paattyy()) {
                qlonglong ennen = alkaa.daysTo( kausi.paattyy() );
                qlonglong jalkeen = kausi.paattyy().daysTo( loppuu );
                jaksotettavaa = 1.00 * jalkeen / (ennen + jalkeen);
            } else if( vientipvm > kausi.paattyy() && alkaa < kausi.alkaa() && loppuu.isValid()) {
                qlonglong ennen = alkaa.daysTo( kausi.alkaa());
                qlonglong jalkeen = kausi.alkaa().daysTo(loppuu);
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
    }

    return ulos;
}
