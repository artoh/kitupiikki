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
#include <QPainter>
#include <QPdfWriter>
#include <QPdfDocument>
#include <QTemporaryDir>

#include "laskutus/tulostus/code128.h"

namespace {

const QString ESIMERKKI = QStringLiteral("479440520200360820048831500000000086851625961989100331");
const QString TARKISTE102 = QStringLiteral("479440520200360820001000000000000000000000000026261231");

struct Rivi {
    int alku = -1;      // Ensimmäinen tumma pikseli
    int loppu = -1;     // Viimeisen tumman pikselin jälkeinen pikseli
    QList<int> ajot;    // Palkkien ja välien leveydet pikseleinä alusta loppuun
};

bool tumma(const QImage& kuva, int x, int y)
{
    return qGray(kuva.pixel(x, y)) < 128;
}

Rivi lueRivi(const QImage& kuva, int y)
{
    Rivi rivi;
    for(int x = 0; x < kuva.width(); x++) {
        if( tumma(kuva, x, y)) {
            if( rivi.alku < 0)
                rivi.alku = x;
            rivi.loppu = x + 1;
        }
    }
    if( rivi.alku < 0)
        return rivi;

    bool palkki = true;
    int pituus = 0;
    for(int x = rivi.alku; x < rivi.loppu; x++) {
        if( tumma(kuva, x, y) == palkki) {
            pituus++;
        } else {
            rivi.ajot.append(pituus);
            palkki = !palkki;
            pituus = 1;
        }
    }
    rivi.ajot.append(pituus);
    return rivi;
}

int tummiaSarakkeessa(const QImage& kuva, int x)
{
    int tummia = 0;
    for(int y = 0; y < kuva.height(); y++)
        if( tumma(kuva, x, y))
            tummia++;
    return tummia;
}

int arvoKuviosta(const QString& kuvio)
{
    for(int arvo = 0; arvo <= Code128::STOP; arvo++)
        if( Code128::kuvio(arvo) == kuvio)
            return arvo;
    return -1;
}

// Puretaan ajot moduuleiksi ja edelleen merkeiksi. Palauttaa tyhjän listan,
// jos ajot eivät muodosta kelvollista koodia.
QList<int> pura(const Rivi& rivi, int moduuleja)
{
    const qreal pikseliaModuulissa = qreal(rivi.loppu - rivi.alku) / moduuleja;
    QString moduulit;
    for(const int ajo : rivi.ajot) {
        const int leveys = qRound( ajo / pikseliaModuulissa );
        if( leveys < 1 || leveys > 4)
            return {};
        moduulit.append( QChar('0' + leveys));
    }

    QList<int> arvot;
    int i = 0;
    while( i + 7 < moduulit.length()) {
        const int arvo = arvoKuviosta(moduulit.mid(i, 6));
        if( arvo < 0)
            return {};
        arvot.append(arvo);
        i += 6;
    }
    const int lopetus = arvoKuviosta(moduulit.mid(i));
    if( lopetus != Code128::STOP)
        return {};
    arvot.append(lopetus);
    return arvot;
}

QImage kuvaPaperille(int dpi, qreal leveysMm, qreal korkeusMm)
{
    const qreal pikseliaMm = dpi / 25.4;
    QImage kuva( qRound(leveysMm * pikseliaMm), qRound(korkeusMm * pikseliaMm), QImage::Format_RGB32);
    kuva.setDotsPerMeterX( qRound(dpi / 0.0254));
    kuva.setDotsPerMeterY( qRound(dpi / 0.0254));
    kuva.fill(Qt::white);
    return kuva;
}

}

class Code128Testi : public QObject
{
    Q_OBJECT

private slots:
    void kuviotaulukko();
    void tunnettuKoodi();
    void tarkiste102();
    void virheellisetSyotteet_data();
    void virheellisetSyotteet();
    void mittarajat_data();
    void mittarajat();
    void edestakainen_data();
    void edestakainen();
    void pyoristys_data();
    void pyoristys();
    void pdf();
};

void Code128Testi::kuviotaulukko()
{
    QSet<QString> kuviot;
    for(int arvo = 0; arvo < Code128::STOP; arvo++) {
        const QString kuvio = Code128::kuvio(arvo);
        QCOMPARE(kuvio.length(), 6);
        QCOMPARE(Code128::moduuleja({arvo}), 11);
        kuviot.insert(kuvio);
    }
    QCOMPARE(kuviot.count(), Code128::STOP);

    QCOMPARE(Code128::kuvio(0), QString("212222"));
    QCOMPARE(Code128::kuvio(102), QString("411131"));
    QCOMPARE(Code128::kuvio(Code128::START_C), QString("211232"));
    QCOMPARE(Code128::kuvio(Code128::STOP), QString("2331112"));
    QCOMPARE(Code128::moduuleja({Code128::STOP}), 13);

    QVERIFY(Code128::kuvio(-1).isEmpty());
    QVERIFY(Code128::kuvio(107).isEmpty());
}

void Code128Testi::tunnettuKoodi()
{
    const QList<int> odotettu = { 105, 47, 94, 40, 52, 2, 0, 36, 8, 20, 4, 88, 31, 50, 0, 0, 0, 0,
                                  86, 85, 16, 25, 96, 19, 89, 10, 3, 31, 84, 106 };
    const QList<int> arvot = Code128::koodaaC(ESIMERKKI);
    QCOMPARE(arvot, odotettu);
    QCOMPARE(Code128::moduuleja(arvot), 332);
}

void Code128Testi::tarkiste102()
{
    const QList<int> arvot = Code128::koodaaC(TARKISTE102);
    QCOMPARE(arvot.value(arvot.count() - 2), 102);
    QCOMPARE(Code128::moduuleja(arvot), 332);

    const QImage kuva = Code128::kuva(TARKISTE102, 3);
    const Rivi rivi = lueRivi(kuva, kuva.height() / 2);
    QCOMPARE(pura(rivi, 332), arvot);
}

void Code128Testi::virheellisetSyotteet_data()
{
    QTest::addColumn<QString>("numerot");
    QTest::newRow("tyhjä") << QString();
    QTest::newRow("pariton") << QString("123");
    QTest::newRow("kirjain") << QString("12A4");
    QTest::newRow("välilyönti") << QString("12 4");
    QTest::newRow("arabialainen numero") << QString("12") + QChar(0x0663) + QChar(0x0664);
}

void Code128Testi::virheellisetSyotteet()
{
    QFETCH(QString, numerot);
    QVERIFY(Code128::koodaaC(numerot).isEmpty());
    QVERIFY(Code128::kuva(numerot, 2).isNull());

    QImage kuva = kuvaPaperille(300, 150, 20);
    QPainter painter(&kuva);
    const qreal mm = kuva.width() * 1.0 / kuva.widthMM();
    QVERIFY(!Code128::piirra(&painter, QRectF(mm * 20, mm * 2, mm * 100, mm * 13), numerot));
}

void Code128Testi::mittarajat_data()
{
    QTest::addColumn<qreal>("leveys");
    QTest::addColumn<qreal>("korkeus");
    QTest::addColumn<bool>("piirretaan");

    QTest::newRow("100 x 13") << 100.0 << 13.0 << true;
    QTest::newRow("70 x 10") << 70.0 << 10.0 << true;
    QTest::newRow("105 x 12") << 105.0 << 12.0 << true;
    QTest::newRow("liian kapea") << 69.0 << 13.0 << false;
    QTest::newRow("liian leveä") << 106.0 << 13.0 << false;
    QTest::newRow("liian matala") << 100.0 << 9.5 << false;
}

void Code128Testi::mittarajat()
{
    QFETCH(qreal, leveys);
    QFETCH(qreal, korkeus);
    QFETCH(bool, piirretaan);

    QImage kuva = kuvaPaperille(600, 150, 20);
    QPainter painter(&kuva);
    const qreal mm = kuva.width() * 1.0 / kuva.widthMM();
    QCOMPARE(Code128::piirra(&painter, QRectF(mm * 20, mm * 2, mm * leveys, mm * korkeus), ESIMERKKI), piirretaan);
    painter.end();

    bool tummia = false;
    for(int x = 0; x < kuva.width() && !tummia; x += 2)
        tummia = tumma(kuva, x, kuva.height() / 2);
    QCOMPARE(tummia, piirretaan);
}

void Code128Testi::edestakainen_data()
{
    QTest::addColumn<QString>("numerot");
    QTest::addColumn<int>("moduuliPx");
    QTest::newRow("esimerkki 1 px") << ESIMERKKI << 1;
    QTest::newRow("esimerkki 3 px") << ESIMERKKI << 3;
    QTest::newRow("tarkiste 102") << TARKISTE102 << 2;
    QTest::newRow("nollat") << QString(54, QChar('0')) << 2;
    QTest::newRow("yhdeksät") << QString(54, QChar('9')) << 2;
}

void Code128Testi::edestakainen()
{
    QFETCH(QString, numerot);
    QFETCH(int, moduuliPx);

    const QImage kuva = Code128::kuva(numerot, moduuliPx);
    QCOMPARE(kuva.height(), Code128::KUVA_KORKEUS_MODUULEINA * moduuliPx);

    const Rivi rivi = lueRivi(kuva, kuva.height() / 2);
    QCOMPARE(rivi.alku, Code128::KUVA_REUNA_MODUULEINA * moduuliPx);
    QCOMPARE(kuva.width() - rivi.loppu, Code128::KUVA_REUNA_MODUULEINA * moduuliPx);
    QCOMPARE(pura(rivi, 332), Code128::koodaaC(numerot));
}

void Code128Testi::pyoristys_data()
{
    QTest::addColumn<int>("dpi");
    QTest::newRow("96 dpi") << 96;
    QTest::newRow("300 dpi") << 300;
    QTest::newRow("600 dpi") << 600;
    QTest::newRow("1200 dpi") << 1200;
}

void Code128Testi::pyoristys()
{
    QFETCH(int, dpi);

    QImage kuva = kuvaPaperille(dpi, 140, 17);
    QPainter painter(&kuva);
    const qreal mm = kuva.width() * 1.0 / kuva.widthMM();
    const QRectF alue(mm * 20, mm * 2, mm * 100, mm * 13);
    QVERIFY(Code128::piirra(&painter, alue, ESIMERKKI));
    painter.end();

    const Rivi rivi = lueRivi(kuva, kuva.height() / 2);
    const qreal leveysMm = (rivi.loppu - rivi.alku) / mm;
    QVERIFY2(leveysMm >= Code128::LEVEYS_MIN_MM && leveysMm <= 100.0 + 1.0 / mm,
             qPrintable(QString("leveys %1 mm").arg(leveysMm)));

    QVERIFY(rivi.alku >= std::floor(alue.left()));
    QVERIFY(rivi.alku <= std::ceil(alue.left()) + 1);

    const qreal korkeusMm = tummiaSarakkeessa(kuva, rivi.alku) / mm;
    QVERIFY2(korkeusMm >= Code128::KORKEUS_MIN_MM && korkeusMm <= Code128::KORKEUS_MM + 1.0 / mm,
             qPrintable(QString("korkeus %1 mm").arg(korkeusMm)));

    QCOMPARE(pura(rivi, 332), Code128::koodaaC(ESIMERKKI));
}

void Code128Testi::pdf()
{
    QTemporaryDir hakemisto;
    QVERIFY(hakemisto.isValid());
    const QString polku = hakemisto.filePath("viivakoodi.pdf");

    {
        QPdfWriter writer(polku);
        writer.setPageSize( QPageSize( QSizeF(140, 17), QPageSize::Millimeter));
        writer.setPageMargins( QMarginsF(0, 0, 0, 0), QPageLayout::Millimeter);
        QPainter painter(&writer);
        const qreal mm = writer.width() * 1.0 / writer.widthMM();
        QVERIFY(Code128::piirra(&painter, QRectF(mm * 20, mm * 2, mm * 100, mm * 13), ESIMERKKI));
    }

    QPdfDocument dokumentti;
    QCOMPARE(dokumentti.load(polku), QPdfDocument::Error::None);

    const int dpi = 1200;
    const qreal pikseliaMm = dpi / 25.4;
    const QImage renderoity = dokumentti.render(0, QSize( qRound(140 * pikseliaMm), qRound(17 * pikseliaMm)));
    QVERIFY(!renderoity.isNull());

    // Renderöinnin tausta on läpinäkyvä
    QImage kuva(renderoity.size(), QImage::Format_RGB32);
    kuva.fill(Qt::white);
    {
        QPainter painter(&kuva);
        painter.drawImage(0, 0, renderoity);
    }

    const Rivi rivi = lueRivi(kuva, kuva.height() / 2);
    QVERIFY(rivi.alku >= 0);
    QVERIFY2( qAbs( rivi.alku / pikseliaMm - 20.0) <= 0.1, qPrintable(QString("alku %1 mm").arg(rivi.alku / pikseliaMm)));
    QVERIFY2( qAbs( (rivi.loppu - rivi.alku) / pikseliaMm - 100.0) <= 0.1,
              qPrintable(QString("leveys %1 mm").arg((rivi.loppu - rivi.alku) / pikseliaMm)));

    const qreal korkeusMm = tummiaSarakkeessa(kuva, rivi.alku + 1) / pikseliaMm;
    QVERIFY2( qAbs(korkeusMm - Code128::KORKEUS_MM) <= 0.1, qPrintable(QString("korkeus %1 mm").arg(korkeusMm)));

    QCOMPARE(pura(rivi, 332), Code128::koodaaC(ESIMERKKI));
}

QTEST_MAIN(Code128Testi)

#include "tst_code128.moc"
