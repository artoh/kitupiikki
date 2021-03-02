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

#include "kieli/monikielinen.h"
#include "kieli/kielet.h"

class MonikielinenTest : public QObject
{
    Q_OBJECT

public:
    MonikielinenTest();
    ~MonikielinenTest();

private slots:
    void test_mapista();
    void test_jsonista();
    void test_oletus();
    void test_kielet();

};

MonikielinenTest::MonikielinenTest()
{
    Kielet::alustaKielet(":/testidata/tulkki.json");
}

MonikielinenTest::~MonikielinenTest()
{

}

void MonikielinenTest::test_mapista()
{
    QVariantMap map;
    map.insert("fi","Suomeksi");
    map.insert("sv", "Ruotsiksi");
    Monikielinen m(map);
    QCOMPARE(m.teksti("sv"), QString("Ruotsiksi"));
    QCOMPARE(m.teksti("fi"), QString("Suomeksi"));
}

void MonikielinenTest::test_jsonista()
{
    Monikielinen m(QString(R"({"fi":"Suomeksi","sv":"Ruotsiksi"})"));
    QCOMPARE(m.teksti("fi"), QString("Suomeksi"));
    QCOMPARE(m.teksti("sv"), QString("Ruotsiksi"));
    QCOMPARE(m.teksti("en"), QString("Suomeksi"));
    QCOMPARE(m.kaannos("en"), QString());
    QCOMPARE(m.kaannos("fi"), QString("Suomeksi"));
}

void MonikielinenTest::test_oletus()
{
    Monikielinen m(QString(R"({"fi":"Suomeksi","sv":"Ruotsiksi"})"));
    Kielet::instanssi()->valitseKieli("sv");
    QCOMPARE(m.teksti(), QString("Ruotsiksi"));
}

void MonikielinenTest::test_kielet()
{    
    Kielet::instanssi()->asetaKielet(QString(R"({"fi":{"fi":"suomi","sv":"finska"},"sv":{"fi":"ruotsi","sv":"svenska"}})"));
    QCOMPARE( Kielet::instanssi()->kielet().count(), 2);
    Kielet::instanssi()->valitseKieli("fi");
    QCOMPARE( Kielet::instanssi()->kielet().value(1).nimi(), "ruotsi");
}

QTEST_APPLESS_MAIN(MonikielinenTest)

#include "tst_monikielinen.moc"
