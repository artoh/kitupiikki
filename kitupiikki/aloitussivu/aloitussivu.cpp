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
#include "aloitussivu.h"

#include "sisalto.h"

#include "db/kirjanpito.h"

AloitusSivu::AloitusSivu()
{
    sisalto = new Sisalto();
    setPage(sisalto);

    connect(sisalto, SIGNAL(toiminto(QString)), this, SIGNAL(toiminto(QString)));
}

void AloitusSivu::lataaAloitussivu(Kirjanpito *kirjanpito)
{

    if( kirjanpito->asetus("nimi").isEmpty())
    {
        sivunAloitus(kirjanpito);
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
        sivunAloitus(kirjanpito);
        kpAvattu(kirjanpito);
        alatunniste();
        sisalto->valmis( kirjanpito->hakemisto().absoluteFilePath("index"));
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

void AloitusSivu::sivunAloitus(Kirjanpito *kirjanpito)
{
    lisaaTxt("<html><head><link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/aloitus/aloitus.css\"></head><body>");

    lisaaTxt("<div class=oikea>"
            "<p class=nappi><a href=ktp:/uusi><img src=qrc:/aloitus/uusinappi.png></a>"
            "<p class=nappi><a href=ktp:/avaa><img src=qrc:/aloitus/avaanappi.png></a>"
            "<hr><h3>Viimeiset tiedostot</h3>");

    QStringList viimeiset = kirjanpito->viimeisetTiedostot();


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

void AloitusSivu::kpAvattu(Kirjanpito *kirjanpito)
{
    if( QFile::exists( kirjanpito->hakemisto().absoluteFilePath("logo128.png")))
    {
        lisaaTxt("<img class=kpkuvake src=logo128.png>");
    }
    lisaaTxt(tr("<h1>%1</h1>").arg( kirjanpito->asetus("nimi")));

    if( kirjanpito->asetus("harjoitus").toInt())
    {
        sisalto->lisaaLaatikko("Harjoittelutila käytössä","Voit nopeuttaa ajan kulumista näytön oikeassa alakulmassa olevasta valinnasta. "
                               "Näin pääset harjoittelemaan tilikauden vaihtamista ja tilinpäätöksen laatimista.");
    }
    if( kirjanpito->asetus("tilinavaus").toInt() > 1)
    {
        sisalto->lisaaLaatikko("Tee tilinavaus","Syötä viimesimmältä tilinpäätökseltä tilien "
                 "avaavat saldot järjestelmään.");
    }

}

void AloitusSivu::alatunniste()
{
    lisaaTxt("<hr><p>Kitupiikki &copy; Arto Hyvättinen 2017<br>Ohjelmaa saa käyttää, levittää ja muokata maksutta GNU General Public License 3:n ehtojen mukaisesti. "
             "Ohjelmalle ei anneta takuuta, eikä ohjelman aiheuttamista vahingoista oteta vastuuta. Ohjelman lähdekoodin löydät kotisivulta:<br>");
    lisaaTxt("<a href=https://artoh.github.io/kitupiikki>artoh.github.io/kitupiikki</a>");

    lisaaTxt("</body></html>");
}
