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

class PdfTuonti : public Tuonti
{
public:
    PdfTuonti();

    bool tuoTiedosto(const QString &tiedostonnimi, KirjausWg *wg) override;

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
     * @brief Hakee pdf-dokumentit tekstit taulukkoon
     * @param pdfDoc
     */
    void haeTekstit(Poppler::Document *pdfDoc);
};

#endif // PDFTUONTI_H
