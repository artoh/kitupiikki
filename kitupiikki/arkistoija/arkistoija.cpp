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
#include <QFile>
#include <QSqlQuery>
#include <QTextStream>

#include "arkistoija.h"
#include "db/tositemodel.h"

#include "raportti/raportoija.h"
#include "raportti/paivakirjaraportti.h"
#include "raportti/paakirjaraportti.h"
#include "raportti/tilikarttaraportti.h"
#include "raportti/tositeluetteloraportti.h"

#include <QDebug>

Arkistoija::Arkistoija(Tilikausi tilikausi)
    : tilikausi_(tilikausi)
{


}

void Arkistoija::luoHakemistot()
{
    hakemisto_ = kp()->hakemisto();
    hakemisto_.cd("arkisto");

    QString arkistonimi = tilikausi_.arkistoHakemistoNimi();

    if( hakemisto_.exists( arkistonimi ) )
    {
        // Jos hakemisto on jo olemassa, poistetaan se
        hakemisto_.cd( arkistonimi);
        hakemisto_.removeRecursively();
        hakemisto_.cdUp();
    }


    hakemisto_.mkdir( arkistonimi );
    hakemisto_.cd( arkistonimi );

    if( QFile::exists( kp()->hakemisto().absoluteFilePath("logo128.png") ))
    {
        QFile::copy( kp()->hakemisto().absoluteFilePath("logo128.png"),
                     hakemisto_.absoluteFilePath("logo.png"));
        onkoLogoa = true;
    }

}

void Arkistoija::arkistoiTositteet()
{
    // TODO: Kommentit ja tyylit jne.

    QSqlQuery kysely( QString("SELECT id FROM tosite WHERE pvm BETWEEN \"%1\" AND \"%2\" ")
                      .arg(tilikausi_.alkaa().toString(Qt::ISODate))
                      .arg(tilikausi_.paattyy().toString(Qt::ISODate)));


    TositeModel *tosite = kp()->tositemodel();
    LiiteModel liiteet(tosite);
    VientiModel viennit(tosite);


    while( kysely.next() )
    {
        int tositeId = kysely.value(0).toInt(); // Tositteen id
        tosite->lataa( tositeId ); // Lataa kyseisen tositteen
        liiteet.lataa();
        viennit.lataa();

        QString tiedostonnimi = QString("%1.html").arg(tositeId, 8, 10, QChar('0'));

        QFile tiedosto( hakemisto_.absoluteFilePath(tiedostonnimi) );
        tiedosto.open( QIODevice::WriteOnly);
        QTextStream out(&tiedosto);

        out.setCodec("UTF-8");

        out << QString("<html><meta charset=\"UTF-8\"><head><title>%1</title></head>").arg( tosite->otsikko());
        out << QString("<body><h1>%1</h1>").arg( tosite->otsikko());
        out << QString("Tositenro: %1%2</p>")
               .arg(tosite->tositelaji().tunnus())
               .arg(tosite->tunniste());
        out << "<h2>Liitteet</h2><ol>";

        // Kopioidaan ensin liitteet
        for(int i=0; i < liiteet.rowCount(QModelIndex());i++)
        {
            QModelIndex index = liiteet.index(i, 0);
            QFile::copy( index.data(LiiteModel::Polkurooli).toString(),
                         hakemisto_.absoluteFilePath( index.data(LiiteModel::TiedostoNimiRooli).toString()));

            out << "<li><a href=" << index.data(LiiteModel::TiedostoNimiRooli).toString();
            out << "\">" << index.data(LiiteModel::OtsikkoRooli).toString() << "</a></li>";
        }

        // Sitten alle vielä viennit taulukkoon;
        out << "</ol>";
        out << "<table><tr><th>Pvm</th><th>Tili</th><th>Kohdennus</th><th>Selite</th><th>Debet</th><th>Kredit</th></th>";

        for(int i=0; i < viennit.rowCount(QModelIndex()); i++)
        {
            QModelIndex index = viennit.index(i,0);
            out << "<tr><td>" << index.data(VientiModel::PvmRooli).toDate().toString(Qt::SystemLocaleShortDate) ;
            out << "</td><td>" << index.sibling(i, VientiModel::TILI).data().toString();
            out << "</td><td>" << index.sibling(i, VientiModel::KOHDENNUS).data().toString();
            out << "</td><td>" << index.sibling(i, VientiModel::SELITE).data().toString();
            out << "</td><td>" << index.sibling(i, VientiModel::DEBET).data().toString();
            out << "</td><td>" << index.sibling(i, VientiModel::KREDIT).data().toString();
            out << "</td></tr>";
        }

        out << "</table></body></html>";
        tiedosto.close();
    }

}

void Arkistoija::kirjoitaIndeksiJaArkistoiRaportit()
{

    QFile tiedosto( hakemisto_.absoluteFilePath("index.html"));
    tiedosto.open( QIODevice::WriteOnly);
    QTextStream out( &tiedosto );
    out.setCodec("UTF-8");

    out << "<html><meta charset=\"UTF-8\"><head><title>";
    out << kp()->asetus("Nimi") + " arkisto";
    out << "</title><link rel='stylesheet' type='text/css' href='arkisto.css'></head><body>";

    out << navipalkki();

    if(onkoLogoa)
        out << "<img src=logo.png class=logo>";

    out << "<h1>" << kp()->asetus("Nimi") << "</h1>";
    out << "<h2>Kirjanpitoarkisto<br>" ;
    out << tilikausi_.kausivaliTekstina();
    out << "</h2>";
    out << "<h3>Kirjanpito</h3>";
    out << "<ul><li><a href=paakirja.html>" << tr("Pääkirja") << "</a></li>";
    out << "<li><a href=paivakirja.html>" << tr("Päiväkirja") << "</a></li>";
    out << "<li><a href=tositeluettelo.html>Tositeluettelo</a></li>";
    out << "<li><a href=tositepaivakirja.html>" << tr("Tositepäiväkirja") << "</a></li>";
    out << "<li><a href=tililuettelo.html>Tililuettelo</a></li>";
    out << "</ul><h3>Raportit</h3><ul>";



    QStringList raportit;
    raportit = kp()->asetukset()->avaimet("Raportti/");

    foreach (QString raportti, raportit)
    {
        QString tiedostonnimi = raportti.mid(9).toLower() + ".html";
        tiedostonnimi.replace(" ","");

        Tilikausi edellinenkausi = kp()->tilikaudet()->tilikausiPaivalle( tilikausi_.alkaa().addDays(-1) );


        Raportoija raportoija(raportti.mid(9));
        if( raportoija.onkoKausiraportti())
        {


            raportoija.lisaaKausi(tilikausi_.alkaa(), tilikausi_.paattyy());
            if( edellinenkausi.alkaa().isValid())
                raportoija.lisaaKausi( edellinenkausi.alkaa(), edellinenkausi.paattyy());

            if( raportoija.tyyppi() == Raportoija::KOHDENNUSLASKELMA)
                raportoija.etsiKohdennukset();
        }
        else
        {

            raportoija.lisaaTasepaiva(tilikausi_.paattyy());
            if( edellinenkausi.paattyy().isValid())
                raportoija.lisaaTasepaiva(edellinenkausi.paattyy());

            if( raportoija.tyyppi() == Raportoija::PROJEKTITASE)
            {
                if( edellinenkausi.alkaa().isValid())
                    raportoija.valitseProjektit(edellinenkausi.alkaa(), tilikausi_.paattyy());
                else
                    raportoija.valitseProjektit(tilikausi_.alkaa(), tilikausi_.paattyy());
            }
        }

        arkistoiTiedosto( tiedostonnimi, raportoija.raportti().html(true) );

        // Kirjoitetaan indeksiin
        out << "<li><a href=\'" << tiedostonnimi << "\'>";
        out << raportti.mid(9);
        out << "</a></li>";
    }
    out << "</ul>";
    out << "<p>Tämä on sähköinen arkisto jne jne";
    out << "</p></body></html>";
}



void Arkistoija::arkistoiTiedosto(const QString &tiedostonnimi, const QString &html)
{
    QFile tiedosto( hakemisto_.absoluteFilePath(tiedostonnimi));
    tiedosto.open( QIODevice::WriteOnly);
    QTextStream out( &tiedosto );
    out.setCodec("UTF-8");

    // Lisätään valikko tuohon kohtaan !
    QString txt = html;
    txt.insert( txt.indexOf("</head>"), "<link rel='stylesheet' type='text/css' href='arkisto.css'>");
    txt.insert( txt.indexOf("<body>") + 6, navipalkki( ));

    out << txt;

    tiedosto.close();
}

QString Arkistoija::navipalkki(int edellinen, int seuraava)
{
    QString navi = "<nav><ul><li class=kotinappi><a href=index.html>";
    if( onkoLogoa )
        navi.append("<img src=logo.png>");
    navi.append( kp()->asetus("Nimi") + " " + tilikausi_.kausivaliTekstina());
    navi.append("</a></li>");

    if(seuraava)
        navi.append( tr("<li><a href=\'%1.html\'>Seuraava</a></li>").arg(seuraava,8,10,QChar('0')));
    if( edellinen )
        navi.append( tr("<li><a href=\'%1.html\'>Edellinen</a></li>").arg(edellinen,8,10,QChar('0')));
    navi.append("<li><a href=ohje.html>Ohje</a></li>");
    navi.append("</ul></nav></div>");

    return navi;
}



void Arkistoija::arkistoi(Tilikausi &tilikausi)
{
    Arkistoija arkistoija(tilikausi);
    arkistoija.luoHakemistot();
    arkistoija.arkistoiTositteet();
    arkistoija.kirjoitaIndeksiJaArkistoiRaportit();

    arkistoija.arkistoiTiedosto("paivakirja.html",
                                PaivakirjaRaportti::kirjoitaRaportti( tilikausi.alkaa(), tilikausi.paattyy(), -1, false, true, true, true).html(true) );
    arkistoija.arkistoiTiedosto("paakirja.html",
                                PaakirjaRaportti::kirjoitaRaportti( tilikausi.alkaa(), tilikausi.paattyy(), -1, true, true).html(true));
    arkistoija.arkistoiTiedosto("tililuettelo.html",
                                TilikarttaRaportti::kirjoitaRaportti(TilikarttaRaportti::KAIKKI_TILIT, tilikausi, false, tilikausi.paattyy()).html(true));
    arkistoija.arkistoiTiedosto("tositeluettelo.html",
                                TositeluetteloRaportti::kirjoitaRaportti( tilikausi.alkaa(), tilikausi.paattyy(), true, true, false, false, true).html(true) );
    arkistoija.arkistoiTiedosto("tositepaivakirja.html",
                                TositeluetteloRaportti::kirjoitaRaportti( tilikausi.alkaa(), tilikausi.paattyy(), true, true, true, true, true).html(true));

}
