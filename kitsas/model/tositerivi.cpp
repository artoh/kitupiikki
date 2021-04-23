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
    else if( laskutetaanKpl().isEmpty()) {
        setLaskutetaanKpl("1");
        setMyyntiKpl(1.0);
    }

    if (yksikko().isEmpty() && unKoodi().isEmpty()) {
        setUNkoodi("C62");
    }
}

double TositeRivi::aBrutto() const
{
    const double vero = alvkoodi() < Lasku::KAYTETYT ?
                alvProsentti() * aNetto() / 100.0 :
                0;
    return aNetto() + vero;
}

void TositeRivi::setABrutto(const double hinta)
{
    if( alvkoodi() < Lasku::KAYTETYT)
        setANetto( 100 * hinta / (100 + alvProsentti())  );
    else
        setANetto(hinta);
}

double TositeRivi::laskettuAleProsentti() const
{
    if( qAbs(aleProsentti()) > 1e-5 )
        return aleProsentti();
    else if( euroAlennus() > 1e-5 && nettoYhteensa() > 1e-5)
        return (   100 * euroAlennus()  / ( euroAlennus() + nettoYhteensa())  );
    else
        return 0;
}

double TositeRivi::bruttoEuroAlennus() const
{
    const double vero = alvkoodi() < Lasku::KAYTETYT ?
                alvProsentti() * euroAlennus() / 100.0 : 0;
    return euroAlennus() + vero;
}

void TositeRivi::setBruttoEuroAlennus(const double euro)
{
    if( alvkoodi() < Lasku::KAYTETYT)
        setEuroAlennus(100 * euro / ( 100 + alvProsentti()));
    else
        setEuroAlennus(euro);
}

double TositeRivi::laskennallinenEuroAlennus() const
{
    if( euroAlennus() )
        return euroAlennus();
    else if( aleProsentti())
        return aNetto() * myyntiKpl() * aleProsentti() / 100.0;
    else
        return 0;
}

double TositeRivi::laskennallinenBruttoEuroAlennus() const
{
    if( bruttoEuroAlennus())
        return bruttoEuroAlennus();
    else if( aleProsentti())
        return aBrutto() * myyntiKpl() * aleProsentti() / 100.0;
    else
        return 0;
}

Euro TositeRivi::bruttoYhteensa() const
{
    const double alennettu = nettoYhteensa();
    const double vero = alvkoodi() < Lasku::KAYTETYT ?
                alvProsentti() * alennettu / 100.0 :
                0 ;

    const double brutto = alennettu + vero;
    return Euro::fromDouble(brutto);
}

void TositeRivi::setBruttoYhteensa(const Euro &euro)
{
    const double brutto = euro.toDouble();

    const double netto = alvkoodi() < Lasku::KAYTETYT ?
                100 * brutto / ( 100 + alvProsentti()) :
                brutto;
    setNettoYhteensa(netto);
}

void TositeRivi::setNettoYhteensa(const double netto)
{
    const double alentamaton = aleProsentti() ?
                100 * netto / ( 100 - aleProsentti()) :
                netto + euroAlennus();

    const double ahinta = alentamaton / myyntiKpl();
    setANetto(ahinta);
}

double TositeRivi::nettoYhteensa() const
{
    const double netto = aNetto() * myyntiKpl();

    const double alennettu = aleProsentti() ?
                ( 100.0 - aleProsentti() ) * netto / 100.0 :
                netto - euroAlennus();

    return alennettu;
}
