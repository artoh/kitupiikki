/*
   Copyright (C) 2019 Arto Hyvättinen

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
#include "tositerivi.h"

#include "lasku.h"

TositeRivi::TositeRivi(const QVariantMap &data)
    : KantaVariantti(data)
{

}

Euro TositeRivi::laskeYhteensa()
{
    const double nettoA = aNetto() * myyntiKpl();

    const double nettoB = aleProsentti() ?
                ( 100.0 - aleProsentti() ) * nettoA / 100.0 :
                nettoA - euroAlennus().toDouble();

    const double vero = alvkoodi() < Lasku::KAYTETYT ?
                alvProsentti() * nettoB / 100.0 :
                0 ;

    Euro tulos = Euro::fromDouble( nettoB + vero);
    setBruttoYhteensa( tulos );
    return tulos;
}

double TositeRivi::laskeYksikko()
{
    const double brutto = bruttoYhteensa().toDouble();

    const double nettoB = alvkoodi() < Lasku::KAYTETYT ?
                100 * brutto / ( 100 + alvProsentti()) :
                brutto;

    const double nettoA = aleProsentti() ?
                100 * nettoB / ( 100 - aleProsentti()) :
                nettoB + euroAlennus().toDouble();

    const double ahinta = nettoA / myyntiKpl();
    setANetto(ahinta);
    return ahinta;
}
