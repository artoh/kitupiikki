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
#include "code128.h"

#include <QPainter>
#include <QPaintEngine>
#include <QRectF>

#include <cmath>

namespace {

const char* const KUVIOT[] = {
    "212222", "222122", "222221", "121223", "121322", "131222", "122213", "122312", "132212", "221213",
    "221312", "231212", "112232", "122132", "122231", "113222", "123122", "123221", "223211", "221132",
    "221231", "213212", "223112", "312131", "311222", "321122", "321221", "312212", "322112", "322211",
    "212123", "212321", "232121", "111323", "131123", "131321", "112313", "132113", "132311", "211313",
    "231113", "231311", "112133", "112331", "132131", "113123", "113321", "133121", "313121", "211331",
    "231131", "213113", "213311", "213131", "311123", "311321", "331121", "312113", "312311", "332111",
    "314111", "221411", "431111", "111224", "111422", "121124", "121421", "141122", "141221", "112214",
    "112412", "122114", "122411", "142112", "142211", "241211", "221114", "413111", "241112", "134111",
    "111242", "121142", "121241", "114212", "124112", "124211", "411212", "421112", "421211", "212141",
    "214121", "412121", "111143", "111341", "131141", "114113", "114311", "411113", "411311", "113141",
    "114131", "311141", "411131", "211412", "211214", "211232", "2331112"
};

constexpr int KUVIOITA = sizeof(KUVIOT) / sizeof(KUVIOT[0]);

bool pyoristetaanPikseleihin(QPainter* painter)
{
    const QPaintEngine* moottori = painter->paintEngine();
    if( !moottori )
        return false;
    const QPaintEngine::Type tyyppi = moottori->type();
    return tyyppi != QPaintEngine::Pdf
        && tyyppi != QPaintEngine::Picture
        && tyyppi != QPaintEngine::SVG;
}

}

QList<int> Code128::koodaaC(const QString &numerot)
{
    if( numerot.isEmpty() || numerot.length() % 2 )
        return {};

    QList<int> arvot;
    arvot.append(START_C);
    int summa = START_C;
    int paino = 1;

    for(int i = 0; i < numerot.length(); i += 2) {
        const QChar eka = numerot.at(i);
        const QChar toka = numerot.at(i + 1);
        if( eka < QChar('0') || eka > QChar('9') || toka < QChar('0') || toka > QChar('9'))
            return {};
        const int arvo = eka.digitValue() * 10 + toka.digitValue();
        arvot.append(arvo);
        summa += paino * arvo;
        paino++;
    }

    arvot.append( summa % 103 );
    arvot.append(STOP);
    return arvot;
}

QString Code128::kuvio(int arvo)
{
    if( arvo < 0 || arvo >= KUVIOITA)
        return QString();
    return QString::fromLatin1(KUVIOT[arvo]);
}

int Code128::moduuleja(const QList<int> &arvot)
{
    int yhteensa = 0;
    for(const int arvo : arvot) {
        const QString merkki = kuvio(arvo);
        for(const QChar& leveys : merkki)
            yhteensa += leveys.digitValue();
    }
    return yhteensa;
}

bool Code128::piirra(QPainter *painter, const QRectF &alue, const QString &numerot)
{
    const QList<int> arvot = koodaaC(numerot);
    if( arvot.isEmpty() || !painter || !painter->device() || !painter->device()->widthMM())
        return false;

    const QPaintDevice* laite = painter->device();
    const qreal mm = laite->width() * 1.0 / laite->widthMM();
    const qreal toleranssi = 0.01;

    const qreal leveysMm = alue.width() / mm;
    if( leveysMm < LEVEYS_MIN_MM - toleranssi || leveysMm > LEVEYS_MAX_MM + toleranssi ||
        alue.height() / mm < KORKEUS_MIN_MM - toleranssi)
        return false;

    const int moduulit = moduuleja(arvot);
    qreal moduuli = alue.width() / moduulit;
    qreal korkeus = qMin( alue.height(), KORKEUS_MM * mm );
    qreal x = alue.left();
    qreal y = alue.top() + ( alue.height() - korkeus ) / 2;

    if( pyoristetaanPikseleihin(painter) ) {
        const QTransform muunnos = painter->deviceTransform();
        const qreal skaala = muunnos.m11();
        const qreal pikselit = std::floor( moduuli * skaala );
        if( pikselit >= 1.0 && pikselit / skaala * moduulit >= LEVEYS_MIN_MM * mm ) {
            moduuli = pikselit / skaala;
            korkeus = std::floor( korkeus * skaala ) / skaala;
            // Vasen reuna pyöristetään oikealle, jotta vasen tyhjä alue ei pienene
            x = ( std::ceil( x * skaala + muunnos.dx() ) - muunnos.dx() ) / skaala;
            y = ( std::round( y * skaala + muunnos.dy() ) - muunnos.dy() ) / skaala;
        }
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false);
    piirraPalkit(painter, x, y, moduuli, korkeus, arvot);
    painter->restore();
    return true;
}

QImage Code128::kuva(const QString &numerot, int moduuliPx, qreal devicePixelRatio)
{
    const QList<int> arvot = koodaaC(numerot);
    if( arvot.isEmpty() || moduuliPx < 1 || devicePixelRatio <= 0.0)
        return QImage();

    const int moduuli = qMax(1, qRound( moduuliPx * devicePixelRatio ));
    const int leveys = ( moduuleja(arvot) + 2 * KUVA_REUNA_MODUULEINA ) * moduuli;
    const int korkeus = KUVA_KORKEUS_MODUULEINA * moduuli;

    QImage kuva(leveys, korkeus, QImage::Format_RGB32);
    kuva.fill(Qt::white);

    QPainter painter(&kuva);
    piirraPalkit(&painter, KUVA_REUNA_MODUULEINA * moduuli, 0, moduuli, korkeus, arvot);
    painter.end();

    kuva.setDevicePixelRatio(devicePixelRatio);
    return kuva;
}

void Code128::piirraPalkit(QPainter *painter, qreal x, qreal y, qreal moduuli, qreal korkeus, const QList<int> &arvot)
{
    int sijainti = 0;
    for(const int arvo : arvot) {
        const QString merkki = kuvio(arvo);
        for(int i = 0; i < merkki.length(); i++) {
            const int leveys = merkki.at(i).digitValue();
            if( i % 2 == 0 )
                painter->fillRect( QRectF( x + sijainti * moduuli, y, leveys * moduuli, korkeus ), Qt::black );
            sijainti += leveys;
        }
    }
}
