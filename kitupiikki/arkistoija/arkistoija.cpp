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
#include <QCryptographicHash>

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


    // Kopioidaan vakitiedostot
    QFile::copy( ":/arkisto/arkisto.css", hakemisto_.absoluteFilePath("arkisto.css"));
    QFile::copy( ":/arkisto/jquery.js", hakemisto_.absoluteFilePath("jquery.js"));

}

void Arkistoija::arkistoiTositteet()
{
    // TODO: Kommentit ja tyylit jne.

    QSqlQuery kysely( QString("SELECT id FROM tosite WHERE pvm BETWEEN \"%1\" AND \"%2\" ORDER BY laji, tunniste ")
                      .arg(tilikausi_.alkaa().toString(Qt::ISODate))
                      .arg(tilikausi_.paattyy().toString(Qt::ISODate)));

    // Haetaan id:t listaan. Näin ollen aina tieto edellisestä ja seuraavasta
    QList<int> idLista;
    while(kysely.next())
        idLista.append( kysely.value(0).toInt());

    // Sitten tositteet
    TositeModel *tosite = kp()->tositemodel();
    LiiteModel liitteet(tosite);
    VientiModel viennit(tosite);

    for( int tositeInd = 0; tositeInd < idLista.count(); tositeInd++)
    {
        int tositeId = idLista.at(tositeInd);

        tosite->lataa( tositeId ); // Lataa kyseisen tositteen
        liitteet.lataa();
        viennit.lataa();

        QByteArray bArray;
        QTextStream out( &bArray );

        out.setCodec("UTF-8");

        out << "<html><meta charset=\"UTF-8\"><head><title>" << tosite->otsikko() << "</title>";
        out << "<link rel='stylesheet' type='text/css' href='arkisto.css'></head><body>";

        // Navigointipalkissa on navigointi edelliseen ja seuraavaan tositteeseen

        int edellinen = 0;
        int seuraava = 0;

        if( tositeInd > 0)
            edellinen = idLista.at(tositeInd- 1);
        if( tositeInd < idLista.count() - 1)
            seuraava = idLista.at(tositeInd+1);

        out << navipalkki(edellinen, seuraava);

        // Mahdollinen liitelaatikko
        // Käytetään pelkästään pdf-liitteitä, muunnos latauksessa
        int liitteita = liitteet.rowCount(QModelIndex());

        if( liitteita )
        {
            // Liitteen laatikko, johon nykyinen liite ladataan
            out << "<object type='application/pdf' width='100%' height='50%' class='liite' id='liite' data='";
            out << liitteet.index(0,0).data(LiiteModel::TiedostoNimiRooli).toString();
            out <<  "'></object>";

            out << "<table class='liiteluettelo'>";

            // Liitteiden kopiointi sekä luettelo
            for(int liiteInd=0; liiteInd < liitteita; liiteInd ++)
            {
                QModelIndex liiteIndeksi = liitteet.index(liiteInd,0);

                out << "<tr><td onclick=\"$('#liite').attr('data','"
                     << liiteIndeksi.data(LiiteModel::TiedostoNimiRooli).toString()
                     << "');\">" << liiteIndeksi.data(LiiteModel::OtsikkoRooli).toString()
                     << "</td><td><a href='" << liiteIndeksi.data(LiiteModel::TiedostoNimiRooli).toString()
                     << "'>Avaa</a></td></tr>\n";

                QFile::copy( liiteIndeksi.data(LiiteModel::Polkurooli).toString(),
                             hakemisto_.absoluteFilePath( liiteIndeksi.data(LiiteModel::TiedostoNimiRooli).toString()));

                shaBytes.append( liiteIndeksi.data(LiiteModel::Sharooli).toByteArray() );
                shaBytes.append(" ");
                shaBytes.append(liiteIndeksi.data(LiiteModel::TiedostoNimiRooli).toString().toLatin1());
                shaBytes.append("\n");
            }
            out << "</table>";
        }

        // Seuraavaksi otsikot
        out << "<table class=tositeotsikot><tr>";
        out << "<td class=paiva>" << tosite->pvm().toString(Qt::SystemLocaleShortDate) << "</td>";
        out << "<td class=tositeotsikko>" << tosite->otsikko() << "</td>";
        out << "<td class=tositetunnus>" << tosite->tositelaji().tunnus() << tosite->tunniste() << "</td>";
        out << "</tr></table>";

        // Sitten viennit

        int vienteja = viennit.rowCount(QModelIndex());
        if( vienteja )
        {
            out << "<table class=viennit>";
            out <<  "<tr><th>Pvm</th><th>Tili</th><th>Kohdennus</th><th>Selite</th><th>Debet</th><th>Kredit</th></tr>";

            for(int vientiInd = 0; vientiInd < vienteja; vientiInd++)
            {
                QModelIndex index = viennit.index(vientiInd,0);
                out << "<tr><td class=pvm>" << index.data(VientiModel::PvmRooli).toDate().toString(Qt::SystemLocaleShortDate) ;
                out << "</td><td><a href='paakirja.html#" << index.data(VientiModel::TiliNumeroRooli).toInt() << "'>"
                    << index.sibling(vientiInd, VientiModel::TILI).data().toString() << "</a>";
                out << "</td><td>" << index.sibling(vientiInd, VientiModel::KOHDENNUS).data().toString();
                out << "</td><td>" << index.sibling(vientiInd, VientiModel::SELITE).data().toString();
                out << "</td><td class=euro>" << index.sibling(vientiInd, VientiModel::DEBET).data().toString();
                out << "</td><td class=euro>" << index.sibling(vientiInd, VientiModel::KREDIT).data().toString();
                out << "</td></tr>\n";
            }
            out << "</table>";
        }


        // Kommentit
        if( !tosite->kommentti().isEmpty())
        {
            out << "<p class=kommentti>";
            out << tosite->kommentti();
            out << "</p>";
        }


        // Ja lopuksi sekalaiset tiedot

        out << "<p class=info>Kirjanpito arkistoitu " << QDate::currentDate().toString(Qt::SystemLocaleDate);

        if( tilikausi_.paattyy() <= kp()->tilitpaatetty() )
            out << " (Tilinpäätös)";
        if( kp()->onkoHarjoitus())
            out << "<br>Kirjanpito on laadittu Kitupiikki-ohjelmiston harjoittelutilassa";
        out << "</p>";


        out << "<script src='jquery.js'></script>";
        out << "</body></html>";


        // Sitten kirjoitetaan
        QString tiedostonnimi = QString("%1.html").arg(tositeId, 8, 10, QChar('0'));

        out.flush();

        shaBytes.append(QCryptographicHash::hash( bArray, QCryptographicHash::Sha256).toHex());
        shaBytes.append(" ");
        shaBytes.append(tiedostonnimi.toLatin1());
        shaBytes.append("\n");

        QFile tiedosto( hakemisto_.absoluteFilePath(tiedostonnimi) );
        tiedosto.open( QIODevice::WriteOnly);
        tiedosto.write( bArray);
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

    out << "<h1 class=etusivu>" << kp()->asetus("Nimi") << "</h1>";
    out << "<h2 class=etusivu>Kirjanpitoarkisto<br>" ;
    out << tilikausi_.kausivaliTekstina();
    out << "</h2>";

    // Jos tilit on päätetty (tilikausi lukittu), tulee liite myös tilinpäätökseen (pdf)
    if( tilikausi_.paattyy() <= kp()->tilitpaatetty() )
        out << "<h3>" << tr("Tilinpäätös") << "</h3>"
            << "<ul><li><a href=tilinpaatos.pdf>" << tr("Tilinpäätös") << "</a></li></ul>";


    out << "<h3>Kirjanpito</h3>";
    out << "<ul><li><a href=paakirja.html>" << tr("Pääkirja") << "</a></li>";
    out << "<li><a href=paivakirja.html>" << tr("Päiväkirja") << "</a></li>";
    out << "<li><a href=tositeluettelo.html>Tositeluettelo</a></li>";
    out << "<li><a href=tositepaivakirja.html>" << tr("Tositepäiväkirja") << "</a></li>";
    out << "<li><a href=tililuettelo.html>Tililuettelo</a></li>";
    out << "</ul><h3>Raportit</h3><ul>";



    QStringList raportit;
    raportit = kp()->asetukset()->avaimet("Raportti/");

    // Kirjoitetaan kaikki kirjanpitoon liittyvät raportit arkistoon
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

    kirjoitaHash();

    out << tr("<p class=info>Tämä kirjanpidon sähköinen arkisto on luotu Kitupiikki-ohjelmalla %1 <br>").arg(QDate::currentDate().toString(Qt::SystemLocaleDate));
    out << tr("Arkiston muuttumattomuus voidaan valvoa sha256-tiivisteellä <code>%1</code> </p>").arg( QString(QCryptographicHash::hash( shaBytes, QCryptographicHash::Sha256).toHex()) );

    out << "</body></html>";
}



void Arkistoija::arkistoiTiedosto(const QString &tiedostonnimi, const QString &html)
{

    QByteArray bArray;
    QTextStream out( &bArray );
    out.setCodec("UTF-8");

    // Lisätään valikko tuohon kohtaan !
    QString txt = html;
    txt.insert( txt.indexOf("</head>"), "<link rel='stylesheet' type='text/css' href='arkisto.css'>");
    txt.insert( txt.indexOf("<body>") + 6, navipalkki( ));
    out << txt;

    out.flush();

    QFile tiedosto( hakemisto_.absoluteFilePath(tiedostonnimi));
    tiedosto.open( QIODevice::WriteOnly);
    tiedosto.write( bArray );
    tiedosto.close();

    // SHA-varmistus
    shaBytes.append(QCryptographicHash::hash( bArray, QCryptographicHash::Sha256).toHex());
    shaBytes.append(" ");
    shaBytes.append(tiedostonnimi.toLatin1());
    shaBytes.append("\n");
}

void Arkistoija::kirjoitaHash()
{
    QFile tiedosto( hakemisto_.absoluteFilePath( "arkisto.sha256" ));
    tiedosto.open( QIODevice::WriteOnly );
    tiedosto.write( shaBytes );
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



QString Arkistoija::arkistoi(Tilikausi &tilikausi)
{
    Arkistoija arkistoija(tilikausi);
    arkistoija.luoHakemistot();
    arkistoija.arkistoiTositteet();

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

    // Tämän pitää tulla lopuksi jotta hash toimii !!!
    arkistoija.kirjoitaIndeksiJaArkistoiRaportit();

    return QString( QCryptographicHash::hash( arkistoija.shaBytes , QCryptographicHash::Sha256).toHex() );
}
