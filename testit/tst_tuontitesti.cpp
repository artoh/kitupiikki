/*
   Copyright (C) 2018 Arto Hyvättinen

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
#include <QCoreApplication>

// add necessary includes here

#include "../kitupiikki/validator/ibanvalidator.h"
#include "../kitupiikki/tuonti/tuontiapu.h"

class TuontiTesti : public QObject
{
    Q_OBJECT

public:
    TuontiTesti();
    ~TuontiTesti();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void ibanTesti();
    void senttiTesti();

};

TuontiTesti::TuontiTesti()
{

}

TuontiTesti::~TuontiTesti()
{

}

void TuontiTesti::initTestCase()
{

}

void TuontiTesti::cleanupTestCase()
{

}

void TuontiTesti::ibanTesti()
{
    QCOMPARE( IbanValidator::kelpo("FI11 3485 1420 0096 37"), IbanValidator::Acceptable );
    QCOMPARE( IbanValidator::kelpo("FI11 3485 1420 0196 37"), IbanValidator::Invalid );
    QCOMPARE( IbanValidator::kelpo("FI5540 043383 000835"), IbanValidator::Acceptable );
    QCOMPARE( IbanValidator::kelpo("FI9671318270006992"), IbanValidator::Acceptable );

}

void TuontiTesti::senttiTesti()
{
    QCOMPARE( TuontiApu::sentteina("4.01"), 401 );
    QCOMPARE( TuontiApu::sentteina("4.2"), 420 );
    QCOMPARE( TuontiApu::sentteina("4.21"), 421 );
    QCOMPARE( TuontiApu::sentteina("15,23"), 1523 );
    QCOMPARE( TuontiApu::sentteina("1 234,56"), 123456 );
    QCOMPARE( TuontiApu::sentteina("-12,34"), -1234 );
    QCOMPARE( TuontiApu::sentteina("-1,4"), -140 );
    QCOMPARE( TuontiApu::sentteina("1,234.56-"), -123456 );
    QCOMPARE( TuontiApu::sentteina("70+"), 7000 );
    QCOMPARE( TuontiApu::sentteina("8"), 800 );
    QCOMPARE( TuontiApu::sentteina("98,0"), 9800 );
    QCOMPARE( TuontiApu::sentteina("0,02-"), -2 );
}

QTEST_MAIN(TuontiTesti)

#include "tst_tuontitesti.moc"
