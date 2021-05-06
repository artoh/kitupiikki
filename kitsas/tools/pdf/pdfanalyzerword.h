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
#ifndef PDFANALYZERWORD_H
#define PDFANALYZERWORD_H

#include <QRectF>
#include <QString>
#include <QList>

/**
 * @brief The Pdf-dokumentin sana
 */
class PdfAnalyzerWord
{
public:
    PdfAnalyzerWord();
    PdfAnalyzerWord(const QRectF& rect, const QString& text, bool spaceAfter_);

    /**
     * @brief Sanaa ympäröivä suorakaide pisteinä (1/72 tuumaa)
     * @return Suorakaide
     */
    QRectF boundingRect() const { return boundingRect_;}
    /**
     * @brief Sana tekstiä
     * @return Sana
     */
    QString text() const { return text_;}
    /**
     * @brief Onko tämän sanan jälkeen välilyönti
     * @return Tosi, jos sanan jälkeen välilyönti
     */
    bool hasSpaceAfter() const { return spaceAfter_;}

private:
    QRectF boundingRect_;
    QString text_;
    bool spaceAfter_;
};

#endif // PDFANALYZERWORD_H
