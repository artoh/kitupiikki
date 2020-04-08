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
#include "tuontitulkki.h"

#include "db/tositetyyppimodel.h"
#include "model/tositevienti.h"
#include "db/kirjanpito.h"

#include <QRegularExpression>
#include <QDebug>

TuontiTulkki::TuontiTulkki(SQLiteModel *model) :
    SQLiteRoute(model, "/tuontitulkki")
{

}

QVariant TuontiTulkki::post(const QString &/*polku*/, const QVariant &data)
{
    QVariantMap map = data.toMap();

    if( map.value("tyyppi").toInt() == TositeTyyppi::TILIOTE)
        return tiliote(map);    

    return QVariant();
}

QVariant TuontiTulkki::tiliote( QVariantMap &map)
{
    QVariantList tapahtumat = map.value("tapahtumat").toList();

    QMutableListIterator<QVariant> iter(tapahtumat);
    while( iter.hasNext()) {
        QVariantMap tapahtuma = iter.next().toMap();        
        if( tapahtuma.value("euro").toDouble() > 0)
            tilioteTulorivi(tapahtuma);
        else tilioteMenorivi(tapahtuma);

        iter.setValue(tapahtuma);
    }
    map.insert("tapahtumat", tapahtumat);
    return map;
}

void TuontiTulkki::tilioteTulorivi(QVariantMap &rivi)
{
    // Ensisijaisesti etsitään viitteellä
    QSqlQuery kysely( db() );

    QString viite = rivi.value("viite").toString();
    if( !viite.isEmpty()) {

        // Etsitään vakioviitettä
        kysely.exec(QString("SELECT tili, kohdennus, otsikko FROM Vakioviite WHERE viite=%1").arg(viite.toInt()));
        if( kysely.next()) {
            rivi.insert("selite", kysely.value("otsikko"));
            rivi.insert("tili", kysely.value("tili"));
            rivi.insert("kohdennus", kysely.value("kohdennus"));
            return;
        }

        // RF-muodossa olevat viitteet muutetaan kansalliseen muotoon
        if( viite.startsWith("RF"))
            viite = viite.mid(4);

        kysely.exec( QString("SELECT Vienti.eraid, Vienti.tili, Vienti.kumppani, Kumppani.nimi, Tosite.pvm, Vienti.selite, Tosite.tunniste, Tosite.sarja FROM Vienti "
                             "JOIN Tosite ON Vienti.tosite=Tosite.id "
                             "LEFT OUTER JOIN Kumppani ON Vienti.kumppani=Kumppani.id "
                             "WHERE Vienti.tyyppi=%1 AND "
                             "Tosite.Viite='%2' AND Tosite.tila >= 100")
                     .arg(TositeVienti::MYYNTI + TositeVienti::VASTAKIRJAUS)
                     .arg(viite));
        if( kysely.next()) {
           rivi.insert("saajamaksajaid", kysely.value(2));
           rivi.insert("saajamaksaja", kysely.value(3));
           QVariantMap eramap;
           eramap.insert("id", kysely.value(0));
           eramap.insert("pvm", kysely.value(4));
           eramap.insert("tunniste", kysely.value(6));
           eramap.insert("sarja", kysely.value(7));
           rivi.insert("era", eramap);
           rivi.insert("tili",kysely.value(1));
           rivi.insert("selite", kysely.value(5));
           return;
        }
    }

    QPair<int, QString> kumppani = kumppaniNimella( rivi.value("saajamaksaja").toString() );
    rivi.insert("saajamaksaja", kumppani.second);
    if( kumppani.first)
        rivi.insert("saajamaksajaid", kumppani.first);

    // Korkotulot kirjataan KTO-koodin perusteella
    if( rivi.value("ktokoodi").toInt() == 750) {
        rivi.insert("tili", kp()->asetukset()->luku("PankkiMaksettavakorko"));
        return;
    }

    if( kumppani.first) {

        QSqlQuery apukysely( db() );
        int tili = 0;
        QVariantMap era;

        // Yritetään löytää tähän kumppaniin liitetty oikean suuruinen erä
        kysely.exec( QString("SELECT Vienti.id, tosite.pvm, tili, tunniste, sarja FROM Vienti JOIN Tosite ON Vienti.tosite=Tosite.id WHERE vienti.tyyppi=%1 AND "
                             "kumppani=%2 AND debetsnt=%3 AND pvm<'%4'")
                     .arg(TositeVienti::MYYNTI + TositeVienti::VASTAKIRJAUS)
                     .arg(kumppani.first)
                     .arg( qRound64(rivi.value("euro").toDouble() * 100) )
                     .arg( rivi.value("pvm").toDate().toString(Qt::ISODate)) );

        qDebug() << kysely.lastQuery();

        while( kysely.next()) {
            apukysely.exec( QString("SELECT SUM(debetsnt), SUM(kreditsnt) FROM Vienti WHERE eraid=%1").arg(kysely.value(0).toInt()));
            if( apukysely.next() && apukysely.value(0).toLongLong() == qRound64(rivi.value("euro").toDouble() * 100) &&
                    apukysely.value(1).toLongLong() == 0l) {
                if(!tili) {
                    era.insert("id", kysely.value(0));
                    era.insert("pvm", kysely.value(1));
                    era.insert("tunniste", kysely.value(2));
                    era.insert("sarja", kysely.value(3));
                    tili = kysely.value(2).toInt();
                } else {
                    tili = -1;      // Löydetty monta, joten ei valita niistä yhtäkään
                }
            }
        }
        if( tili > 0) {
            rivi.insert("era", era);
            rivi.insert("tili", tili);
            return;
        }

        // Viimeisenä toivona etsitään sopivaa tulotiliä ;)

        kysely.exec(QString("SELECT tili FROM Vienti WHERE tyyppi=%1 AND kumppani=%2 GROUP BY tili ORDER BY count(tili) LIMIT 1")
                    .arg(TositeVienti::MYYNTI + TositeVienti::KIRJAUS).arg(kumppani.first));
        if( kysely.next())
            rivi.insert("tili", kysely.value(0));


    }
}

void TuontiTulkki::tilioteMenorivi(QVariantMap &rivi)
{
    // Yritetään löytää vastapuoli ensisijaisesti tilinumerolla;
    QPair<int,QString> kumppani;

    if( rivi.contains("iban"))
        kumppani = kumppaniIbanilla(rivi.value("iban").toString());
    if( !kumppani.first )
        kumppani = kumppaniNimella( rivi.value("saajamaksaja").toString());

    if( kumppani.first) {
        rivi.insert("saajamaksajaid", kumppani.first);
        rivi.insert("saajamaksaja", kumppani.second);
    }

    // VEROHALLINNON TUNNISTAMINEN
    // Tunnistetaan viitenumerosta, onko oma-aloitteista veroa vai ennakkoveroa
    if( rivi.value("saajamaksaja").toString().toUpper() == "VEROHALLINTO") {
        QString viite = rivi.value("viite").toString();
        if( kp()->asetus("VeroTuloViite").endsWith(viite)) {
            rivi.insert("tili", kp()->asetukset()->luku("Tuloveroennakkotili"));
            return;
        } else {
            rivi.insert("tili", kp()->asetukset()->luku("VeroOmaverotili"));
            return;
        }

    }

    // Pankkimaksut
    if( rivi.value("ktokoodi").toInt() == 730)
        rivi.insert("tili", kp()->asetukset()->luku("PankkiPalvelumaksutili"));
    else if( rivi.value("ktokoodi").toInt() == 740)
        rivi.insert("tili", kp()->asetukset()->luku("PankkiMaksettavakorko"));
    else if( kumppani.first){

        // 1) Viitemaksun etsiminen
        QSqlQuery kysely( db() );

        kysely.exec( QString("SELECT Vienti.eraid, Vienti.tili, Vienti.selite, Tosite.pvm, Tosite.tunniste, Tosite.sarja FROM Vienti "
                             "JOIN Tosite ON Vienti.tosite=Tosite.id "
                             "WHERE Vienti.tyyppi=%1 AND "
                             "Tosite.Viite='%2' AND Vienti.kumppani=%3 AND Tosite.tila >= 100")
                     .arg(TositeVienti::OSTO + TositeVienti::VASTAKIRJAUS)
                     .arg(rivi.value("viite").toString())
                     .arg(kumppani.first));
        if( kysely.next()) {
           QVariantMap eramap;
           eramap.insert("id", kysely.value(0));
           eramap.insert("pvm", kysely.value(3));
           eramap.insert("tunniste", kysely.value(4));
           eramap.insert("sarja", kysely.value(5));
           rivi.insert("era", eramap);
           rivi.insert("tili",kysely.value(1));
           rivi.insert("selite", kysely.value(2));
           return;
        }
        // 2) Viitteettömän erän etsiminen
        QSqlQuery apukysely( db() );
        int tili = 0;
        QDate pvm;
        QVariantMap era;

        // Yritetään löytää tähän kumppaniin liitetty oikean suuruinen erä
        kysely.exec( QString("SELECT id, tosite.pvm, tili, tunniste, sarja FROM Vienti "
                             "JOIN Tosite ON Vienti.tosite=Tosite.id WHERE vienti.tyyppi=%1 AND "
                             "kumppani=%2 AND kreditsnt=%3 AND pvm<'%4' AND Tosite.tila>=100")
                     .arg(TositeVienti::OSTO + TositeVienti::VASTAKIRJAUS)
                     .arg(kumppani.first)
                     .arg( qRound64(rivi.value("euro").toDouble() * 100) )
                     .arg( rivi.value("pvm").toDate().toString(Qt::ISODate)) );

        qDebug() << kysely.lastQuery();

        while( kysely.next()) {
            apukysely.exec( QString("SELECT SUM(debetsnt), SUM(kreditsnt) FROM Vienti WHERE eraid=%1").arg(kysely.value(0).toInt()));
            if( apukysely.next() && apukysely.value(1).toLongLong() == qRound64(rivi.value("euro").toDouble() * 100) &&
                    apukysely.value(0).toLongLong() == 0l) {
                if(!tili) {
                    era.insert("id", kysely.value(0));
                    era.insert("pvm", kysely.value(1));
                    era.insert("tunniste", kysely.value(3));
                    era.insert("sarja", kysely.value(4));
                    tili = kysely.value(2).toInt();
                } else {
                    tili = -1;      // Löydetty monta, joten ei valita niistä yhtäkään
                }
            }
        }
        if( tili > 0) {
            rivi.insert("era", era);
            rivi.insert("tili", tili);
            rivi.insert("laskupvm",pvm);
            return;
        }

        // 3) Viimeisenä toivona etsitään sopivaa menotiliä ;)

        kysely.exec(QString("SELECT tili FROM Vienti WHERE tyyppi=%1 AND kumppani=%2 GROUP BY tili ORDER BY count(tili) LIMIT 1")
                    .arg(TositeVienti::OSTO + TositeVienti::KIRJAUS).arg(kumppani.first));
        if( kysely.next())
            rivi.insert("tili", kysely.value(0));

    }
}

QPair<int,QString> TuontiTulkki::kumppaniNimella(const QString &nimi)
{
    QSqlQuery kysely( db());

    // Siivotaan skandeja, jotta saadaan merkkikokoriippumaton haku
    QString hakunimi(nimi);
    hakunimi.replace(QRegularExpression("[åäöÅÄÖ']"),"_");

    kysely.exec( QString("SELECT id, nimi FROM Kumppani WHERE nimi LIKE '%1'").arg(hakunimi));
    if( kysely.next()) {
        return qMakePair( kysely.value(0).toInt(), kysely.value(1).toString() );
    }

    // Muuten tehdään nimihaku paloista, jolloin useimmat henkilönimet löytyvät
    QStringList paloina = hakunimi.split(QRegularExpression("\\s"));
    if( paloina.size() > 1) {
        kysely.exec( "SELECT id,nimi FROM Kumppani WHERE nimi LIKE '%" + paloina.value(0)
                     + "%' AND nimi LIKE '%" + paloina.value(1) + "%' ");
        if( kysely.next()) {
            return qMakePair( kysely.value(0).toInt(), kysely.value(1).toString() );
        }
    }

    return qMakePair(0, nimi);
}

QPair<int, QString> TuontiTulkki::kumppaniIbanilla(const QString &iban)
{
    QSqlQuery kysely( db());
    kysely.exec(QString("SELECT id, nimi FROM KumppaniIban JOIN Kumppani ON KumppaniIban.kumppani=Kumppani.id WHERE iban='%1'")
                .arg(iban));
    if( kysely.next())
        return qMakePair(kysely.value(0).toInt(), kysely.value(1).toString());
    return qMakePair(0, QString());
}
