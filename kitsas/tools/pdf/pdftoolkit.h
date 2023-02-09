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
#include <QObject>

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
class PdfAnalyzerDocument : public QObject {
    Q_OBJECT

public:
    virtual ~PdfAnalyzerDocument();
    /**
     * @brief Tiedoston sivumäärä
     * @return Tiedostossa sivuja
     */
    virtual int pageCount() = 0;
    /**
     * @brief Purkaa sivun sisällön
     * @param page Sivunumero, alkaen nollasta
     * @return Olio, joka sisältää sivun tekstit
     */
    virtual PdfAnalyzerPage page(int page) = 0;
    /**
     * @brief Purkaa kaikkien sivujen tekstit
     * @return Lista olioista, joissa sivujen tekstit
     */
    virtual QList<PdfAnalyzerPage> allPages() = 0;
    /**
     * @brief Pdf:n otsikko (title)
     * @return Otsikko
     */
    virtual QString title() const = 0;
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
    /**
     * @brief Tiedoston sivumäärä
     * @return Tiedostossa sivuja
     */
    virtual int pageCount() = 0;
    /**
     * @brief Renderöi sivun QImageen
     * @param page Sivunumero, alkaa nollasta
     * @param resolution Resoluutio (dpi)
     * @return QImage, johon pdf renderöity
     */
    virtual QImage renderPage(int page, double resolution = 72.0) = 0;
    /**
     * @brief Renderöi sivun QImageen, jonka leveys määritelty
     * @param page Sivunumero, alkaa nollasta
     * @param width Tuotettavan kuvan leveys
     * @return QImage, johon pdf renderöity
     */
    virtual QImage renderPageToWidth(int page, double width) = 0;
    /**
     * @brief Onko tiedosto lukittu (salasanasuojauksen takia)
     * @return Tosi, jos tiedostoa ei voi käsitellä
     */
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
