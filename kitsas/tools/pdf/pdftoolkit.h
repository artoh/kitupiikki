/*
   Copyright (C) 2021 Arto Hyvättinen

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
#ifndef PDFTOOLKIT_H
#define PDFTOOLKIT_H

#include <QByteArray>
#include <QImage>

class PdfAnalyzerPage;

/**
 * @brief The Pdf:n analysoinnin rajapinta
 *
 * Käytetään pdf-muotoisista tositteista
 * tietoja poimittaessa
 *
 * Tästä rajapinnasta toteutetaan luokat
 * Popplerille ja muille pdf-kirjastoille
 */
class PdfAnalyzerDocument {
public:
    virtual int pageCount() = 0;
    virtual PdfAnalyzerPage page(int page) = 0;
};

/**
 * @brief Pdf:n renderöinnin rajapinta
 *
 * Tästä rajapinnasta toteutetaan luokat
 * Popplerille ja muille renderöijille
 */
class PdfRendererDocument {
public:
    virtual ~PdfRendererDocument();
    virtual int pageCount() = 0;
    virtual QImage page(int page, double xres = 72.0, double yres = 72.0) = 0;
    virtual bool locked() const { return false; }
};

/**
 * @brief Pdf:n käsittelyiden käärintä
 *
 * Tämän toteutuksessa palautetaan #ifdefien avulla
 * oikean toteutuksen mukaiset luokat
 *
 */
class PdfToolkit {
public:
    static PdfRendererDocument* renderer(const QByteArray& data);
    static PdfAnalyzerDocument* analyzer(const QByteArray& data);
};




#endif // PDFTOOLKIT_H
