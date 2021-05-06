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
#ifndef PDFANALYZERPAGE_H
#define PDFANALYZERPAGE_H

#include <QRectF>
#include <QList>
#include "pdfanalyzertext.h"
#include "pdfanalyzerrow.h"

/**
 * @brief The Pdf:n sivun tekstit
 */
class PdfAnalyzerPage
{
public:
    PdfAnalyzerPage(QSizeF size = QSizeF());
    ~PdfAnalyzerPage();

    /**
     * @brief Sivun koko
     * @return Sivukoko pisteinä ( 1/72 tuumaa)
     */
    QSizeF size() const { return size_;}
    void setSize(const QSizeF size);

    /**
     * @brief Sivun tekstirivit
     *
     * Rivit pidetään järjestyksessä alhaalta ylöspäin.
     * Sivun marginaalissa mahdollisesti olevaa pystysuoraa tekstiä ei sisällytetä
     * riveihin.
     *
     * @return Tekstirivit
     */
    QList<PdfAnalyzerRow> rows() const;

    /**
     * @brief Lisää rivin
     * @param row
     */
    void addRow(const PdfAnalyzerRow& row);


private:
    QList<PdfAnalyzerRow> rows_;
    QSizeF size_;
};

#endif // PDFANALYZERPAGE_H
