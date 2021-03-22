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
    if (yksikko().isEmpty() && unKoodi().isEmpty()) {
        setUNkoodi("C62");
    }

}

double TositeRivi::nettoYhteensa() const
{
    const double netto = aNetto() * myyntiKpl();

    const double alennettu = aleProsentti() ?
                ( 100.0 - aleProsentti() ) * netto / 100.0 :
                netto - euroAlennus().toDouble();

    return alennettu;
}

Euro TositeRivi::laskeYhteensa()
{
    const double alennettu = nettoYhteensa();
    const double vero = alvkoodi() < Lasku::KAYTETYT ?
                alvProsentti() * alennettu / 100.0 :
                0 ;

    const double brutto = alennettu + vero;
    setBruttoYhteensa( Euro::fromDouble(brutto) );
    return brutto;
}

double TositeRivi::laskeYksikko()
{
    const double brutto = bruttoYhteensa().toDouble();

    const double netto = alvkoodi() < Lasku::KAYTETYT ?
                100 * brutto / ( 100 + alvProsentti()) :
                brutto;

    const double alentamaton = aleProsentti() ?
                100 * netto / ( 100 - aleProsentti()) :
                netto + euroAlennus().toDouble();

    const double ahinta = alentamaton / myyntiKpl();
    setANetto(ahinta);
    return ahinta;
}
