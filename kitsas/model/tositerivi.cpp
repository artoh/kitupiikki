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
    if( laskutetaanKpl().isEmpty() && qAbs(myyntiKpl()) > 1e-5 )
        setLaskutetaanKpl( QString("%1").arg(myyntiKpl(),0, 'f', 2) );
    if( !bruttoYhteensa().cents())
        laskeYhteensa();
}

Euro TositeRivi::laskeYhteensa()
{
    const double netto = aNetto() * myyntiKpl();

    const double vero = alvkoodi() < Lasku::KAYTETYT ?
                alvProsentti() * netto / 100.0 :
                0 ;

    const double brutto = netto + vero;

    const double alennettu = aleProsentti() ?
                ( 100.0 - aleProsentti() ) * brutto / 100.0 :
                brutto - euroAlennus().toDouble();


    Euro tulos = Euro::fromDouble( alennettu);
    setBruttoYhteensa( tulos );
    return tulos;
}

double TositeRivi::laskeYksikko()
{
    const double brutto = bruttoYhteensa().toDouble();

    const double alentamaton = aleProsentti() ?
                100 * brutto / ( 100 - aleProsentti()) :
                brutto + euroAlennus().toDouble();

    const double netto = alvkoodi() < Lasku::KAYTETYT ?
                100 * alentamaton / ( 100 + alvProsentti()) :
                alentamaton;


    const double ahinta = netto / myyntiKpl();
    setANetto(ahinta);
    return ahinta;
}
