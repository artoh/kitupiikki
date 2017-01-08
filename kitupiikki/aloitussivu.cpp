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

#include <QUrl>
#include <QDebug>
#include <QStringList>

AloitusSivu::AloitusSivu()
{
    setOpenLinks(false);    // Linkkejä ei avata automaattisesti

    setSearchPaths( QStringList() << ":/pic");

    connect(this, SIGNAL(anchorClicked(QUrl)), this, SLOT(linkkiKlikattu(QUrl)));

    document()->setDefaultFont(QFont("Helvetica",12));

    lisaaTaulu("Tervetuloa","Avaa tiedosto tai tee kokonaan uusi!",
               "<a href=uusi><img src=uusinappi.png><br><a href=avaa><img src=avaanappi.png></img>","Possu64.png");



}

void AloitusSivu::linkkiKlikattu(const QUrl &url)
{

    emit toiminto( url.fileName());
}

void AloitusSivu::lisaaTaulu(const QString &otsikko, const QString &sisalto, const QString &linkit, const QString& kuva)
{
    QString txt = "<table width=100%>";
    txt += "<tr><td colspan=2><b>" + otsikko + "</b></td>";
    txt += "<tr><td width=80px valign=center><img src=" + kuva + "</img></td><td width=99%>";
    txt += sisalto + "</font></td><td width=135px align=right>" + linkit + "</td></tr></table>";
    insertHtml(txt);
}
