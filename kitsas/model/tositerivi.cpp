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

void TositeRivi::setAleProsentti(const double prosentti)
{
    if( qAbs(prosentti) < 1e-3)
        unset("aleprosentti");
    else
        set("aleprosentti", prosentti);
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

void TositeRivi::setEuroAlennus(const double euro)
{
    if( qAbs(euro) < 1e-3)
        unset("euroalennus");
    else
        set("euroalennus", euro );
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
    const double veroSnt = alvkoodi() < Lasku::KAYTETYT ?
                alvProsentti() * alennettu:
                0 ;

    const double bruttoSnt = alennettu * 100.0 + veroSnt;
    return Euro(qRound64(bruttoSnt));
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

    if( ahinta < 0) {
        // Jos kappalehinnan etumerkki olisi muuttumassa,
        // pitääkin muuttaa myyntikappaleen etumerkkiä,
        // koska kappalehinnan pitää olla aina positiivinen!
        setMyyntiKpl( 0 - myyntiKpl());
        if( laskutetaanKpl().startsWith('-')) {
            setLaskutetaanKpl( laskutetaanKpl().mid(1) );
        } else {
            setLaskutetaanKpl("-" + laskutetaanKpl());
        }
        setANetto( 0 - ahinta );
    } else {
        setANetto(ahinta);
    }

}

double TositeRivi::nettoYhteensa() const
{
    const double netto = aNetto() * myyntiKpl();

    const double alennettu = aleProsentti() ?
                ( 100.0 - aleProsentti() ) * netto / 100.0 :
                netto - euroAlennus();

    return alennettu;
}
