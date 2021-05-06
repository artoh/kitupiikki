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
#include "pdfanalyzertext.h"


PdfAnalyzerText::PdfAnalyzerText()
{

}

QRectF PdfAnalyzerText::boundingRect() const
{
    QRectF rect;
    for( const auto& word : words_) {
        rect = rect.united(word.boundingRect());
    }
    return rect;
}

QString PdfAnalyzerText::text() const
{
    QString text;
    for( const auto& word: words_) {
        text.append(word.text());
        if( word.hasSpaceAfter())
            text.append(" ");
    }
    return text;
}

void PdfAnalyzerText::addWord(const QRectF &rect, const QString &text, bool spaceAfter)
{
    if( words_.isEmpty() || words_.last().boundingRect().left() < rect.left() ) {
        words_.append(PdfAnalyzerWord(rect, text, spaceAfter));
    } else {
        for(int i=0; i < words_.count(); i++ ) {
            if( rect.left() < words_.at(i).boundingRect().left()) {
                words_.insert(i, PdfAnalyzerWord(rect, text, spaceAfter));
                return;
            }
        }
    }
}
