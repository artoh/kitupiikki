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
#include "tulomenorivitesti.h"

#include "apuri/tulomenorivi.h"
#include "db/verotyyppimodel.h"

#include <QJsonDocument>

TuloMenoRiviTesti::TuloMenoRiviTesti(QObject *parent) : QObject(parent)
{

}

void TuloMenoRiviTesti::kotimaaAlvLaskenta()
{
    TulomenoRivi rivi;

    rivi.setAlvkoodi( AlvKoodi::MYYNNIT_NETTO );
    rivi.setAlvprosentti( 24.00);
    rivi.setNetto( 10000 );

    QCOMPARE( rivi.brutto(), 12400);
}

void TuloMenoRiviTesti::brutostaNetto()
{
    TulomenoRivi rivi;

    rivi.setAlvkoodi( AlvKoodi::MYYNNIT_NETTO );
    rivi.setAlvprosentti( 24.00);
    rivi.setBrutto( 12400 );

    QCOMPARE( rivi.netto(), 10000);
}

void TuloMenoRiviTesti::verottomanRiviLuku()
{
    QByteArray teksti = R"(
                        {
                        "id": 5,
                        "kredit": 100,
                        "pvm": "2019-11-09",
                        "tili": 3000,
                        "tyyppi": 201
                    })";

    QVariantMap map = QJsonDocument::fromJson(teksti).toVariant().toMap();
    TulomenoRivi rivi(map);

    QCOMPARE( rivi.netto(), 10000);
    QCOMPARE( rivi.brutto(), 10000);
    QCOMPARE( rivi.alvprosentti(), 0.0 );
    QCOMPARE( rivi.alvkoodi(), 0 );
    QCOMPARE( rivi.tilinumero(), 3000);
}

void TuloMenoRiviTesti::verollisenRivinLuku()
{
    QByteArray teksti = R"({
                        "alvkoodi": 11,
                        "alvprosentti": 24,
                        "id": 2,
                        "kredit": 100,
                        "pvm": "2019-11-09",
                        "selite": "Alvillista myyntiä",
                        "tili": 7390,
                        "tyyppi": 201
                    })";

    QVariantMap map = QJsonDocument::fromJson(teksti).toVariant().toMap();
    TulomenoRivi rivi(map);

    QCOMPARE( rivi.netto(), 10000);
    QCOMPARE( rivi.brutto(), 12400);
    QCOMPARE( rivi.alvprosentti(), 24.0 );
    QCOMPARE( rivi.alvkoodi(), 11 );
}
