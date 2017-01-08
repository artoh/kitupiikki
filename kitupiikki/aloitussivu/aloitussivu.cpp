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


#include "aloitussivu.h"

#include "sisalto.h"

AloitusSivu::AloitusSivu()
{
    sisalto = new Sisalto();
    setPage(sisalto);

    connect(sisalto, SIGNAL(toiminto(QString)), this, SIGNAL(toiminto(QString)));
}

void AloitusSivu::lataaAloitussivu()
{
    sisalto->lisaaTxt("<h1>Tervetuloa</h1>");
    sisalto->lisaaTxt("<p>Tässä on kuvausta siitä, kuinka hieno ohjelma kitupiikki oikein onkaan siltä varalta"
                      "ettei käyttäjä ole sitä itse tajunnut eika hoksannut.</p>");
    sisalto->lisaaLaatikko("Aika tehdä yhtä ja toista",
                           "Tarkemmat ohjeet siitä, mitä tämä yksi ja toinen voisivat oikein olla silleen mukavasti kerrottuna");
    sisalto->lisaaTxt("&copy; Ohjelman alaviitetietoja ja joissain linkki kotisivulle.");
    sisalto->lisaaTxt("<a href=https://artoh.github.io/kitupiikki>Kotisivu</a>");

    sisalto->valmis();
}

void AloitusSivu::lataaOhje()
{
    sisalto->load(QUrl("qrc:/aloitus/ohje.html"));
}
