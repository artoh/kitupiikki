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
#include "pdfanalyzerpage.h"
#include "pdfanalyzertext.h"

PdfAnalyzerPage::PdfAnalyzerPage(QSizeF size) :
    size_(size)
{

}

PdfAnalyzerPage::~PdfAnalyzerPage()
{
    PdfAnalyzerText* text = firstText();
    while( text ) {
        PdfAnalyzerText* next = text->next();
        delete text;
        text = next;
    }
}

PdfAnalyzerText *PdfAnalyzerPage::firstText() const
{
    return first_;
}

void PdfAnalyzerPage::addText(const QRectF &boundingRect, const QString &text)
{
    PdfAnalyzerText* newText = new PdfAnalyzerText(boundingRect, text);
    if( !first_ )
        first_ = newText;
    if( last_ )
        last_->setNext(newText);
    last_ = newText;
}

void PdfAnalyzerPage::setSize(const QSizeF size)
{
    size_ = size;
}

double PdfAnalyzerPage::mmToPoints(double mm)
{
    return mm * 72 / 25.4;
}

double PdfAnalyzerPage::pointsTomm(double points)
{
    return points * 25.4 / 72;
}
