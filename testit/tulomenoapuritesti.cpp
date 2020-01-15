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
#include "tulomenoapuritesti.h"

#include "testiapu.h"
#include "apuri/tulomenoapuri.h"
#include "model/tosite.h"
#include "sqlite/sqlitemodel.h"

#include "db/tilinvalintaline.h"
#include "tools/kpeuroedit.h"

#include "db/verotyyppimodel.h"

#include "kirjaus/kirjauswg.h"
#include "lisaikkuna.h"
#include "kirjaus/kirjaussivu.h"

#include "model/tositeviennit.h"
#include "model/tositevienti.h"

#include <QJsonDocument>
#include <QDebug>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QLineEdit>

#include <QTabWidget>
#include <QSplitter>


TulomenoApuriTesti::TulomenoApuriTesti(QObject *parent) : QObject(parent)
{

    // Verollinen tulotosite
    QByteArray data = R"({
                      "id": 3,
                      "liitteet": [
                      ],
                      "loki": [
                          {
                              "aika": "2019-11-09 18:15:24",
                              "data": {
                                  "liita": [
                                  ],
                                  "otsikko": "Verollinen tulo",
                                  "pvm": "2019-01-01",
                                  "sarja": "ML",
                                  "tila": 100,
                                  "tyyppi": 200,
                                  "viennit": [
                                      {
                                          "debet": 124,
                                          "id": 0,
                                          "pvm": "2019-01-01",
                                          "selite": "Verollinen tulo",
                                          "tili": 1910,
                                          "tyyppi": 202
                                      },
                                      {
                                          "alvkoodi": 11,
                                          "alvprosentti": 24,
                                          "kohdennus": 0,
                                          "kredit": 100,
                                          "pvm": "2019-01-01",
                                          "selite": "Verollinen tulo",
                                          "tili": 7390,
                                          "tyyppi": 201
                                      },
                                      {
                                          "alvkoodi": 111,
                                          "alvprosentti": 24,
                                          "kredit": 24,
                                          "pvm": "2019-01-01",
                                          "selite": "Verollinen tulo",
                                          "tili": 2984,
                                          "tyyppi": 203
                                      }
                                  ]
                              },
                              "tila": 100
                          }
                      ],
                      "otsikko": "Verollinen tulo",
                      "pvm": "2019-01-01",
                      "rivit": [
                      ],
                      "sarja": "ML",
                      "tila": 100,
                      "tunniste": 2,
                      "tyyppi": 200,
                      "viennit": [
                          {
                              "debet": 124,
                              "id": 3,
                              "pvm": "2019-01-01",
                              "selite": "Verollinen tulo",
                              "tili": 1910,
                              "tyyppi": 202
                          },
                          {
                              "alvkoodi": 11,
                              "alvprosentti": 24,
                              "id": 4,
                              "kredit": 100,
                              "pvm": "2019-01-01",
                              "selite": "Verollinen tulo",
                              "tili": 7390,
                              "tyyppi": 201
                          },
                          {
                              "alvkoodi": 111,
                              "alvprosentti": 24,
                              "id": 5,
                              "kredit": 24,
                              "pvm": "2019-01-01",
                              "selite": "Verollinen tulo",
                              "tili": 2984,
                              "tyyppi": 203
                          }
                      ]
                    })";

    verollinenTuloTosite_ = QJsonDocument::fromJson( data ).toVariant();

}

void TulomenoApuriTesti::initTestCase()
{
    Kirjanpito::asetaInstanssi(new Kirjanpito);
    TestiApu::alustaKirjanpito();
}

void TulomenoApuriTesti::init()
{

}

void TulomenoApuriTesti::clean()
{

}


void TulomenoApuriTesti::yksinkertainenTulotosite()
{

    QByteArray data = R"({
                        "id": 2,
                        "liitteet": [
                        ],
                        "loki": [
                            {
                                "aika": "2019-11-09 17:36:50",
                                "data": {
                                    "liita": [
                                    ],
                                    "otsikko": "Yksinkertainen tulotosite 10 € jäsenmaksuja",
                                    "pvm": "2019-01-01",
                                    "sarja": "ML",
                                    "tila": 100,
                                    "tyyppi": 200,
                                    "viennit": [
                                        {
                                            "debet": 10,
                                            "id": 0,
                                            "pvm": "2019-01-01",
                                            "selite": "Yksinkertainen tulotosite 10 € jäsenmaksuja",
                                            "tili": 1910,
                                            "tyyppi": 202
                                        },
                                        {
                                            "alvkoodi": 0,
                                            "kohdennus": 0,
                                            "kredit": 10,
                                            "pvm": "2019-01-01",
                                            "selite": "Yksinkertainen tulotosite 10 € jäsenmaksuja",
                                            "tili": 7310,
                                            "tyyppi": 201
                                        }
                                    ]
                                },
                                "tila": 100
                            }
                        ],
                        "otsikko": "Yksinkertainen tulotosite 10 € jäsenmaksuja",
                        "pvm": "2019-01-01",
                        "rivit": [
                        ],
                        "sarja": "ML",
                        "tila": 100,
                        "tunniste": 1,
                        "tyyppi": 200,
                        "viennit": [
                            {
                                "debet": 10,
                                "id": 1,
                                "pvm": "2019-01-01",
                                "selite": "Yksinkertainen tulotosite 10 € jäsenmaksuja",
                                "tili": 1910,
                                "tyyppi": 202
                            },
                            {
                                "id": 2,
                                "kredit": 10,
                                "pvm": "2019-01-01",
                                "selite": "Yksinkertainen tulotosite 10 € jäsenmaksuja",
                                "tili": 7310,
                                "tyyppi": 201
                            }
                        ]
                    })";

    QVariant var = QJsonDocument::fromJson( data ).toVariant();

    Tosite tosite;
    tosite.lataaData( &var);

    TuloMenoApuri* apuri = new TuloMenoApuri(nullptr, &tosite);

    apuri->reset();

    TilinvalintaLine *tiliEdit = apuri->findChild<TilinvalintaLine*>("tiliEdit");
    KpEuroEdit *maaraEdit = apuri->findChild<KpEuroEdit*>("maaraEdit");

    QCOMPARE( tiliEdit->valittuTilinumero(), 7310);
    QCOMPARE( maaraEdit->asCents(), 1000);

}

void TulomenoApuriTesti::verollinenTulotosite()
{

    Tosite tosite;
    tosite.lataaData( &verollinenTuloTosite_);

    TuloMenoApuri* apuri = new TuloMenoApuri(nullptr, &tosite);

    apuri->reset();

    TilinvalintaLine *tiliEdit = apuri->findChild<TilinvalintaLine*>("tiliEdit");
    KpEuroEdit *maaraEdit = apuri->findChild<KpEuroEdit*>("maaraEdit");
    KpEuroEdit *verotonEdit = apuri->findChild<KpEuroEdit*>("verotonEdit");
    QDoubleSpinBox *alvSpin = apuri->findChild<QDoubleSpinBox*>("alvSpin");
    QComboBox *alvCombo = apuri->findChild<QComboBox*>("alvCombo");

    QCOMPARE( tiliEdit->valittuTilinumero(), 7390);
    QCOMPARE( maaraEdit->asCents(), 12400);
    QCOMPARE( verotonEdit->asCents(), 10000 );
    QCOMPARE( alvSpin->value(), 24.00);
    QCOMPARE( alvCombo->currentData(VerotyyppiModel::KoodiRooli).toInt(), AlvKoodi::MYYNNIT_NETTO);
}

void TulomenoApuriTesti::verollinenTuloKirjausWglla()
{
    KirjausWg* kwg = new KirjausWg();

    kwg->tosite()->lataaData( &verollinenTuloTosite_ );

    QComboBox *tositetyyppiCombo = kwg->findChild<QComboBox*>("tositetyyppiCombo");
    QCOMPARE( tositetyyppiCombo->currentText(), "Tulo" );

    QTabWidget* tab = kwg->findChild<QTabWidget*>("tabWidget");
    TuloMenoApuri *apuri = qobject_cast<TuloMenoApuri*> (tab->widget(0) );

    QVERIFY( apuri  != nullptr );

    TilinvalintaLine *tiliEdit = apuri->findChild<TilinvalintaLine*>("tiliEdit");
    KpEuroEdit *maaraEdit = apuri->findChild<KpEuroEdit*>("maaraEdit");
    KpEuroEdit *verotonEdit = apuri->findChild<KpEuroEdit*>("verotonEdit");
    QDoubleSpinBox *alvSpin = apuri->findChild<QDoubleSpinBox*>("alvSpin");
    QComboBox *alvCombo = apuri->findChild<QComboBox*>("alvCombo");

    QCOMPARE( tiliEdit->valittuTilinumero(), 7390);
    QCOMPARE( maaraEdit->asCents(), 12400);
    QCOMPARE( verotonEdit->asCents(), 10000 );
    QCOMPARE( alvSpin->value(), 24.00);
    QCOMPARE( alvCombo->currentData(VerotyyppiModel::KoodiRooli).toInt(), AlvKoodi::MYYNNIT_NETTO);


}

void TulomenoApuriTesti::verollinenTuloKirjausSivulla()
{
    KirjausSivu* sivu = new KirjausSivu(nullptr, nullptr);

    KirjausWg *kwg = sivu->findChild<KirjausWg*>("kirjausWg");
    QVERIFY( kwg != nullptr);

    kwg->tosite()->lataaData( &verollinenTuloTosite_ );

    QComboBox *tositetyyppiCombo = kwg->findChild<QComboBox*>("tositetyyppiCombo");
    QCOMPARE( tositetyyppiCombo->currentText(), "Tulo" );

    QTabWidget* tab = kwg->findChild<QTabWidget*>("tabWidget");
    TuloMenoApuri *apuri = qobject_cast<TuloMenoApuri*> (tab->widget(0) );

    QVERIFY( apuri  != nullptr );

    TilinvalintaLine *tiliEdit = apuri->findChild<TilinvalintaLine*>("tiliEdit");
    KpEuroEdit *maaraEdit = apuri->findChild<KpEuroEdit*>("maaraEdit");
    KpEuroEdit *verotonEdit = apuri->findChild<KpEuroEdit*>("verotonEdit");
    QDoubleSpinBox *alvSpin = apuri->findChild<QDoubleSpinBox*>("alvSpin");
    QComboBox *alvCombo = apuri->findChild<QComboBox*>("alvCombo");

    QCOMPARE( tiliEdit->valittuTilinumero(), 7390);
    QCOMPARE( maaraEdit->asCents(), 12400);
    QCOMPARE( verotonEdit->asCents(), 10000 );
    QCOMPARE( alvSpin->value(), 24.00);
    QCOMPARE( alvCombo->currentData(VerotyyppiModel::KoodiRooli).toInt(), AlvKoodi::MYYNNIT_NETTO);


}

void TulomenoApuriTesti::menonMuodostusTesti()
{
    KirjausSivu* sivu = new KirjausSivu(nullptr, nullptr);
    sivu->show();

    KirjausWg *kwg = sivu->findChild<KirjausWg*>("kirjausWg");
    QComboBox *tositetyyppiCombo = kwg->findChild<QComboBox*>("tositetyyppiCombo");
    tositetyyppiCombo->setCurrentText("Meno");

    QCOMPARE( tositetyyppiCombo->currentText(), "Meno" );
    QCOMPARE( kwg->tosite()->tyyppi(), 100);


    QTabWidget* tab = kwg->findChild<QTabWidget*>("tabWidget");
    TuloMenoApuri *apuri = qobject_cast<TuloMenoApuri*> (tab->widget(0) );
    QVERIFY( apuri  != nullptr );

    KpDateEdit *tositePvmEdit = kwg->findChild<KpDateEdit*>("tositePvmEdit");
    QLineEdit *otsikkoEdit = kwg->findChild<QLineEdit*>("otsikkoEdit");
    QComboBox *maksutapaCombo = kwg->findChild<QComboBox*>("maksutapaCombo");
    maksutapaCombo->setCurrentText("Käteinen");

    tositePvmEdit->setDate(QDate(2019,03,15));

    QCOMPARE( tositePvmEdit->date(), QDate(2019,03,15));

    QTest::keyClicks( otsikkoEdit, "Menokokeilu");

    QCOMPARE( kwg->tosite()->otsikko(), "Menokokeilu");
    QCOMPARE( kwg->tosite()->pvm(), QDate(2019,3,15));

    TilinvalintaLine *tiliEdit = apuri->findChild<TilinvalintaLine*>("tiliEdit");
    KpEuroEdit *maaraEdit = apuri->findChild<KpEuroEdit*>("maaraEdit");


    QTest::keyClicks( maaraEdit, "15,80");
    QTest::keyClicks( tiliEdit, "4951");

    QTest::mouseClick( otsikkoEdit, Qt::LeftButton);


    TositeViennit *viennit = kwg->tosite()->viennit();
    QCOMPARE( viennit->rowCount(), 2 );
    TositeVienti vasta = kwg->tosite()->viennit()->vienti(0);
    TositeVienti vienti = kwg->tosite()->viennit()->vienti(1);

    qDebug() << vasta;
    qDebug() << vienti;

    QCOMPARE( vasta.tili(), 1900);
    QCOMPARE( vasta.kredit(), 15.80);
    QCOMPARE( vasta.selite(), "Menokokeilu");

    QCOMPARE( vienti.tili(), 4951);
    QCOMPARE( vienti.debet(), 15.80);
    QCOMPARE( vienti.pvm(), QDate(2019,3,15));




}


