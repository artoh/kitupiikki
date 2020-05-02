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
#include "tilitesti.h"

#include "db/tili.h"
#include "db/kirjanpito.h"

#include <QJsonDocument>

TiliTesti::TiliTesti()
{
}

TiliTesti::~TiliTesti()
{

}

void TiliTesti::initTestCase()
{
    Kirjanpito::asetaInstanssi(new Kirjanpito);
}

void TiliTesti::cleanupTestCase()
{
    kp()->deleteLater();
}

void TiliTesti::lueTili()
{
    QByteArray teksti = R"(        {
                     "iban": "FI2112345600000785",
                     "nimi": {
                         "fi": "Pankkitili"
                     },
                     "numero": 1910,
                     "laajuus": 1,
                     "tyyppi": "ARP"
                 })";
    QVariantMap map = QJsonDocument::fromJson(teksti).toVariant().toMap();
    Tili tili(map);

    QCOMPARE( tili.numero(), 1910);
    QCOMPARE( tili.nimi(), "Pankkitili");
    QCOMPARE( tili.tyyppi().koodi(), "ARP" );
    QCOMPARE( tili.tyyppi().onko(TiliLaji::VASTAAVAA), true);

}
