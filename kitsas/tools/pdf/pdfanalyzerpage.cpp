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

}


QList<PdfAnalyzerText> PdfAnalyzerPage::textList()
{
    return textList_;
}

void PdfAnalyzerPage::setSize(const QSizeF size)
{
    size_ = size;
}

void PdfAnalyzerPage::addText(PdfAnalyzerText text)
{
    textList_.append(text);
}

double PdfAnalyzerPage::mmToPoints(double mm)
{
    return mm * 72 / 25.4;
}

double PdfAnalyzerPage::pointsTomm(double points)
{
    return points * 25.4 / 72;
}
