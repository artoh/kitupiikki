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
#ifndef PDFANALYZERROW_H
#define PDFANALYZERROW_H

#include "pdfanalyzertext.h"
#include <QList>

/**
 * @brief Yksi tiedoston rivi
 */
class PdfAnalyzerRow
{
public:
    PdfAnalyzerRow();

    /**
     * @brief Lista rivillä olevista tekstielementeistä
     *
     * Tekstielementit ovat järjestyksessä vasemmalta oikealle
     *
     * @return Tekstiluettelo
     */
    QList<PdfAnalyzerText> textList() const;

    /**
     * @brief Lisää tekstin
     *
     * Olio ylläpitää tekstien järjestystä
     *
     * @param text
     */
    void addText(const PdfAnalyzerText& text);

    /**
     * @brief Rivillä olevien tekstielementtien lukumäärä
     * @return Tekstielementtien määrä
     */
    int textCount() const;

    /**
     * @brief Koko rivin tekstielementtien ympäröivä suorakaide
     * @return Ympäröivä suorakaide
     */
    QRectF boundingRect() const;

    /**
     * @brief Koko rivin tekstit yhtenä merkkijonona
     * @return Rivin tekstit
     */
    QString text() const;

private:
    QList<PdfAnalyzerText>  textList_;

};

#endif // PDFANALYZERROW_H
