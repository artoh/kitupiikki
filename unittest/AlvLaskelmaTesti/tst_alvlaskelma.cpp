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
#include <QFile>
#include <QJsonDocument>
#include <QApplication>
#include <QJsonDocument>

#include <QDebug>

#include "../../kitsas/db/kirjanpito.h"
#include "../../kitsas/model/tosite.h"
#include "../../kitsas/model/tositeviennit.h"
#include "../../kitsas/model/tositevienti.h"
#include "../../kitsas/db/verotyyppimodel.h"
#include "db/tositetyyppimodel.h"

#include "../../kitsas/alv/alvlaskelma.h"
#include "sqlite/sqlitemodel.h"

// add necessary includes here

class AlvLaskelmaTest : public QObject
{
    Q_OBJECT

public:
    AlvLaskelmaTest();
    ~AlvLaskelmaTest();

private slots:
    void initTestCase();
    void init();
    void bruttoMyynti();
    void bruttoOsto();
    void nettoMyynti();
    void nettoOsto();
    void cleanup();


protected:


};

AlvLaskelmaTest::AlvLaskelmaTest()
{
}

AlvLaskelmaTest::~AlvLaskelmaTest()
{
}

void AlvLaskelmaTest::initTestCase() {
    char *argv[] = {"Test"};
    int argc = 1;
    QApplication *app = new QApplication(argc, argv);
    kp()->asetaInstanssi(new Kirjanpito());
}

void AlvLaskelmaTest::init()
{
    const QString FILE = "/tmp/alv_laskelma_testi_1.kitsas";

    QFile::remove(FILE);
    QFile::copy(":/testidata/oy.kitsas",FILE);
    QFile::setPermissions(FILE, QFileDevice::WriteUser | QFileDevice::ReadUser );
    kp()->avaaTietokanta("/tmp/alv_laskelma_testi_1.kitsas");
}

void AlvLaskelmaTest::bruttoMyynti()
{
    Tosite tosite;
    tosite.asetaPvm(QDate(2020,01,10));
    tosite.asetaTyyppi(TositeTyyppi::TULO);

    TositeVienti vasta;
    vasta.setPvm(QDate(2020,01,10));
    vasta.setTyyppi(TositeVienti::MYYNTI + TositeVienti::VASTAKIRJAUS);
    vasta.setDebet(124.00);
    vasta.setTili(1910);

    tosite.viennit()->lisaa(vasta);

    TositeVienti myynti;
    myynti.setPvm(QDate(2020,01,10));
    myynti.setTyyppi(TositeVienti::MYYNTI + TositeVienti::KIRJAUS);
    myynti.setTili(3000);
    myynti.setKredit(124.00);
    myynti.setAlvKoodi(AlvKoodi::MYYNNIT_BRUTTO);
    myynti.setAlvProsentti(24.00);

    tosite.viennit()->lisaa(myynti);
    tosite.tallenna();


    AlvLaskelma laskelma;
    laskelma.laske(QDate(2020,01,01), QDate(2020,01,31));    

    qDebug() << laskelma.tosite_->tallennettava();
    qDebug() << laskelma.koodattu_;

    QVERIFY( laskelma.koodattu_.value(301) == 2400);
    QVERIFY( laskelma.koodattu_.value(308) == 2400);
    QVERIFY( laskelma.maksettava() == 2400);

    laskelma.tallenna();
    kp()->tilit()->haeSaldot();

    QVERIFY( kp()->tilit()->saldo(3000) == 100.0);
    QVERIFY( kp()->tilit()->saldo(2920) == 24.0);

}

void AlvLaskelmaTest::bruttoOsto()
{
    Tosite tosite;
    tosite.asetaPvm(QDate(2020,01,10));
    tosite.asetaTyyppi(TositeTyyppi::MENO);

    TositeVienti vasta;
    vasta.setPvm(QDate(2020,01,10));
    vasta.setTili(1910);
    vasta.setTyyppi(TositeVienti::OSTO + TositeVienti::VASTAKIRJAUS);
    vasta.setKredit(124.00);

    tosite.viennit()->lisaa(vasta);

    TositeVienti myynti;
    myynti.setPvm(QDate(2020,01,10));
    myynti.setTyyppi(TositeVienti::OSTO + TositeVienti::KIRJAUS);
    myynti.setTili(4000);
    myynti.setDebet(124.00);
    myynti.setAlvKoodi(AlvKoodi::OSTOT_BRUTTO);
    myynti.setAlvProsentti(24.00);

    tosite.viennit()->lisaa(myynti);

    tosite.tallenna();


    AlvLaskelma laskelma;
    laskelma.laske(QDate(2020,01,01), QDate(2020,01,31));

    qDebug() << laskelma.koodattu_;

    QVERIFY( laskelma.koodattu_.value(307) == 2400);
    QVERIFY( laskelma.koodattu_.value(308) == -2400);
    QVERIFY( laskelma.maksettava() == -2400);

    laskelma.tallenna();
    kp()->tilit()->haeSaldot();

    QVERIFY( kp()->tilit()->saldo(4000) == -100.0);
    QVERIFY( kp()->tilit()->saldo(2920) == -24.0);
}

void AlvLaskelmaTest::nettoMyynti() {
    Tosite tosite;
    tosite.asetaPvm(QDate(2020,01,10));
    tosite.asetaTyyppi(TositeTyyppi::TULO);

    TositeVienti vasta;
    vasta.setPvm(QDate(2020,01,10));
    vasta.setTyyppi(TositeVienti::MYYNTI + TositeVienti::VASTAKIRJAUS);
    vasta.setDebet(124.00);
    vasta.setTili(1910);

    tosite.viennit()->lisaa(vasta);

    TositeVienti myynti;
    myynti.setPvm(QDate(2020,01,10));
    myynti.setTyyppi(TositeVienti::MYYNTI + TositeVienti::KIRJAUS);
    myynti.setTili(3000);
    myynti.setKredit(100.00);
    myynti.setAlvKoodi(AlvKoodi::MYYNNIT_NETTO);
    myynti.setAlvProsentti(24.00);

    tosite.viennit()->lisaa(myynti);

    TositeVienti vero;
    vero.setPvm(QDate(2020,01,10));
    vero.setTyyppi(TositeVienti::MYYNTI + TositeVienti::ALVKIRJAUS);
    vero.setTili(2939);
    vero.setKredit(24.00);
    vero.setAlvKoodi(AlvKoodi::MYYNNIT_NETTO + AlvKoodi::ALVKIRJAUS);
    vero.setAlvProsentti(24.00);

    tosite.viennit()->lisaa(vero);

    tosite.tallenna();

    AlvLaskelma laskelma;
    laskelma.laske(QDate(2020,01,01), QDate(2020,01,31));

    qDebug() << laskelma.tosite_->tallennettava();
    qDebug() << laskelma.koodattu_;

    QVERIFY( laskelma.koodattu_.value(301) == 2400);
    QVERIFY( laskelma.koodattu_.value(308) == 2400);
    QVERIFY( laskelma.maksettava() == 2400);

    laskelma.tallenna();
    kp()->tilit()->haeSaldot();

    QVERIFY( kp()->tilit()->saldo(3000) == 100.0);
    QVERIFY( kp()->tilit()->saldo(2939) == 0.0);
    QVERIFY( kp()->tilit()->saldo(2920) == 24.0);

}

void AlvLaskelmaTest::nettoOsto() {
    Tosite tosite;
    tosite.asetaPvm(QDate(2020,01,10));
    tosite.asetaTyyppi(TositeTyyppi::MENO);

    TositeVienti vasta;
    vasta.setPvm(QDate(2020,01,10));
    vasta.setTyyppi(TositeVienti::OSTO + TositeVienti::VASTAKIRJAUS);
    vasta.setKredit(124.00);
    vasta.setTili(1910);

    tosite.viennit()->lisaa(vasta);

    TositeVienti osto;
    osto.setPvm(QDate(2020,01,10));
    osto.setTyyppi(TositeVienti::OSTO + TositeVienti::KIRJAUS);
    osto.setTili(4000);
    osto.setDebet(100.00);
    osto.setAlvKoodi(AlvKoodi::OSTOT_NETTO);
    osto.setAlvProsentti(24.00);

    tosite.viennit()->lisaa(osto);

    TositeVienti vero;
    vero.setPvm(QDate(2020,01,10));
    vero.setTyyppi(TositeVienti::OSTO + TositeVienti::ALVKIRJAUS);
    vero.setTili(1763);
    vero.setDebet(24.00);
    vero.setAlvKoodi(AlvKoodi::OSTOT_NETTO + AlvKoodi::ALVVAHENNYS);
    vero.setAlvProsentti(24.00);

    tosite.viennit()->lisaa(vero);

    tosite.tallenna();

    AlvLaskelma laskelma;
    laskelma.laske(QDate(2020,01,01), QDate(2020,01,31));

    qDebug() << laskelma.tosite_->tallennettava();
    qDebug() << laskelma.koodattu_;

    QVERIFY( laskelma.koodattu_.value(307) == 2400);
    QVERIFY( laskelma.koodattu_.value(308) == -2400);
    QVERIFY( laskelma.maksettava() == -2400);

    laskelma.tallenna();
    kp()->tilit()->haeSaldot();

    QVERIFY( kp()->tilit()->saldo(4000) == -100.0);
    QVERIFY( kp()->tilit()->saldo(1763) == 0.0);
    QVERIFY( kp()->tilit()->saldo(2920) == -24.0);

}

void AlvLaskelmaTest::cleanup()
{
    kp()->sqlite()->sulje();
}

QTEST_APPLESS_MAIN(AlvLaskelmaTest)

#include "tst_alvlaskelma.moc"
