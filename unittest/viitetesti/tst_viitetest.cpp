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

// add necessary includes here

#include "laskutus/viitenumero.h"

class ViiteTest : public QObject
{
    Q_OBJECT

public:
    ViiteTest();
    ~ViiteTest();

private slots:
    void lyhyt_vakioviite();
    void vanha_viite();
    void generoi_asiakas();
    void tunnista_asiakas();

};

ViiteTest::ViiteTest()
{

}

ViiteTest::~ViiteTest()
{

}

void ViiteTest::lyhyt_vakioviite()
{
    ViiteNumero vakio("1009");
    QCOMPARE( vakio.tyyppi(), ViiteNumero::VAKIOVIITE);
    QCOMPARE( vakio.kanta(), "100");
    QCOMPARE( vakio.viite(), "1009");

}

void ViiteTest::vanha_viite()
{
    ViiteNumero vanha("10690002");
    QCOMPARE( vanha.tyyppi(), ViiteNumero::LASKU);
    QCOMPARE( vanha.kanta(), "1069");
    QCOMPARE( vanha.valeilla(),"106 90002");
}

void ViiteTest::generoi_asiakas()
{
    ViiteNumero asiakas(ViiteNumero::ASIAKAS, 123);
    QCOMPARE( asiakas.viite(), "3012396");
    QCOMPARE( asiakas.valeilla(), "30 12396");
    QCOMPARE( asiakas.rfviite(), "RF06 3012 396");
}

void ViiteTest::tunnista_asiakas()
{
    ViiteNumero asiakas("RF06 3012 396");
    QCOMPARE( asiakas.tyyppi(), ViiteNumero::ASIAKAS);
    QCOMPARE( asiakas.numero(), 123);
    QCOMPARE( asiakas.eraId(), -1233);
}




QTEST_APPLESS_MAIN(ViiteTest)

#include "tst_viitetest.moc"
