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
#include "kpdateedittesti.h"
#include "db/kirjanpito.h"
#include "testiapu.h"
#include "tools/kpdateedit.h"
#include "kieli/kielet.h"

#include <QTest>

KpDateEditTesti::KpDateEditTesti(QObject *parent) : QObject(parent)
{

}

void KpDateEditTesti::initTestCase()
{
    Kielet::alustaKielet("");
    Kirjanpito::asetaInstanssi(new Kirjanpito());
    TestiApu::alustaKirjanpito();
}

void KpDateEditTesti::pvmRajat()
{
    KpDateEdit* edit = new KpDateEdit();

    QCOMPARE( edit->minimumDate(), QDate(2019,01,01));
    QCOMPARE( edit->maximumDate(), QDate(2019,12,31));

    delete edit;
}


