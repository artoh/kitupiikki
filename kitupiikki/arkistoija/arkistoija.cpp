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
    hakemisto_.mkdir("liitteet");

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

    QDir liiteHakemisto = hakemisto_;
    liiteHakemisto.cd("liitteet");

    qDebug() << liiteHakemisto.absolutePath();

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
                         liiteHakemisto.absoluteFilePath( index.data(LiiteModel::TiedostoNimiRooli).toString()));

            out << "<li><a href=\"liitteet/" << index.data(LiiteModel::TiedostoNimiRooli).toString();
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

void Arkistoija::arkistoiRaportit()
{
    QStringList raportit;
    raportit << "Tase" << "Tuloslaskelma" << "Tuloslaskelma eritelty";

    foreach (QString raportti, raportit)
    {
        QString tiedostonnimi = raportti.toLower() + ".html";
        tiedostonnimi.replace(" ","");

        Raportoija raportoija(raportti);
        if( raportoija.onkoKausiraportti())
        {
            raportoija.lisaaKausi(tilikausi_.alkaa(), tilikausi_.paattyy());
        }
        else
        {
            raportoija.lisaaTasepaiva(tilikausi_.paattyy());
        }

        arkistoiTiedosto( tiedostonnimi, raportoija.raportti().html(true) );
    }



}



void Arkistoija::arkistoiTiedosto(const QString &tiedostonnimi, const QString &html)
{
    QFile tiedosto( hakemisto_.absoluteFilePath(tiedostonnimi));
    tiedosto.open( QIODevice::WriteOnly);
    QTextStream out( &tiedosto );
    out.setCodec("UTF-8");

    // Lisätään valikko tuohon kohtaan !
    QString txt = html;
    txt.insert( txt.indexOf("<body>") + 6, "<p>Tähän tulee valikko</p>");

    out << txt;

    tiedosto.close();
}

void Arkistoija::arkistoi(Tilikausi &tilikausi)
{
    Arkistoija arkistoija(tilikausi);
    arkistoija.luoHakemistot();
    arkistoija.arkistoiTositteet();
    arkistoija.arkistoiRaportit();

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
