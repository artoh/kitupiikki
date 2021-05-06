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
#ifndef PDFANALYZERTEXT_H
#define PDFANALYZERTEXT_H

#include "pdfanalyzerword.h"
#include <QRectF>
#include <QString>
#include <QList>

/**
 * @brief Yksi tekstielementti
 */
class PdfAnalyzerText
{
public:
    PdfAnalyzerText();

    /**
     * @brief Tekstielementtiä ympäröivä suorakaide pisteinä (1/72 tuumaa)
     * @return Ympäröivä suorakaide
     */
    QRectF boundingRect() const;
    /**
     * @brief Tekstielementin sisältö yhtenä merkkijonona
     * @return Teksti
     */
    QString text() const;

    /**
     * @brief Tekstielementin sanat
     *
     * Sanat ovat järjestyksessä vasemmalta oikealle
     *
     * @return Sanat
     */
    QList<PdfAnalyzerWord> words() const { return words_;}

    /**
     * @brief Lisää yksittäisen sanan
     * @param rect Ympröivä suorakaide
     * @param text Sana
     * @param spaceAfter Tuleeko sanan jälkeen välilyönti
     */
    void addWord(const QRectF& rect, const QString& text, bool spaceAfter = false);
private:
    QList<PdfAnalyzerWord> words_;    
};


#endif // PDFANALYZERTEXT_H
