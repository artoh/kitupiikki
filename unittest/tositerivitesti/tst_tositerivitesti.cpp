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
    void nettosumma1();
    void summa1();
    void summa2();
    void summa3();
    void summa4();
    void hyvitys1();

    void ahintanetto1();
    void ahinta1();

};

TositeRiviTesti::TositeRiviTesti()
{

}

TositeRiviTesti::~TositeRiviTesti()
{

}

void TositeRiviTesti::nettosumma1()
{
    TositeRivi rivi;
    rivi.setANetto(10.0);
    rivi.setMyyntiKpl(5.0);

    QCOMPARE( Euro::fromDouble(rivi.nettoYhteensa()).cents(), 5000  );
}

void TositeRiviTesti::summa1()
{
    TositeRivi rivi;
    rivi.setANetto(10.0);
    rivi.setMyyntiKpl(5.0);

    QCOMPARE( rivi.bruttoYhteensa().cents(), 5000 );
}

void TositeRiviTesti::summa2()
{
    TositeRivi rivi;
    rivi.setANetto(10.0);
    rivi.setMyyntiKpl(10.0);
    rivi.setAlvKoodi(11);
    rivi.setAlvProsentti(24.0);

    QCOMPARE(rivi.bruttoYhteensa().cents(), 12400 );
}

void TositeRiviTesti::summa3()
{
    TositeRivi rivi;
    rivi.setANetto(100.0);
    rivi.setMyyntiKpl(10.0);
    rivi.setAleProsentti(25);

    QCOMPARE( rivi.bruttoYhteensa().cents(), 75000 );
}

void TositeRiviTesti::summa4()
{
    TositeRivi rivi;
    rivi.setANetto(50.0);
    rivi.setMyyntiKpl(10);
    rivi.setEuroAlennus(Euro("50.00"));

    QCOMPARE( rivi.bruttoYhteensa(), Euro(45000) );
}

void TositeRiviTesti::hyvitys1()
{
    TositeRivi rivi;
    rivi.setANetto(100.0);
    rivi.setMyyntiKpl(-1.0);

    QCOMPARE(rivi.bruttoYhteensa().cents(),-10000);
}

void TositeRiviTesti::ahintanetto1()
{
    TositeRivi rivi;
    rivi.setMyyntiKpl(5.0);
    rivi.setNettoYhteensa(100.0);
    QCOMPARE( Euro::fromDouble(rivi.aNetto()).cents(), 2000);
}

void TositeRiviTesti::ahinta1()
{
    TositeRivi rivi;    
    rivi.setAlvKoodi(11);
    rivi.setAlvProsentti(24.0);
    rivi.setMyyntiKpl(2.0);
    rivi.setBruttoYhteensa(Euro("124.00"));

    QCOMPARE( Euro::fromDouble(rivi.aNetto()).cents(), 5000);
}



QTEST_APPLESS_MAIN(TositeRiviTesti)

#include "tst_tositerivitesti.moc"
