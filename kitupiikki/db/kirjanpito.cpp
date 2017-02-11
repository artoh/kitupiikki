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
    tietokanta_ = QSqlDatabase::addDatabase("QSQLITE");
    QSettings settings;

    // Ladataan viimeisten tiedostojen lista.
    QStringList viimelista = settings.value("viimeiset").toStringList();
    foreach (QString rivi,viimelista)
    {
        QStringList split = rivi.split(";");
        if( QFile::exists( split[0] ))
            viimetiedostot[split[0]]=split[1];
    }

    asetusModel_ = new AsetusModel(tietokanta_, this);
    tositelajiModel_ = new TositelajiModel(this);
    tiliModel_ = new TiliModel( tietokanta_, this);
    tilikaudetModel_ = new TilikausiModel(tietokanta_, this);
}

Kirjanpito::~Kirjanpito()
{
    tietokanta_.close();
}

QString Kirjanpito::asetus(const QString &avain) const
{
    return asetukset()->asetus(avain);
}


QDir Kirjanpito::hakemisto()
{
    QFileInfo finfo(polkuTiedostoon_);
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


Tilikausi Kirjanpito::tilikausiPaivalle(const QDate &paiva) const
{
    return tilikaudet()->tilikausiPaivalle(paiva);
}

TositeModel *Kirjanpito::tositemodel(QObject *parent) const
{
    return new TositeModel( tietokanta_ , parent);
}



bool Kirjanpito::avaaTietokanta(const QString &tiedosto)
{
    tietokanta_.setDatabaseName(tiedosto);

    if( !tietokanta_.open() )
        return false;

    // Ladataankin asetukset yms modelista
    asetusModel_->lataa();
    tositelajiModel_->lataa();
    tiliModel_->lataa();
    tilikaudetModel_->lataa();


    polkuTiedostoon_ = tiedosto;

    // Lisätään viimeisten tiedostojen listaan
    if( onkoHarjoitus() )
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

bool Kirjanpito::lataaUudelleen()
{
    return avaaTietokanta(hakemisto().absoluteFilePath("kitupiikki.sqlite"));
}

void Kirjanpito::asetaHarjoitteluPvm(const QDate &pvm)
{
    harjoitusPvm = pvm;
}

void Kirjanpito::muokattu()
{
    emit kirjanpitoaMuokattu();
}

Kirjanpito *Kirjanpito::db()
{
    if( !instanssi__ )
        instanssi__ = new Kirjanpito;
    return instanssi__;

}

Kirjanpito* Kirjanpito::instanssi__ = 0;

Kirjanpito *kp()  { return Kirjanpito::db(); }
