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

#include "raportinkorostin.h"

#include <QDebug>

RaportinKorostin::RaportinKorostin(QTextDocument *parent) :
    QSyntaxHighlighter( parent)
{
    lihava.setBold(true);
    tilinroRe.setPattern("^(\\d{1,8})(\\.\\.\\d{1,8})?[\\+-]?$");
}


void RaportinKorostin::highlightBlock(const QString &text)
{
    // Määrittelyt alkavat tablulaattorin tai neljän tyhjän jälkeen, kaikki muu
    // on otsikkoa.

    int tyhjanpaikka = text.indexOf('\t');

    if( tyhjanpaikka < 0 )
        tyhjanpaikka = text.indexOf("    ");

    // Lihavoidaan, jos on bold-määre
    if( tyhjanpaikka > 0 && text.mid(tyhjanpaikka).contains("bold"))
        setFormat(0, tyhjanpaikka, lihava);

    if( tyhjanpaikka < 0 )
        return;

    // Sitten määritellään tyhjänpaikasta eteenpäin
    QString nykysana;

    int sanaAlkoi = -1;
    bool valissa = true;
    bool tyyppikerrottu = false;
    bool summakentta = false;
    bool tilikentta = false;
    int valialkaa = tyhjanpaikka;

    int i = tyhjanpaikka;

    while( i < text.length() )
    {
        QChar merkki = text.at(i);
        bool onkovali = merkki == ' ' || merkki == '\t' || merkki == ',';

        // 1) Sana jatkuu
        if( !valissa && !onkovali)
            nykysana.append( merkki );
        // 2) Uusi sana alkaa
        else if( valissa && !onkovali)
        {
            // Värjätään väli (jos siinä vaikka pilkkuja)
            setFormat(valialkaa, i-1, QColor(Qt::darkMagenta));

            sanaAlkoi = i;
            valissa = false;
            nykysana = QString(merkki);
        }

        // 2) Sana tulee valmiiksi tai ollaan rivin lopussa
        if( (!valissa && onkovali ) || i == text.length() - 1 )
        {

            if( !nykysana.isEmpty())
            {

                // Tässä käsitellään tämä välikkö
                if( nykysana == "bold" )
                    setFormat(sanaAlkoi, i, QColor(Qt::magenta));
                else if( tilinroRe.match(nykysana).hasMatch() && !summakentta)
                {
                    // Kelvollinen tilinumero tai tilinumeroväli
                    setFormat(sanaAlkoi, i, QColor(Qt::blue));
                    tilikentta = true;     // Ei voi laittaa = -summan kanssa
                }
                else if( (nykysana == "=") && !tilikentta && !summakentta)
                {
                    summakentta = true;
                    setFormat(sanaAlkoi, i, QColor(Qt::darkBlue));
                }
                else if( nykysana == "==")
                    setFormat(sanaAlkoi, i, QColor(Qt::darkBlue));
                else
                {
                    // Leikataan lopusta pois sisennysnumero
                    if( nykysana.at( nykysana.count()-1 ).isDigit())
                        nykysana = nykysana.left( nykysana.count()-1);

                    if(( nykysana == "s" || nykysana == "S"
                                             || nykysana == "sum" || nykysana == "SUM"
                                             || nykysana =="h" || nykysana == "header"
                                             || nykysana =="d" || nykysana == "details") && !tyyppikerrottu )
                    {
                        tyyppikerrottu = true;  // Vain yksi tyyppisana kelpaa
                        setFormat(sanaAlkoi, i, QColor(Qt::darkGreen));

                    }
                    else
                        setFormat(sanaAlkoi, i, QColor(Qt::red));
                }

                valialkaa = i;

            }

            valissa = true;
            nykysana = QString();
        }

        i++;
    }
}
