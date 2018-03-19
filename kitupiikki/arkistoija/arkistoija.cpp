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
#include <QSqlError>
#include <QTextStream>
#include <QCryptographicHash>
#include <QApplication>

#include "arkistoija.h"
#include "db/tositemodel.h"

#include "raportti/raportoija.h"
#include "raportti/paivakirjaraportti.h"
#include "raportti/paakirjaraportti.h"
#include "raportti/tilikarttaraportti.h"
#include "raportti/tositeluetteloraportti.h"
#include "raportti/taseerittely.h"

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
    QFile::copy( ":/arkisto/ohje.html", hakemisto_.absoluteFilePath("ohje.html"));
    QFile::copy( ":/pic/aboutpossu.png", hakemisto_.absoluteFilePath("kitupiikki.png"));

}

/**
 * @brief Tiliotteen tiedot arkistoijan sisäiseen käyttöön
 */
struct TilioteTieto
{
    int tilinumero;
    QDate alkaa;
    QDate paattyy;
    int tositeId;
};

void Arkistoija::arkistoiTositteet()
{

    TositeModel *tosite = kp()->tositemodel();
    LiiteModel liitteet(tosite);
    VientiModel viennit(tosite);


    // Tositelistassa tositteen tunnus ja id
    // Tositelistaan tulevat myös kaikki ne tositteet, joihin vientikirjauksia sekä
    // ne tositteet, joihin tase-erät viittaavat

    QMap<QString,int> tositeLista;

    QSqlQuery kysely( QString("SELECT id,tiliote, tunniste, laji FROM tosite WHERE pvm BETWEEN \"%1\" AND \"%2\" ")
                      .arg(tilikausi_.alkaa().toString(Qt::ISODate))
                      .arg(tilikausi_.paattyy().toString(Qt::ISODate)));



    // Haetaan id:t listaan. Näin ollen aina tieto edellisestä ja seuraavasta
    QList<int> idLista;
    QList<TilioteTieto> tilioteLista;

    while(kysely.next())
    {
        // Lisätään tositteet tositetunnuksen mukaan
        QString tunnus = QString("%1%2/%3").arg( kp()->tositelajit()->tositelaji( kysely.value("laji").toInt() ).tunnus() )
                .arg( kysely.value("tunniste").toInt() )
                .arg( tilikausi_.kausitunnus());

        tositeLista.insert( tunnus, kysely.value("id").toInt() );

        idLista.append( kysely.value(0).toInt());

        // Jos tämä tosite on tiliote, lisätään se tilioteluetteloon, jotta tällä välillä tiliin tehtäviin
        // kirjauksiin voidaan lisätä myös viittaus tiliotteeseen

        if( kysely.value(1).toInt())
        {
            TilioteTieto otetieto;
            otetieto.tilinumero = kp()->tilit()->tiliIdlla( kysely.value(1).toInt() ).numero();
            if( otetieto.tilinumero )
            {
                tosite->lataa( kysely.value(0).toInt());
                otetieto.alkaa = tosite->json()->date("TilioteAlkaa");
                otetieto.paattyy = tosite->json()->date("TilioteLoppuu");
                otetieto.tositeId = kysely.value(0).toInt();
                tilioteLista.append(otetieto);
            }
        }

    }
    // Sitten lisätään vielä vientien mukaan, jotta kaikki varmasti mukana

    kysely.exec(QString("SELECT tosite.id, tosite.tunniste, tosite.laji, tosite.pvm, eraid FROM vienti,tosite WHERE vienti.tosite=tosite.id "
                "AND vienti.pvm BETWEEN '%1' AND '%2'")
                .arg(tilikausi_.alkaa().toString(Qt::ISODate))
                .arg(tilikausi_.paattyy().toString(Qt::ISODate)));

    while( kysely.next())
    {
        QString tunnus = QString("%1%2/%3").arg( kp()->tositelajit()->tositelaji( kysely.value("tosite.laji").toInt() ).tunnus() )
                .arg( kysely.value("tosite.tunniste").toInt() )
                .arg( kp()->tilikaudet()->tilikausiPaivalle( kysely.value("tosite.pvm").toDate() ).kausitunnus() );

        tositeLista.insert( tunnus, kysely.value("id").toInt() );

        // Ja sitten vielä tase-erät
        int taseEra = kysely.value("eraid").toInt();
        if( taseEra)
        {
            QSqlQuery eraKysely(QString("SELECT tosite.id, tosite.tunniste, tosite.laji, tosite.pvm FROM vienti,tosite WHERE vienti.tosite=tosite.id "
                        "AND vienti.id=%1").arg(taseEra)  );
            while( eraKysely.next())
            {
                QString eratunnus = QString("%1%2/%3").arg( kp()->tositelajit()->tositelaji( eraKysely.value("tosite.laji").toInt() ).tunnus() )
                        .arg( eraKysely.value("tosite.tunniste").toInt() )
                        .arg( kp()->tilikaudet()->tilikausiPaivalle( eraKysely.value("tosite.pvm").toDate() ).kausitunnus()  );
                tositeLista.insert(eratunnus, eraKysely.value("tosite.id").toInt());
            }
        }

    }


    // Sitten tositteet


    QMapIterator<QString, int> tositeIter(tositeLista);

    while( tositeIter.hasNext() )
    {
        qApp->processEvents();      // Jotta odotusikkuna näkyisi...

        tositeIter.next();
        int tositeId = tositeIter.value();

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

        if( tositeIter.hasPrevious())
        {
            tositeIter.previous();
            if( tositeIter.hasPrevious())
            {
                tositeIter.previous();
                edellinen = tositeIter.value();
                tositeIter.next();
            }
            tositeIter.next();
        }
        if( tositeIter.hasNext())
        {
            tositeIter.next();
            seuraava = tositeIter.value();
            tositeIter.previous();
        }

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

                QFile::copy( liiteIndeksi.data(LiiteModel::PolkuRooli).toString(),
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
        out << "<td class=paiva>" << tosite->pvm().toString("dd.MM.yyyy") << "</td>";
        out << "<td class=tositeotsikko>" << tosite->otsikko() << "</td>";
        out << QString("<td class=tositetunnus>%1%2/%3</td>")
               .arg(tosite->tositelaji().tunnus()).arg(tosite->tunniste()).arg( kp()->tilikaudet()->tilikausiPaivalle( tosite->pvm() ).kausitunnus() );
        out << "</tr></table>";

        // Sitten viennit

        QString eraLaatikko;
        int seuratutTaseErat = 0;

        int vienteja = viennit.rowCount(QModelIndex());
        if( vienteja )
        {

            out << "<table class=viennit>";
            out <<  "<tr><th>Pvm</th><th>Tili</th><th>Kohdennus</th><th>Selite</th><th>Debet</th><th>Kredit</th></tr>";

            for(int vientiRivi = 0; vientiRivi < vienteja; vientiRivi++)
            {
                QModelIndex index = viennit.index(vientiRivi,0);

                // Mahdollisen tase-erän seuranta
                QSqlQuery eraKysely(QString("SELECT tosite.id, tosite.tunniste, tosite.laji, tosite.pvm, vienti.pvm, vienti.selite, vienti.debetsnt, vienti.kreditsnt FROM vienti,tosite WHERE vienti.tosite=tosite.id "
                            "AND vienti.eraid=%1 AND vienti.pvm <= '%2' ORDER BY vienti.pvm")
                                    .arg( index.data(VientiModel::IdRooli).toInt() )
                                    .arg( tilikausi_.paattyy().toString(Qt::ISODate))  );

                qlonglong eraSaldo = index.data(VientiModel::DebetRooli).toInt() - index.data(VientiModel::KreditRooli).toInt();

                bool taseEraSeurannassa = false;
                while( eraKysely.next())
                {
                    if( !taseEraSeurannassa)
                    {
                        eraLaatikko.append(tr("<p><sup>%2)</sup> Tase-erä tilillä %1")
                                       .arg( index.sibling(vientiRivi, VientiModel::TILI).data().toString() )
                                       .arg( ++seuratutTaseErat));
                        eraLaatikko.append("<table class=viennit><th>Tosite</th><th>Pvm</th><th>Selite</th><th>Kredit</th><th>Debit</th></tr>");
                        taseEraSeurannassa = true;
                    }
                    QString eradebet;
                    if( eraKysely.value("debetsnt").toInt())
                        eradebet = QString("%L1").arg( eraKysely.value("vienti.debetsnt").toDouble() /  100.0 ,0,'f',2);
                    QString erakredit;
                    if( eraKysely.value("kreditsnt").toInt())
                        erakredit = QString("%L1").arg( eraKysely.value("vienti.kreditsnt").toDouble() /  100.0 ,0,'f',2);


                    eraLaatikko.append( QString("<tr><td class=tili><a href=%8.html>%1%2/%3</a></td><td class=pvm>%4</td><td class=selite>%5</td><td class=euro>%6</td><td class=euro>%7</td></tr>")
                                        .arg( kp()->tositelajit()->tositelaji(eraKysely.value("tosite.laji").toInt()).tunnus() )
                                        .arg( eraKysely.value("tosite.tunniste").toInt())
                                        .arg( kp()->tilikausiPaivalle( eraKysely.value("tosite.pvm").toDate()  ).kausitunnus())
                                        .arg( eraKysely.value("vienti.pvm").toDate().toString("dd.MM.yyyy"))
                                        .arg( eraKysely.value("vienti.selite").toString())
                                        .arg( eradebet )
                                        .arg( erakredit )
                                        .arg( eraKysely.value("tosite.id").toInt(), 8,10,QChar('0')));
                    eraSaldo += eraKysely.value("vienti.debetsnt").toLongLong() - eraKysely.value("vienti.kreditsnt").toLongLong();
                }
                if( taseEraSeurannassa)
                {
                    eraLaatikko.append( tr("<tr><td colspan=3 class=erasaldo>Saldo %1</td>").arg(tilikausi_.paattyy().toString("dd.MM.yyyy")));
                    if( eraSaldo > 0)
                        eraLaatikko.append(QString("<td class=euro>%L1</td><td class=euro></td>").arg( (double) eraSaldo /  100.0 ,0,'f',2 ));
                    else if( eraSaldo < 0)
                        eraLaatikko.append(QString("<td class=euro></td><td class=euro>%L1</td>").arg( (double) 0 - eraSaldo /  100.0 ,0,'f',2 ));
                    else
                        eraLaatikko.append("<td class=euro></td><td class=euro></td>");
                    eraLaatikko.append("</tr></table>");
                }   // Tase-erän seuranta


                out << "<tr><td class=pvm>" << index.data(VientiModel::PvmRooli).toDate().toString("dd.MM.yyyy") ;
                out << "</td><td class=tili><a href='paakirja.html#" << index.data(VientiModel::TiliNumeroRooli).toInt() << "'>"
                    << index.sibling(vientiRivi, VientiModel::TILI).data().toString() << "</a>";
                // Mahdollinen tiliotelinkki
                foreach (TilioteTieto ote, tilioteLista) {
                    if( ote.tilinumero == index.data(VientiModel::TiliNumeroRooli) &&
                        ote.alkaa <= index.data(VientiModel::PvmRooli).toDate() &&
                        ote.paattyy >= index.data(VientiModel::PvmRooli).toDate())
                    {
                        // Tämä vienti oikealla tilillä ja päivämäärävälillä
                        if( ote.tositeId != tositeId)
                            out << "&nbsp;<a href=" << QString("%1.html").arg( ote.tositeId, 8, 10, QChar('0')) << ">(Tiliote)</a>";
                        break;
                    }
                }
                out << "</td><td class=kohdennus>";

                // Kohdennukset: Jos kohdennetaan tase-erään, on tase-erän tunnus linkkinä
                int eranid = index.data(VientiModel::EraIdRooli).toInt();
                if( eranid )
                {
                    QSqlQuery kohdennusKysely(QString("SELECT tosite FROM vienti WHERE id=%1").arg(eranid));
                    if( kohdennusKysely.next())
                        eranid = kohdennusKysely.value("tosite").toInt();
                }

                QString kohdennusTxt = index.sibling(vientiRivi, VientiModel::KOHDENNUS).data().toString();

                if( eranid)
                    out << QString("<a href=%1.html>%2</a>").arg( eranid, 8, 10, QChar('0')).arg(kohdennusTxt);
                else if(kohdennusTxt != "VIITE")        // VIITE-tekstiä ei tulosteta
                    out << kohdennusTxt;
                if(taseEraSeurannassa)      // Jos muodostaa tase-erän, tulee viittaus sen erittelyyn
                    out << QString("<sup>%1)</sup>").arg(seuratutTaseErat);

                out << "</td><td class=selite>" << index.sibling(vientiRivi, VientiModel::SELITE).data().toString();
                out << "</td><td class=euro>" << index.sibling(vientiRivi, VientiModel::DEBET).data().toString();
                out << "</td><td class=euro>" << index.sibling(vientiRivi, VientiModel::KREDIT).data().toString();
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

        out << eraLaatikko;


        // Ja lopuksi sekalaiset tiedot

        out << "<p class=info>Kirjanpito arkistoitu " << QDate::currentDate().toString(Qt::SystemLocaleDate);

        if( tilikausi_.paattyy() > kp()->tilitpaatetty() )
            out << " (Keskener&auml;inen kirjanpito)";
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

    // Jos tilit on päätetty (tilikausi lukittu), tulee linkki myös tilinpäätökseen (pdf)
    if( tilikausi_.paattyy() <= kp()->tilitpaatetty() )
        out << "<h3>" << tr("Tilinpäätös") << "</h3>"
            << "<ul><li><a href=tilinpaatos.pdf>" << tr("Tilinpäätös") << "</a></li>"
            << "<li><a href=taseerittely.html>" << tr("Tase-erittely") << "</a></li></ul>";


    out << "<h3>Kirjanpito</h3>";
    out << "<ul><li><a href=paakirja.html>" << tr("Pääkirja") << "</a></li>";
    out << "<li><a href=paivakirja.html>" << tr("Päiväkirja") << "</a></li>";
    out << "<li><a href=tositeluettelo.html>Tositeluettelo</a></li>";
    out << "<li><a href=tositepaivakirja.html>" << tr("Tositepäiväkirja") << "</a></li>";
    out << "<li><a href=tililuettelo.html>Tililuettelo</a></li>";
    out << "</ul><h3>Raportit</h3><ul>";

    QStringList raportit;
    raportit = kp()->asetukset()->lista("ArkistoRaportit");
    raportit.sort(Qt::CaseInsensitive);

    // Kirjoitetaan kaikki kirjanpitoon liittyvät raportit arkistoon
    foreach (QString raportti, raportit)
    {
        if( raportti.length() > 1 )
        {

            QString tiedostonnimi = raportti.toLower() + ".html";
            tiedostonnimi.replace(" ","");

            Tilikausi edellinenkausi = kp()->tilikaudet()->tilikausiPaivalle( tilikausi_.alkaa().addDays(-1) );


            Raportoija raportoija(raportti);
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
            }

            arkistoiTiedosto( tiedostonnimi, raportoija.raportti().html(true) );

            // Kirjoitetaan indeksiin
            out << "<li><a href=\'" << tiedostonnimi << "\'>";
            out << raportti;
            out << "</a></li>";
        }
    }

    // Tase-erittely myös keskeneräiseen kirjanpitoon
    if( tilikausi_.paattyy() > kp()->tilitpaatetty() )
        out << "<li><a href=taseerittely.html>" << tr("Tase-erittely") << "</a></li>";
    out << "</ul>";

    kirjoitaHash();

    out << tr("<p class=info>Tämä kirjanpidon sähköinen arkisto on luotu %1 <a href=https://artoh.github.io/kitupiikki>Kitupiikki-ohjelman</a> versiolla %2 <br>")
           .arg(QDate::currentDate().toString(Qt::SystemLocaleDate))
           .arg(qApp->applicationVersion());
    out << tr("Arkiston muuttumattomuus voidaan valvoa sha256-tiivisteellä <code>%1</code> </p>").arg( QString(QCryptographicHash::hash( shaBytes, QCryptographicHash::Sha256).toHex()) );
    if( tilikausi_.paattyy() > kp()->tilitpaatetty() )
        out << "Kirjanpito on viel&auml; keskener&auml;inen.";


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

    navi.append("<li class=nappi><a href=ohje.html target=_blank>Ohje</a></li>");


    if(seuraava)
        navi.append( tr("<li class=nappi><a href=\'%1.html\'>Seuraava &rarr;</a></li>").arg(seuraava,8,10,QChar('0')));
    else
        navi.append( "<li class=nappi> </li>");

    if( edellinen )
        navi.append( tr("<li class=nappi><a href=\'%1.html\'>&larr; Edellinen</a></li>").arg(edellinen,8,10,QChar('0')));
    else
        navi.append("<li class=nappi> </li>");


    navi.append("</ul></nav></div>");

    return navi;
}



QString Arkistoija::arkistoi(Tilikausi &tilikausi)
{
    Arkistoija arkistoija(tilikausi);
    arkistoija.luoHakemistot();
    arkistoija.arkistoiTositteet();

    arkistoija.arkistoiTiedosto("taseerittely.html",
                                 TaseErittely::kirjoitaRaportti( tilikausi.alkaa(), tilikausi.paattyy()).html(true) );
    arkistoija.arkistoiTiedosto("paivakirja.html",
                                PaivakirjaRaportti::kirjoitaRaportti( tilikausi.alkaa(), tilikausi.paattyy(), -1, false, true, true, true).html(true) );
    arkistoija.arkistoiTiedosto("paakirja.html",
                                PaakirjaRaportti::kirjoitaRaportti( tilikausi.alkaa(), tilikausi.paattyy(), -1, true, true).html(true));
    arkistoija.arkistoiTiedosto("tililuettelo.html",
                                TilikarttaRaportti::kirjoitaRaportti(TilikarttaRaportti::KAYTOSSA_TILIT, tilikausi, false, tilikausi.paattyy(),true).html(true));
    arkistoija.arkistoiTiedosto("tositeluettelo.html",
                                TositeluetteloRaportti::kirjoitaRaportti( tilikausi.alkaa(), tilikausi.paattyy(), true, true, false, false, true).html(true) );
    arkistoija.arkistoiTiedosto("tositepaivakirja.html",
                                TositeluetteloRaportti::kirjoitaRaportti( tilikausi.alkaa(), tilikausi.paattyy(), true, true, true, true, true).html(true));



    // Tämän pitää tulla lopuksi jotta hash toimii !!!
    arkistoija.kirjoitaIndeksiJaArkistoiRaportit();

    return QString( QCryptographicHash::hash( arkistoija.shaBytes , QCryptographicHash::Sha256).toHex() );
}
