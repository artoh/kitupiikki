/*
   Copyright (C) 2018 Arto Hyvättinen

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

#ifndef PDFTUONTI_H
#define PDFTUONTI_H

#include <QMap>

#include "tuonti.h"

namespace Poppler {
  class Document;
}

/**
 * @brief Pdf-tiedoston tietojen poiminta
 */
class PdfTuonti : public Tuonti
{
public:
    PdfTuonti(KirjausWg *wg);

    bool tuo(const QByteArray &data) override;

protected:
    /**
     * @brief Tiedostossa olevat tekstit
     *
     * Suhteellisella koordinaatistolla, jossa sivun leveys on 100 ja
     * korkeus 200 -> sivu koko on 20 000
     * sijainti = x + y * 100
     */
    QMap<int,QString> tekstit_;


    /**
     * @brief Tuo pdf-muodossa olevan laskun
     */
    void tuoPdfLasku();

    /**
     * @brief Tuo pdf-muodossa olevan tiliotteen
     */
    void tuoPdfTiliote();

    void tuoTiliTapahtumat(bool kirjausPvmRivit, int vuosiluku);


    /**
     * @brief Hakee pdf-dokumentit tekstit taulukkoon
     * @param pdfDoc
     */
    void haeTekstit(Poppler::Document *pdfDoc);

    /**
     * @brief Hakee lähimpiä merkkijonoja
     * @param y Looginen y-koordinaatti (rivi)
     * @param x Looginen x-koordinaatti (sarake)
     * @param dy Enimmäisetäisyys y
     * @param dx Enimmäisetäisyys x
     * @return
     */
    QStringList haeLahelta(int y, int x, int dy=15, int dx=30);

    /**
     * @brief Hakee regexpin sijainteja
     * @param teksti Haettava teksti
     * @param alkukorkeus Korkeus, josta haku aloitetaan
     * @param loppukorkeus Korkeus, johon haku lopetetaan
     * @param alkusarake Sarake, josta alkaa
     * @param loppusarake Sarake, johon loppuu
     * @return Lista sijainneista
     */
    QList<int> sijainnit(const QString &teksti, int alkukorkeus = 0, int loppukorkeus = 0,
                         int alkusarake = 0, int loppusarake = 100);

    /**
     * @brief Hakee ensimmäisen sijainnin
     * @param teksti
     * @param alkukorkeus Haun alku
     * @param loppukorkeus Haun loppu
     * @param alkusarake Sarake, josta alkaa
     * @param loppusarake Sarake, johon loppuu
     * @return
     */
    int etsi(const QString &teksti, int alkukorkeus=0, int loppukorkeus=0, int alkusarake = 0, int loppusarake = 100);
};

#endif // PDFTUONTI_H
