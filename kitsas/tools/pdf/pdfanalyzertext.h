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

#include <QRectF>
#include <QString>

class PdfAnalyzerText
{
public:
    PdfAnalyzerText(QRectF boundingRect,
                    QString text);

    QRectF boundingRect() const { return boundingRect_;}
    QString text() const { return text_;}
    PdfAnalyzerText* next() const { return next_;}

    void setNext(PdfAnalyzerText* next);
private:
    QRectF boundingRect_;
    QString text_;
    PdfAnalyzerText* next_ = nullptr;

};

#endif // PDFANALYZERTEXT_H
