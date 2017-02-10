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

#include "raporttirivi.h"

RaporttiRivi::RaporttiRivi()
{

}

void RaporttiRivi::lisaa(const QString &teksti, int sarakkeet, bool tasaaOikealle)
{
    RaporttiRiviSarake uusi;
    uusi.teksti = teksti;
    uusi.leveysSaraketta = sarakkeet;
    uusi.tasaaOikealle = tasaaOikealle;
    sarakkeet_.append(uusi);
}

void RaporttiRivi::lisaa(int sentit, bool tulostanollat)
{
    if( !sentit && !tulostanollat)
        // Ei tulosta nollalukuja
        lisaa(QString());
    else
        lisaa( QString("%L1").arg( ((double) sentit) / 100.0 ,0,'f',2 ), 1, true);
}

void RaporttiRivi::lisaa(const QDate &pvm)
{
    lisaa( pvm.toString(Qt::SystemLocaleShortDate));
}


