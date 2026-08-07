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

#include "../../kitsas/validator/ibanvalidator.h"
#include "../../kitsas/validator/maksuviestivalidator.h"

class ValidatorTest : public QObject
{
    Q_OBJECT

public:
    ValidatorTest();
    ~ValidatorTest();

private slots:
    void test_iban();
    void test_maksuViesti_validate();
    void test_maksuViesti_fixup();
    void test_maksuViesti_unicodeLetter();

};

ValidatorTest::ValidatorTest()
{

}

ValidatorTest::~ValidatorTest()
{

}

void ValidatorTest::test_iban()
{
    IbanValidator validator;
    QVERIFY(validator.kelpaako("FI83 4782 4837 0001 63") == true);
    QVERIFY(validator.kelpaako("FI8347824837000163") == true);
    QVERIFY(validator.kelpaako("FI83 4782 4837 0001 64") == false);
    QVERIFY(validator.kelpaako("FI8347824837000164") == false);
    QVERIFY(validator.kelpaako("FI834782483700016") == false);
}

void ValidatorTest::test_maksuViesti_validate()
{
    MaksuViestiValidator v;
    QString s;
    int pos = 0;
    QCOMPARE(v.validate(s, pos), QValidator::Intermediate);

    s = QStringLiteral("Maksu 1.4, a-ö");
    QCOMPARE(v.validate(s, pos), QValidator::Acceptable);

    s = QStringLiteral("hello@world");
    QCOMPARE(v.validate(s, pos), QValidator::Invalid);

    s = QStringLiteral("rivi\nuusi");
    QCOMPARE(v.validate(s, pos), QValidator::Invalid);

    s = QString(MaksuViestiValidator::maxPituus() + 1, QChar(u'a'));
    QCOMPARE(v.validate(s, pos), QValidator::Invalid);
}

void ValidatorTest::test_maksuViesti_fixup()
{
    MaksuViestiValidator v;
    QString s = QStringLiteral("ab@c,de");
    v.fixup(s);
    QCOMPARE(s, QStringLiteral("abc,de"));

    s = QString(200, QChar(u'x'));
    v.fixup(s);
    QCOMPARE(s.length(), MaksuViestiValidator::maxPituus());
    QVERIFY(s == QString(MaksuViestiValidator::maxPituus(), QChar(u'x')));
}

void ValidatorTest::test_maksuViesti_unicodeLetter()
{
    MaksuViestiValidator v;
    QString s = QStringLiteral("Тест 123");
    int pos = 0;
    QCOMPARE(v.validate(s, pos), QValidator::Acceptable);
}

QTEST_APPLESS_MAIN(ValidatorTest)

#include "tst_validatortest.moc"
