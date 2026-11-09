/*
   Copyright (C) 2026 Kitsas Oy

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
#include <QFontDatabase>
#include <QPainter>
#include <QPdfWriter>
#include <QPdfDocument>
#include <QTemporaryDir>

#include "db/kirjanpito.h"
#include "db/asetusmodel.h"
#include "db/tilimodel.h"
#include "kieli/kielet.h"
#include "laskutus/iban.h"
#include "laskutus/tulostus/code128.h"
#include "laskutus/tulostus/laskunalaosa.h"
#include "model/lasku.h"
#include "raportti/raportinkirjoittaja.h"

namespace {

const QString IBAN = QStringLiteral("FI7944052020036082");
const int DPI = 300;
const qreal PIKSELIA_MM = DPI / 25.4;

bool tumma(const QImage& kuva, int x, int y)
{
    return qGray(kuva.pixel(x, y)) < 128;
}

bool valkoinen(const QImage& kuva, int x, int y)
{
    return qGray(kuva.pixel(x, y)) >= 240;
}

int vaihtoja(const QImage& kuva, int y, int alku, int loppu)
{
    int vaihdot = 0;
    for(int x = alku + 1; x < loppu; x++)
        if( tumma(kuva, x, y) != tumma(kuva, x - 1, y))
            vaihdot++;
    return vaihdot;
}

QImage renderoi(const QString& polku)
{
    QPdfDocument dokumentti;
    if( dokumentti.load(polku) != QPdfDocument::Error::None)
        return QImage();
    const QSizeF koko = dokumentti.pagePointSize(0);
    const QImage renderoity = dokumentti.render(0, QSize( qRound(koko.width() / 72.0 * DPI),
                                                          qRound(koko.height() / 72.0 * DPI)));
    // Renderöinnin tausta on läpinäkyvä
    QImage kuva(renderoity.size(), QImage::Format_RGB32);
    kuva.fill(Qt::white);
    QPainter painter(&kuva);
    painter.drawImage(0, 0, renderoity);
    return kuva;
}

}

class LaskuViivakoodiTesti : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void tyhjatAlueet_data();
    void tyhjatAlueet();
    void alvLaskelma_data();
    void alvLaskelma();

private:
    void lataaAsetukset(bool tilisiirto, bool pitkat);
    void tarkistaViivakoodi(const QString& polku, qreal etsintaAlkuMm, qreal etsintaLoppuMm);
};

void LaskuViivakoodiTesti::initTestCase()
{
    QFontDatabase::addApplicationFont(":/aloitus/FreeSans.ttf");
    Kielet::alustaKielet(":/tr/tulkki.json");
    Kirjanpito::asetaInstanssi(new Kirjanpito());
}

void LaskuViivakoodiTesti::tyhjatAlueet_data()
{
    QTest::addColumn<bool>("tilisiirto");
    QTest::addColumn<qreal>("marginaali");
    QTest::addColumn<bool>("pitkat");

    QTest::newRow("tilisiirto, PDF-marginaali 10 mm") << true << 10.0 << false;
    QTest::newRow("tilisiirto, tulostinmarginaali 5 mm") << true << 5.0 << false;
    QTest::newRow("ilman tilisiirtoa, 10 mm") << false << 10.0 << false;
    QTest::newRow("ilman tilisiirtoa, 5 mm") << false << 5.0 << false;
    QTest::newRow("ilman tilisiirtoa, pitkät tiedot, 10 mm") << false << 10.0 << true;
    QTest::newRow("ilman tilisiirtoa, pitkät tiedot, 5 mm") << false << 5.0 << true;
}

void LaskuViivakoodiTesti::tyhjatAlueet()
{
    QFETCH(bool, tilisiirto);
    QFETCH(qreal, marginaali);
    QFETCH(bool, pitkat);
    lataaAsetukset(tilisiirto, pitkat);

    const Lasku lasku( QVariantMap{
        { "summa", "4883.15" },
        { "viite", "1232" },
        { "erapvm", "2026-12-31" },
        { "kieli", "FI" },
        { "maksutapa", Lasku::LASKU },
    });
    QCOMPARE(lasku.virtuaaliviivakoodi( Iban(IBAN) ).length(), 54);

    QTemporaryDir hakemisto;
    QVERIFY(hakemisto.isValid());
    const QString polku = hakemisto.filePath("lasku.pdf");
    {
        QPdfWriter writer(polku);
        writer.setPageSize( QPageSize(QPageSize::A4));
        writer.setPageMargins( QMarginsF(marginaali, marginaali, marginaali, marginaali), QPageLayout::Millimeter);
        QPainter painter(&writer);
        LaskunAlaosa alaosa(kp());
        alaosa.lataa(lasku, "Vastaanottaja Oy\nKatu 2\n00200 Helsinki", false);
        alaosa.laske(&painter);
        alaosa.piirra(&painter, lasku);
    }

    tarkistaViivakoodi(polku, 15, 125);
}

void LaskuViivakoodiTesti::alvLaskelma_data()
{
    QTest::addColumn<bool>("raidat");
    QTest::newRow("ilman taustaraitoja") << false;
    QTest::newRow("taustaraidoilla") << true;
}

void LaskuViivakoodiTesti::alvLaskelma()
{
    QFETCH(bool, raidat);
    lataaAsetukset(false, false);

    RaportinKirjoittaja rk;
    rk.asetaOtsikko("ARVONLISÄVEROLASKELMA");
    rk.lisaaPvmSarake();
    rk.lisaaSarake("TOSITE12345");
    rk.lisaaVenyvaSarake();
    rk.lisaaVenyvaSarake();
    rk.lisaaSarake("24,00 ");
    rk.lisaaEurosarake();

    // Raidoitus osuu riveille 3-5, joten viivakoodirivi on raidan kohdalla
    for(int i = 0; i < 3; i++) {
        RaporttiRivi rivi;
        rivi.lisaa(QString("Rivi %1").arg(i), 6);
        rk.lisaaRivi(rivi);
    }
    RaporttiRivi koodiRivi(RaporttiRivi::VIIVAKOODI);
    koodiRivi.lisaa("", 2);
    koodiRivi.lisaa(QStringLiteral("579440520200360820048831512000000000000000000123261231"), 2);
    rk.lisaaRivi(koodiRivi);

    QTemporaryDir hakemisto;
    QVERIFY(hakemisto.isValid());
    const QString polku = hakemisto.filePath("alv.pdf");
    QFile tiedosto(polku);
    QVERIFY(tiedosto.open(QIODevice::WriteOnly));
    tiedosto.write( rk.pdf(raidat, true) );
    tiedosto.close();

    tarkistaViivakoodi(polku, 0, 210);
}

void LaskuViivakoodiTesti::lataaAsetukset(bool tilisiirto, bool pitkat)
{
    const QString nimi = pitkat ? QStringLiteral("Esimerkkiyhdistys Pitkänimisten Toiminimien Seura ry") : QStringLiteral("Esimerkki Oy");
    kp()->asetukset()->lataa({
        { "Nimi", nimi },
        { "Katuosoite", pitkat ? QStringLiteral("Pitkäkatuisen Kaupunginosan Esimerkkikatu 123 B 45") : QStringLiteral("Katu 1") },
        { "Postinumero", "00100" },
        { "Kaupunki", "Helsinki" },
        { "Puhelin", "040 123 4567" },
        { "Sahkoposti", pitkat ? QStringLiteral("laskutus.ja.reskontra@esimerkkiyhdistys-pitkanimisten-seura.fi") : QStringLiteral("info@esimerkki.fi") },
        { "Kotisivu", pitkat ? QStringLiteral("https://www.esimerkkiyhdistys-pitkanimisten-seura.fi") : QStringLiteral("esimerkki.fi") },
        { "Ytunnus", "1234567-8" },
        { "Kotipaikka", "Helsinki" },
        { "LaskuIbanit", IBAN },
        { "LaskuViivakoodi", "ON" },
        { "LaskuTilisiirto", tilisiirto ? "ON" : "EI" },
    });
    kp()->tilit()->lataa({ QVariantMap{ { "numero", 1910 }, { "tyyppi", "ARP" }, { "iban", IBAN } } });
}

void LaskuViivakoodiTesti::tarkistaViivakoodi(const QString &polku, qreal etsintaAlkuMm, qreal etsintaLoppuMm)
{
    const QImage kuva = renderoi(polku);
    QVERIFY(!kuva.isNull());

    const QString tallennus = qEnvironmentVariable("KITSAS_TESTI_PDF");
    if( !tallennus.isEmpty())
        QFile::copy(polku, QDir(tallennus).filePath( QString("%1_%2.pdf").arg(QTest::currentTestFunction(), QTest::currentDataTag()).replace(' ', '_')));

    // Viivakoodi on pisin yhtenäinen joukko samanlaisia rivejä, joilla on paljon
    // palkkien ja välien vaihtoja. Tekstirivit eroavat toisistaan.
    const int etsintaAlku = qRound(etsintaAlkuMm * PIKSELIA_MM);
    const int etsintaLoppu = qMin( kuva.width(), qRound(etsintaLoppuMm * PIKSELIA_MM));
    auto rivi = [&kuva, etsintaAlku, etsintaLoppu](int y) {
        return QByteArray::fromRawData( reinterpret_cast<const char*>(kuva.constScanLine(y)) + etsintaAlku * 4,
                                        (etsintaLoppu - etsintaAlku) * 4);
    };
    int ylin = -1;
    int alin = -1;
    int alku = -1;
    for(int y = 0; y < kuva.height(); y++) {
        const bool koodirivi = vaihtoja(kuva, y, etsintaAlku, etsintaLoppu) > 100;
        if( koodirivi && alku >= 0 && rivi(y) == rivi(y - 1)) {
            if( y - alku > alin - ylin) {
                ylin = alku;
                alin = y;
            }
        } else {
            alku = koodirivi ? y : -1;
        }
    }
    QVERIFY2(ylin >= 0, "Viivakoodia ei löytynyt");

    const qreal korkeusMm = (alin - ylin + 1) / PIKSELIA_MM;
    QVERIFY2( korkeusMm >= Code128::KORKEUS_MIN_MM && korkeusMm <= 12.7,
              qPrintable(QString("korkeus %1 mm").arg(korkeusMm)));

    for(int y = ylin; y <= alin; y++) {
        int ensimmainen = -1;
        int viimeinen = -1;
        for(int x = etsintaAlku; x < etsintaLoppu; x++) {
            if( tumma(kuva, x, y)) {
                if( ensimmainen < 0)
                    ensimmainen = x;
                viimeinen = x;
            }
        }
        QVERIFY(ensimmainen >= 0);

        const qreal alkuMm = ensimmainen / PIKSELIA_MM;
        const qreal leveysMm = (viimeinen + 1 - ensimmainen) / PIKSELIA_MM;
        QVERIFY2( alkuMm >= Code128::HILJAINEN_ALUE_MM - 0.1,
                  qPrintable(QString("rivi %1: viivat alkavat %2 mm").arg(y).arg(alkuMm)));
        QVERIFY2( qAbs(leveysMm - 100.0) <= 0.2,
                  qPrintable(QString("rivi %1: leveys %2 mm").arg(y).arg(leveysMm)));

        // Palkin viereinen pikseli voi olla reunanpehmennyksen takia harmaa
        for(int x = 0; x < ensimmainen - 1; x++)
            QVERIFY2( valkoinen(kuva, x, y),
                      qPrintable(QString("rivi %1: ei valkoista vasemmalla %2 mm").arg(y).arg(x / PIKSELIA_MM)));

        const int oikeaLoppu = qMin( kuva.width(), viimeinen + 1 + qRound(Code128::HILJAINEN_ALUE_MM * PIKSELIA_MM));
        for(int x = viimeinen + 2; x < oikeaLoppu; x++)
            QVERIFY2( valkoinen(kuva, x, y),
                      qPrintable(QString("rivi %1: ei valkoista oikealla %2 mm").arg(y).arg(x / PIKSELIA_MM)));
    }
}

QTEST_MAIN(LaskuViivakoodiTesti)

#include "tst_laskuviivakoodi.moc"
