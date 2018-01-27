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
#include <QPrinter>
#include <QMessageBox>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>

#include "kirjanpito.h"


Kirjanpito::Kirjanpito(QObject *parent) : QObject(parent),
    harjoitusPvm( QDate::currentDate())
{
    tietokanta_ = QSqlDatabase::addDatabase("QSQLITE");
    QSettings settings;
    asetusModel_ = new AsetusModel(&tietokanta_, this);
    tositelajiModel_ = new TositelajiModel(&tietokanta_, this);
    tiliModel_ = new TiliModel( &tietokanta_, this);
    tilikaudetModel_ = new TilikausiModel(&tietokanta_, this);
    kohdennukset_ = new KohdennusModel(&tietokanta_, this);
    veroTyypit_ = new VerotyyppiModel(this);
    tiliTyypit_ = new TilityyppiModel(this);
    tuotteet_ = new TuoteModel(this);
    printer_ = new QPrinter(QPrinter::HighResolution);
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

TositeModel *Kirjanpito::tositemodel(QObject *parent)
{
    return new TositeModel( &tietokanta_ , parent);
}

void Kirjanpito::ohje(const QString &ohjesivu)
{
    QString osoite("https://artoh.github.io/kitupiikki/");
    osoite.append(ohjesivu);
    QDesktopServices::openUrl( QUrl(osoite));
}



bool Kirjanpito::avaaTietokanta(const QString &tiedosto)
{
    tietokanta_.setDatabaseName(tiedosto);

    if( !tietokanta_.open() )
        return false;

    // Ladataankin asetukset yms modelista
    asetusModel_->lataa();


    // Tarkistaa, ettei kirjanpitoa ole tehty uudemmalla versiolla.
    // Myöhemmin valvottava myös aikaisempi tietokantaversio ja tehtävä
    // tarvittavat muutokset

    if( asetusModel_->luku("KpVersio") > TIETOKANTAVERSIO )
    {
        // Luotu uudemmalla tietokannalla, sellainen ei kelpaa!
        QMessageBox::critical(0, tr("Kirjanpitoa %1 ei voi avata").arg(asetusModel_->asetus("Nimi")),
                              tr("Kirjanpito on luotu Kitupiikin versiolla %1, eikä käytössäsi oleva versio %2 pysty avaamaan sitä.\n\n"
                                 "Voidaksesi avata tiedoston, sinun on asennettava uudempi versio Kitupiikistä. Lataa ohjelma "
                                 "osoitteesta https://artoh.github.io/kitupiikki").arg( asetusModel_->asetus("LuotuVersiolla"))
                              .arg( qApp->applicationVersion() ));

        tietokanta()->close();
        asetusModel_->lataa();  // Tyhjentää asetukset
        emit tietokantaVaihtui();

        return false;
    }

    tositelajiModel_->lataa();
    tiliModel_->lataa();
    tilikaudetModel_->lataa();
    kohdennukset_->lataa();
    tuotteet_->lataa();


    polkuTiedostoon_ = tiedosto;

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


Kirjanpito *Kirjanpito::db()
{
    if( !instanssi__ )
        instanssi__ = new Kirjanpito;
    return instanssi__;

}

Kirjanpito* Kirjanpito::instanssi__ = 0;

Kirjanpito *kp()  { return Kirjanpito::db(); }
