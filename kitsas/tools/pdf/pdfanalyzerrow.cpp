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
#include "pdfanalyzerrow.h"

PdfAnalyzerRow::PdfAnalyzerRow()
{

}

QList<PdfAnalyzerText> PdfAnalyzerRow::textList() const
{
    return textList_;
}

void PdfAnalyzerRow::addText(const PdfAnalyzerText &text)
{
    if( textList_.isEmpty() || textList_.last().boundingRect().left() < text.boundingRect().left() ) {
        textList_.append(text);
    } else {
        // Ylläpidetään järjestystä!!!
        for(int i=0; i < textList_.count(); i++) {
            if( textList_.at(i).boundingRect().left() > text.boundingRect().left()) {
                textList_.insert(i, text);
                return;
            }
        }
    }
}

int PdfAnalyzerRow::textCount() const
{
    return textList_.count();
}

QRectF PdfAnalyzerRow::boundingRect() const
{
    if( textList_.isEmpty())
        return QRectF();
    else
        return QRectF(textList_.first().boundingRect().topLeft(),
                      textList_.last().boundingRect().bottomRight());
}

QString PdfAnalyzerRow::text() const
{
    QString rowText;
    for(const auto& ti : textList_) {
        rowText.append(ti.text());
        rowText.append(" ");
    }
    return rowText.trimmed();
}
