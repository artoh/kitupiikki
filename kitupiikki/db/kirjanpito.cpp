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
#include <QSqlError>
#include <QTextStream>

#include <ctime>

#include "kirjanpito.h"
#include "tools/pdfikkuna.h"


Kirjanpito::Kirjanpito(QObject *parent) : QObject(parent),
    harjoitusPvm( QDate::currentDate()), tempDir_(0)
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
    printer_->setPaperSize(QPrinter::A4);
}

Kirjanpito::~Kirjanpito()
{
    tietokanta_.close();
    delete tempDir_;
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
    if(!QDesktopServices::openUrl( QUrl(osoite)))
        QMessageBox::critical(0, tr("Ohjeen näyttäminen epäonnistui"),
                              tr("Kitupiikki ei saanut käynnistettyä selainta ohjeen näyttämiseksi. Onhan järjestelmässäsi "
                                 "määritelty oletusselain avaamaan internet-sivuja?\n\n"
                                 "Ohjelman ohjeet löytyvät sivulta https://artoh.github.io/kitupiikki"));
}

void Kirjanpito::avaaUrl(const QUrl &url)
{
    if( url.fileName().endsWith(".pdf"))
        PdfIkkuna::naytaPdf( url.path() );
    else if( !QDesktopServices::openUrl(url) )
    {
        if( url.fileName().endsWith("html"))
            QMessageBox::critical(0, tr("Selaimen käynnistäminen epäonnistui"),
                                  tr("Kitupiikki ei saanut käynnistettyä selainta tiedoston %1 näyttämiseksi. Onhan järjestelmässäsi "
                                     "määritelty oletusselain avaamaan internet-sivuja?\n\n").arg(url.toDisplayString()));
        else
            QMessageBox::critical(0, tr("Tiedoston näyttäminen epäonnistui"),
                                  tr("Kitupiikki ei saanut käynnistettyä ulkoista ohjelmaa tiedoston %1 näyttämiseksi.").arg(url.toDisplayString() ));
    }
}

QString Kirjanpito::tilapainen(QString nimi) const
{
    return tempDir_->filePath(nimi.replace("XXXX", satujono(8)));
}

bool Kirjanpito::onkoMaksuperusteinenAlv(const QDate &paiva) const
{
    // Onko annettuna päivänä maksuperusteinen alv käytössä
    if( !asetukset()->onko("AlvVelvollinen") || !asetukset()->onko("MaksuAlvAlkaa"))
        return false;
    if( asetukset()->pvm("MaksuAlvAlkaa") > paiva )
        return false;
    if( asetukset()->onko("MaksuAlvLoppuu") && asetukset()->pvm("MaksuAlvLoppuu") <= paiva )
        return false;
    return true;
}



bool Kirjanpito::avaaTietokanta(const QString &tiedosto)
{
    tietokanta_.setDatabaseName(tiedosto);

    if( !tietokanta_.open() )
    {
        QMessageBox::critical(0, tr("Tiedostoa %1 ei voi avata").arg(tiedosto),
                              tr("Tiedoston avaamisessa tapahtui virhe\n %1").arg( tietokanta_.lastError().text() ));
        return false;
    }

    // Ladataankin asetukset yms modelista
    asetusModel_->lataa();


    if( asetusModel_->asetus("Nimi").isEmpty() || !asetusModel_->luku("KpVersio"))
    {
        // Tämä ei ole lainkaan kelvollinen tietokanta
        QMessageBox::critical(0, tr("Tiedostoa %1 ei voi avata").arg(tiedosto),
                              tr("Valitsemasi tiedosto ei ole Kitupiikin tietokanta, tai tiedosto on vahingoittunut."));
        tietokanta()->close();
        asetusModel_->lataa();
        emit tietokantaVaihtui();
        return false;
    }


    // Tarkistaa, ettei kirjanpitoa ole tehty uudemmalla versiolla.

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

    //
    // Tiedostoversion muuttuessa tähän muutettava yhteensopivuusversio !!
    //
    if( asetusModel_->luku("KpVersio") < TIETOKANTAVERSIO )
    {
        if( QMessageBox::question(0, tr("Kirjanpidon %1 päivittäminen").arg(asetusModel_->asetus("Nimi")),
                                  tr("Kirjanpito on luotu Kitupiikin versiolla %1 ja se täytyy päivittää, ennen kuin sitä "
                                     "voi käyttää nykyisellä versiolla %2.\n\n"
                                     "Päivittämisen jälkeen kirjanpitoa ei voi enää avata vanhemmilla versioilla kuin 0.10.\n\n"
                                     "On erittäin suositeltavaa varmuuskopioida kirjanpito ennen päivittämistä!\n\n"
                                     "Päivitetäänkö tietokanta Kitupiikin nykyiselle versiolle?").arg(asetusModel_->asetus("LuotuVersiolla"))
                                     .arg(qApp->applicationVersion()),
                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes )
        {
            tietokanta()->close();
            asetusModel_->lataa();
            emit tietokantaVaihtui();
            return false;
        }
        // Tietokanta päivitetään suorittamalla update-komennot
        // nykyiseen tietokantaversioon saakka
        for(int i = asetusModel_->luku("KpVersio") + 1; i <= TIETOKANTAVERSIO; i++)
            paivita( i );

        asetusModel_->aseta("KpVersio", TIETOKANTAVERSIO);
        asetusModel_->aseta("LuotuVersiolla", qApp->applicationVersion());
        QMessageBox::information(0, tr("Kirjanpito päivitetty"),
                                 tr("Kirjanpito päivitetty käytössä olevaan versioon."));

    }



    tositelajiModel_->lataa();
    tiliModel_->lataa();
    tilikaudetModel_->lataa();
    kohdennukset_->lataa();
    tuotteet_->lataa();


    polkuTiedostoon_ = tiedosto;

    // #124 Jos väliaikaistiedosto ei toimi...
    if( tempDir_ )
        delete tempDir_;

    tempDir_ = new QTemporaryDir();

    if( !tempDir_->isValid())
    {
        delete tempDir_;
        tempDir_ = new QTemporaryDir( hakemisto().absoluteFilePath("Temp")  );
        if( !tempDir_->isValid())
            QMessageBox::critical(0, tr("Tilapäishakemiston luominen epäonnistui"),
                                  tr("Kitupiikki ei onnistunut luomaan tilapäishakemistoa. Raporttien ja laskujen esikatselu ei toimi."));
    }

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
    return instanssi__;
}

void Kirjanpito::asetaInstanssi(Kirjanpito *kp)
{
    instanssi__ = kp;
}


QString Kirjanpito::satujono(int pituus)
{
    qsrand( std::time(NULL) );

    // https://stackoverflow.com/questions/18862963/qt-c-random-string-generation
    const QString merkit("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");

    QString randomString;
    for(int i=0; i<pituus; ++i)
    {
       int index = qrand() % merkit.length();
       QChar nextChar = merkit.at(index);
       randomString.append(nextChar);
    }
    return randomString;
}

void Kirjanpito::paivita(int versioon)
{
    QFile sqltiedosto( QString(":/sql/update%1.sql").arg(versioon));
    sqltiedosto.open(QIODevice::ReadOnly);
    QTextStream in(&sqltiedosto);
    QString sqluonti = in.readAll();
    sqluonti.replace("\n","");
    QStringList sqlista = sqluonti.split(";");
    QSqlQuery query;

    foreach (QString kysely,sqlista)
    {
        query.exec(kysely);
        qApp->processEvents();
    }
}

Kirjanpito* Kirjanpito::instanssi__ = 0;

Kirjanpito *kp()  { return Kirjanpito::db(); }
