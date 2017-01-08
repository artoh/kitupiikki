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

#include "sisalto.h"

#include <QDebug>
#include <QDesktopServices>

Sisalto::Sisalto()
{

}

void Sisalto::lisaaTxt(const QString &teksti)
{
    txt += teksti;
}

void Sisalto::lisaaLaatikko(const QString &otsikko, const QString &sisalto)
{
    txt += "<div class=loota><div class=otsake>" + otsikko + "</div>";
    txt += sisalto + "</div>\n";
}

void Sisalto::valmis()
{
    QString alku = "<html><head><link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/aloitus/aloitus.css\"></head><body>";

    alku += "<div class=oikea>"
            "<p class=nappi><a href=ktp:/uusi><img src=qrc:/aloitus/uusinappi.png></a>"
            "<p class=nappi><a href=ktp:/avaa><img src=qrc:/aloitus/avaanappi.png></a>"
            "<hr><h3>Viimeiset tiedostot</h3>"
            "<a href=avaa:/tmp/kokeilu>Kokeilukirjanpito</a>"
            "</div>";

    QString loppu = "</body></html>";

    setHtml( alku + txt + loppu );
    txt = "";
}

bool Sisalto::acceptNavigationRequest(const QUrl &url, QWebEnginePage::NavigationType type, bool isMainFrame)
{
    qDebug() << url;
    qDebug() << url.path();

    if( url.scheme() == "ktp")
    {
        emit( toiminto( url.fileName()) );
        return false;
    }
    else if( url.scheme() == "avaa")
    {
        emit( toiminto(url.path()));
        return false;
    }
    else
        return true;
}
