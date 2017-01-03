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



    // Siirretään asetustauluun tilikartan tiedot
    // TODO: Vain osa, tietyllä tavalla (esim *-alku)
    QMapIterator<QString,QStringList> i(kartta);
    while( i.hasNext())
    {
        i.next();
        query.addBindValue("tk/" + i.key() );
        query.addBindValue(i.value().join("\n"));
        query.exec();
    }

    // Tilien kirjoittaminen


    // Otsikkojen kirjoittaminen


    // Tilikausien kirjoittaminen


    db.close();

    return true;

}
