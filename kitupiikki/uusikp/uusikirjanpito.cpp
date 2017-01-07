/*
   Copyright (C) 2017 Arto Hyvättinen

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

#include <QDir>

#include <QPixmap>
#include <QIcon>

#include <QFile>
#include <QTextStream>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QMapIterator>

#include <QApplication>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

#include "uusikirjanpito.h"

#include "introsivu.h"
#include "nimisivu.h"
#include "tilikarttasivu.h"
#include "tilikausisivu.h"
#include "sijaintisivu.h"
#include "loppusivu.h"

UusiKirjanpito::UusiKirjanpito() :
    QWizard()
{
    setWindowIcon(QIcon(":/pic/Possu64.png"));
    setWindowTitle("Uuden kirjanpidon luominen");
    setPixmap( WatermarkPixmap, QPixmap(":/pic/Possu64.png") );
    addPage( new IntroSivu());
    addPage( new NimiSivu());
    addPage( new TilikarttaSivu);
    addPage( new TilikausiSivu );
    addPage( new SijaintiSivu );
    addPage( new LoppuSivu );
}


QString UusiKirjanpito::aloitaUusiKirjanpito()
{
    UusiKirjanpito velho;

    velho.exec();

    if( velho.alustaKirjanpito())
        // Palautetaan uuden kirjanpidon hakemistopolku
        return velho.field("sijainti").toString() + "/" + velho.field("hakemisto").toString();
    else
        // Epäonnistui, tyhjä merkkijono
        return QString();
}

QMap<QString, QStringList> UusiKirjanpito::lueKtkTiedosto(const QString &polku)
{
    // ktk-tiedosto koostuu osista, jotka merkitään [otsikko] ja
    // niiden väleissä olevista tiedoista. Rivi voidaan
    // kommentoida #-merkillä


    QMap<QString, QStringList> tiedot;

    QFile tiedosto(polku);
    if( tiedosto.open(QIODevice::ReadOnly))
    {
        QTextStream in(&tiedosto);
        in.setCodec("utf8");
        QString nykyavain;
        QStringList nykytieto;

        while( !in.atEnd())
        {
            QString rivi = in.readLine();
            if( rivi.startsWith('[') && rivi.endsWith(']'))
            {
                // Tallennetaan nykyinen
                if( !nykyavain.isEmpty())
                    tiedot[nykyavain] = nykytieto;
                // Aloitetaan uusi
                nykyavain = rivi.mid(1, rivi.length() - 2);
                nykytieto.clear();
            }
            else if( !rivi.startsWith('#') && !nykyavain.isEmpty())
                nykytieto.append(rivi);
        }
        // Tiedoston lopussa päätetään viimeinen tieto
        if( !nykyavain.isEmpty())
            tiedot[nykyavain] = nykytieto;
    }

    return tiedot;
}

bool UusiKirjanpito::alustaKirjanpito()
{
    QString hakemistopolku = field("sijainti").toString() + "/" + field("hakemisto").toString();
    QDir hakemisto;

    // Luodaan hakemisto
    if( !hakemisto.mkdir(hakemistopolku) || !hakemisto.cd(hakemistopolku))
        return false;

    // Luodaan alihakemistot
    hakemisto.mkdir("tositteet");
    hakemisto.mkdir("arkisto");

    // Luodaan tietokanta
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE","luonti");
    db.setDatabaseName(hakemistopolku + "/kitupiikki.sqlite");
    if( !db.open())
    {
        return false;
    }
    QSqlQuery query(db);

    // Luodaan tietokanta
    // Tietokannan luontikäskyt ovat resurssitiedostossa luo.sql
    QFile sqltiedosto(":/sql/luo.sql");
    sqltiedosto.open(QIODevice::ReadOnly);
    QTextStream in(&sqltiedosto);
    QString sqluonti = in.readAll();
    sqluonti.replace("\n","");
    QStringList sqlista = sqluonti.split(";");
    foreach (QString kysely,sqlista)
    {
        query.exec(kysely);
    }


    // Ladataan karttatiedosto
    QMap<QString,QStringList> kartta = lueKtkTiedosto( field("tilikartta").toString() );

    // Asetustauluun kirjoittaminen
    query.prepare("INSERT INTO asetus (avain, arvo)"
                  "VALUES (?,?)");

    // Kirjataan tietokannan perustietoja
    query.addBindValue("nimi"); query.addBindValue( field("nimi").toString() ); query.exec();
    query.addBindValue("ytunnus"); query.addBindValue( field("ytunnus").toString() ); query.exec();
    if( field("todellinen").toBool())
        { query.addBindValue("harjoitus"); query.addBindValue( 0 ); }
    else
        { query.addBindValue("harjoitus"); query.addBindValue( 1 ); }
    query.exec();

    query.addBindValue("luotu"); query.addBindValue( QDate::currentDate().toString(Qt::ISODate) ); query.exec();
    query.addBindValue("versio"); query.addBindValue( qApp->applicationVersion() ); query.exec();

    if( field("onekakausi").toBool())    {
        // Ensimmäinen tilikausi, tilinavausta ei tarvita
        query.addBindValue("tilinavaus"); query.addBindValue( 0 ); query.exec();
    } else {
        // Tilinavaustiedot puuttuvat
        query.addBindValue("tilinavaus"); query.addBindValue( 2 ); query.exec();
    }

    // Siirretään asetustauluun tilikartan tiedot
    // jotka alkavat *-merkillä
    QMapIterator<QString,QStringList> i(kartta);
    while( i.hasNext())
    {
        i.next();
        if( i.key().startsWith("*"))
        {
            query.addBindValue( i.key() );
            query.addBindValue( i.value().join("\n"));
            query.exec();
        }
    }

    // Tilien kirjoittaminen
    query.prepare("INSERT INTO tili(nro,nimi,tyyppi) values(?,?,?) ");
    QStringList tililista = kartta.value("tilit");
    foreach ( QString tili, tililista)
    {
        // Tilitietueet ovat muotoa tyyppi;numero;nimi
        QStringList splitti = tili.split(";");
        if( splitti.count() > 2)
        {
            query.addBindValue(splitti[1].toInt() );
            query.addBindValue(splitti[2]);
            query.addBindValue(splitti[0]);
            query.exec();
        }
    }

    // Otsikkojen kirjoittaminen
    query.prepare("INSERT INTO tiliotsikko(tilista,tiliin,otsikko,tyyppi) values(?,?,?,?)");
    foreach (QString otsikkorivi, kartta.value("otsikot"))
    {
        // Otsikkotietueet tyyppi;mista..mihin;otsikko
        QStringList splitti = otsikkorivi.split(";");
        if( splitti.count() > 2)
        {
            QStringList mimisplit = splitti[1].split("..");
            if( mimisplit.count() != 2)
                continue;   // Epäkelpo
            query.addBindValue( mimisplit[0].toInt() ); // Mistä
            query.addBindValue( mimisplit[1].toInt() ); // Mihin
            query.addBindValue( splitti[2]);    // Otsikko
            query.addBindValue( splitti[0]);    // tyyppi
            query.exec();
        }
    }


    // Tilikausien kirjoittaminen
    // Nykyinen tilikausi
    query.prepare("INSERT INTO tilikausi(alkaa,loppuu,lukittu) values(?,?,?)");
    query.addBindValue( field("alkaa").toDate() );
    query.addBindValue( field("paattyy").toDate());
    query.addBindValue( QVariant() );
    query.exec();

    if( !field("onekakausi").toBool())
    {
        // Edellinen tilikausi. Lukitaan, ei estä tilin avausta.
        query.addBindValue( field("edalkoi").toDate());
        query.addBindValue( field("edpaattyi").toDate());
        query.addBindValue( field("edpaattyi").toDate());
        query.exec();
    }


    db.close();

    return true;

}
