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

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>
#include <QFileInfo>
#include <QSettings>
#include "kirjanpito.h"


Kirjanpito::Kirjanpito(QObject *parent) : QObject(parent),
    harjoitusPvm( QDate::currentDate())
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    QSettings settings;

    // Ladataan viimeisten tiedostojen lista.
    QStringList viimelista = settings.value("viimeiset").toStringList();
    foreach (QString rivi,viimelista)
    {
        QStringList split = rivi.split(";");
        if( QFile::exists( split[0] ))
            viimetiedostot[split[0]]=split[1];
    }
}

Kirjanpito::~Kirjanpito()
{
    db.close();
}

QString Kirjanpito::asetus(const QString &avain) const
{
    return asetukset.value(avain);
}

void Kirjanpito::aseta(const QString &avain, const QString &arvo)
{
    QSqlQuery query;
    if( asetukset.contains(avain))
    {
        // Asetus on jo, se vain päivitetään
        query.prepare("UPDATE asetus SET arvo=:arvo where AVAIN=:avain");
    }
    else
    {
        // Luodaan uusi asetus
        query.prepare("INSERT INTO asetus(avain,arvo) VALUES(:avain,:arvo)");
    }
    query.bindValue(":avain", avain);
    query.bindValue(":arvo",arvo);
    query.exec();
    asetukset[avain] = arvo;
    db.commit();
}

QDir Kirjanpito::hakemisto()
{
    QFileInfo finfo(polkuTiedostoon);
    return finfo.absoluteDir();
}

QStringList Kirjanpito::viimeisetTiedostot() const
{
    QStringList tallelista;
    QMapIterator<QString,QString> iter(viimetiedostot);

    while( iter.hasNext())
    {
        iter.next();
        tallelista.append(iter.key() + ";" + iter.value());
    }
    return tallelista;
}

QDate Kirjanpito::paivamaara() const
{
    if( onkoHarjoitus())
        return harjoitusPvm;
    else
        return QDate::currentDate();
}


bool Kirjanpito::avaaTietokanta(const QString &tiedosto)
{
    db.setDatabaseName(tiedosto);

    if( !db.open() )
        return false;

    // Ladataan asetukset
    QSqlQuery query;
    query.exec("SELECT avain,arvo FROM asetus");
    while( query.next())
    {
        asetukset[query.value(0).toString()] = query.value(1).toString();
    }

    polkuTiedostoon = tiedosto;

    // Lisätään viimeisten tiedostojen listaan
    if(asetus("harjoitus").toInt())
        viimetiedostot[ tiedosto ] = asetus("nimi") + " (harjoittelu)";
    else
        viimetiedostot[ tiedosto ] = asetus("nimi");
    // Tallennetaan lista

    QSettings settings;
    settings.setValue("viimeiset",viimeisetTiedostot());


    // Ilmoitetaan, että tietokanta on vaihtunut
    emit tietokantaVaihtui();

    return true;
}

void Kirjanpito::asetaHarjoitteluPvm(const QDate &pvm)
{
    harjoitusPvm = pvm;
}
