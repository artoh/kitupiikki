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

#include <QFile>
#include <QStringList>
#include <QSqlQuery>
#include "aloitussivu.h"

#include "sisalto.h"

#include "db/kirjanpito.h"

AloitusSivu::AloitusSivu()
{
    sisalto = new Sisalto();
    setPage(sisalto);

    connect(sisalto, SIGNAL(toiminto(QString)), this, SIGNAL(toiminto(QString)));
}

void AloitusSivu::lataaAloitussivu()
{

    if( Kirjanpito::db()->asetus("nimi").isEmpty())
    {
        sivunAloitus();
        lisaaTxt("<h1>Tervetuloa!</h1>"
                 "<p>Kitupiikki on suomalainen avoimen lähdekoodin kirjanpito-ohjelmisto. Ohjelmistoa saa käyttää, kopioida ja muokata "
                 "maksutta.</p>"
                 "<p>Tutustu lukemalla ohjeita, tai aloita heti kokeilemalla <a href=ktp:uusi>uuden kirjanpidon luomista</a>. Ohjelmisto neuvoo sekä "
                 "uuden kirjanpidon aloittamisessa että myöhemmin matkan varrella.<p>");
        alatunniste();
        sisalto->valmis("qrc:/aloitus/");

    }
    else
    {
        sivunAloitus();
        kpAvattu();
        alatunniste();
        sisalto->valmis( Kirjanpito::db()->hakemisto().absoluteFilePath("index"));
    }

}

void AloitusSivu::lataaOhje()
{
    sisalto->load(QUrl("qrc:/aloitus/ohje.html"));
}

void AloitusSivu::lisaaTxt(const QString &txt)
{
    sisalto->lisaaTxt(txt);
}

void AloitusSivu::sivunAloitus()
{
    lisaaTxt("<html><head><link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/aloitus/aloitus.css\"></head><body>");

    lisaaTxt("<div class=oikea>"
            "<p class=nappi><a href=ktp:/uusi><img src=qrc:/aloitus/uusinappi.png></a>"
            "<p class=nappi><a href=ktp:/avaa><img src=qrc:/aloitus/avaanappi.png></a>"
            "<hr><h3>Viimeiset tiedostot</h3>");

    QStringList viimeiset = Kirjanpito::db()->viimeisetTiedostot();


    foreach (QString viimeinen,viimeiset)
    {
        QStringList splitti = viimeinen.split(";");
        QString polku = splitti[0];
        QString nimi = splitti[1];
        QString kuvake = QFileInfo(polku).absoluteDir().absoluteFilePath("logo64.png");
        if( QFile::exists(kuvake))
            lisaaTxt( QString("<div class=vtiedosto><a href=\"avaa:%1\"><img src=\"%3\">%2</a></div>").arg(polku, nimi, kuvake ) );
        else
            lisaaTxt( QString("<div class=vtiedosto><a href=\"avaa:%1\" class=tiedosto>%2</a></div>").arg(polku, nimi) );
    }

    sisalto->lisaaTxt("</div>");
}

void AloitusSivu::kpAvattu()
{
    if( QFile::exists( Kirjanpito::db()->hakemisto().absoluteFilePath("logo128.png")))
    {
        lisaaTxt("<img class=kpkuvake src=logo128.png>");
    }
    lisaaTxt(tr("<h1>%1</h1>").arg( Kirjanpito::db()->asetus("nimi")));

    if( Kirjanpito::db()->asetus("tilinavaus").toInt() > 1)
    {
        sisalto->lisaaLaatikko("Tee tilinavaus","Syötä viimesimmältä tilinpäätökseltä tilien "
                 "avaavat saldot järjestelmään.");
    }
    saldot();

}

void AloitusSivu::saldot()
{
    Tilikausi tilikausi = Kirjanpito::db()->tilikausiPaivalle( Kirjanpito::db()->paivamaara());

    lisaaTxt(tr("<h2>Tilikausi %1 - %2</h2>").arg(tilikausi.alkaa().toString(Qt::SystemLocaleShortDate))
             .arg(tilikausi.paattyy().toString(Qt::SystemLocaleShortDate)));

    QSqlQuery kysely;

    kysely.exec(QString("select tili, nimi, sum(debetsnt), sum(kreditsnt) from vientivw where tyyppi like \"AR%\" and pvm <= \"%1\" group by tili")
                .arg(tilikausi.paattyy().toString(Qt::ISODate)));

    lisaaTxt("<h3>Rahavarat</h3><table>");
    int saldosumma = 0;
    while( kysely.next())
    {
        int saldosnt = kysely.value(2).toInt() - kysely.value(3).toInt();
        saldosumma += saldosnt;
        lisaaTxt( tr("<tr><td>%1 %2</td><td class=euro>%L3 €</td></tr>").arg(kysely.value(0).toInt())
                                                           .arg(kysely.value(1).toString())
                                                           .arg( ((double) saldosnt ) / 100,0,'f',2 ) );
    }
    lisaaTxt( tr("<tr class=summa><td>Rahavarat yhteensä</td><td class=euro>%L1 €</td></tr>").arg( ((double) saldosumma ) / 100,0,'f',2 ) );
    lisaaTxt("</table>");

    // Sitten tulot
    kysely.exec(QString("select tili, nimi, sum(debetsnt), sum(kreditsnt) from vientivw where tyyppi like \"T%\" AND pvm BETWEEN \"%1\" AND \"%2\" group by tili")
                .arg(tilikausi.alkaa().toString(Qt::ISODate)  )
                .arg(tilikausi.paattyy().toString(Qt::ISODate)));

    lisaaTxt("<h3>Tulot</h3><table>");
    int summatulot = 0;

    while( kysely.next())
    {
        int saldosnt = kysely.value(3).toInt() - kysely.value(2).toInt();
        summatulot += saldosnt;
        lisaaTxt( tr("<tr><td>%1 %2</td><td class=euro>%L3 €</td></tr>").arg(kysely.value(0).toInt())
                                                           .arg(kysely.value(1).toString())
                                                           .arg( ((double) saldosnt ) / 100,0,'f',2 ) );
    }
    lisaaTxt( tr("<tr class=summa><td>Tulot yhteensä</td><td class=euro>%L1 €</td></tr>").arg( ((double) summatulot ) / 100,0,'f',2 ) );
    lisaaTxt("</table>");


    // Lopuksi menot
    kysely.exec(QString("select tili, nimi, sum(debetsnt), sum(kreditsnt) from vientivw where tyyppi like \"M%\" AND pvm BETWEEN \"%1\" AND \"%2\" group by tili")
                .arg(tilikausi.alkaa().toString(Qt::ISODate)  )
                .arg(tilikausi.paattyy().toString(Qt::ISODate)));


    lisaaTxt("<h3>Menot</h3><table>");
    int summamenot = 0;

    while( kysely.next())
    {
        int saldosnt = kysely.value(2).toInt() - kysely.value(3).toInt();
        summamenot += saldosnt;
        lisaaTxt( tr("<tr><td>%1 %2</td><td class=euro>%L3 €</td></tr>").arg(kysely.value(0).toInt())
                                                           .arg(kysely.value(1).toString())
                                                           .arg( ((double) saldosnt ) / 100,0,'f',2 ) );
    }
    lisaaTxt( tr("<tr class=summa><td>Menot yhteensä</td><td class=euro>%L1 €</td></tr>").arg( ((double) summamenot ) / 100,0,'f',2 ) );
    lisaaTxt("</table>");

    lisaaTxt( tr("<p><table><tr class=kokosumma><td>Yli/alijäämä</td><td class=euro> %L1 €</td></tr></table>").arg(( ((double) (summatulot - summamenot) ) / 100), 0,'f',2 )) ;


}

void AloitusSivu::alatunniste()
{
    lisaaTxt("<hr><p>Kitupiikki &copy; Arto Hyvättinen 2017<br>Ohjelmaa saa käyttää, levittää ja muokata maksutta GNU General Public License 3:n ehtojen mukaisesti. "
             "Ohjelmalle ei anneta takuuta, eikä ohjelman aiheuttamista vahingoista oteta vastuuta. Ohjelman lähdekoodin löydät kotisivulta:<br>");
    lisaaTxt("<a href=https://artoh.github.io/kitupiikki>artoh.github.io/kitupiikki</a>");

    lisaaTxt("</body></html>");
}
