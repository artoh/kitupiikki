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

#include "tilisailo.h"

TiliSailo::TiliSailo(QObject *parent) : QObject(parent)
{

}

TiliSailo::~TiliSailo()
{
    tyhjenna();
}

QList<Tili *> TiliSailo::tilit(Tyyppisuodatin tyyppisuodatin, int tilasuodatin, int otsikkosuodatin)
{
    QList<Tili*> lista;
    foreach (Tili* tili, kasijarjestyslista)
    {
        if( tili->tila() < tilasuodatin)
            continue;   // Tila ei kelpaa

        // Erilaiset tyyppisuodattimet
        if( tyyppisuodatin == TASE && !tili->onkoTasetili())
            continue;
        if( tyyppisuodatin == TULOS && tili->onkoTasetili())
            continue;
        if( tyyppisuodatin == TULO && !tili->onkoTulotili())
            continue;
        if( tyyppisuodatin == MENO && !tili->onkoMenotili() )


        if( !tyyppisuodatin.isEmpty() && !tili->tyyppi().startsWith(tyyppisuodatin))
            continue;   // Tyyppi ei kelpaa
        if( otsikkosuodatin == 0 )
        {
            if( tili->otsikkotaso())
                continue;   // Haettiin vain tilejä
        }
        else if( otsikkosuodatin > 0)
        {
            // Haetaan otsikkoja max (otsikkosuodatin) tasolta, ei tilejä
            if( tili->otsikkotaso() == 0 || tili->otsikkotaso() > otsikkosuodatin)
                continue;
        }
        // Kaikki ehdot on nyt täytetty

        lista.append(tili);
    }
    return lista;

}

void TiliSailo::indeksoi()
{
    kasijarjestyslista.clear();
    hakutaulu.clear();

    foreach (Tili *ntili, tililista)
    {
        kasijarjestyslista[ ntili->kasitunnus() ] = ntili;
        hakutaulu[ ntili->numero() ] = ntili;
    }
}
