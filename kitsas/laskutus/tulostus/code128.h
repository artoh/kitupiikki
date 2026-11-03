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
#ifndef CODE128_H
#define CODE128_H

#include <QList>
#include <QString>
#include <QImage>

class QPainter;
class QRectF;

/**
 * @brief Code 128C -viivakoodin piirtäminen suorakaiteina
 *
 * Mitat pankkiviivakoodioppaan mukaan: viivojen leveys 70–105 mm,
 * korkeus 10,0–12,7 mm ja 20 mm tyhjää kummallakin puolella.
 */
class Code128
{
public:
    /**
     * @brief Koodaa numerot Code C -merkistöllä
     * @return Arvot [105, data..., tarkiste, 106] tai tyhjä lista,
     *         jos syötteessä on muuta kuin numeroita tai pariton määrä numeroita
     */
    static QList<int> koodaaC(const QString& numerot);

    /**
     * @brief Merkin kuvio moduulileveyksinä alkaen palkista, esim. 105 -> "211232"
     */
    static QString kuvio(int arvo);

    /**
     * @brief Viivojen leveys moduuleina ilman tyhjiä alueita
     */
    static int moduuleja(const QList<int>& arvot);

    /**
     * @brief Piirtää viivakoodin
     *
     * Alue on viivojen alue ilman tyhjiä alueita. Viivat alkavat alueen
     * vasemmasta reunasta. Kutsujan vastuulla on jättää alueen kummallekin
     * puolelle HILJAINEN_ALUE_MM tyhjää.
     *
     * @return false, jos numerot ovat virheelliset tai alue ei ole sallitun kokoinen
     */
    static bool piirra(QPainter* painter, const QRectF& alue, const QString& numerot);

    /**
     * @brief Viivakoodin kuva näytölle valkoisine reunoineen
     */
    static QImage kuva(const QString& numerot, int moduuliPx, qreal devicePixelRatio = 1.0);

    static constexpr int START_C = 105;
    static constexpr int STOP = 106;

    static constexpr qreal LEVEYS_MIN_MM = 70.0;
    static constexpr qreal LEVEYS_MAX_MM = 105.0;
    static constexpr qreal KORKEUS_MIN_MM = 10.0;
    static constexpr qreal KORKEUS_MM = 12.0;
    static constexpr qreal HILJAINEN_ALUE_MM = 20.0;

    static constexpr int KUVA_REUNA_MODUULEINA = 65;
    static constexpr int KUVA_KORKEUS_MODUULEINA = 40;

private:
    static void piirraPalkit(QPainter* painter, qreal x, qreal y,
                             qreal moduuli, qreal korkeus, const QList<int>& arvot);
};

#endif // CODE128_H
