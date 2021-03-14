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
#include <QtTest>

#include "model/tositerivi.h"

class TositeRiviTesti : public QObject
{
    Q_OBJECT

public:
    TositeRiviTesti();
    ~TositeRiviTesti();

private slots:
    void summa1();
    void summa2();
    void summa3();
    void summa4();

    void ahinta1();

};

TositeRiviTesti::TositeRiviTesti()
{

}

TositeRiviTesti::~TositeRiviTesti()
{

}

void TositeRiviTesti::summa1()
{
    TositeRivi rivi;
    rivi.setANetto(10.0);
    rivi.setMyyntiKpl(5.0);
    rivi.laskeYhteensa();

    QCOMPARE( "50.00", rivi.bruttoYhteensa() );
}

void TositeRiviTesti::summa2()
{
    TositeRivi rivi;
    rivi.setANetto(10.0);
    rivi.setMyyntiKpl(10.0);
    rivi.setAlvKoodi(11);
    rivi.setAlvProsentti(24.0);
    rivi.laskeYhteensa();

    QCOMPARE( "124.00", rivi.bruttoYhteensa() );
}

void TositeRiviTesti::summa3()
{
    TositeRivi rivi;
    rivi.setANetto(100.0);
    rivi.setMyyntiKpl(10.0);
    rivi.setAleProsentti(25);
    rivi.laskeYhteensa();

    QCOMPARE( "750.00", rivi.bruttoYhteensa() );
}

void TositeRiviTesti::summa4()
{
    TositeRivi rivi;
    rivi.setANetto(50.0);
    rivi.setMyyntiKpl(10);
    rivi.setEuroAlennus(Euro("50.00"));
    rivi.laskeYhteensa();

    QCOMPARE( "450.00", rivi.bruttoYhteensa() );
}

void TositeRiviTesti::ahinta1()
{
    TositeRivi rivi;
    rivi.setBruttoYhteensa(Euro("124.00"));
    rivi.setAlvKoodi(11);
    rivi.setAlvProsentti(24.0);
    rivi.setMyyntiKpl(2.0);
    rivi.laskeYksikko();

    QVERIFY( qAbs( rivi.aNetto() - 50.00 ) < 1e-5 );
}



QTEST_APPLESS_MAIN(TositeRiviTesti)

#include "tst_tositerivitesti.moc"
