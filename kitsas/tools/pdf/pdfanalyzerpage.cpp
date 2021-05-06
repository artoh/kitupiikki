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



void PdfAnalyzerPage::setSize(const QSizeF size)
{
    size_ = size;
}

QList<PdfAnalyzerRow> PdfAnalyzerPage::rows() const
{
    return rows_;
}

void PdfAnalyzerPage::addRow(const PdfAnalyzerRow &row)
{
    if( rows_.isEmpty() || row.boundingRect().y() > rows_.last().boundingRect().y()) {
        rows_.append(row);
    } else {
        for(int i=0; i < rows_.count(); i++) {
            if( rows_.at(i).boundingRect().y() < row.boundingRect().y()) {
                rows_.insert(i, row);
                return;
            }
        }
    }
}

